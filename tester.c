#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#define INITIAL_BUFFER_SIZE 4096

void checkMalloc(void* ptr) {
	if (!ptr) {
		printf("Malloc failed\n");
		exit(1);
	}
}

// returns # for which command it is
/*
configure 0
checkout 1
update 2
upgrade 3
commit 4
push 5
create 6
destroy 7
add 8
remove 9
currentversion 10
history 11
rollback 12
*/
int executeInput(int file) {
	char* buffer = (char*) malloc(INITIAL_BUFFER_SIZE);
	int length = 0, size = INITIAL_BUFFER_SIZE;
	checkMalloc(buffer);
	bzero(buffer, INITIAL_BUFFER_SIZE);
	char c = '?';
	char* nextBuffer = NULL;
	int status = read(file, &c, 1);
	int charCounter = 1;
	char code[] = "???";
	int getCode = 0;
	int j = 0;
	while (status && c != '\n') {
		if (length + 1 > size) {
			// double size
			size *= 2;
			nextBuffer = (char*) malloc(size);
			checkMalloc(nextBuffer);
			memcpy(nextBuffer, buffer, length);
			free(buffer);
			buffer = nextBuffer;
			nextBuffer = NULL;
		}
		*(buffer+length) = c;
		length++;
		if (length >= 7 && strncmp("./WTF ", buffer + length - 7, 6) == 0) {
			getCode = 3;
		}
		if (getCode > 0) {
			code[j++] = c;
			getCode--;
		}
		status = read(file, &c, 1);
		charCounter++;
	}
	// execute command in buffer and insert output (if any) into testfile
	char command[4096];
	sprintf(command, "(cd clientDir; %s > ../testfile)", buffer); // run in clientDir
	bzero(command + 27 + strlen(buffer), 4096 - 27 - strlen(buffer));
	system(command);
	free(buffer);
	if (strcmp(code, "con") == 0) {
		return 0;
	} else if (strcmp(code, "che") == 0) {
		return 1;
	} else if (strcmp(code, "upd") == 0) {
		return 2;
	} else if (strcmp(code, "upg") == 0) {
		return 3;
	} else if (strcmp(code, "com") == 0) {
		return 4;
	} else if (strcmp(code, "pus") == 0) {
		return 5;
	} else if (strcmp(code, "cre") == 0) {
		sleep(1);
		return 6;
	} else if (strcmp(code, "des") == 0) {
		return 7;
	} else if (strcmp(code, "add") == 0) {
		return 8;
	} else if (strcmp(code, "rem") == 0) {
		return 9;
	} else if (strcmp(code, "cur") == 0) {
		return 10;
	} else if (strcmp(code, "his") == 0) {
		return 11;
	} else if (strcmp(code, "rol") == 0) {
		return 12;
	} else {
		return -1; // shouldn't happen
	}
}

// testfile only useful if the command prints to stdout
int checkOutput(int file, int testfile, int code) {
	char buffer[4096];
	int clientManifest;
	switch (code) {
		case 0: // configure, check if .configure file was made
			if (access("./clientDir/.configure", F_OK) != -1) {
				return 1;
			}
			break;
		case 6: // create, check if client and server have the dir with .Manifest in it
			if (access("./serverDir/myproject/.Manifest", F_OK) != -1 && access("./clientDir/myproject/.Manifest", F_OK) != -1) {
				return 1;
			}
			break;
		case 8: // add, check if client's manifest has the file
			clientManifest = open("./clientDir/myproject/.Manifest", O_RDONLY);
			int bytes = 0;
			int status = read(clientManifest, buffer + bytes, 1);
			char filePath[] = "myproject/myfile";
			while (status > 0) {
				bytes++;
				if (bytes >= 16 && strcmp(filePath, buffer + bytes - 16) == 0) { // found it
					close(clientManifest);
					return 1;
				}
				status = read(clientManifest, buffer + bytes, 1);
			}
			close(clientManifest);
			break;
		default:
			return 0; // shouldn't be here
	}
	return 0;
}

int main(int argc, char* argv[]) {
	char c = '?';
	int size = INITIAL_BUFFER_SIZE;
	int length = 0;
	int status;
	int testCounter = 1;
	int file = open("testcases.txt", O_RDONLY);
	int testfile = open("testfile", O_RDONLY);
	if (file == -1 || testfile == -1) {
		perror("Error");
		return 1;
	}
	int code;
	status = read(file, &c, 1);
	while (status) {
		if (c == '$') {
			read(file, &c, 1); // space
			code = executeInput(file);
			if (checkOutput(file, testfile, code) == 1) {
				printf("Test %d passed\n", testCounter);
			} else {
				printf("Test %d failed\n", testCounter);
			}
			testCounter++;
		}
		status = read(file, &c, 1);
	}
	if (close(file) == -1) {
		perror("Error");
		return 1;
	}
	if (close(testfile) == -1) {
		perror("Error");
		return 1;
	}
	return 0;
}
