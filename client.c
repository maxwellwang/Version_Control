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
#include "util.h"
#define DEBUG 0

char * id;

// returns socket fd of stream, exits if fails
int c_connect() {
  if (access(".id", F_OK) == -1) { //generate ID for commit and push
    system2("printf `date +%%N` > .id", 0);
  }
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
  	close(configfd);
    printf("Error: Socket creation failed\n");
    exit(1);
  }


  char * hostbuffer = read_space(configfd);
  int portno = atoi(read_space(configfd));
  close(configfd);
  struct hostent* result = gethostbyname(hostbuffer);
  if (result == NULL) {
  	close(sockfd);
    herror("Error");
    exit(1);
  }
  struct sockaddr_in serverAddress;
  bzero(&serverAddress, sizeof(serverAddress));
  serverAddress.sin_family = AF_INET;
  serverAddress.sin_port = htons(portno);
  bcopy((char*)result->h_addr, (char*)&serverAddress.sin_addr.s_addr, result->h_length);
  int status = connect(sockfd, (struct sockaddr *) &serverAddress, sizeof(serverAddress));
  while (status) {
  	if (status < 0) {
  		//close(sockfd);
  		perror("Error");
  		exit(1);
  	}
    printf("Connecting to server failed, attemping again in 3s\n");
    sleep(3);
    status = connect(sockfd, (struct sockaddr *) &serverAddress, sizeof(serverAddress));
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

    //remove files like .History and .Commits
    system2("cd %s && rm -rf .History .*Commit", argv[2]);
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
    printf("Error: %s project does not exist on client\n", projectname);
    exit(1);
  }
  closedir(dir);
  int socket = c_connect();
  writen2(socket, "11 %s 0 ", argv[2]);
  handle_response(socket);
  // manifest fetched successfully, now compare to determine case
  int serverManifest = open("./._wtf_dir/.Manifest", O_RDONLY);
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
  // match, write blank .Update, delete .Conflict, and stop
  if (strcmp(clientManifestVersion, serverManifestVersion) == 0) { 
    if (access(updatePath, F_OK) != -1) remove(updatePath);
    updateFile = open(updatePath, O_WRONLY | O_CREAT, 00600);
    close(updateFile);
    if (access(conflictPath, F_OK) != -1) remove(conflictPath);
    printf("Up To Date\n");
    close(serverManifest);
    close(clientManifest);
    close(conflictFile);
    remove("./._wtf_dir/.Manifest");
    return;
  }
  
  // begin comparing manifests
  int conflictDetected = 0;
  unsigned char liveHash[33];
  char filePath[4096], version[4096], storedHash[4096], c = '?', updateMessage[4096], conflictMessage[4096];
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
    // read stored hash
    storedHashLength = 0;
    memset(storedHash, 0, 4096);
    read(clientManifest, &c, 1);
    while (c != '\n') {
      storedHash[storedHashLength++] = c;
      read(clientManifest, &c, 1);
    }
    // line read in, check modify, delete, and conflict
    if (!fileInManifest("./._wtf_dir/.Manifest", filePath)) { // delete detected
      printf("D %s\n", filePath);
      memset(updateMessage, 0, 4096);
      sprintf(updateMessage, "D %s %s\n", filePath, storedHash);
      writen(updateFile, updateMessage, strlen(updateMessage));
    } else { // file in server manifest
      if (versionNo != getServerFileVersion("./._wtf_dir/.Manifest", filePath)) {
	if (strcmp(getServerHash("./._wtf_dir/.Manifest", filePath), storedHash) != 0 && strcmp(liveHash, storedHash) == 0) {
	  // modify detected
	  printf("M %s\n", filePath);
	  memset(updateMessage, 0, 4096);
	  sprintf(updateMessage, "M %s %s\n", filePath, getServerHash("./._wtf_dir/.Manifest", filePath));
	  writen(updateFile, updateMessage, strlen(updateMessage));
	}
      } else if (strcmp(storedHash, liveHash) != 0 && strcmp(storedHash, getServerHash("./._wtf_dir/.Manifest", filePath)) != 0) {
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
    memset(storedHash, 0, 4096);
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
    close(updateFile);
    remove(updatePath);
    close(conflictFile);
    printf("You must resolve conflicts before you can update\n");
    remove("./._wtf_dir/.Manifest");
    return;
  } else { close(conflictFile); remove(conflictPath);}
  close(updateFile);
  close(conflictFile);
  printf("Command .Update succeeded!");
  remove("./._wtf_dir/.Manifest");
  return;
}

