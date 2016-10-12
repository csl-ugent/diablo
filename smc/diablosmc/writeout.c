#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define __NR_writespecial		  4
#define __NR_openspecial		  5
#define __NR_closespecial		  6

#define __syscall_return(type, res) \
  do { \
    	if ((unsigned long)(res) >= (unsigned long)(-125)) { \
	  		errno = -(res); \
	  		res = -1; \
	  	} \
    	return (type) (res); \
  } while (0)

#define _syscall3(type,name,type1,arg1,type2,arg2,type3,arg3) \
  type __##name(type1 arg1,type2 arg2,type3 arg3) \
{ \
  long __res; \
  __asm__ volatile ("int $0x80" \
      	: "=a" (__res) \
      	: "0" (__NR_##name),"b" ((long)(arg1)),"c" ((long)(arg2)), \
      		  "d" ((long)(arg3))); \
  __syscall_return(type,__res); \
}

#define _syscall1(type,name,type1,arg1) \
    type __##name(type1 arg1) \
{ \
  long __res; \
  __asm__ volatile ("int $0x80" \
      	: "=a" (__res) \
      	: "0" (__NR_##name),"b" ((long)(arg1))); \
  __syscall_return(type,__res); \
}
  
static int errno;  
static _syscall3(long,writespecial,int,fd,const void *,buf,unsigned int,size)
static _syscall1(int,closespecial,int,fd)
static _syscall3(int,openspecial, const char *, pathname, int, flags, int, mode)
  
  
void writeout(int * value,int counter)
{
  int i;
  int file;
  
  file = __openspecial("output.prof",O_WRONLY|O_CREAT,0544);
  
  for(i=0;i<counter;i++)
  {
    __writespecial(file,&value[i*2],sizeof(int));
    __writespecial(file,&value[i*2+1],sizeof(int));
  }
  __closespecial(file);
}
