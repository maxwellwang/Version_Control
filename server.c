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

int main(int argc, char* argv[]) {
  // check arg count
  if (argc != 2) {
    printf("Error: Expected 2 args, received %d\n", argc);
    return 1;
  }
  // check port number inputted
  int portNo = atoi(argv[1]);
  if (portNo == 0 && argv[1][0] != '0') {
    printf("Error: Expected port number to be an integer, received %s\n", argv[1]);
    return 1;
  }
  if (portNo < 0 || portNo > 65535) { 
   printf("Error: Invalid port number, received %d\n", portNo);
    return 1;
  }
  
  // init socket file descriptor
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd == -1) {
    printf("Socket creation failed\n");
    return 1;
  }
  
  // make socketaddr structure
  struct sockaddr_in serverAddress;
  bzero((char *) &serverAddress, sizeof(serverAddress));
  serverAddress.sin_family = AF_INET;
  serverAddress.sin_addr.s_addr = INADDR_ANY;
  serverAddress.sin_port = htons(portNo);
  // bind name to socket
  int status = bind(sockfd, (struct sockaddr*) &serverAddress, sizeof(serverAddress));
  if (status == -1) {
    printf("Error: Failed to bind to socket\n");
    return 1;
  }
  // listen for connections
  listen(sockfd, CONNECTION_QUEUE_SIZE);
  // accept connections
  struct sockaddr_in clientAddress;
  int clientLength = sizeof(clientAddress);
  int newsockfd = accept(sockfd, (struct sockaddr*) &clientAddress, &clientLength);
  if (newsockfd == -1) {
    printf("Error: Failed to accept new connection\n");
    return 1;
  }
  
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
  return 0;
}
