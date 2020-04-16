#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#define INITIAL_BUFFER_SIZE 50

// global vars
char c = '?';
int size = INITIAL_BUFFER_SIZE;
int length = 0;
char* nextBuffer;
int status;
char inputTerminator = '\n';
char outputTerminator = '$';
int testCounter = 1;

// functions
void executeInput(int file, char* buffer, char* head) {
	status = read(file, &c, 1);
	while (status && c != inputTerminator) {
		if (status == -1) {
			if (errno != EINTR) {
				perror("Error");
				exit(1);
			}
			status = read(file, &c, 1);
			continue;
		}
		if (length + 1 > size) {
			// double size
			size *= 2;
			nextBuffer = (char*) malloc(size);
			if (!buffer) {
				printf("Error: Malloc failed\n");
				exit(1);
			}
			memcpy(nextBuffer, buffer, length);
			free(buffer);
			buffer = nextBuffer;
			nextBuffer = NULL;
			head = buffer + length;
		}
		*head = c;
		head++;
		length++;
		status = read(file, &c, 1);
	}
	// execute command in buffer and insert into testfile
	char insertPart[] = " > testfile";
	int i;
	for (i = 0; i < 11; i++) {
		if (length + 1 > size) {
			// double size
			size *= 2;
			nextBuffer = (char*) malloc(size);
			if (!buffer) {
				printf("Error: Malloc failed\n");
				exit(1);
			}
			memcpy(nextBuffer, buffer, length);
			free(buffer);
			buffer = nextBuffer;
			nextBuffer = NULL;
			head = buffer + length;
		}
		*head = insertPart[i];
		head++;
		length++;
	}
	system(buffer);
	// reset everything
	head = buffer;
	length = 0;
	return;
}
int matchesOutput(int file, int testfile, char* buffer) {
	int ret = 1;
	status = read(file, &c, 1);
	char c2 = '?';
	int status2 = read(testfile, &c2, 1);
	while (status && c != outputTerminator && status2) {
		if (status == -1) {
			if (errno != EINTR) {
				perror("Error");
				exit(1);
			}
			status = read(file, &c, 1);
			continue;
		}
		if (status2 == -1) {
			if (errno != EINTR) {
				perror("Error");
				exit(1);
			}
			status2 = read(testfile, &c2, 1);
			continue;
		}
		if (c != c2) {
			ret = 0; // different char detected
		}
		status = read(file, &c, 1);
		status2 = read(testfile, &c2, 1);
	}
	return ret;
}

int main(int argc, char* argv[]) {
	int file = open("testcases.txt", O_RDONLY | O_NONBLOCK);
	char* buffer = (char*) malloc(INITIAL_BUFFER_SIZE);
	char* head = buffer;
	int testfile = open("testfile", O_RDONLY | O_NONBLOCK);
	if (file == -1 || testfile == -1) {
		perror("Error");
		return 1;
	}
	if (!buffer) {
		printf("Error: Malloc failed\n");
		return 1;
	}
	status = read(file, &c, 1);
	while (status) {
		if (status == -1) {
			if (errno != EINTR) {
				perror("Error");
				return 1;
			}
			// signal interrupted read, try again
			status = read(file, &c, 1);
			continue;
		}
		if (c == '$') {
			executeInput(file, buffer, head);
			if (matchesOutput(file, testfile, buffer)) {
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
	free(buffer);
	return 0;
}
