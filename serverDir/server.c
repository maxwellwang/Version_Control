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
#include <pthread.h>
#include "../util.h"
#define CONNECTION_QUEUE_SIZE 10
#define DEBUG 1

// global vars -> the files, sockets, pointers that exit function needs


typedef struct {
	int sockfd;
	int new_socket;
} params;


void exitFunction() {
	// free stuff and close sockets/threads... use global vars
	return;
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


void checkout(packet * p, int socket) {
  //printf("Dir: [%s]", p->args[0]);
  if (mkdir(p->args[0], 0700) == -1) {
    //tar up directory name
    send_proj(socket, p->args[0]);
  } else { //does not exist
    char buf[4096];
    memset(buf, 0, 4096);
    sprintf(buf, "rm -r %s", p->args[0]);
    system(buf);
    writen(socket, "01 f 0 ", 7);
  }
  
}
void update(packet * p ) {
}
void upgrade(packet * p ) {
}
void commit_a(packet * p, int socket ) {
	char* projectname = p->args[0];
	// send error if project doesn't exist
	DIR* dir = opendir(projectname);
	if (!dir) {
		writen(socket, "01 p 0 ", 7);
		closedir(dir);
		return;
	}
	closedir(dir);
	char manifestPath[strlen(projectname) + 15];
	sprintf(manifestPath, "./%s/.Manifest", projectname);
	// send error if manifest doesn't exist in project
	if (access(manifestPath, F_OK) == -1) {
		writen(socket, "01 m 0 ", 7);
		return;
	}
	// good to go, send manifest to client
	if (DEBUG) printf("about to send manifest\n");
	send_file(socket, manifestPath);
	if (DEBUG) printf("done sending manifest\n");
	return;
}
void commit_b(packet * p ) {
}
void push(packet * p ) {
}
void create(packet * p, int socket) {
	char message[42 + strlen(p->args[0])];
	if (mkdir(p->args[0], 0700) == -1) { // dir already exists
		sprintf(message, "Error: %s project already exists on server\n", p->args[0]);
		writen(socket, message, strlen(message));
		return;
	}
	int pathLength = 12 + strlen(p->args[0]);
	char manifestPath[pathLength];
	sprintf(manifestPath, "./%s/.Manifest", p->args[0]);
	int manifest = open(manifestPath, O_WRONLY | O_CREAT, 00600);
	writen(manifest, "1\n", 2);
	close(manifest);
	writen(socket, "60 ", 3);
	send_file(socket, manifestPath); // send manifest to client
	return;
}
void destroy(packet * p, int socket) {
	DIR* dir = opendir(p->args[0]);
	char message[42 + strlen(p->args[0]) + strlen(strerror(ENOENT))];
	if (dir) { // dir exists, delete it
		closedir(dir);
	} else if (errno == ENOENT) { // project does not exist, command fails
		sprintf(message, "Error: %s project does not exist on server\n", p->args[0]);
		writen(socket, message, strlen(message));
		return;
	} else { // failed for some other reason, command fails
		sprintf(message, "Error: %s\n", strerror(errno));
		writen(socket, message, strlen(message));
		return;
	}
	return;
}
void currentversion(packet * p, int socket) {
	if (DEBUG) printf("reached currver\n");
	char message[42 + strlen(p->args[0])];
	DIR* dir = opendir(p->args[0]);
	if (dir) { // dir exists, list its files and versions
		int pathLength = 12 + strlen(p->args[0]);
		char manifestPath[pathLength];
		sprintf(manifestPath, "./%s/.Manifest", p->args[0]);
		int manifest = open(manifestPath, O_RDONLY);
		char c = '?';
		if(DEBUG) printf("about to read manifest\n");
		int status = read(manifest, &c, 1);
		while (c != '\n') {
			read(manifest, &c, 1);
		}
		int spaceNum = 0;
		status = read(manifest, &c, 1);
		while (status > 0) {
			if (DEBUG) printf("Read %c\n", c);
			if (c == ' ') {
				spaceNum++;
				if (spaceNum == 1) { // write space
					writen(socket, &c, 1);
				} else if (spaceNum == 2) { // reached hash, skip to next line (don't write space)
					spaceNum = 0;
					while (c != '\n') {
						read(manifest, &c, 1);
					}
					writen(socket, "\n", 1);
				}
			} else {
				writen(socket, &c, 1);
			}
			status = read(manifest, &c, 1);
		}
		close(manifest);
		return;
	} else if (errno == ENOENT) { // project does not exist, command fails
		sprintf(message, "Error: %s project does not exist on server\n", p->args[0]);
		writen(socket, message, strlen(message));
		return;
	} else {
		sprintf(message, "Error: %s\n", strerror(errno));
		writen(socket, message, strlen(message));
		return;
	}
	return;
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
    checkout(p, socket);
    break;
  case '1':
    update(p);
    break;
  case '2':
    upgrade(p);
    break;
  case '3':
    commit_a(p, socket);
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
    currentversion(p, socket);
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


void* myThreadFun(void* sockfd) {
  int sock = *((int *) sockfd);
  packet * p = parse_request(sock);
  handle_request(p, sock);
  close(sock);
  free(sockfd);
  int i = 0;
  for (; i < p->argc; i++) {
    free((p->args)[i]);
  }
  free(p);
  return NULL;
}

int main(int argc, char* argv[]) {
  atexit(exitFunction);
  int port = init_port(argc, argv);
  int sockfd, new_socket;
  struct sockaddr_in serverAddress, clientAddress;
  
  // init socket file descriptor
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd == -1) {
    perror("Error");
    exit(1);
  }
  
  int reuse = 1;
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse)) < 0) {
    perror("setsockopt(SO_REUSEADDR) failed");
  }

  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, (const char*)&reuse, sizeof(reuse)) < 0) {
    perror("setsockopt(SO_REUSEPORT) failed");
  }
  
  // make socketaddr structure
  bzero((char *) &serverAddress, sizeof(serverAddress));
  serverAddress.sin_family = AF_INET;
  serverAddress.sin_addr.s_addr = INADDR_ANY;
  serverAddress.sin_port = htons(port);

  // bind name to socket
  if (bind(sockfd, (struct sockaddr*) &serverAddress, sizeof(serverAddress)) == -1) {
    perror("Error");
    exit(1);
  }
  
  // listen for connections
  if (listen(sockfd, CONNECTION_QUEUE_SIZE) == -1) {
    perror("Error");
    exit(1);
  }


  if (DEBUG) printf("Waiting for connections\n");
  // accept connections, multithreaded
  pthread_t tids[CONNECTION_QUEUE_SIZE];
  int i = 0;
  while (1) {
    int clientLength = sizeof(clientAddress);
    if ((new_socket = accept(sockfd, (struct sockaddr*) &clientAddress, &clientLength)) == -1) {
      printf("Error: Failed to accept new connection\n");
      continue;
    }
    printf("Connection accepted from client\n");
    int * sock = malloc(sizeof(int));
    *sock = new_socket;
    if ((pthread_create(&(tids[i]), NULL, myThreadFun, (void*) sock)) == 0) {
      pthread_detach(tids[i++]);
    } else {
    	error("Error");
    	exit(1);
    }
    printf("Disconnected from client\n");
  }
  
  return 0;
}
