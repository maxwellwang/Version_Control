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
  if (opendir(p->args[0]) != NULL) {
    send_proj(socket, p->args[0]);
  } else { //does not exist
    writen(socket, "01 e 0 ", 7);
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
    writen(socket, "01 e 0 ", 7);
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
  writen2(socket, "31 i ", 0);
  send_file(socket, manifestPath);
  //first sent packet^^
  packet * e = parse_request(socket);
  system2("cp ./_wtf_dir/.Commit %s", projectname);
  writen2(socket, "31 t 0 ", 0);
  return;
}
void commit_b(packet * p ) {
}
void push(packet * p, int socket ) {
  DIR* dir = opendir(p->args[0]);
  if (dir == NULL) {
    writen2(socket, "51 e 0 ", 0);
    return;
  } else {
    closedir(dir);
  }
  //ensure .Commit matches
  char path[4096];
  sprintf(path, "./%s/.Commit", p->args[0]);
  char * serverC = readFile(path);
  char * clientC = readFile("./_wtf_dir/.Commit");
  int b = strcmp(serverC, clientC);
  free(serverC);
  free(clientC);

  if (b != 0) {
    writen2(socket, "51 c 0 ", 0);
    return;
  } else {
    writen2(socket, "51 i 0 ", 0);
  }

  //get manifest version to store
  char manPath[4096];
  sprintf(manPath, "%s/.Manifest", p->args[0]);
  char * manifest = readFile(manPath);
  int version = atoi(strtok(manifest, " "));
  version++;
  sprintf(manPath, ".%dv%s", version, p->args[0]);
  //move to backup dir. if push ultimately fails, restore it
  system3("tar -zcf %s %s", manPath, p->args[0]);
  
  //restore it
  //system2("tar -xf .TODOv%s", p->args[2]);
  //system2("rm .TODOv%s", p->args[2]);
  
  //recieve files needed (add and modified)
  packet * p2 = parse_request(socket);
  //update all files from _wtf_dir
  system2("cd _wtf_dir && cp -rfp . ../", 0);
  //delete deleted files
  system2("cd %s && find . -type f -perm 704 -delete", p->args[0]);

  //update history TODO: add client ID to commit
  system2("cd %s && cat .Commit >> .History", p->args[0]);
  //update project/file versions, hashes, status codes

  //put entries to delete into list
  char commitPath[4096];
  char * entries[8192];
  sprintf(commitPath, "%s/.Commit", p->args[0]);
  char * commit = readFile(commitPath);
  char * tok;
  char * path2;
  char code;
  int first = 1;
  int i = 0;
  while (1) {
    tok = strtok(first == 1 ? commit : NULL, " \n");
    first = 0;
    if (tok == NULL) {
      break;
    }
    code = tok[0];
    tok = strtok(NULL, " \n");
    if (code == 'D') { //add path to list
      entries[i] = malloc(strlen(tok) + 1);
      memcpy(entries[i], tok, strlen(tok));
      i++;
    }
    tok = strtok(NULL, " \n");
    tok = strtok(NULL, " \n");
  }
  //go through Manifest and write out entries not needed to be deleted
  i = 0;
  //manifest version
  
  int j, flag;
  int ver = atoi(strtok(manifest, "\n"));
  system3("echo '%d' >> %s/.Manifest2", ++ver, p->args[0]);
  while (1) {
    tok = strtok(NULL, " \n"); //file path
    path2 = tok;
    if (tok == NULL) {
      break;
    }
    printf("2\n");
    code = tok[0];
    tok = strtok(NULL, " \n"); //file version
    flag = 0;
    printf("bye\n");
    //if it is in deleted list, skip it
    for (j = 0; j < i; j++) {
      if (strcmp(entries[j], path2) == 0) {
	strtok(NULL, " \n");
	flag = 1;
      }
    }
    //not in list, write out the regular info
    if (flag == 0) {
      system3("echo -n '%s ' >> %s/.Manifest2 ", path2, p->args[0]);
      version = atoi(tok);
      system3("echo -n '%d ' >> %s/.Manifest2 ", version, p->args[0]);
      tok = strtok(NULL, " \n"); // file hash
      system3("echo '%s' >> %s/.Manifest2 ", tok, p->args[0]);
    }
  }
  //append deleted (add or modify) entries
  system3("sed '/^D/ d' < %s/.Commit > %s/.Commit2", p->args[0], p->args[0]);
  system3("sed 's/^..//' %s/.Commit2 >> %s/.Manifest2", p->args[0], p->args[0]);
  //replace original manifest with new
  system2("cd %s && mv .Manifest2 .Manifest", p->args[0]);
  system2("cd %s && usleep 100000 && rm -f .Commit2 && rm -f .Manifest2", p->args[0]);

  //expire commits
  system2("rm %s/.*Commit", p->args[0]);
  
  //send back success
  writen2(socket, "51 t 0 ", 0);

  free(manifest);
  free(commit);
}

