#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include "util.h"

void hash(char* filename, char c[]) {
  char command[strlen(filename) + 19];
  sprintf(command, "md5sum %s > hashfile", filename);
  system(command);
  int hashfile = open("hashfile", O_RDONLY);
  int i;
  for (i = 0; i < 32; i++) {
    read(hashfile, c + i, 1);
  }
  close(hashfile);
  return;
}

//reads input until a space and returns the string
char * read_space(int socket) {
  int num_bytes;
  char * buf = malloc(4096); // global var now
  checkMalloc(buf);
  memset(buf, 0, 4096);
  int buf_pos = 0;
  while (buf_pos == 0 || buf[buf_pos-1]  != ' ') {
    read(socket, buf + buf_pos, 1);
    //printf("Read: [%c]", buf[buf_pos]);
    buf_pos++;
  }
  buf[buf_pos-1] = 0;
  return buf;
}


//populates args and argc of packet
void read_args(int socket, packet * p) {
	//printf("in read args func\n");
  char * tmp = read_space(socket);
  int argc = atoi(tmp);
  free(tmp);
  char ** args = malloc(argc * sizeof(char*));
  checkMalloc(args);
  p->argc = argc;
  //printf("will read in %d args\n", argc);
  char * arg;
  int i;
  for (i = 0; i < argc; i++) {
    arg = read_space(socket);
    //printf("read in %s\n", arg);
    args[i] = arg;
  }
  p->args = args;
  return;
}

//reads len bytes into _wtf_tmp_.tgz
void read_to_file(int socket, int len) {
  int fd = open("./_wtf_tar", O_RDWR | O_CREAT, 00600);
  if (fd == -1) {
    printf("Error: could not open file\n");
    exit(1); //only exiting the child
  }
  
  f2f(socket, fd, len);
  close(fd);
}

void checkMalloc(void* ptr) {
  if (!ptr) {
    printf("Error: Malloc failed\n");
    exit(1);
  }
}


//read from one fd to another, for len bytes
void f2f(int fd1, int fd2, int len) {
  char buff[4096];
  int to_read, num_read, num_write;
  memset(buff, 0, 4096);

  while (len > 0) {
    to_read = 4096 > len ? 4096 : len;
    num_read = read(fd1, buff, to_read);
    if (num_read == 0) {
      return;
    }

    num_write = writen(fd2, buff, num_read);
    len -= num_read;
  }
}

//can only send 1 file!!
void send_file(int sockfd, char * filename) {
  zip_init();
  zip_add(filename);
  zip_tar();
  char * size = malloc(64);
  memset(size, 0, 64);
  sprintf(size, "%d", zip_size());
  writen(sockfd, size, strlen(size));
  writen(sockfd, " ", 1);
  int tarfd = open("./_wtf_tar", O_RDONLY);
  f2f(tarfd, sockfd, zip_size());
}

int writen(int fd, char * buf, int n) {
  int left = n;
  int written = 0;
  
  while (left > 0) {
    if ((written = write(fd, buf, left)) <= 0) {
      if (written < 0 && errno == EINTR) {
	written = 0;
      }	else {
	return -1;
      }
    }
    left -= written;
    buf += written;
  }
  return n;
}

//soley for checkout
void zip_proj(char * proj) {
  char buf[4096];
  memset(buf, 0, 4096);
  sprintf(buf, "tar -zcf _wtf_tar %s", proj);
  system(buf);
}

//remove any existing tar/directory
void zip_init() {
  system("rm -r _wtf_dir 2>/dev/null");
  system("rm _wtf_tar 2>/dev/null");
  system("mkdir _wtf_dir 2>/dev/null");
}

//add file to tar
void zip_add(char * path) {
  int len = 25 + strlen(path);
  char * str = malloc(len);
  memset(str, 0, len);
  snprintf(str, len, "cp %s _wtf_dir 2>/dev/null", path);
  system(str);
}

//create tar and remove dir
void zip_tar() {
  system("tar -zcf _wtf_tar _wtf_dir 2>/dev/null");
  system("rm -r _wtf_dir 2>/dev/null");
}

//untar and remove tar, leaving _wtf_dir
void zip_untar() {
  system("rm -r _wtf_dir 2>/dev/null");
  system("tar -xf _wtf_tar 2>/dev/null");
  system("rm -r _wtf_tar 2>/dev/null");
}

//get size of tar (used for when sending)
int zip_size() {
  struct stat st;
  stat("_wtf_tar", &st);
  return st.st_size;
}

//actual function used to send compressed files
void zip_send(int fd, int len, char ** paths) {
  zip_init();
  int i = 0;
  for (i = 0; i < len; i++) {
    zip_add(paths[i]);
  }
  zip_tar();
  int size = zip_size();
}

//whoeverse calling this should free p
packet * parse_request(int socket){
  packet * p = malloc(sizeof(packet));
  checkMalloc(p);
  int len;
  char c = '?';
  read(socket, &c, 1);
  p->code = c;
  read_args(socket, p);
  len = atoi(read_space(socket));
  p->filelen = len;
  if (len > 0) {
   read_to_file(socket, len);
   zip_untar();
  }
  return p;
}
