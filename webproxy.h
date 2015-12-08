#include<netdb.h>
#include<limits.h>
#include<string.h>
#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<sys/socket.h>
#include<linux/netfilter_ipv4.h>
#include<netinet/in.h>
#include<errno.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<pthread.h>
#include<sys/stat.h>

#define MAX_CLIENTS 5
#define ERROR -1

struct HTTP_RequestParams {
  char *method;
  char *fullURI;
  char *relativeURI;
  char *httpversion;
  char *host;
};

void return_socket_information(int sock, char *ip_address_remote, int *port_remote, char *ip_address_client, int *port_client);
int setup_remote_socket(int port_number, char *ip_address);
int get_valid_remote_ip(char *hostname);
void construct_new_request(char *request, char *message, struct HTTP_RequestParams *params);
void client_handler(int client);
void extract_request_parameters(char *response, struct HTTP_RequestParams *params);
int setup_socket(int port_number, int max_clients);
void deleteSubstring(char *original_string,const char *sub_string);