void create(packet * p, int socket) {
  if (mkdir(p->args[0], 0700) == -1) { // dir already exists
    writen2(socket, "61 f 0 ", 0);
    return;
  }
  int pathLength = 12 + strlen(p->args[0]);
  char manifestPath[pathLength];
  sprintf(manifestPath, "./%s/.Manifest", p->args[0]);
  int manifest = open(manifestPath, O_WRONLY | O_CREAT, 00600);
  writen(manifest, "1\n", 2);
  close(manifest);
  writen(socket, "61 t ", 5);
  send_file(socket, manifestPath); // send manifest to client
  return;
}
void destroy(packet * p, int socket) {
  DIR* dir = opendir(p->args[0]);
  if (dir) { // dir exists, delete it
    closedir(dir);
    system2("rm -r %s", p->args[0]);
    writen2(socket, "71 t 0 ", 0);
  } else if (errno == ENOENT) { // project does not exist, command fails
    writen2(socket, "71 e 0 ", 0);
  } else { // failed for some other reason, command fails
    writen2(socket, "71 u 0 ", 0);
  }
  return;
}
void currentversion(packet * p, int socket) {
  if (DEBUG) printf("reached currver\n");
  DIR* dir = opendir(p->args[0]);
  if (dir) { // dir exists, list its files and versions
    writen2(socket, "81 t 0 ", 0);
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
    writen2(socket, "81 e 0 ", 0);
    return;
  } else {
    writen2(socket, "81 u 0 ", 0);
    return;
  }
  return;
}
void history(packet * p, int socket) {
  char* projectname = p->args[0];
  // send error if project doesn't exist
  DIR* dir = opendir(projectname);
  if (!dir) {
    writen2(socket, "91 e 0 ", 0);
    closedir(dir);
    return;
  }
  closedir(dir);
  char historyPath[strlen(projectname) + 15];
  sprintf(historyPath, "./%s/.History", projectname);
  // send error if history doesn't exist in project
  if (access(historyPath, F_OK) == -1) {
    writen2(socket, "91 m 0 ", 0);
    return;
  }
  
  // good to go, send history to client
  if (DEBUG) printf("about to send history\n");
  writen2(socket, "91 t ", 0);
  send_file(socket, historyPath);
  //first sent packet^^
  return;
}
void rollback(packet * p, int socket) {
	char* projectname = parse_dir(p->args[0]);
	DIR* dir = opendir(projectname);
	int version = atoi(p->args[1]);
	if (!dir) { // project doesn't exist on server
    	writen2(socket, "a1 e 0 ", 0);
    	return;
    }
    char replacementDir[4096];
    sprintf(replacementDir, ".%dv%s", version, projectname);
    char replacementDirPath[4096];
    sprintf(replacementDirPath, "%s/%s", projectname, replacementDir);
    char command[4096];
    struct dirent* item = readdir(dir);
    while (item) {
    	if (item->d_type == DT_DIR) {
    		if (strcmp(item->d_name, replacementDir) == 0) { // replacement dir found
    			sprintf(command, "mv %s ..", replacementDirPath);
    			system(command);
    			memset(command, 0, 4096);
    			sprintf(command, "rm -rf %s", projectname);
    			system(command);
    			memset(command, 0, 4096);
    			sprintf(command, "mv %s %s", replacementDir, projectname);
    			system(command);
    			closedir(dir);
    			writen2(socket, "a1 t 0 ", 0);
    			return;
    		}
    	}
    }
    // didn't find it, write error
    writen2(socket, "a1 v 0 ", 0);
    closedir(dir);
  	return;
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
  //  if (DEBUG) printf("Handling request\n");
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
    push(p, socket);
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
    history(p, socket);
    break;
  case 'a':
    rollback(p, socket);
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
  printf("Disconnected from client\n");
  close(sock);
  free(sockfd);
  int i = 0;
  for (; i < p->argc; i++) {
    free((p->args)[i]);
  }
  free(p->args);
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

  }
  
  return 0;
}
