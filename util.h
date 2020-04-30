#ifndef _UTIL_H
#define _UTIL_H

typedef struct {
  char ** args;
  int argc;
  int filelen;
  char code;  
} packet;

void hash(char* filename, char c[]);
  
packet * parse_request(int socket);

void handle_response(packet *);

char * read_space(int socket);

void read_args(int socket, packet * p);

void f2f(int fd1, int fd2, int len);

void read_to_file(int socket, int len);

void checkMalloc(void* ptr);

char * parse_dir(char *);

void check_args(int, int);

void system2(char *, char *);

void writen2(int, char *, char *);

//read from one fd to another, for len bytes
void f2f(int fd1, int fd2, int len);
//can only send 1 file!!
void send_file(int sockfd, char * filename);

int writen(int fd, char * buf, int n);

//actual function used to send compressed files
void zip_send(int fd, int len, char ** paths);

//tar entire proj dir, for checkout
void send_proj(int fd, char * proj);
  
//remove any existing tar/directory
void zip_init();

//add file to tar
void zip_add(char * path);

//create tar and remove dir
void zip_tar();

//untar and remove tar, leaving _wtf_dir
void zip_untar();

//get size of tar (used for when sending)
int zip_size();
#endif
