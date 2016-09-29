all:
	gcc -o whoosh whoosh.c -Wall -Werror

clean:
	rm whoosh -f 
