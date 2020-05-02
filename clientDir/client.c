#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include "../util.h"
#define DEBUG 0

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
  char hostbuffer[256]; 
  char *IPbuffer; 
  struct hostent *host_entry; 
  int hostname; 
  // To retrieve hostname 
  hostname = gethostname(hostbuffer, sizeof(hostbuffer));  
  // To retrieve host information 
  host_entry = gethostbyname(hostbuffer);
  // To convert an Internet network address into ASCII string 
  IPbuffer = inet_ntoa(*((struct in_addr*) host_entry->h_addr_list[0]));
  serverAddress.sin_addr.s_addr = inet_addr(IPbuffer);
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

void checkout(int argc, char* argv[]) {
  check_args(argc, 3);
  DIR * dir = opendir(argv[2]);
  if (dir != NULL) {
    printf("Error: project already exists on client\n");
    closedir(dir);
    exit(1);
  } else { //does not exist
    int sock = c_connect();
    writen2(sock, "01 %s 0 ", argv[2]);

    handle_response(sock);
    close(sock);
    printf("Disconnected from server\n");
  }  
}

void configure(int argc, char* argv[]) {
  check_args(argc, 4);
  int fd = open("./.configure", O_WRONLY | O_CREAT, 00600);
  writen2(fd, "%s ", argv[2]);
  writen2(fd, "%s ", argv[3]);
  close(fd);
  printf("Successfully configured\n");
  return;
}