void upgrade(int argc, char* argv[]) {
  check_args(argc, 3);
  char path[4096];
  sprintf(path, "%s/.Conflict", argv[2]);
  if (access(path, F_OK) != -1) {
    printf("Error: Conflicts exist, upgrade failed\n");
    exit(1);
  }
  sprintf(path, "%s/.Update", argv[2]);
  if (access(path, F_OK) == -1) {
    printf("Error: No update found, upgrade failed\n");
    exit(1);
  }
  
  //request files from server
  //send server the .Update 
  int sockfd = c_connect();
  writen2(sockfd, "21 %s ", argv[2]);
  send_file(sockfd, path);
  //tar with all needed things
  handle_response(sockfd);
  //update all files from ._wtf_dir, also has the manifest
  system2("cd ._wtf_dir && cp -rfp . ../", 0);
  system2("rm -rf %s", path);
  close(sockfd);
}

void commit(int argc, char* argv[]) {
  check_args(argc, 3);
  // argv[2] project name get rid of / if there is one
  char* projectname = parse_dir(argv[2]);
  // fail if project does not exist
  DIR* dir = opendir(projectname);
  if (!dir) {
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
  if(access(path, F_OK) != -1) {
    char * update = readFile(path);
    if (strlen(update) > 0) {
      printf("Error: .Update exists and is nonempty\n");
      exit(1);	  
    }
  }
  if (access(path2, F_OK) != -1) {
    printf("Error: %s exists\n", path2);
    exit(1);
  	
  }
  // fetch server project's .Manifest
  int sockfd = c_connect();
  writen2(sockfd, "31 %s 0 ", projectname);
  handle_response(sockfd);

  // manifest fetched successfully, now compare to determine case
  int serverManifest = open("./._wtf_dir/.Manifest", O_RDONLY);
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
    writen(sockfd, "31 z 0 ", 7);
    close(sockfd);
    printf("Disconnected from server\n");
    exit(1);
  }
  // compute live hashes for client project's files
  char commitPath[4096];
  char * id = readFile(".id");
  sprintf(commitPath, "./%s/.%sCommit", projectname, id);
  if (access(commitPath, F_OK) != -1) { // alredy exists, remove first so we can create new one
    remove(commitPath);
  }
  int commitFile = open(commitPath, O_WRONLY | O_CREAT, 00600);
  char version[10];
  int versionLength = 0;
  int versionNo;
  char filePath[4096];
  int filePathLength = 0;
  unsigned char hashcode[4096]; // the live hash
  char manifestHash[33];
  int manifestHashLength = 0;
  int sameHash = 1;
  int sa;
  int status = read(clientManifest, &clientC, 1);
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
    sameHash = (strcmp(hashcode, manifestHash) == 0);
    // decide modify, add, or nothing
    sa = checkMA("./._wtf_dir/.Manifest", filePath, versionNo, manifestHash, sameHash, commitFile, hashcode);
    if (sa == -1) { // not synced before commit, delete .Commit
      close(commitFile);
      close(serverManifest);
      close(clientManifest);
      close(sockfd);
      printf("Disconnected from server\n");
      remove(commitPath);
      remove("./._wtf_dir/.Manifest");
      return;
    }
    status = read(clientManifest, &clientC, 1);
  }
  // check for delete...
  checkD("./._wtf_dir/.Manifest", manifestPath, commitFile);
  close(commitFile);
  close(serverManifest);
  close(clientManifest);

  char commitPath2[4096];
  sprintf(commitPath2, "%s/.%sCommit", argv[2], id);
  char * commit2 = readFile(commitPath2);
  if (strlen(commit2) <= 0) {
    printf("No changes detected\n");
    system2("rm %s", commitPath2);
    writen2(sockfd, "31 n 0 ", 0);
    free(id);
    close(sockfd);
    return;
  }
  // send .Commit to server and declare success
  writen2(sockfd, "32 d %s ", id);
  send_file(sockfd, commitPath);
  handle_response(sockfd);
  free(id);
  close(sockfd);
  printf("Disconnected from server\n");
  remove("./._wtf_dir/.Manifest");
  return;
}

