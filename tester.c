#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#define INITIAL_BUFFER_SIZE 50

int main(int argc, char* argv[]) {
	int file = open("testcases.txt", O_RDONLY | O_NONBLOCK);
	if (file == -1) {
		perror("Error");
		return EXIT_FAILURE;
	}
	char c = '?';
	char* buffer = (char*) malloc(INITIAL_BUFFER_SIZE);
	if (!buffer) {
		printf("Error: Malloc failed\n");
		return EXIT_FAILURE;
	}
	int size = INITIAL_BUFFER_SIZE;
	char* head = buffer;
	int length = 0;
	char* nextBuffer;
	int status = read(file, &c, 1);
	while (status) {
		if (status == -1) {
			if (errno != EINTR) {
				perror("Error");
				return EXIT_FAILURE;
			}
			// signal interrupted read, try again
			status = read(file, &c, 1);
			continue;
		}
		if (c == 'I') {
			// read command terminated by newline and execute
			status = read(file, &c, 1);
			while (status && c != '\n') {
				if (status == -1) {
					if (errno != EINTR) {
						perror("Error");
						return EXIT_FAILURE;
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
						return EXIT_FAILURE;
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
			
		}
		status = read(file, &c, 1);
	}
	if (close(file) == -1) {
		perror("Error");
		return EXIT_FAILURE;
	}
	free(buffer);
	return EXIT_SUCCESS;
}
