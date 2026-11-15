#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
int main(int argc,char**argv){
 if(argc!=2)return 2;
 char src[512],dst[512];snprintf(src,sizeof src,"%s/source",argv[1]);snprintf(dst,sizeof dst,"%s/dest",argv[1]);
 const size_t size=32*1024*1024;char block[65536];for(size_t i=0;i<sizeof block;i++)block[i]=(char)(i*17+13);
 for(int run=0;run<8;run++){
 int in=open(src,O_RDWR|O_CREAT|O_TRUNC,0600),out=open(dst,O_WRONLY|O_CREAT|O_TRUNC,0600);
 if(in<0||out<0){perror("open");return 1;}
 for(size_t i=0;i<size;i+=sizeof block){size_t off=0;while(off<sizeof block){ssize_t n=write(in,block+off,sizeof block-off);if(n<=0){perror("seed");return 1;}off+=n;}}
 if(fsync(in)){perror("fsync");return 1;}
 char*p=mmap(0,size,PROT_READ,MAP_PRIVATE,in,0);if(p==MAP_FAILED){perror("mmap");return 1;}
 size_t off=0;while(off<size){ssize_t n=write(out,p+off,size-off);if(n<=0){fprintf(stderr,"round %d offset %zu: %s\n",run,off,strerror(errno));return 1;}off+=n;}
 munmap(p,size);close(in);close(out);
 printf("round %d passed\n",run);fflush(stdout);
 }
 return 0;
}
