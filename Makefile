all: something.c somethingelse.c
	gcc -o "WTF" something.c; gcc -o "WTFserver" somethingelse.c

test: anotherthing.c
	gcc -o "WTFtest" anotherthing.c

clean:
	rm WTF; rm WTFserver