void update(int argc, char* argv[]) {
  check_args(argc, 3);
  // argv[2] project name get rid of / if there is one
  char* projectname = parse_dir(argv[2]);
  // fail if project does not exist
  DIR* dir = opendir(projectname);
  if (!dir) {
    closedir(dir);
    printf("Error: %s project does not exist on client\n", projectname);
    exit(1);
  }
  closedir(dir);
  int socket = c_connect();
  packet* p = parse_request(socket);
  if (strcmp(p->args[0], "p") == 0) {
    printf("Project %s does not exist on server\n", projectname);
    free(p);
    close(socket);
    printf("Disconnected from server\n");
    exit(1);
  } else if (strcmp(p->args[0], "m") == 0) {
    printf("Project %s Manifest does not exist on server\n", projectname);
    free(p);
    close(socket);
    printf("Disconnected from server\n");
    exit(1);
  }
  close(socket);
  printf("Disconnected from server\n");
  free(p);
  // manifest fetched successfully, now compare to determine case
  int serverManifest = open("./_wtf_dir/.Manifest", O_RDONLY);
  char manifestPath[13 + strlen(projectname)];
  sprintf(manifestPath, "./%s/.Manifest", projectname);
  char updatePath[15+ strlen(projectname)];
  sprintf(updatePath, "./%s/.Update", projectname);
  char conflictPath[15+ strlen(projectname)];
  sprintf(conflictPath, "./%s/.Conflict", projectname);
  int clientManifest = open(manifestPath, O_RDONLY);
  // check if manifest versions match
  char clientManifestVersion[100], serverManifestVersion[100];
  char clientC, serverC;
  int clientManifestVersionLength = 0, serverManifestVersionLength = 0;
  read(clientManifest, &clientC, 1);
  read(serverManifest, &serverC, 1);
  while (clientC != '\n') {
    clientManifestVersion[clientManifestVersionLength++] = clientC;
    read(clientManifest, &clientC, 1);
  }
  while (serverC != '\n') {
    serverManifestVersion[serverManifestVersionLength++] = serverC;
    read(serverManifest, &serverC, 1);
  }
  int updateFile = open(updatePath, O_WRONLY | O_CREAT | O_APPEND, 00600);
  int conflictFile = open(conflictPath, O_WRONLY | O_CREAT | O_APPEND, 00600);
  if (strcmp(clientManifestVersion, serverManifestVersion) == 0) { // match, write blank .Update, delete .Conflict, and stop
    if (access(updatePath, F_OK) != -1) remove(updatePath);
    updateFile = open(updatePath, O_WRONLY | O_CREAT, 00600);
    close(updateFile);
    if (access(conflictPath, F_OK) != -1) remove(conflictPath);
    printf("Up To Date\n");
    close(serverManifest);
    close(clientManifest);
    remove("./_wtf_dir/.Manifest");
    return;
  }
  // begin comparing manifests
  int conflictDetected = 0;
  unsigned char liveHash[33];
  char filePath[4096], version[10], storedHash[33], c = '?', updateMessage[4096], conflictMessage[4096];
  int filePathLength, versionLength, storedHashLength, versionNo;
  int status = read(clientManifest, &c, 1);
  while (status) {
    // filePath
    filePathLength = 0;
    memset(filePath, 0, 4096);
    // first read already done
    while (c != ' ') {
      filePath[filePathLength++] = c;
      read(clientManifest, &c, 1);
    }
    // compute live hash
    hash(filePath, liveHash);
    // version
    versionLength = 0;
    memset(version, 0, 4096);
    read(clientManifest, &c, 1);
    while (c != ' ') {
      version[versionLength++] = c;
      read(clientManifest, &c, 1);
    }
    versionNo = atoi(version);
    // hash
    storedHashLength = 0;
    memset(hash, 0, 4096);
    read(clientManifest, &c, 1);
    while (c != '\n') {
      storedHash[storedHashLength++] = c;
      read(clientManifest, &c, 1);
    }
    // line read in, check modify, delete, and conflict
    if (!fileInManifest("./_wtf_dir/.Manifest", filePath)) { // delete detected
      printf("D %s\n", filePath);
      memset(updateMessage, 0, 4096);
      sprintf(updateMessage, "D %s %s\n", filePath, storedHash);
      writen(updateFile, updateMessage, strlen(updateMessage));
    } else { // file in server manifest
      if (versionNo != getServerFileVersion("./_wtf_dir/.Manifest", filePath)) {
	if (strcmp(getServerHash("./_wtf_dir/.Manifest", filePath), storedHash) != 0 && strcmp(liveHash, storedHash) == 0) {
	  // modify detected
	  printf("M %s\n", filePath);
	  memset(updateMessage, 0, 4096);
	  sprintf(updateMessage, "M %s %s\n", filePath, getServerHash("./_wtf_dir/.Manifest", filePath));
	  writen(updateFile, updateMessage, strlen(updateMessage));
	}
      } else if (strcmp(storedHash, liveHash) != 0 && strcmp(storedHash, getServerHash("./_wtf_dir/.Manifest", filePath)) != 0) {
	// conflict detected
	conflictDetected = 1;
	printf("C %s\n", filePath);
	memset(conflictMessage, 0, 4096);
	sprintf(conflictMessage, "C %s %s\n", filePath, liveHash);
	writen(conflictFile, conflictMessage, strlen(conflictMessage));
      }
    }
    status = read(clientManifest, &c, 1);
  }
  // check add
  status = read(serverManifest, &c, 1); // first char of first filePath
  while (status) {
    // filePath
    filePathLength = 0;
    memset(filePath, 0, 4096);
    // first read already done
    while (c != ' ') {
      filePath[filePathLength++] = c;
      read(serverManifest, &c, 1);
    }
    // version
    versionLength = 0;
    memset(version, 0, 4096);
    read(serverManifest, &c, 1);
    while (c != ' ') {
      version[versionLength++] = c;
      read(serverManifest, &c, 1);
    }
    versionNo = atoi(version);
    // hash
    storedHashLength = 0;
    memset(hash, 0, 4096);
    read(serverManifest, &c, 1);
    while (c != '\n') {
      storedHash[storedHashLength++] = c;
      read(serverManifest, &c, 1);
    }
    // line read in, if file is not in clientManifest then detect add
    if (!fileInManifest(manifestPath, filePath)) { // add detected
      printf("A %s\n", filePath);
      memset(updateMessage, 0, 4096);
      sprintf(updateMessage, "A %s %s\n", filePath, storedHash);
      writen(updateFile, updateMessage, strlen(updateMessage));
    }
    status = read(serverManifest, &c, 1);
  }
  close(serverManifest);
  close(clientManifest);
  if (conflictDetected) { // delete .Update, print message
    remove(updatePath);
    close(conflictFile);
    printf("You must resolve conflicts before you can update\n");
    remove("./_wtf_dir/.Manifest");
    return;
  }
  close(updateFile);
  close(conflictFile);
  remove("./_wtf_dir/.Manifest");
  return;
}
void upgrade(int argc, char* argv[]) {
  check_args(argc, 3);
}

