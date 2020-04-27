#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include "../util.h"

// returns socket fd of stream, exits if fails
int c_connect() {
  if (access("./.configure", F_OK) == -1) {
    printf("Error: Expected to run configure before this, no .configure file found\n");
    exit(1);
  }
  int configfd = open("./.configure", O_RDONLY);
  if (configfd == -1) {
  	printf("Error: Expected to run configure before this, no .configure file found\n");
	exit(1);
  }
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd == -1) {
    printf("Error: Socket creation failed\n");
    exit(1);
  }
  
  struct sockaddr_in serverAddress;
  bzero(&serverAddress, sizeof(serverAddress));
  serverAddress.sin_family = AF_INET;
  //TODO: read ip and port from a file, and maybe even hostname?
  char* buffer = read_space(configfd);
  serverAddress.sin_addr.s_addr = inet_addr(buffer);
  int portno = atoi(read_space(configfd));
  serverAddress.sin_port = htons(portno);
  close(configfd);
  while (connect(sockfd, (struct sockaddr *) &serverAddress, sizeof(serverAddress))) {
    printf("Connecting to server failed, attemping again in 3s\n");
    sleep(3);
  }
  printf("Connected to server\n");
  return sockfd;
}


void configure(int argc, char* argv[]) {
  if (argc != 4) {
    printf("Error: Expected 4 args for configure, received %d\n", argc);
    exit(1);
  }
  int fd = open("./.configure", O_WRONLY | O_CREAT, 00600);
  writen(fd, argv[2], strlen(argv[2]));
  writen(fd, " ", 1);
  writen(fd, argv[3], strlen(argv[3]));
  writen(fd, " ", 1);
  close(fd);
  return;
}
void update(int argc, char* argv[]) {
  if (argc != 3) {
    printf("Error: Expected 3 args, received %d\n", argc);
    exit(1);
  }

}
void upgrade(int argc, char* argv[]) {
  if (argc != 3) {
    printf("Error: Expected 3 args, received %d\n", argc);
    exit(1);
  }

}
void commit(int argc, char* argv[]) {
  if (argc != 3) {
    printf("Error: Expected 3 args, received %d\n", argc);
    exit(1);
  }

}
void push(int argc, char* argv[]) {
  if (argc != 3) {
    printf("Error: Expected 3 args, received %d\n", argc);
    exit(1);
  }

}
void create(int argc, char* argv[]) {
  if (argc != 3) {
    printf("Error: Expected 3 args, received %d\n", argc);
    exit(1);
  }
  int sockfd = c_connect();
  // argv[2] project name get rid of / if there is one
  char* projectname = argv[2];
  if (argv[2][strlen(argv[2]) - 1] == '/') {
  	projectname[strlen(argv[2]) - 1] = '\0';
  }
  writen(sockfd, "61 ", 3); // code 6, 1 arg
  writen(sockfd, projectname, strlen(projectname)); // project name
  writen(sockfd, " ", 1); // last space
  writen(sockfd, "0 ", 2); //no file
  if (mkdir(projectname, 0700) == -1) {
    printf("Error: %s project already exists on client\n", argv[2]);
    return;
  }
  // place received .Manifest into project dir
  packet * p = parse_request(sockfd);
  char buf[4096];
  sprintf(buf, "cp ./_wtf_dir/.Manifest %s", projectname);
  system(buf);
  close(sockfd);
  printf("Disconnected from server\n");
  return;
}
void destroy(int argc, char* argv[]) {
  if (argc != 3) {
    printf("Error: Expected 3 args, received %d\n", argc);
    exit(1);
  }
  int sockfd = c_connect();
  // argv[2] project name get rid of / if there is one
  char* projectname = argv[2];
  if (argv[2][strlen(argv[2]) - 1] == '/') {
  	projectname[strlen(argv[2]) - 1] = '\0';
  }
  writen(sockfd, "71 ", 3); // code 7, 1 arg
  writen(sockfd, projectname, strlen(projectname)); // project name
  writen(sockfd, " ", 1); // last space
  close(sockfd);
  printf("Disconnected from server\n");
  return;
}
void add(int argc, char* argv[]) {
  if (argc != 4) {
    printf("Error: Expected 4 args, received %d\n", argc);
    exit(1);
  }
  // argv[2] project name get rid of / if there is one
  char* projectname = argv[2];
  if (argv[2][strlen(argv[2]) - 1] == '/') {
  	projectname[strlen(argv[2]) - 1] = '\0';
  }
  char manifestPath[13 + strlen(projectname)];
  sprintf(manifestPath, "./%s/.Manifest", projectname);
  char filePath[4 + strlen(projectname) + strlen(argv[3])];
  sprintf(filePath, "./%s/%s", projectname, argv[3]);
  if (opendir(projectname)) { // dir exists, add to manifest if file isn't there yet
  	if (access(filePath, F_OK) != -1) { // file exists, try to add to manifest
  		int manifest = open(manifestPath, O_RDWR | O_APPEND);
  		if (manifest == -1) {
  			printf("Error: Failed to open %s\n", manifestPath);
  			return;
  		}
  		char c = '?';
  		char path[4096];
  		int length = 0;
  		int status = read(manifest, &c, 1);
  		while (status > 0) {
  			if (c == '/') {
  				status = read(manifest, &c, 1);
  				while (c != ' ') { // get file path to compare
  					*(path + length) = c;
  					length++;
  					status = read(manifest, &c, 1);
  				}
  				if (strcmp(argv[3], path) == 0) { // file already in manifest
  					printf("Error: %s file is already in manifest\n", argv[3]);
  					return;
  				} else { // not the same file, keep going
  					length = 0;
  					memset(path, 0, 4096);
  				}
  			}
  			status = read(manifest, &c, 1);
  		}
  		// add file to manifest
  		writen(manifest, "1 ", 2); // version 1
  		writen(manifest, projectname, strlen(projectname)); // project name
  		writen(manifest, "/", 1);
  		writen(manifest, argv[3], strlen(argv[3])); // file name
  		writen(manifest, " ", 1);
  		unsigned char hashcode[4096];
  		hash(filePath, hashcode);
  		writen(manifest, hashcode, 32); // hashcode
  		writen(manifest, "\n", 1);
  		close(manifest);
  	} else { // file does not exist (this also checks if project exists), command fails
  		printf("Error: %s does not exist\n", filePath);
  		return;
  	}
  } else if (errno == ENOENT) { // dir does not exit, command fails
  	printf("Error: %s project does not exist on client\n", projectname);
   	return;
  } else { // opendir failed for some other reason, command fails
  	perror("Error");
  	return;
  }
  return;
}
void c_remove(int argc, char* argv[]) {
  if (argc != 4) {
    printf("Error: Expected 4 args, received %d\n", argc);
    exit(1);
  }
  // argv[2] project name get rid of / if there is one
  char* projectname = argv[2];
  if (argv[2][strlen(argv[2]) - 1] == '/') {
  	projectname[strlen(argv[2]) - 1] = '\0';
  }
  char manifestPath[13 + strlen(projectname)];
  sprintf(manifestPath, "./%s/.Manifest", projectname);
  char tempPath[9 + strlen(projectname)];
  sprintf(tempPath, "./%s/.temp", projectname);
  char filePath[4 + strlen(projectname) + strlen(argv[3])];
  sprintf(filePath, "./%s/%s", projectname, argv[3]);
  if (opendir(projectname)) { // dir exists, add to manifest if file isn't there yet
  	if (access(filePath, F_OK) != -1) { // file exists, try to remove from manifest
  		int manifest = open(manifestPath, O_RDONLY);
  		int temp = open(tempPath, O_WRONLY | O_CREAT, 00600); // create temp to write everything except deleted line
  		if (manifest == -1) {
  			printf("Error: Failed to open %s\n", manifestPath);
  			return;
  		}
  		if (temp == -1) {
  			printf("Error: Failed to create %s\n", tempPath);
  			return;
  		}
  		char c = '?';
  		char path[4096];
  		char firstPart[4096];
  		int length = 0;
  		int firstPartLength = 0;
  		int status = read(manifest, &c, 1);
  		while (c != '\n') {
  			writen(temp, &c, 1);
  			read(manifest, &c, 1);
  		}
  		writen(temp, "\n", 1);
  		status = read(manifest, &c, 1);
  		while (status > 0) {
  			if (c == '/') {
  				status = read(manifest, &c, 1);
  				while (c != ' ') { // get file path to compare
  					*(path + length) = c;
  					length++;
  					*(firstPart + firstPartLength) = c;
  					firstPartLength++;
  					status = read(manifest, &c, 1);
  				}
  				if (strcmp(argv[3], path) == 0) { // file to delete found, skip it for temp and finish
  					while (c != '\n') {
  						read(manifest, &c, 1);
  					}
  					status = read(manifest, &c, 1);
  					while (status > 0) {
  						writen(temp, &c, 1);
  						read(manifest, &c, 1);
  					}
  					// replace manifest with temp
  					remove(manifestPath);
  					rename(tempPath, manifestPath);
  					return;
  				} else { // not the same file, add the line to temp
  					writen(temp, firstPart, firstPartLength);
  					writen(temp, " ", 1);
  					read(manifest, &c, 1);
  					while (c != '\n') {
  						writen(temp, &c, 1);
  						read(manifest, &c, 1);
  					}
  					length = 0;
  					memset(path, 0, 4096);
  					firstPartLength = 0;
  					memset(firstPart, 0, 4096);
  				}
  			} else {
  				*(firstPart + firstPartLength) = c;
  				firstPartLength++;
  			}
  			status = read(manifest, &c, 1);
  		}
  		// finished reading manifest and file not found
  		close(manifest);
  		close(temp);
  		remove(tempPath);
  		printf("Error: %s is not in manifest\n", filePath);
  		return;
  	} else { // file does not exist (this also checks if project exists), command fails
  		printf("Error: %s does not exist\n", filePath);
  		return;
  	}
  } else if (errno == ENOENT) { // dir does not exit, command fails
  	printf("Error: %s project does not exist on client\n", projectname);
   	return;
  } else { // opendir failed for some other reason, command fails
  	perror("Error");
  	return;
  }
  return;
}
void currentversion(int argc, char* argv[]) {
  if (argc != 3) {
    printf("Error: Expected 3 args, received %d\n", argc);
    exit(1);
  }
  int sockfd = c_connect();
  // argv[2] project name get rid of / if there is one
  char* projectname = argv[2];
  if (argv[2][strlen(argv[2]) - 1] == '/') {
  	projectname[strlen(argv[2]) - 1] = '\0';
  }
  writen(sockfd, "81 ", 3);
  writen(sockfd, projectname, strlen(projectname));
  return;
}
void history(int argc, char* argv[]) {
  if (argc != 3) {
    printf("Error: Expected 3 args, received %d\n", argc);
    exit(1);
  }

}
void rollback(int argc, char* argv[]) {
  if (argc != 4) {
    printf("Error: Expected 4 args, received %d\n", argc);
    exit(1);
  }
}


int main(int argc, char* argv[]) {
  //debug tests
  /*
    int sockfd = c_connect();
    printf("Connected\n");
    char *testbuf = "t10 asdkfja d difjisd e ajisdj f e f f 0 ";//10 abcabcabcd";
    int sent = writen(sockfd, testbuf, strlen(testbuf));
    printf("Sent message %d\n", sent);
    //sendint file
    zip_init();
    zip_add("tartest");
    zip_add("tartest2");
    zip_tar();
    char * size = malloc(64);
    memset(size, 0, 64);
    sprintf(size, "%d", zip_size());
    writen(sockfd, size, strlen(size));
    writen(sockfd, " ", 1);
    int tarfd = open("./_wtf_tar", O_RDONLY);
    f2f(tarfd, sockfd, zip_size());
    return 1;
  */
  // check arg count
  if (argc < 3) {
    printf("Error: Expected at least 3 args, received %d\n", argc);
    return 1;
  }
  // determine command to execute
  if (strcmp(argv[1], "configure") == 0) {
    configure(argc, argv);
  } else if (strcmp(argv[1], "checkout") == 0) {
    //checkout(argc, argv);
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
