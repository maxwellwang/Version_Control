all: client.c server.c util.o
	gcc -o "WTF" client.c util.o
	gcc -o "WTFserver" server.c -lpthread util.o

util.o: util.c
	gcc -c util.c

test: clean all tester.c
	gcc -g -o "WTFtest" tester.c
	mkdir clientDir
	mkdir serverDir
	mv WTF clientDir
	mv WTFserver serverDir

clean:
	rm -rf util.o WTF WTFserver WTFtest clientDir serverDir