void commit(int argc, char* argv[]) {
  check_args(argc, 3);
  // argv[2] project name get rid of / if there is one
  char* projectname = parse_dir(argv[2]);
  // fail if project does not exist
  DIR* dir = opendir(projectname);
  if (!dir) {
    closedir(dir);
    printf("Error: %s project does not exist on client\n", projectname);
    exit(1);
  }
  closedir(dir);
  // fail if there is nonempty .Update file or there is .Conflict file 
  struct stat stat_record;
  char path[11 + strlen(projectname)];
  sprintf(path, "./%s/.Update", projectname);
  char path2[13 + strlen(projectname)];
  sprintf(path2, "./%s/.Conflict", projectname);
  if(access(path, F_OK) != -1 && stat_record.st_size > 1) {
    printf("Error: %s exists and is nonempty\n", path);
    exit(1);
  }
  if (access(path2, F_OK) != -1) {
    printf("Error: %s exists\n", path2);
    exit(1);
  	
  }
  // fetch server project's .Manifest
  int sockfd = c_connect();
  writen2(sockfd, "31 %s 0 ", projectname);
  if (DEBUG) printf("sent request for server manifest\n");
  handle_response(sockfd);
  if (DEBUG) printf("got server manifest\n");

  // manifest fetched successfully, now compare to determine case
  int serverManifest = open("./_wtf_dir/.Manifest", O_RDONLY);
  char manifestPath[13 + strlen(projectname)];
  sprintf(manifestPath, "./%s/.Manifest", projectname);
  int clientManifest = open(manifestPath, O_RDONLY);
  // check if manifest versions match
  char clientManifestVersion[100], serverManifestVersion[100];
  char clientC, serverC;
  int clientManifestVersionLength = 0, serverManifestVersionLength = 0;
  read(clientManifest, &clientC, 1);
  read(serverManifest, &serverC, 1);
  while (clientC != '\n') {
    clientManifestVersion[clientManifestVersionLength++] = clientC;
    read(clientManifest, &clientC, 1);
  }
  while (serverC != '\n') {
    serverManifestVersion[serverManifestVersionLength++] = serverC;
    read(serverManifest, &serverC, 1);
  }
  if (strcmp(clientManifestVersion, serverManifestVersion) != 0) { // no match, exit
    printf("Error: Manifest versions do not match, update local project first\n");
    close(serverManifest);
    close(clientManifest);
    close(sockfd);
    printf("Disconnected from server\n");
    exit(1);
  }
  // compute live hashes for client project's files
  int status = read(clientManifest, &clientC, 1);
  char commitPath[11 + strlen(projectname)];
  sprintf(commitPath, "./%s/.Commit", projectname);
  if (access(commitPath, F_OK) != -1) { // alredy exists, remove first so we can create new one
    remove(commitPath);
  }
  int commitFile = open(commitPath, O_WRONLY | O_CREAT, 00600);
  char version[10];
  int versionLength = 0;
  int versionNo;
  char filePath[4096];
  int filePathLength = 0;
  unsigned char hashcode[4096];
  char manifestHash[33];
  int manifestHashLength = 0;
  int sameHash = 1;
  int sa;
  while (status > 0) {
    // read into filePath
    memset(filePath, 0, 4096);
    filePathLength = 0;
    while (clientC != ' ') {
      filePath[filePathLength++] = clientC;
      read(clientManifest, &clientC, 1);
    }
    // read into VersionNo
    memset(version, 0, 10);
    versionLength = 0;
    read(clientManifest, &clientC, 1);
    while (clientC != ' ') {
      version[versionLength++] = clientC;
      read(clientManifest, &clientC, 1);
    }
    versionNo = atoi(version);
    // at hash, compute live hash and compare
    hash(filePath, hashcode); // assigns hash to hashcode
    read(clientManifest, &clientC, 1);
    memset(manifestHash, 0, 33);
    manifestHashLength = 0;
    while (clientC != '\n') {
      manifestHash[manifestHashLength++] = clientC;
      read(clientManifest, &clientC, 1);
    }
    sameHash = !(strcmp(hashcode, manifestHash));
    // decide modify, add, or nothing
    sa = checkMA("./_wtf_dir/.Manifest", filePath, versionNo, manifestHash, sameHash, commitFile);
    if (sa == -1) { // not synced before commit, delete .Commit
      close(commitFile);
      close(serverManifest);
      close(clientManifest);
      close(sockfd);
      printf("Disconnected from server\n");
      remove(commitPath);
      remove("./_wtf_dir/.Manifest");
      return;
    }
    status = read(clientManifest, &clientC, 1);
    while (status > 0 && isspace(clientC)) { // read until non whitespace or end of file
      status = read(clientManifest, &clientC, 1);
    }
  }
  // check for delete...
  checkD("./_wtf_dir/.Manifest", manifestPath, commitFile);
  close(commitFile);
  close(serverManifest);
  close(clientManifest);
  // send .Commit to server and declare success
  writen(sockfd, "31 d ", 5);
  send_file(sockfd, commitPath);
  handle_response(sockfd);
  close(sockfd);
  printf("Disconnected from server\n");
  remove("./_wtf_dir/.Manifest");
  return;
}

