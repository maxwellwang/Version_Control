#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <netinet/in.h>
#include <string.h>

int main(int argc, char* argv[]) {
	// check arg count
	if (argc < 3) {
		printf("Error: Expected at least 3 args, received %d\n", argc);
		return 1;
	}
	// determine command to execute
	if (strcmp(argv[1], "configure") == 0) {
	
	} else if (strcmp(argv[1], "checkout") == 0) {
	
	} else if (strcmp(argv[1], "update") == 0) {
	
	} else if (strcmp(argv[1], "upgrade") == 0) {
	
	} else if (strcmp(argv[1], "commit") == 0) {
	
	} else if (strcmp(argv[1], "push") == 0) {
	
	} else if (strcmp(argv[1], "create") == 0) {
	
	} else if (strcmp(argv[1], "destroy") == 0) {
	
	} else if (strcmp(argv[1], "add") == 0) {
	
	} else if (strcmp(argv[1], "remove") == 0) {
	
	} else if (strcmp(argv[1], "currentversion") == 0) {
	
	} else if (strcmp(argv[1], "history") == 0) {
	
	} else if (strcmp(argv[1], "rollback") == 0) {
	
	} else {
		printf("Error: Expected configure, checkout, update, uprade, commit, push, create, destroy, add, remove, currentversion,"
			" history, or rollback, received %s\n", argv[1]);
		return 1;
	}
	return 0;
}
