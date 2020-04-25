all: clientDir/client.c serverDir/server.c
	gcc -o "WTF" clientDir/client.c; mv WTF clientDir; gcc -o "WTFserver" serverDir/server.c; mv WTFserver serverDir

test: clientDir/tester.c
	gcc -o "WTFtest" clientDir/tester.c; touch testfile; mv WTFtest clientDir; mv testfile clientDir

clean:
	rm -f clientDir/WTF serverDir/WTFserver clientDir/WTFtest clientDir/testfile clientDir/.configure
