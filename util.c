#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include "util.h"

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

//reads input until a space and returns the string
char * read_space(int socket) {
  int num_bytes;
  char * buf = malloc(4096); // global var now
  checkMalloc(buf);
  memset(buf, 0, 4096);
  int buf_pos = 0;
  while (buf_pos == 0 || buf[buf_pos-1]  != ' ') {
    read(socket, buf + buf_pos, 1);
    //printf("Read: [%c]", buf[buf_pos]);
    buf_pos++;
  }
  buf[buf_pos-1] = 0;
  return buf;
}

//reads input until particular char
char * read_ws(int socket, char c) {
  return NULL;
}


//populates args and argc of packet
void read_args(int socket, packet * p) {
  //printf("in read args func\n");
  char * tmp = read_space(socket);
  int argc = atoi(tmp);
  free(tmp);
  char ** args = malloc(argc * sizeof(char*));
  checkMalloc(args);
  p->argc = argc;
  //printf("will read in %d args\n", argc);
  char * arg;
  int i;
  for (i = 0; i < argc; i++) {
    arg = read_space(socket);
    //printf("read in %s\n", arg);
    args[i] = arg;
  }
  p->args = args;
  return;
}

//reads len bytes into _wtf_tmp_.tgz
void read_to_file(int socket, int len) {
  int fd = open("./_wtf_tar", O_RDWR | O_CREAT, 00600);
  if (fd == -1) {
    printf("Error: could not open file\n");
    exit(1); //only exiting the child
  }
  
  f2f(socket, fd, len);
  close(fd);
}

void checkMalloc(void* ptr) {
  if (!ptr) {
    printf("Error: Malloc failed\n");
    exit(1);
  }
}

char * parse_dir(char * path) {
  if (path[strlen(path) - 1] == '/') {
    path[strlen(path) - 1] = 0;
  }
  return path;
}

void check_args(int given, int target) {
  if (given != target) {
    printf("Error: Expected %d args, received %d\n", target, given);
    exit(1);
  }
}

void system2(char * fstr, char * arg) {
  char buf[4096];
  memset(buf, 0, 4096);
  sprintf(buf, fstr, arg);
  system(buf);
}

void system3(char * fstr, char * arg, char * arg2) {
  char buf[4096];
  memset(buf, 0, 4096);
  sprintf(buf, fstr, arg, arg2);
  system(buf);
}

