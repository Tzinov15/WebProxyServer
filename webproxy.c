#include "webproxy.h"

int main(int argc, char ** argv)
{
  // Check for right number of parameters
  if (argc < 2) {
    printf("Please specify a port number\n");
    exit(1); 
  }

  // set the port number, main socket
  int port_number, main_socket, pid, cli_socket;
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
  ssize_t read_size = 0;
  struct HTTP_RequestParams params;
  
  // client messge is used to store the full original message from client
  char client_message[1024], client_message_copy[1024], remote_request[1024];
  memset(&client_message, 0, sizeof(client_message));
  memset(&client_message_copy, 0, sizeof(client_message_copy));
  memset(&remote_request, 0, sizeof(remote_request));

  read_size = recv(client, client_message, 1024, 0);
  //strcpy(client_message_copy, client_message);
  extract_request_parameters(client_message, &params);
  printf("This is the method extracted from the request: %s\n", params.method);
  printf("This is the full URI extracted from the request: %s\n", params.fullURI);
  printf("This is the realtiveURI extracted from the request: %s\n", params.relativeURI);
  printf("This is the httpversion extracted from the request: %s\n", params.httpversion);
  printf("This is the host extracted from the request: %s\n", params.host);

  construct_new_request(remote_request, client_message_copy, &params);
  
  // Free all the strings allocated for the HTTP params struct
  free(params.method);
  free(params.fullURI);
  free(params.relativeURI);
  free(params.httpversion);
  free(params.host);
}
void construct_new_request(char *request, char *message, struct HTTP_RequestParams *params) {
  char *first_line, *rest_of_request;
  printf("\n");
  printf("\n");
  printf("This is the old client request that I got from the client: \n%s\n", message);
  /*
  first_line = strtok_r(message, "\n", &rest_of_request);
  message+=(strlen(first_line));
  printf("This should be the rest of the body of the message: \n%s\n", message);
  printf("As should this: \n%s\n", rest_of_request);
  */
  strncpy(request, params->method, strlen(params->method));
  strncat(request, " ", 1);
  strncat(request, params->relativeURI, strlen(params->relativeURI));
  strncat(request, " ", 1);
  strncat(request, params->httpversion, strlen(params->httpversion));
  printf("This is the first line for the new request: \n%s\n", request);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * extract_request_parameters - this function will be mainly responsible for parsing and extracting the path from the HTTP request from the client
 *------------------------------------------------------------------------------------------------------------------------------------------- */
void extract_request_parameters(char *response, struct HTTP_RequestParams *params) {
  printf("hello\n");
  char *first_line, *rest_of_request, *host;

  // first_line will contain the first line of the request, rest_of_request will contain the rest 
  first_line = strtok_r(response, "\n", &rest_of_request);
  printf("poop1\n");

  // first_line now contains the first word of the first line of the request 
  first_line = strtok(first_line, " ");
  params->method = malloc(strlen(first_line)+1);
  strcpy(params->method, first_line);
  printf("poop2\n");

  // first_line now contains the second word of the first line of the request 
  first_line = strtok(NULL, " ");
  params->fullURI = malloc(strlen(first_line)+1);
  strcpy(params->fullURI, first_line);
  printf("poop3\n");

  // first_line now contains the third word of the first line of the request 
  first_line = strtok(NULL, " ");
  params->httpversion = malloc(strlen(first_line)+1);
  strcpy(params->httpversion, first_line);
  printf("poop4\n");

  // host will contain the first word of the second line of the request
  host = strtok(rest_of_request, ":");
  // host will contain the second word of the second line of the request
  host = strtok(NULL, "\n");

  // remove the first character 
  host++;
  // remove the last character
  host[strlen(host)-1] = 0;
  printf("poop5\n");

  params->host = malloc(strlen(host)+1);
  strcpy(params->host, host);
  printf("poop6\n");

  // this string will hold the relative url of the file requested
  char relative_url[strlen(params->fullURI)];

  // start out by copying our full URL
  strcpy(relative_url, params->fullURI);
  printf("poop7\n");

  // delete the http:// portion that will exist on every full url
  deleteSubstring(relative_url, "http://");
  printf("poop8\n");
  
  // delete the host name from the full path to get just the relative path
  printf("This is our relative url so far: %s\n", relative_url);
  printf("This is our host: %s\n", params->host);
  deleteSubstring(relative_url, params->host);
  printf("This is our new relative url: %s\n", relative_url);
  params->relativeURI = malloc(strlen(relative_url)+1);
  strcpy(params->relativeURI, relative_url);

  printf("bye\n");
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