void push(int argc, char* argv[]) {
  check_args(argc, 3);
  char commitPath[4096];
  sprintf(commitPath, "%s/.Commit", argv[2]);

  //create and send tar w/ the files
  char * commit = readFile(commitPath);
  if (strlen(commit) <= 0) {
    printf("Error: nothing to commit!\n");
    return;
  }
  
  if(access(commitPath, F_OK) == -1) {
    printf("Error: no commits found\n");
    return;
  }
  int sock = c_connect();
  writen2(sock, "51 %s ", argv[2]);
  send_file(sock, commitPath);
  //initial check of project & commit
  handle_response(sock);
  //send files over
  printf("Got here\n");
  

  char * tok;
  char code;
  char * path;
  char * hash;
  int version;
  int first = 1;
  zip_init();
  while (1) {
    tok = strtok(first == 1 ? commit : NULL, " \n");
    first = 0;
    if (tok == NULL) {
      break;
    }
    code = tok[0];
    tok = strtok(NULL, " \n");
    path = tok;
    tok = strtok(NULL, " \n");
    version = atoi(tok);
    tok = strtok(NULL, " \n");
    hash = tok;
    //    printf("C[%c]P[%s]V[%d]H[%s]\n", code, path, version, hash);
    zip_add2(path);
    //set permissions so we know to delete
    if (code == 'A') {
      system2("chmod 700 ./_wtf_dir/%s", path);
    }
  }
  zip_tar();
  writen2(sock, "50 ", 0);
  char * size = malloc(64);
  memset(size, 0, 64);
  sprintf(size, "%d", zip_size());
  writen(sock, size, strlen(size));
  free(size);
  writen(sock, " ", 1);
  int tarfd = open("./_wtf_tar", O_RDONLY);
  f2f(tarfd, sock, zip_size());
  close(tarfd);
  free(commit);
  return;
    

  
  //recieve success message
  handle_response(sock);
  close(sock);
  printf("Disconnected from server\n");
  return;
}
void create(int argc, char* argv[]) {
  check_args(argc, 3);
  char* projectname = parse_dir(argv[2]);
  if (opendir(projectname) != NULL) {
    printf("Error: %s project already exists on client\n", argv[2]);
    return;
  }
  
  int sockfd = c_connect();
  if (mkdir(projectname, 0700) == -1) {
    printf("Error: could not create project %s\n", argv[2]);
    return;
  }
  writen2(sockfd, "61 %s 0 ", projectname);

  handle_response(sockfd);  
  system2("mv ./_wtf_dir/.Manifest %s", projectname);
  close(sockfd);
  printf("Disconnected from server\n");
}

void destroy(int argc, char* argv[]) {
  check_args(argc, 3);
  int sockfd = c_connect();
  char* projectname = parse_dir(argv[2]);
  writen2(sockfd, "71 %s 0 ", projectname);
  handle_response(sockfd);
  close(sockfd);
  printf("Disconnected from server\n");
}

void add(int argc, char* argv[]) {
  check_args(argc, 4);
  // argv[2] project name get rid of / if there is one
  char* projectname = parse_dir(argv[2]);
  if (DEBUG) printf("projectname: %s\n", projectname);
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
      writen(manifest, projectname, strlen(projectname)); // project name
      writen(manifest, "/", 1);
      writen(manifest, argv[3], strlen(argv[3])); // file name
      writen(manifest, " 1 ", 3); // version 1
      unsigned char hashcode[4096];
      hash(filePath, hashcode);
      writen(manifest, hashcode, 32); // hashcode
      writen(manifest, "\n", 1);
      close(manifest);
      printf("Successfully added file %s to project %s manifest\n", argv[3], argv[2]);
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
  check_args(argc, 4);
  // argv[2] project name get rid of / if there is one
  char* projectname = parse_dir(argv[2]);
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
	    printf("Successfully removed file %s from project %s\n", argv[3], argv[2]);
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
  check_args(argc, 3);
  int sockfd = c_connect();
  // argv[2] project name get rid of / if there is one
  char* projectname = parse_dir(argv[2]);
  writen2(sockfd, "81 %s 0 ", projectname);
  // read and output info
  handle_response(sockfd);
  
  char c = '?';
  int status = read(sockfd, &c, 1);
  while (status > 0) {
    printf("%c", c);
    status = read(sockfd, &c, 1);
  }
  close(sockfd);
  printf("Disconnected from server\n");
  return;
}

void history(int argc, char* argv[]) {
  check_args(argc, 3);
  int socket = c_connect();
  // argv[2] project name get rid of / if there is one
  char* projectname = parse_dir(argv[2]);
  writen2(socket, "91 %s 0 ", projectname);
  // read and output info
  packet* p = parse_request(socket);
  free(p);
  close(socket);
  int history = open("./_wtf_dir/.History", O_RDONLY);
  // print the history file
  char c = '?';
  int status = read(history, &c, 1);
  while (status > 0) {
  	printf("%c", c);
  	status = read(history, &c, 1);
  }
  close(history);
  remove("./_wtf_dir/.History");
  return;
}

void rollback(int argc, char* argv[]) {
  check_args(argc, 4);
  char* projectname = parse_dir(argv[2]);
  int socket = c_connect();
  writen2(socket, "a1 %s 0 ", projectname);
  handle_response(socket);
  close(socket);
  return;
}


int main(int argc, char* argv[]) {
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
  //sleep for 100 ms
  usleep(1000*100);
  return 0;
}
