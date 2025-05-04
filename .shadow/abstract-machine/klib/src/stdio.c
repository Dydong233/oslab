#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>


#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

inline int itos(int val,char *buf)
{
	bool st = true;	// target positive or negative
	if(val<0)	st = false;
	val = abs(val);
	int len = 0;
	while(val)	{
		buf[len++] = (val%10)+'0';
		val/=10;
	}
	if(!st)	buf[len++] = '-';
	return len;
}

int printf(const char *fmt, ...) {
	va_list ap;
	va_start(ap,fmt);
	char buf[2048];
	int len = vsprintf(buf,fmt,ap);
	for(int i=0;i<len;i++)	putch(buf[i]);
	va_end(ap);
    return len;
}

int vsprintf(char *out, const char *fmt, va_list ap) {
	char *des = out;
	const char *src = fmt;
	while(*src){
		if(*src != '%'){
			*des++ = *src++;
			continue;
		}
		src++;	// skip %
		if(*src == 'd'){      // input an int
			int val = va_arg(ap,int);
			char buf[25];
			int len = itos(val,buf);
			for(--len;len>=0;len--)	*des++ = buf[len];
		}
		else if(*src == 's'){	// input an string
			char *s = va_arg(ap,char *);
			for(;*s;s++)	*des++ = *s;	
		}
		else if(*src == 'p'){	// input an pointer
			void *ptr = va_arg(ap,void *);
			unsigned long val = (unsigned long)ptr;
			char buf[25];
			int len = 0;
			*des++ = '0';
			*des++ = 'x';
			if(val == 0)	*des++ = '0';
			else{
				while(val){
					int t = val%16;
					if(t<10)	buf[len++] = t+'0';
					else buf[len++] = t-10+'a';
					val /= 16;
				}
				for(--len;len>=0;len--)	*des++ = buf[len];
			}
		}	
		else panic("Input other formats!");
		src++;
	}
	*des = '\x00';
	return des-out;
}

int sprintf(char *out, const char *fmt, ...) {
  panic("Not implemented");
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
  panic("Not implemented");
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
  panic("Not implemented");
}

#endif
