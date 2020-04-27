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
#define DEBUG 0

// global vars -> the files, sockets, pointers that exit function needs
char* g_str_bytes;
char** g_args;

typedef struct {
	int sockfd;
	int new_socket;
} params;


void exitFunction() {
	// free stuff and close sockets/threads... use global vars
	free(g_str_bytes);
	free(g_args);
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
		
	} else if (errno == ENOENT) { // dir does not exist, command fails
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


void* myThreadFun(void* vargp) {
	sleep(1);
	if (DEBUG) printf("Thread created!%d and %d\n", ((params*)vargp)->sockfd, ((params*)vargp)->new_socket); // *vargp is params struct
	int sock = ((params*)vargp)->new_socket;
	close(((params*)vargp)->sockfd); // close sockfd in thread
	packet * p = parse_request(sock);
	handle_request(p, sock);
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
  pthread_t tid;
  params* myParams = NULL;
  while (1) {
    int clientLength = sizeof(clientAddress);
    if ((new_socket = accept(sockfd, (struct sockaddr*) &clientAddress, &clientLength)) == -1) {
      //printf("Error: Failed to accept new connection\n");
      continue;
    }
    printf("Connection accepted from client\n");
    myParams = (params*) malloc(sizeof(params));
    checkMalloc(myParams);
    myParams->sockfd = sockfd;
    myParams->new_socket = new_socket;
    if ((pthread_create(&tid, NULL, myThreadFun, (void*)myParams)) == 0) {
      pthread_join(tid, NULL); // exit child when done
    } else {
    	perror("Error");
    	exit(1);
    }
    if (myParams) {
    	free(myParams);
    	myParams = NULL;
    }
    printf("Disconnected from client\n");
    close(new_socket);
  }
  
  return 0;
}
