all: client.c server.c
	gcc -o "WTF" client.c; gcc -o "WTFserver" server.c

test: tester.c
	gcc -o "WTFtest" tester.c; touch testfile

clean:
	rm -f WTF WTFserver WTFtest testfile .configure
