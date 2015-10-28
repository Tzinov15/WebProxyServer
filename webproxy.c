#include "webproxy.h"

int main(int argc, char ** argv)
{
  // Check for right number of parameters
  if (argc < 2) {
    printf("Please specify a port number\n");
    exit(1); 
  }

  // set the port number
  int port_number;
  port_number = atoi(argv[1]);

  printf("Welcome to the Proxy Server running on port number: %d\n", port_number);
}
