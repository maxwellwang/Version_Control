#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>


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

//just for testing
//int main() {
//  zip_init();
//  zip_add("tartest");
//  zip_add("tartest2");
//  zip_tar();
//  printf("Size: [%d]\n", zip_size());
//  return 0;
//}
//
