#include "webproxy.h"

int main(int argc, char ** argv)
{
  // Check for right number of parameters
  if (argc < 2) {
    printf("Please specify a port number\n");
    exit(1); 
  }

  // set the port number, main socket
  int port_number, main_socket, pid;
  int cli_socket;
  struct sockaddr_in client;
  unsigned int sockaddr_len = sizeof(struct sockaddr_in);
  port_number = atoi(argv[1]);
  main_socket = setup_socket(port_number, MAX_CLIENTS);

  printf("Welcome to the Proxy Server running on port number: %d\n", port_number);
  while (1) {

    if ( (cli_socket = accept(main_socket, (struct sockaddr *)&client, &sockaddr_len)) < 0) {
      perror("ERROR on accept");
      exit(1);
    }

    pid = fork();
    if (pid < 0){
      perror("ERROR on fork");
      exit(1);
    }

    /* The child process will handle individual clients, therefore we can 
     * close the master socket for the childs (clients) address space */
    if (pid == 0) {
      close(main_socket);
      client_handler(cli_socket);
      //close(cli);
      exit(0);
    }

    /* The parent will simply sit in this while look acceptting new clients,
     * it has no need to maintain active sockets with all clients */
    if (pid > 0) {
      close(cli_socket);
      waitpid(0, NULL, WNOHANG);
    }
  }
}



/*------------------------------------------------------------------------------------------------
 * client_handler - this is the function that gets first called after setting up the master socket
 *------------------------------------------------------------------------------------------------*/
void client_handler(int client) {
  ssize_t read_size;
  read_size = 0;
  // client messge is used to store the full original message from client
  char client_message[1024];
  memset(&client_message, 0, sizeof(client_message));
  //printf("About to read the client message...\n");
  read_size = recv(client, client_message, 1024, 0);
  //printf("This is what was just ready from our web proxy server: \n%s\n", client_message);
  extract_request_parameters(client_message, &params);
  //printf("THis is the method extracted from the request: %s\n", params.method);
  //printf("THis is the full URI extracted from the request: %s\n", params.fullURI);
  //printf("THis is the realtiveURI extracted from the request: %s\n", params.relativeURI);
  //printf("THis is the httpversion extracted from the request: %s\n", params.httpversion);
  //printf("THis is the host extracted from the request: %s\n", params.host);
  free(params.method);
  free(params.fullURI);
  free(params.relativeURI);
  free(params.httpversion);
  free(params.host);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * extract_request_parameters - this function will be mainly responsible for parsing and extracting the path from the HTTP request from the client
 *------------------------------------------------------------------------------------------------------------------------------------------- */
void extract_request_parameters(char *response, struct HTTP_RequestParams *params) {
  char *first_line, *rest_of_request, *method, *version, *uri, *host;

  // first_line will contain the first line of the request, rest_of_request will contain the rest 
  first_line = strtok_r(response, "\n", &rest_of_request);
  //printf("This should be the first line of the request: %s\n", first_line);
  //printf("This should be the rest of the request: %s\n", rest_of_request);

  // first_line will contain the first word of the first line of the request 
  first_line = strtok(first_line, " ");
  //printf("This should be the first word of the first line of the request: %s\n", first_line);
  params->method = malloc(strlen(first_line)+1);
  strcpy(params->method, first_line);

  // first_line will contain the second word of the first line of the request 
  first_line = strtok(NULL, " ");
  //printf("This should be the second word of the first line of the request: %s\n", first_line);
  params->fullURI = malloc(strlen(first_line)+1);
  strcpy(params->fullURI, first_line);

  first_line = strtok(NULL, " ");
  // first_line will contain the third word of the first line of the request 
  //printf("This should be the third word of the first line of the request: %s\n", first_line);
  params->httpversion = malloc(strlen(first_line)+1);
  strcpy(params->httpversion, first_line);

  host = strtok(rest_of_request, ":");
  //printf("This should be the first word of the second line of the request: %s\n", host);
  host = strtok(NULL, ":");
  host = strtok(host, "\n");
  deleteSubstring(host, " ");
  //printf("This should be the second word of the second line of the request: %s\n", host);
  params->host = malloc(strlen(host)+1);
  strcpy(params->host, host);

  char back_up_string[strlen(params->fullURI)];
  strcpy(back_up_string, params->fullURI);
  printf("This is the copied full path URI: %s\n", back_up_string);

  deleteSubstring(back_up_string, "http://");
  printf("this should be path of our request without http: %s\n", back_up_string);

  params->host[strlen(params->host)-1] = 0;
  printf("this should be our host name path: %s\n", params->host);

  
  deleteSubstring(back_up_string, params->host);
  printf("And this finally should be just the relative path of our request: %s\n", back_up_string);
  params->relativeURI = malloc(strlen(relativeURI)+1);
  strcpy(params->relativeURI, back_up_string);

}

/*----------------------------------------------------------------------------------------------
 * setup_socket - allocate and bind a server socket using TCP, then have it listen on the port
 *---------------------------------------------------------------------------------------------- */
int setup_socket(int port_number, int max_clients)
{
  printf("Hello from setup_socket\n");
  sleep (1);
  /* The data structure used to hold the address/port information of the server-side socket */
  struct sockaddr_in server;

  /* This will be the socket descriptor that will be returned from the socket() call */
  int sock;

  /* Socket family is INET (used for Internet sockets) */
  server.sin_family = AF_INET;
  /* Apply the htons command to convert byte ordering of port number into Network Byte Ordering (Big Endian) */
  server.sin_port = htons(port_number);
  /* Allow any IP address within the local configuration of the server to have access */
  server.sin_addr.s_addr = INADDR_ANY;
  /* Zero off remaining sockaddr_in structure so that it is the right size */
  memset(server.sin_zero, '\0', sizeof(server.sin_zero));

  /* Allocate the socket */
  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == ERROR) {
    perror("server socket: ");
    exit(-1);
  }


  /* Bind it to the right port and IP using the sockaddr_in structuer pointed to by &server */
  if ((bind(sock, (struct sockaddr *)&server, sizeof(struct sockaddr_in))) == ERROR) {
    perror("could not bind: ");
    exit(-1);
  }

  /* Have the socket listen to a max number of max_clients connections on the given port */
  if ((listen(sock, max_clients)) == ERROR) {
    perror("Listen");
    exit(-1);
  }
  return sock;
}
/*-------------------------------------------------------------------------------------------------------------------------------------------
 * deleteSubstring - this function is a helper function that is used when extracting the path that the client sends a GET request on
 *------------------------------------------------------------------------------------------------------------------------------------------- */
void deleteSubstring(char *original_string,const char *sub_string) {
  while( (original_string=strstr(original_string,sub_string)) )
    memmove(original_string,original_string+strlen(sub_string),1+strlen(original_string+strlen(sub_string)));
}
