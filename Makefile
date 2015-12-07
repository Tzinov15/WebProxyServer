#-Wno-unused-variable
all: webproxy

webproxy: webproxy.c webproxy.h
	
	gcc -Wall -Wno-deprecated-declarations -g webproxy.c -o webproxy

clean:
	rm webproxy

