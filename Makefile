all:
  gcc -o "WTF" client.c; gcc -o "WTFserver" server.c

test: thing.c
  gcc -o "WTFtest" thing.c

clean:
  rm WTF; rm WTFserver
