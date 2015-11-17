#include "webproxy.h"

int main(int argc, char ** argv)
{
  if (argc < 2) {
    printf("Please specify a port number\n");
    exit(1); 
  }

  // port_number used to store user input, main_socket used for listening for clients, pid for forking, cli_socket for individual client sockets
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

  // receive / intercept the request from the client
  read_size = recv(client, client_message, 1024, 0);
  // make a copy of the client request since the original will be altered when calling strtok
  strcpy(client_message_copy, client_message);
  
  // parse the user request and get everything that we need from it 
  extract_request_parameters(client_message, &params);

  // construct the new, relative path request that will be sent to the remote server
  construct_new_request(remote_request, client_message_copy, &params);
  printf("This should be our remote request that we will be sending: \n%s\n", remote_request);
  int remote_socket;
  // retrieve a valid remote socket connection 
  // remote_socket = get_valid_remote_ip(params.host);
  
  // Free all the strings allocated for the HTTP params struct
  free(params.method);
  free(params.fullURI);
  free(params.relativeURI);
  free(params.httpversion);
  free(params.host);
}
int get_valid_remote_ip(char *hostname) {
  printf("Hello from get_valid_remote_ip\n");

  int server_socket;
  struct addrinfo hints; // specify the parameters for this connection
  struct addrinfo *list; // a list of returned addr structs
  struct addrinfo *address_pointer; // the current address struct
  int return_value;

  char *server_name = "www.google.com";

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  
  // this will generate the linked list of addrinfo structs that each contain potential IP addresses for the given server_name
  if ( (return_value = getaddrinfo(server_name, 0, &hints, &list)) != 0)
  {
    printf("error with getaddrinfo\n");
    exit(1);
  }

  // iterate through each of the addrinfo structs and attempt to connect via socket
  char possible_ip[NI_MAXHOST];
  for (address_pointer = list; address_pointer != NULL; address_pointer = address_pointer->ai_next)
  {
    printf("Hello from the loop\n");
    getnameinfo(address_pointer->ai_addr, address_pointer->ai_addrlen, possible_ip, sizeof(possible_ip), NULL, 0, NI_NUMERICHOST);
    printf("THis is the potential IP: %s\n", possible_ip);
    // if we cannot create a socket, continue to the next
    if ((server_socket = socket(address_pointer->ai_family, address_pointer->ai_socktype, 0)) == -1) {
      printf("Could not create socket\n");
      continue;
    }
    // if we create a socket and are able to connect, break. We have a valid remote socket
    if (connect(server_socket, address_pointer->ai_addr, address_pointer->ai_addrlen) == 0)  {
      printf("Connected, should be breaking\n");
      break;
    }
    // if we are able to create a socket but not connect, ditch the socket and try again on the next struct
    close (server_socket);
  }

  if (address_pointer != NULL) {
    char host[NI_MAXHOST];
    getnameinfo(address_pointer->ai_addr, address_pointer->ai_addrlen, host, sizeof(host), NULL, 0, NI_NUMERICHOST);
    printf("%s can be reached at %s\n", server_name, host);
  }
  else {
    printf("No remote address found\n");
    exit(1);
  }

  return server_socket;

}
void construct_new_request(char *request, char *message, struct HTTP_RequestParams *params) {
  char *first_line, *rest_of_request;
  first_line = strtok_r(message, "\n", &rest_of_request);
  strncpy(request, params->method, strlen(params->method));
  strncat(request, " ", 1);
  strncat(request, params->relativeURI, strlen(params->relativeURI));
  strncat(request, " ", 1);
  strncat(request, params->httpversion, strlen(params->httpversion));
  strcat(request, "\n");
  strncat(request, rest_of_request, strlen(rest_of_request));
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * extract_request_parameters - this function will be mainly responsible for parsing and extracting the path from the HTTP request from the client
 *------------------------------------------------------------------------------------------------------------------------------------------- */
void extract_request_parameters(char *response, struct HTTP_RequestParams *params) {
  char *first_line, *rest_of_request, *host;

  // first_line will contain the first line of the request, rest_of_request will contain the rest 
  first_line = strtok_r(response, "\n", &rest_of_request);

  // first_line now contains the first word of the first line of the request 
  first_line = strtok(first_line, " ");
  params->method = malloc(strlen(first_line)+1);
  strcpy(params->method, first_line);

  // first_line now contains the second word of the first line of the request 
  first_line = strtok(NULL, " ");
  params->fullURI = malloc(strlen(first_line)+1);
  strcpy(params->fullURI, first_line);

  // first_line now contains the third word of the first line of the request 
  first_line = strtok(NULL, " ");
  params->httpversion = malloc(strlen(first_line)+1);
  strcpy(params->httpversion, first_line);

  // host will contain the first word of the second line of the request
  host = strtok(rest_of_request, ":");
  // host will contain the second word of the second line of the request
  host = strtok(NULL, "\n");

  // remove the first character 
  host++;
  // remove the last character
  host[strlen(host)-1] = 0;

  params->host = malloc(strlen(host)+1);
  strcpy(params->host, host);

  // this string will hold the relative url of the file requested
  char relative_url[strlen(params->fullURI)];

  // start out by copying our full URL
  strcpy(relative_url, params->fullURI);

  // delete the http:// portion that will exist on every full url
  deleteSubstring(relative_url, "http://");
  
  // delete the host name from the full path to get just the relative path
  deleteSubstring(relative_url, params->host);
  params->relativeURI = malloc(strlen(relative_url)+1);
  strcpy(params->relativeURI, relative_url);
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
