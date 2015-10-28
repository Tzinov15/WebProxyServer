#-Wno-unused-variable
all: webproxy

webproxy: webproxy.c webproxy.h
	
	gcc -lssl -lcrypto -Wall  -Wno-deprecated-declarations -g webproxy.c -lpthread -o webproxy

clean:
	rm webproxy

