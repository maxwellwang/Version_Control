all: clientDir/client.c serverDir/server.c
	gcc -o "WTF" clientDir/client.c; mv WTF clientDir; gcc -o "WTFserver" serverDir/server.c; mv WTFserver serverDir

test: tester.c
	gcc -o "WTFtest" tester.c; touch testfile

clean:
	rm -f clientDir/WTF serverDir/WTFserver WTFtest testfile clientDir/.configure