void writen2(int sock, char * fstr, char * arg) {
  char buf[4096];
  memset(buf, 0, 4096);
  sprintf(buf, fstr, arg);
  writen(sock, buf, strlen(buf));
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

int writen(int fd, char * buf, int n) {
  int left = n;
  int written = 0;
  
  while (left > 0) {
    if ((written = write(fd, buf, left)) <= 0) {
      if (written < 0 && errno == EINTR) {
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

//edits path to only hold dir, returns the file
char * get_dir(char * path) {
  int l = strlen(path) - 1;
  for (; l > 0; l--) {
    if (path[l] == '/') {
      char * path2 = malloc(4096);
      memset(path2, 0, 4096);
      memcpy(path2, path, l);
      return path2;
    }
  }
  return NULL;
}

char * get_file(char * path) {
  int l = strlen(path) - 1;
  for (; l > 0; l--) {
    if (path[l] == '/') {
      char * path2 = malloc(4096);
      memset(path2, 0, 4096);
      memcpy(path2, path+l+1, strlen(path)-(l+1));
      return path2;
    }
  }
  return NULL;
}

void zip_add2(char * path) {
  char * dir = get_dir(path);
  char * file = get_file(path);
  system2("mkdir -p _wtf_dir/%s", dir);
  system3("cp %s ./_wtf_dir/%s", path, dir);
}


//soley for checkout
void send_proj(int sockfd, char * proj) {
  char buf[4096];
  memset(buf, 0, 4096);
  sprintf(buf, "tar -zcf _wtf_tar %s", proj);
  system(buf);
  sprintf(buf, "01 t %d ", 5, zip_size());
  writen(sockfd, buf, strlen(buf));
  int tarfd = open("./_wtf_tar", O_RDONLY);
  f2f(tarfd, sockfd, zip_size());
}

//remove any existing tar/directory
void zip_init() {
  system("rm -r _wtf_dir 2>/dev/null");
  system("rm _wtf_tar 2>/dev/null");
  system("mkdir _wtf_dir 2>/dev/null");
}

//add file to tar
void zip_add(char * path) {
  int len = 25 + strlen(path);
  char * str = malloc(len);
  memset(str, 0, len);
  snprintf(str, len, "cp %s _wtf_dir 2>/dev/null", path);
  system(str);
}

//create tar and remove dir
void zip_tar() {
  system("tar -zcf _wtf_tar _wtf_dir 2>/dev/null");
  system("rm -r _wtf_dir 2>/dev/null");
}

//untar and remove tar, leaving _wtf_dir
void zip_untar() {
  system("rm -r _wtf_dir 2>/dev/null");
  system("tar -xf _wtf_tar 2>/dev/null");
  system("rm -r _wtf_tar 2>/dev/null");
}

//get size of tar (used for when sending)
int zip_size() {
  struct stat st;
  stat("_wtf_tar", &st);
  return st.st_size;
}

//actual function used to send compressed files
void zip_send(int fd, int len, char ** paths) {
  zip_init();
  int i = 0;
  for (i = 0; i < len; i++) {
    zip_add(paths[i]);
  }
  zip_tar();
  int size = zip_size();
}

//whoeverse calling this should free p
packet * parse_request(int socket){
  packet * p = malloc(sizeof(packet));
  checkMalloc(p);
  int len;
  char c = '?';
  read(socket, &c, 1);
  p->code = c;
  read_args(socket, p);
  len = atoi(read_space(socket));
  p->filelen = len;
  if (len > 0) {
    read_to_file(socket, len);
    zip_untar();
  }
  return p;
}

void handle_response(int sock) {
  packet * p = parse_request(sock);
  char * cmd;
  switch (p->code) {
  case '0':
    cmd = "checkout";
    break;
  case '1':
    cmd = "update";
    break;
  case '2':
    cmd = "upgrade";
    break;
  case '3':
    cmd = "commit";
    break;
  case '4':
    break;
  case '5':
    cmd = "push";
    break;
  case '6':
    cmd = "create";
    break;
  case '7':
    cmd = "destroy";
    break;
  case '8':
    cmd = "currentversion";
    break;
  case '9':
    cmd = "history";
    break;
  case 'a':
    cmd = "rollback";
    break;
  }
  if (strcmp(p->args[0], "t") == 0) {
    printf("Command %s succeeded\n", cmd);
    return;
  } else if (strcmp(p->args[0], "i") == 0) {
    printf("Command %s in progress\n", cmd);
    return;
  } else if (strcmp(p->args[0], "e") == 0) {
    printf("Command %s failed: project does not exist on server\n", cmd);
  } else if (strcmp(p->args[0], "e") == 0) {
    printf("Command %s failed: commit does not match server\n", cmd);
  } else if (strcmp(p->args[0], "f") == 0) {
    printf("Command %s failed: project already exists on server\n", cmd);
  } else if (strcmp(p->args[0], "m") == 0) {
    printf("Command %s failed: manifest does not exist on server\n", cmd);
  } else if (strcmp(p->args[0], "u") == 0) {
    printf("Command %s failed: Unknown error", cmd);
  } else if (strcmp(p->args[0], "v") == 0) {
  	printf("Command %s failed: Requested project version does not exist\n", cmd);
  } else {
    printf("Command %s failed, code %s\n", cmd, p->args[0]);
  }
  free(p);
  close(sock);
  printf("Disconnected from server\n");
  exit(1);
}

char * readFile(char * filename) {
  int buff_size = 4096;
  char * huff_buffer = (char*)malloc(buff_size);
  if (!huff_buffer) {
    printf("Error: Malloc failed\n");
    exit(EXIT_FAILURE);
  }
  memset(huff_buffer, 0, 4096);
     
  int huff_read = 0;
  int huff_fd = open(filename, O_RDONLY);
  int huff_status;

  do {
    huff_status = read(huff_fd, huff_buffer+huff_read, buff_size-1-huff_read);
    huff_read += huff_status;
    if (huff_status == 0) {
      break;
    }
    char * huff_tmp = (char*)malloc(buff_size*2);
    if (!huff_tmp) {
      printf("Error: Malloc failed\n");
      exit(EXIT_FAILURE);
    }
    memset(huff_tmp, 0, buff_size*2);
    memcpy(huff_tmp, huff_buffer, huff_read);
    free(huff_buffer);
    huff_buffer = huff_tmp;
    buff_size *= 2;
  }
  while (huff_status > 0);
  close(huff_fd);
  return huff_buffer;
}


/*


MANIFEST STUFF


*/

char* getServerHash(char serverManifestPath[], char inputPath[]) {
  int manifest = open(serverManifestPath, O_RDONLY);
  char filePath[4096], version[10], c = '?';
  char* storedHash = (char*) malloc(33);
  int filePathLength, versionLength, storedHashLength, versionNo;
  int status = read(manifest, &c, 1);
  while (status) {
    // filePath
    filePathLength = 0;
    memset(filePath, 0, 4096);
    // first read already done
    while (c != ' ') {
      filePath[filePathLength++] = c;
      read(manifest, &c, 1);
    }
    // version
    versionLength = 0;
    memset(version, 0, 4096);
    read(manifest, &c, 1);
    while (c != ' ') {
      version[versionLength++] = c;
      read(manifest, &c, 1);
    }
    versionNo = atoi(version);
    // hash
    storedHashLength = 0;
    memset(hash, 0, 4096);
    read(manifest, &c, 1);
    while (c != '\n') {
      storedHash[storedHashLength++] = c;
      read(manifest, &c, 1);
    }
    // line read in, if file path matches then return hash
    if (strcmp(inputPath, filePath) == 0) {
      close(manifest);
      return storedHash;
    }
  	
    status = read(manifest, &c, 1);
  }
  close(manifest);
  return NULL;
}

int getServerFileVersion(char serverManifestPath[], char inputPath[]) {
  int manifest = open(serverManifestPath, O_RDONLY);
  char filePath[4096], version[10], c = '?';
  char storedHash[33];
  int filePathLength, versionLength, storedHashLength, versionNo;
  int status = read(manifest, &c, 1);
  while (status) {
    // filePath
    filePathLength = 0;
    memset(filePath, 0, 4096);
    // first read already done
    while (c != ' ') {
      filePath[filePathLength++] = c;
      read(manifest, &c, 1);
    }
    // version
    versionLength = 0;
    memset(version, 0, 4096);
    read(manifest, &c, 1);
    while (c != ' ') {
      version[versionLength++] = c;
      read(manifest, &c, 1);
    }
    versionNo = atoi(version);
    // hash
    storedHashLength = 0;
    memset(hash, 0, 4096);
    read(manifest, &c, 1);
    while (c != '\n') {
      storedHash[storedHashLength++] = c;
      read(manifest, &c, 1);
    }
    // line read in, if file path matches then return versionNo
    if (strcmp(inputPath, filePath) == 0) {
      close(manifest);
      return versionNo;
    }
  	
    status = read(manifest, &c, 1);
  }
  close(manifest);
  return -1;
}


int fileInManifest(char manifestPath[], char filePath[]) {
  int manifest = open(manifestPath, O_RDONLY);
  char c = '?';
  char tempFilePath[4096];
  memset(tempFilePath, 0, 4096);
  int length = 0;
  int status = read(manifest, &c, 1);
  while (status > 0) {
    if (c == ' ') {
      // read file path and compare to param file path
      memset(tempFilePath, 0, 4096);
      length = 0;
      read(manifest, &c, 1);
      while (c != ' ') {
	tempFilePath[length++] = c;
	read(manifest, &c, 1);
      }
      if (strcmp(tempFilePath, filePath) == 0) { // found it, return true
	close(manifest);
	return 1;
      } else { // read to newline
	while (c != '\n') {
	  read(manifest, &c, 1);
	}
      }
    }
    status = read(manifest, &c, 1); // if file is done, status will be 0
  }
  close(manifest);
  return 0;
}

int checkMA(char serverManifestPath[], char filePath[], int versionNo, char manifestHash[], int sameHash, int commitFile) {
  int serverManifest = open(serverManifestPath, O_RDONLY);
  char c = '?', code = '?';
  char tempFilePath[4096];
  memset(tempFilePath, 0, 4096);
  int length = 0;
  char serverHash[33];
  memset(serverHash, 0, 33);
  int serverHashLength = 0;
  char version[10];
  memset(version, 0, 10);
  int versionLength = 0;
  int serverFileVersionNo;
  read(serverManifest, &c, 1); // skip manifest version
  while (c != '\n') read(serverManifest, &c, 1);
  int status = read(serverManifest, &c, 1);
  while (status > 0) {
    // read file path and compare
    length = 0;
    memset(tempFilePath, 0, 4096);
    read(serverManifest, &c, 1);
    while (c != ' ') {
      tempFilePath[length++] = c;
      read(serverManifest, &c, 1);
    }
    if (strcmp(tempFilePath, filePath) == 0) { // file found
      // read version
      versionLength = 0;
      memset(version, 0, 10);
      read(serverManifest, &c, 1);
      while (c != ' ') {
	version[versionLength++] = c;
	read(serverManifest, &c, 1);
      }
      serverFileVersionNo = atoi(version);
      memset(serverHash, 0, 4096);
      serverHashLength = 0;
      read(serverManifest, &c, 1);
      while (c != '\n') {
	serverHash[serverHashLength++] = c;
	read(serverManifest, &c, 1);
      }
      if (strcmp(manifestHash, serverHash) == 0 && !sameHash) { // modify code detected
	code = 'M';
	break;
      }
      // if server file version is geq client file version, error
      if (serverFileVersionNo >= versionNo) {
	printf("Error: Must sync with repository before committing changes\n");
	return -1;
      }
    } else { // this isn't the same file
      while (c != '\n') read(serverManifest, &c, 1);
    }
    status = read(serverManifest, &c, 1);
  }
  if (!fileInManifest(serverManifestPath, filePath)) { // add code detected
    code = 'A';
  }
  if (code == '?') { // neither modify nor add
    return 0;
  }
  char commitWrite[4096];
  sprintf(commitWrite, "%c %s %d %s\n", code, filePath, versionNo + 1, manifestHash);
  writen(commitFile, commitWrite, strlen(commitWrite));
  printf("%c %s\n", code, filePath);
  close(serverManifest);
  return 0;
}

void checkD(char serverManifestPath[], char clientManifestPath[], int commitFile) {
  int serverManifest = open(serverManifestPath, O_RDONLY);
  int clientManifest = open(clientManifestPath, O_RDONLY);
  char c = '?';
  char commitMessage[4096];
  memset(commitMessage, 0, 4096);
  char tempFilePath[4096];
  memset(tempFilePath, 0, 4096);
  int length = 0;
  char serverHash[33];
  memset(serverHash, 0, 33);
  int serverHashLength = 0;
  char version[10];
  int versionLength = 0;
  int versionNo;
  // read mani version on first line first
  while (c != '\n') read(serverManifest, &c, 1);
  int status = read(serverManifest, &c, 1);
  while (status > 0) {
    // read file path and compare
    length = 0;
    memset(tempFilePath, 0, 4096);
    read(serverManifest, &c, 1);
    while (c != ' ') {
      tempFilePath[length++] = c;
      read(serverManifest, &c, 1);
    }
    // read version
    versionLength = 0;
    memset(version, 0, 10);
    while (c != ' ') {
      version[versionLength++] = c;
      read(serverManifest, &c, 1);
    }
    versionNo = atoi(version);
    // read hash
    serverHashLength = 0;
    memset(serverHash, 0, 4096);
    read(serverManifest, &c, 1);
    while (c != '\n') {
      serverHash[serverHashLength++] = c;
      read(serverManifest, &c, 1);
    }
    if (!fileInManifest(clientManifestPath, tempFilePath)) { // delete code detected 
      memset(commitMessage, 0, 4096);
      sprintf(commitMessage, "D %s %d %s", tempFilePath, versionNo + 1, serverHash);
      writen(commitFile, commitMessage, strlen(commitMessage));
      printf("D %s\n", tempFilePath);
    }
    status = read(serverManifest, &c, 1);
  }
  close(serverManifest);
  close(clientManifest);
  return;
}
