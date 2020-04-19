#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>



int configure(int argc, char* argv[]) {
}
int update(int argc, char* argv[]) {
}
int upgrade(int argc, char* argv[]) {
}
int commit(int argc, char* argv[]) {
}
int push(int argc, char* argv[]) {
}
int create(int argc, char* argv[]) {
}
int destroy(int argc, char* argv[]) {
}
int add(int argc, char* argv[]) {
}
int c_remove(int argc, char* argv[]) {
}
int currentversion(int argc, char* argv[]) {
}
int history(int argc, char* argv[]) {
}
int rollback(int argc, char* argv[]) {
}



//returns socket fd of stream, exits if fails
int c_connect() { 
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (socket == -1) {
    printf("Socket creation failed\n");
    exit(0);
  }
  
  struct sockaddr_in serverAddress;
  bzero(&serverAddress, sizeof(serverAddress));
  serverAddress.sin_family = AF_INET;
  //TODO: read ip and port from a file, and maybe even hostname?
  serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");
  serverAddress.sin_port = htons(8080);

  while (connect(sockfd, (struct sockaddr *) &serverAddress, sizeof(serverAddress))) {
    printf("Connecting to server failed, attemping again in 3s\n");
    sleep(3);
  }
  
  return sockfd;
}


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

int main(int argc, char* argv[]) {
  //debug tests
  int sockfd = c_connect();
  printf("Connected\n");
  char *testbuf = "t10 asdkfja d difjisd e ajisdj f e f f 0 10 abcabcabcd";
  int sent = writen(sockfd, testbuf, strlen(testbuf));
  printf("Sent message %d\n", sent);
  return 1;
  
  // check arg count
  if (argc < 3) {
    printf("Error: Expected at least 3 args, received %d\n", argc);
    return 1;
  }
  // determine command to execute
  if (strcmp(argv[1], "configure") == 0) {
    configure(argc, argv);
  } else if (strcmp(argv[1], "checkout") == 0) {
    checkout(argc, argv);
  } else if (strcmp(argv[1], "update") == 0) {
    configure(argc, argv);
  } else if (strcmp(argv[1], "upgrade") == 0) {
    upgrade(argc, argv);
  } else if (strcmp(argv[1], "commit") == 0) {
    commit(argc, argv);
  } else if (strcmp(argv[1], "push") == 0) {
    push(argc, argv);
  } else if (strcmp(argv[1], "create") == 0) {
    create(argc, argv);
  } else if (strcmp(argv[1], "destroy") == 0) {
    destroy(argc, argv);
  } else if (strcmp(argv[1], "add") == 0) {
    add(argc, argv);
  } else if (strcmp(argv[1], "remove") == 0) {
    c_remove(argc, argv);
  } else if (strcmp(argv[1], "currentversion") == 0) {
    currentversion(argc, argv);
  } else if (strcmp(argv[1], "history") == 0) {
    history(argc, argv);
  } else if (strcmp(argv[1], "rollback") == 0) {
    rollback(argc, argv);
  } else {
    printf("Error: Expected configure, checkout, update, uprade, commit, push, create, destroy, add, remove, currentversion,"
	   " history, or rollback, received %s\n", argv[1]);
    return 1;
  }
  return 0;
}
