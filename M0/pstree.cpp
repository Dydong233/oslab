#include <iostream>
#include <cstring>
#include <dirent.h>
#include <vector>

using namespace std;

struct Node{
	string pid,proc_name;
	bool st = false;
	vector<int> son;
	void init(string _pid,string _proc_name)
	{
		pid = _pid;
		proc_name = _proc_name;
		st = true;
	}
}node[65536];

int string_to_int(string str)
{
	int res=0,len=str.length();
	for(int i=0;i<len;i++)
	{
		res*=10;
		res+=(str[i]-'0');
	}
	return res;
}

void show_the_tree(int idx,int depth)
{
	if(!node[idx].st&&idx!=0)	return;
	if(idx){
		for(int i=1;i<depth;i++)	cout<<"\t";
		cout<<"The level is "<<depth<<":";
		cout<<node[idx].proc_name<<"("<<node[idx].pid<<")"<<endl;
	}
	for(auto x:node[idx].son)	show_the_tree(x,depth+1);
}

int main(int argc, char *argv[]) {

	// step1: read the information && construct the list
	const char* path = "/proc";
        DIR* dir = opendir(path);
        if (!dir) return 0;
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
                if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)        continue;
                if (entry->d_name[0]<'0' || entry->d_name[0]>'9' )      continue;
                
                // get_proc_name
                string pid,proc_name;
                int Pid;
                pid = entry->d_name;
                Pid = string_to_int(pid);
                string comm_path = string("/proc/")+pid+string("/comm");
                FILE *fp = fopen(comm_path.c_str(), "r");
                char buf[256];
                if (fgets(buf, sizeof(buf), fp)) {
                    buf[strcspn(buf, "\n")] = '\0';
                    proc_name = buf;
                }
                fclose(fp);
                if(proc_name == "kthreadd")     continue;

                // get_proc_ppid
                
                string status_path = string("/proc/")+pid+string("/status");
                FILE *status_fp = fopen(status_path.c_str(),"r");
                int PPid;       string ppid;
                if (status_fp)
                        while (fgets(buf, sizeof(buf), status_fp)) {
                                if (strncmp(buf, "PPid:", 5) == 0) {
                                        sscanf(buf, "PPid:\t%d", &PPid);
                                        buf[strcspn(buf, "\n")] = '\0';
                                        ppid = buf;
                                        break;
                                }
                        }
                fclose(status_fp);
                if(PPid == 2)   continue;
                if(Pid > 65535) continue;
                node[Pid].init(pid,proc_name);
                node[PPid].son.push_back(Pid);
                // cout<<"pid: "<<pid<<"  proc_name: "<<proc_name<<"  ppid: "<<ppid<<endl;
        }
        closedir(dir);

	// step2: output the tree
	show_the_tree(0,0);
	
  	return 0;
}