void push(int argc, char* argv[]) {
  check_args(argc, 3);
  char* projectname = parse_dir(argv[2]);
  // fail if project does not exist
  DIR* dir = opendir(projectname);
  if (!dir) {
    printf("Error: %s project does not exist on client\n", projectname);
    exit(1);
  }
  
  closedir(dir);
  char commitPath[4096];
  char * id = readFile(".id");
  sprintf(commitPath, "%s/.%sCommit", argv[2], id);
  if(access(commitPath, F_OK) == -1) {
    printf("Error: no commits found\n");
    return;
  }
  char * commit = readFile(commitPath);
  if (strlen(commit) <= 0) {
    printf("Error: nothing to commit!\n");
    return;
  }
  
  //create and send tar w/ the files
  int sock = c_connect();
  char buf[4096];
  sprintf(buf, "52 %s %s ", argv[2], id);
  writen2(sock, buf, 0);
  send_file(sock, commitPath);
  //initial check of project & commit
  handle_response(sock);
  //send files over
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
    //ensure permissions allow us to delete it
    if (code == 'D') {
      system2("chmod 704 ./._wtf_dir/%s", path);
    }
  }
  zip_tar();
  writen2(sock, "51 %s ", id);
  char * size = malloc(64);
  sprintf(size, "%d", zip_size());
  writen(sock, size, strlen(size));
  free(size);
  writen(sock, " ", 1);
  int tarfd = open("./._wtf_tar", O_RDONLY);
  f2f(tarfd, sock, zip_size());
  close(tarfd);
  free(commit);
  
  //remove .Commit
  system3("rm %s/.%sCommit", argv[2], id);
  free(id);  
  //update manifest on push success
  handle_response(sock);
  system2("cp ./._wtf_dir/.Manifest %s 2>/dev/null", argv[2]);
  close(sock);
  printf("Disconnected from server\n");
  return;
}
void create(int argc, char* argv[]) {
  check_args(argc, 3);
  char* projectname = parse_dir(argv[2]);
  DIR* dir = opendir(projectname);
  if (dir) {
  	closedir(dir);
    printf("Error: %s project already exists on client\n", argv[2]);
    return;
  }
  
  int sockfd = c_connect();
  writen2(sockfd, "61 %s 0 ", projectname);
  handle_response(sockfd);
  if (mkdir(projectname, 0700) == -1) {
    printf("Error: could not create project %s\n", argv[2]);
    return;
  }
  system2("mv ./._wtf_dir/.Manifest %s", projectname);
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
  char manifestPath[13 + strlen(projectname)];
  sprintf(manifestPath, "./%s/.Manifest", projectname);
  char filePath[4 + strlen(projectname) + strlen(argv[3])];
  sprintf(filePath, "./%s/%s", projectname, argv[3]);
  DIR* dir = opendir(projectname);
  if (dir) { // dir exists, add to manifest if file isn't there yet
  	closedir(dir);
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
      writen(manifest, " 0 ", 3); // version 0
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
  DIR* dir = opendir(projectname);
  if (dir) { // dir exists
  		closedir(dir);
      int manifest = open(manifestPath, O_RDONLY);
      int temp = open(tempPath, O_WRONLY | O_CREAT, 00600); // create temp to write everything except deleted line
      if (manifest == -1) {
	printf("Error: Failed to open %s\n", manifestPath);
	return;
      }
      if (temp == -1) {
      close(manifest);
	printf("Error: Failed to create %s\n", tempPath);
	return;
      }
      char c = '?';
      char filename[4096];
      int length = 0;
      int status = read(manifest, &c, 1);
      while (c != '\n') {
	writen(temp, &c, 1);
	read(manifest, &c, 1);
      }
      writen(temp, "\n", 1);
      status = read(manifest, &c, 1);
      while (status > 0) {
      	// first read projectname and /
      	while (c != '/') read(manifest, &c, 1);
      	// now read in file name
      	memset(filename, 0, 4096);
      	length = 0;
      	read(manifest, &c, 1);
      	while (c != ' ') {
      		filename[length++] = c;
      		read(manifest, &c, 1);
      	}
      	if ( strcmp(filename, argv[3]) == 0 ) { // file found, skip it and print the rest, then replace manifest
      		while (c != '\n') read(manifest, &c, 1);
      		status = read(manifest, &c, 1);
      		while (status > 0) {
      			writen(temp, &c, 1);
      			status = read(manifest, &c, 1);
      		}
      		close(manifest);
      		close(temp);
      		remove(manifestPath);
      		rename(tempPath, manifestPath);
		printf("Successfully removed file\n");
      		return;
      	}
      	// not found, print the line and continue to next line
      	writen(temp, projectname, strlen(projectname));
      	writen(temp, "/", 1);
      	writen(temp, filename, strlen(filename));
      	writen(temp, " ", 1);
      	read(manifest, &c, 1);
      	while (c != '\n') {
      		writen(temp, &c, 1);
      		read(manifest, &c, 1);
      	}
      	writen(temp, "\n", 1);
      	status = read(manifest, &c, 1);
      }
      // finished reading manifest and file not found
      close(manifest);
      close(temp);
      remove(tempPath);
      printf("Error: %s is not in manifest\n", filePath);
      return;
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
  close(sockfd);
  printf("Disconnected from server\n");
  // manifest is in wtf
  int manifest = open ("./._wtf_dir/.Manifest", O_RDONLY);
  char c = '?';
  while (c != '\n') {
    read(manifest, &c, 1);
    printf("%c", c);	
  }
  int status = read(manifest, &c, 1);
  while (status > 0) {
  	while (c != ' ') { // reads file path
  		printf("%c", c);
  		read(manifest, &c, 1);
  	}
  	printf(" ");
  	read(manifest, &c, 1);
  	while (c != ' ') { // reads version No
  		printf("%c", c);
  		read(manifest, &c, 1);
  	}
  	printf("\n");
  	read(manifest, &c, 1);
  	while (c != '\n') { // skips hash
  		read(manifest, &c, 1);
  	}
  	status = read(manifest, &c, 1);
  }
  close(manifest);
  remove("./._wtf_dir/.Manifest");
  return;
}

void history(int argc, char* argv[]) {
  check_args(argc, 3);
  int socket = c_connect();
  // argv[2] project name get rid of / if there is one
  char* projectname = parse_dir(argv[2]);
  writen2(socket, "91 %s 0 ", projectname);
  // read and output info
  handle_response(socket);
  close(socket);
  system2("cat ./._wtf_dir/.History", 0);
  return;
}

void rollback(int argc, char* argv[]) {
  check_args(argc, 4);
  char* projectname = parse_dir(argv[2]);
  int socket = c_connect();
  char buf[4096];
  sprintf(buf,"a2 %s %s 0 ", projectname, argv[3]);
  writen2(socket, buf, 0);
  handle_response(socket);
  close(socket);
  return;
}


int main(int argc, char* argv[]) {
  if (argc < 2) {
    printf("Error: please enter a command\n");
  }
  // determine command to execute
  if (strcmp(argv[1], "configure") == 0) {
    configure(argc, argv);
  } else if (strcmp(argv[1], "checkout") == 0) {
    checkout(argc, argv);
  } else if (strcmp(argv[1], "update") == 0) {
    update(argc, argv);
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
  system("rm -r ._wtf_dir > /dev/null 2>&1");
  system("rm ._wtf_tar > /dev/null 2>&1");
  usleep(1000*100);
  return 0;
}
