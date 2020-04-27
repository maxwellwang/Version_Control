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
#include "../zipper.h"

void send_file(int sock, char * filename);

void checkMalloc(void* ptr) {
  if (!ptr) {
    printf("Error: Malloc failed\n");
    exit(1);
  }
}

int writen(int fd, char * buf, int n) {
  int left = n;
  int written = 0;
  
  while (left > 0) {
    if ((written = write(fd, buf, left)) <= 0) {
      if (written < 0 && errno == EINTR) {
	//printf("Error: Write interrupted!\n");
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

// reads input until a space and returns the string
char * read_space(int fd) {
  int num_bytes = 1;
  char * str_bytes = malloc(4096);
  checkMalloc(str_bytes);
  memset(str_bytes, 0, 4096);
  int buf_pos = 0;
  while (num_bytes > 0 && (buf_pos == 0 || str_bytes[buf_pos-1]  != ' ')) {
    num_bytes = read(fd, str_bytes + buf_pos, 1);
    buf_pos++;
  }
  str_bytes[buf_pos-1] = 0;
  return str_bytes;
}

// returns socket fd of stream, exits if fails
int c_connect() {
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

void configure(int argc, char* argv[]) {
  if (argc != 4) {
    printf("Error: Expected 4 args for configure, received %d\n", argc);
    exit(1);
  }
  int fd = open("./.configure", O_WRONLY | O_CREAT, 00600);
  writen(fd, argv[2], strlen(argv[2]));
  writen(fd, " ", 1);
  writen(fd, argv[3], strlen(argv[3]));
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
  writen(sockfd, "61 ", 3); // code 6, 1 arg
  writen(sockfd, argv[2], strlen(argv[2])); // project name
  writen(sockfd, " ", 1); // last space
  if (mkdir(argv[2], 0700) == -1) {
    printf("Error: %s project already exists on client\n", argv[2]);
    return;
  }
  // place received .Manifest into project dir
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
  writen(sockfd, "71 ", 3); // code 7, 1 arg
  writen(sockfd, argv[2], strlen(argv[2])); // project name
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
  		writen(manifest, " 0", 2); // not reflected on server yet
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

}
void currentversion(int argc, char* argv[]) {
  if (argc != 3) {
    printf("Error: Expected 3 args, received %d\n", argc);
    exit(1);
  }

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

int main(int argc, char* argv[]) {
  //TOOD: don't need this check for add/remove
//  if (access("./.configure", F_OK) == -1) {
//    printf("Error: Expected to run configure before this, no .configure file found\n");
//    exit(1);
//  }
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
