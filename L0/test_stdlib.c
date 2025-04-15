#include<am.h>
#include<amdev.h>
#include<klib-macros.h>
#include<klib.h>

int main()
{
	malloc(0x10);
	malloc(0x14);
	return 0;
}
