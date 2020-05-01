#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#define DEBUG 0

int codeToNum(char code[]) {
	if (strncmp(code, "con", 3) == 0) {
    return 0;
  } else if (strncmp(code, "che", 3) == 0) {
    return 1;
  } else if (strncmp(code, "upd", 3) == 0) {
    return 2;
  } else if (strncmp(code, "upg", 3) == 0) {
    return 3;
  } else if (strncmp(code, "com", 3) == 0) {
    return 4;
  } else if (strncmp(code, "pus", 3) == 0) {
    return 5;
  } else if (strncmp(code, "cre", 3) == 0) {
    return 6;
  } else if (strncmp(code, "des", 3) == 0) {
    return 7;
  } else if (strncmp(code, "add", 3) == 0) {
    return 8;
  } else if (strncmp(code, "rem", 3) == 0) {
    return 9;
  } else if (strncmp(code, "cur", 3) == 0) {
    return 10;
  } else if (strncmp(code, "his", 3) == 0) {
    return 11;
  } else if (strncmp(code, "rol", 3) == 0) {
    return 12;
  } else {
    return -1; // shouldn't happen
    }
}

int runAndCheck(char command[]) {
  char clientCommand[4096];
  memset(clientCommand, 0, 4096);
  sprintf(clientCommand, "(cd clientDir; %s)", command);
  system(clientCommand);
  if (strncmp("./WTF", command, 5) != 0 && strncmp(" ./WTF", command, 6) != 0) return 1; // not WTF command
  // get args
  int argc = 0;
  char argv[10][4096];
  // Extract the first token
  char copy[4096];
  strncpy(copy, command, 4096);
  char * token = strtok(copy, " ");
  // loop through the string to extract all other tokens
  while( token != NULL ) {
  	strcpy(argv[argc++], token);
  	token = strtok(NULL, " ");
  }
  char code3[4];
  memcpy(code3, argv[1], 3);
  code3[3] = '\0';
  int code = codeToNum(code3);
  if (code != 0) { // make sure projectname doesn't end in /
  	if (argv[1][strlen(argv[1]) - 1] == '/') {
  		argv[1][strlen(argv[1]) - 1] = 0;
  	}
  }
  char buffer[4096];
  char filePath[4096];
  char serverProjectPath[4096];
  char clientManifestPath[4096];
  char serverManifestPath[4096];
  char serverCommitPath[4096];
  char clientCommitPath[4096];
  int clientManifest, bytes, status;
  switch (code) {
  case 0: // configure, check if .configure file was made
    if (access("./clientDir/.configure", F_OK) != -1) {
      return 1;
    }
    break;
  case 1: //same checks as create
  	sprintf(serverManifestPath, "./serverDir/%s/.Manifest", argv[2]);
  	sprintf(clientManifestPath, "./clientDir/%s/.Manifest", argv[2]);
    if (access(serverManifestPath, F_OK) != -1 && access(clientManifestPath, F_OK) != -1) {
      return 1;
    }
    break;
  case 4:
  	sprintf(serverCommitPath, "./serverDir/%s/.Commit", argv[2]);
  	sprintf(clientCommitPath, "./clientDir/%s/.Commit", argv[2]);
    if (access(clientCommitPath, F_OK) != -1 && access(serverCommitPath, F_OK) != -1) return 1;
    break;
  case 6: // create, check if client and server have the dir with .Manifest in it
    sprintf(serverManifestPath, "./serverDir/%s/.Manifest", argv[2]);
    sprintf(clientManifestPath, "./clientDir/%s/.Manifest", argv[2]);
    if (access(serverManifestPath, F_OK) != -1 && access(clientManifestPath, F_OK) != -1) {
      return 1;
    }
    break;
  case 7: // destroy, make sure dir in server is gone
  	sprintf(serverProjectPath, "./serverDir/%s", argv[2]);
  	DIR* dir = opendir(serverProjectPath);
    if (dir) {
    	closedir(dir);
      return 0;
    }
    closedir(dir);
    return 1;
  case 8: // add, check if client's manifest has the file
  	sprintf(filePath, "%s/%s", argv[2], argv[3]);
  	sprintf(clientManifestPath, "./clientDir/%s/.Manifest", argv[2]);
    clientManifest = open(clientManifestPath, O_RDONLY);
    bytes = 0;
    status = read(clientManifest, buffer + bytes, 1);
    bytes++;
    while (status > 0) {
      if (bytes >= 16 && strncmp(filePath, buffer + bytes - 16, 16) == 0) { // found it
	close(clientManifest);
	return 1;
      }
      status = read(clientManifest, buffer + bytes, 1);
      bytes++;
    }
    close(clientManifest);
    break;
  case 9: // remove, check if client manifest lacks the file
  	sprintf(filePath, "%s/%s", argv[2], argv[3]);
  	sprintf(clientManifestPath, "./clientDir/%s/.Manifest", argv[2]);
    clientManifest = open(clientManifestPath, O_RDONLY);
    bytes = 0;
    status = read(clientManifest, buffer + bytes, 1);
    bytes++;
    while (status > 0) {
      if (bytes >= 16 && strncmp(filePath, buffer + bytes - 16, 16) == 0) { // found it
	close(clientManifest);
	return 0;
      }
      status = read(clientManifest, buffer + bytes, 1);
      bytes++;
    }
    close(clientManifest);
    return 1;
    break;
  case 10: // currentversion
    break;
  default:
    return 0; // shouldn't be here
  }
  return 0;
}

int main() {
  int testCounter = 1;
  int file = open("testcases.txt", O_RDONLY);
  char c = '?';
  char command[4096];
  int commandLength = 0;
  int numCommands;
  int status = read(file, &c, 1); 
  while (status) {
    if (c == '$') {
    	printf("Test %d:\n", testCounter);
    	read(file, &c, 1); // space
    	numCommands = 0;
      	// parse commands and check results
	  	while (c != '\n') {
	  		numCommands++;
		    // read until ; or \n to get command
		    memset(command, 0, 4096);
		    commandLength = 0;
		    //if (numCommands > 1) read(file, &c, 1); // extra space
	        read(file, &c, 1);
	        while (c != '\n' && c != ';') {
		      	command[commandLength++] = c;
		      	read(file, &c, 1);
		    }
		    printf ("Command: %s\n", command);
		    if (runAndCheck(command) == 1) {
		    	printf("Command successful\n");
		    } else {
		    	printf("Command failed\n");
		    	printf("Test %d failed\n", testCounter);
		    	return EXIT_FAILURE;
		    }
		 }
		 printf("Test %d passed\n\n\n", testCounter);
      	 testCounter++;
    }
    status = read(file, &c, 1);
  }
  close(file);
  return EXIT_SUCCESS;
}
