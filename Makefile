all: clientDir/client.c serverDir/server.c zipper.o
	(cd clientDir; gcc -o "WTF" client.c ../zipper.o; touch hashfile); (cd serverDir; gcc -o "WTFserver" server.c ../zipper.o);

zipper.o: zipper.c
	gcc -c zipper.c

test: tester.c
	gcc -o "WTFtest" tester.c; touch testfile

clean:
	rm -rf clientDir/WTF serverDir/WTFserver WTFtest testfile clientDir/.configure serverDir/myproject zipper.o clientDir/myproject clientDir/hashfile
