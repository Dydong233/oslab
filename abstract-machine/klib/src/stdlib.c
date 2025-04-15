#include <am.h>
#include <klib.h>
#include <klib-macros.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)
static unsigned long int next = 1;
static void *addr = NULL;

int rand(void) {
  // RAND_MAX assumed to be 32767
  next = next * 1103515245 + 12345;
  return (unsigned int)(next/65536) % 32768;
}

void srand(unsigned int seed) {
  next = seed;
}

int abs(int x) {
  return (x < 0 ? -x : x);
}

int atoi(const char* nptr) {
  int x = 0;
  while (*nptr == ' ') { nptr ++; }
  while (*nptr >= '0' && *nptr <= '9') {
    x = x * 10 + *nptr - '0';
    nptr ++;
  }
  return x;
}

void *malloc(size_t size) {
  // Therefore do not call panic() here, else it will yield a dead recursion
  // Heap will be initialized during loading
  // typedef struct {void *start, *end;} Area;
#if !(defined(__ISA_NATIVE__) && defined(__NATIVE_USE_KLIB__))
  
  if(addr == NULL) addr = (void *)ROUNDUP(heap.start,8);
  size = (size_t)ROUNDUP(size,8);
  char *old = addr;
  addr+=size;
  assert(IN_RANGE(addr,heap));
  for(char *p=old;p<(char*)addr;p++)  *p=0;
  // Log
  printf("Log: The heap range is %d to %d\n",heap.start,heap.end);
  printf("Log: Need size is %d\n",size);
  printf("Log: The need range is %d to %d\n",old,addr);
  return old;

#endif
  return NULL;
}

void free(void *ptr) {
}

#endif
