#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <arpa/inet.h>
#include <strings.h>
#include <unistd.h>
#include <limits.h>
#include <netinet/in.h>
#define CONNECTION_QUEUE_SIZE 10

typedef struct {
  char code;
  int arglen;
  char ** args;
  char * dirname;
} packet;

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
  
  return port;
}

int parse_request(int socket) {
  packet * p = malloc(sizeof(packet));
  //read in code
  int c;
  read(socket, &c, 1);
  p->code = c;

  //read in arguments
  int num_bytes;
  char * str_bytes = malloc(64);
  memset(str_bytes, 0, 64);
  int buf_pos = 0;
  while (str_bytes[buf_pos-1]  != ' ') {
    read(socket, str_bytes + buf_pos, 1);
    buf_pos++;
  }
  str_bytes[buf_pos-1] = 0;
  num_bytes = atoi(str_bytes);
  
  //read num_bytes bytes
  
  //repeat for file
  
  handle_request(p);
}

int handle_request(packet p) {
  //read in information according to protocol
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
  case '2':
    commit_a(p);
    break;
  case '2':
    commit_b(p);
    break;
  case '2':
    push(p);
    break;
  case '2':
    create(p);
    break;
  case '2':
    destroy(p);
    break;
  case '2':
    currentversion(p);
    break;
  case '2':
    history(p);
    break;
  case '2':
    rollback(p);
    break;
    
  }
  
}
  


int main(int argc, char* argv[]) {
  int port = init_port(argc, argv);
  int sockfd, socket, childpid;
  struct sockaddr_in serverAddress, clientAddress;
  
  // init socket file descriptor
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd == -1) {
    printf("Socket creation failed\n");
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

  
  // accept connections, multithreaded
  while (1) {
    int clientLength = sizeof(clientAddress);
    if ((socket = accept(sockfd, (struct sockaddr*) &clientAddress, &clientLength)) == -1) {
      printf("Error: Failed to accept new connection\n");
      continue;
    }
    if ((childpid = Fork()) == 0) {
      close(sockfd); //close stream in child, still open in parent
      handle_request(socket);
      exit(0); //exit child when done
    }
    close(socket);
  }
  
  /*
  // prepare buffer
  char* buffer = (char*) malloc(256);
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
