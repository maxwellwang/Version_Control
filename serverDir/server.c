#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <arpa/inet.h>
#include <strings.h>
#include <unistd.h>
#include <limits.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <string.h>
#include <dirent.h>
#include "../zipper.h"
#define CONNECTION_QUEUE_SIZE 10

#define DEBUG 0
typedef struct {
  char ** args;
  int argc;
  int filelen;
  char code;  
} packet;

void checkMalloc(void* ptr) {
	if (!ptr) {
	printf("Error: Malloc failed\n");
	exit(1);
	}
}

void exitFunction() {
	// free stuff and close sockets/threads... use global vars
	return;
}

void send_file(int sock, char * filename);

int writen(int fd, char * buf, int n) {
  int left = n;
  int written = 0;
  
  while (left > 0) {
    if ((written = write(fd, buf, left)) <= 0) {
      if (written < 0 && errno == EINTR) {
	printf("interrupted!\n");
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

int init_port(int argc, char * argv[]) {
  // check arg count
  if (argc != 2) {
    printf("Error: Expected 2 args, received %d\n", argc);
    exit(1);
  }
  // check port number inputted
  int portNo = atoi(argv[1]);
  if (portNo == 0 && argv[1][0] != '0') {
    printf("Error: Expected port number to be an integer, received %s\n", argv[1]);
    exit(1);
  }
  if (portNo < 0 || portNo > 65535) { 
    printf("Error: Invalid port number, received %d\n", portNo);
    exit(1);
  }
  
  return portNo;
}


//reads input until a space and returns the string
char * read_space(int socket) {
  int num_bytes;
  char* str_bytes = malloc(4096);
  checkMalloc(str_bytes);
  memset(str_bytes, 0, 4096);
  int buf_pos = 0;
  while (buf_pos == 0 || str_bytes[buf_pos-1]  != ' ') {
    read(socket, str_bytes + buf_pos, 1);
    //printf("Read: [%c]", str_bytes[buf_pos]);
    buf_pos++;
  }
  str_bytes[buf_pos-1] = 0;
  return str_bytes;
}


//populates args and argc of packet
void read_args(int socket, packet * p) {
  int argc = atoi(read_space(socket));
  char ** args = malloc(argc * sizeof(char*));
  checkMalloc(args);
  p->argc = argc;
  char * arg;
  int i;
  for (i = 0; i < argc; i++) {
    arg = read_space(socket);
    args[i] = arg;
  }
  p->args = args;
  
  return;
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

    num_write = write(fd2, buff, num_read);
    len -= num_read;
  }
}

//reads len bytes into _wtf_tmp_.tgz
void read_to_file(int socket, int len) {
  int fd = open("./_wtf_tar_server", O_RDWR | O_CREAT, 00600);
  if (fd == -1) {
    printf("Error: could not open file\n");
    exit(1); //only exiting the child
  }
  
  f2f(socket, fd, len);
  close(fd);
}

void checkout(packet * p ) {
}
void update(packet * p ) {
}
void upgrade(packet * p ) {
}
void commit_a(packet * p ) {
}
void commit_b(packet * p ) {
}
void push(packet * p ) {
}
void create(packet * p, int socket) {
	char message[42 + strlen(p->args[0])];
	if (mkdir(p->args[0], 0700) == -1) { // dir already exists
		sprintf(message, "Error: %s project already exists on server\n", p->args[0]);
		write(socket, message, strlen(message));
		return;
	}
	int pathLength = 12 + strlen(p->args[0]);
	char path[pathLength];
	sprintf(path, "./%s/.Manifest", p->args[0]);
	int manifest = open(path, O_WRONLY | O_CREAT, 00600);
	write(manifest, "1\n", 2);
	close(manifest);
	send_file(socket, ".Manifest");
	return;
}
void destroy(packet * p, int socket) {
	DIR* dir = opendir(p->args[0]);
	char message[42 + strlen(p->args[0]) + strlen(strerror(ENOENT))];
	if (dir) { // dir exists, delete it
		
	} else if (errno == ENOENT) { // dir does not exist, command fails
		sprintf(message, "Error: %s project does not exist on server\n", p->args[0]);
		write(socket, message, strlen(message));
		return;
	} else { // failed for some other reason, command fails
		sprintf(message, "Error: %s\n", strerror(errno));
		write(socket, message, strlen(message));
		return;
	}
	return;
}
void currentversion(packet * p ) {
}
void history(packet * p ) {
}
void rollback(packet * p ) {
}
void testfunc(packet * p ) {
  printf("Reached the test function!\n");

  printf("%d args, they are (in [brackets]):\n", p->argc);
  int i;
  for (i = 0; i < p->argc; i++) {
    printf("Arg %d: [%s]\n", i, (p->args)[i]);
  }
  printf("Length of file is: %d\n", p->filelen);
}

int handle_request(packet * p, int socket) {
  //read in information according to protocol
  if (DEBUG) printf("Handling request\n");
  switch (p->code) {
  case '0':
    checkout(p);
    break;
  case '1':
    update(p);
    break;
  case '2':
    upgrade(p);
    break;
  case '3':
    commit_a(p);
    break;
  case '4':
    commit_b(p);
    break;
  case '5':
    push(p);
    break;
  case '6':
    create(p, socket);
    break;
  case '7':
    destroy(p, socket);
    break;
  case '8':
    currentversion(p);
    break;
  case '9':
    history(p);
    break;
  case 'a':
    rollback(p);
    break;
  case 't':
    testfunc(p);
    break;
    
  }
  
}

int parse_request(int socket) {
  if (DEBUG) printf("Parsing request\n");
  packet * p = malloc(sizeof(packet));
  checkMalloc(p);
  int c, len;
  read(socket, &c, 1);
  p->code = c;
  if (DEBUG) printf("Got code\n");
  read_args(socket, p);
  if (DEBUG) printf("Got args\n");
  /*len = atoi(read_space(socket));
  p->filelen = len;
  if (len > 0) {
    read_to_file(socket, len);
  }
  if (DEBUG) printf("Got file\n");*/
  handle_request(p, socket);
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

int main(int argc, char* argv[]) {
  atexit(exitFunction);
  int port = init_port(argc, argv);
  int sockfd, new_socket, childpid;
  struct sockaddr_in serverAddress, clientAddress;
  
  // init socket file descriptor
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd == -1) {
    printf("Error: Socket creation failed\n");
    exit(1);
  }
  
  // make socketaddr structure
  bzero((char *) &serverAddress, sizeof(serverAddress));
  serverAddress.sin_family = AF_INET;
  serverAddress.sin_addr.s_addr = INADDR_ANY;
  serverAddress.sin_port = htons(port);

  // bind name to socket
  if (bind(sockfd, (struct sockaddr*) &serverAddress, sizeof(serverAddress)) == -1) {
    printf("Error: Failed to bind to socket\n");
    exit(1);
  }
  
  // listen for connections
  if (listen(sockfd, CONNECTION_QUEUE_SIZE) == -1) {
    printf("Error: Failed to listen\n");
    exit(1);
  }


  if (DEBUG) printf("Waiting for connections\n");
  // accept connections, multithreaded
  while (1) {
    int clientLength = sizeof(clientAddress);
    if ((new_socket = accept(sockfd, (struct sockaddr*) &clientAddress, &clientLength)) == -1) {
      printf("Error: Failed to accept new connection\n");
      continue;
    }
    printf("Connection accepted from client\n");
    if ((childpid = fork()) == 0) {
      if (DEBUG) printf("Child created!\n");
      close(sockfd); //close stream in child, still open in parent
      parse_request(new_socket);
      
      exit(0); //exit child when done
    }
    printf("Disconnected from client\n");
    close(new_socket);
  }
  
  /*
  // prepare buffer
  char* buffer = (char*) malloc(256);
  checkMalloc(buffer);
  bzero(buffer, 256);
  // read message from client
  int numBytes = read(newsockfd, buffer, 255);
  if (numBytes == -1) {
  perror("Error");
  return 1;
  }
  printf("Here is the message: %s\n",buffer);
  // write message back
  numBytes = write(newsockfd, "I got your message", 18);
  if (numBytes == -1) {
  perror("Error");
  return 1;
  }

  // close socket file descriptor
  close(sockfd);
  // free buffer
  free(buffer);
  */
  return 0;
}
