#include "webproxy.h"

int main(int argc, char ** argv)
{
  printf("| Welcome to the Proxy Server! |\n");

  // Check for right number of parameters
  if (argc < 2) {
    printf("Please specify a port number\n");
    exit(1); 
  }

  // set the port number
  int port_number;
  port_number = atoi(argv[2]);

  printf("This is the specified port number: %d\n", port_number);
}
