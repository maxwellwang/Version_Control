all: client.c server.c
  gcc -o "WTF" client.c; gcc -o "WTFserver" server.c

test: tester.c
  gcc -o "WTFtest" tester.c

clean:
  rm WTF WTFserver WTFtest
