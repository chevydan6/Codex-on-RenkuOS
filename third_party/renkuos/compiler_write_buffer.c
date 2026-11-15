#define _GNU_SOURCE
#include <dlfcn.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
ssize_t write(int fd, const void *data, size_t len) {
    ssize_t (*real_write)(int,const void*,size_t) = dlsym(RTLD_NEXT,"write");
    if (!real_write) { errno=ENOSYS; return -1; }
    if (!len) return real_write(fd,data,len);
    size_t n=len>65536?65536:len;
    void *buffer=malloc(n);
    if (!buffer) {errno=ENOMEM;return -1;}
    memcpy(buffer,data,n);
    ssize_t result=real_write(fd,buffer,n);
    int saved=errno;free(buffer);errno=saved;return result;
}
