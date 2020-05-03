all: clientDir/client.c serverDir/server.c util.o
	(cd clientDir; gcc -g -o "WTF" client.c ../util.o; touch hashfile); (cd serverDir; gcc -g -o "WTFserver" server.c -lpthread ../util.o);

util.o: util.c
	gcc -c util.c

test: tester.c
	gcc -o "WTFtest" tester.c

clean:
	rm -rf clientDir/WTF serverDir/WTFserver WTFtest clientDir/.configure serverDir/myproject* util.o clientDir/myproject* clientDir/hashfile clientDir/_* serverDir/_* serverDir/.*v* *~ clientDir/.*
