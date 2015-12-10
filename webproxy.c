#include "webproxy.h"

int main(int argc, char ** argv)
{
	// add the dnat rule to redirect any tcp traffic from the client to our proxy 
	system("iptables -t nat -A PREROUTING -p tcp -i eth0 -j DNAT --to 192.168.0.1:9999");

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




void return_socket_information(int sock, char *ip_address_remote, int *port_remote, char *ip_address_client, int *port_client) {
	socklen_t len;
	struct sockaddr_storage addr;
	int client_port, status;
	len = sizeof(addr);
	getpeername(sock, (struct sockaddr *)&addr, &len);
	if ( (addr.ss_family = AF_INET) ) {
		struct sockaddr_in *s = (struct sockaddr_in *)&addr;
		client_port = ntohs(s->sin_port);
		inet_ntop(AF_INET, &s->sin_addr, ip_address_client, 256);
	}
	else {
		struct sockaddr_in6 *s = (struct sockaddr_in6 *)&addr;
		client_port = ntohs(s->sin6_port); 	
		inet_ntop(AF_INET6, &s->sin6_addr, ip_address_client,256);
	}

	*port_client = client_port;
	//printf("Peer IP address: %s\n", ipstr);
	//printf("Peer port: %d\n", client_port);

	struct sockaddr_in destaddr;
	socklen_t destlen = sizeof(destaddr);
	char destinationName[256];
	int portnumber;
	destinationName[0] = '\0';
	status = getsockopt(sock, SOL_IP,SO_ORIGINAL_DST, (struct sockaddr *) &destaddr, &destlen);
	if (status == 0) {
		inet_ntop(AF_INET, (void *) &destaddr.sin_addr, ip_address_remote, 256);
		portnumber = ntohs(destaddr.sin_port);
		//ssize_t dl = strlen(destinationName);
		//sprintf(&destinationName[dl], ":%d", portnumber);
		//printf("Socket:: getsockopt : %s\n", destinationName);
	}
	*port_remote = portnumber;
}


/*------------------------------------------------------------------------------------------------
 * client_handler - this is the function that gets first called after setting up the master socket
 *------------------------------------------------------------------------------------------------*/
void client_handler(int client) {
	printf(">>>>>>> NEW CLIENT\n\n");
	ssize_t response_read_size = 0;
	ssize_t request_read_size = 0;
	struct HTTP_RequestParams params;

	// client messge is used to store the full original message from client
	unsigned char client_message[1024], client_message_copy[1024], remote_request[1024], remote_response[1024];
	memset(&client_message, 0, sizeof(client_message));
	memset(&client_message_copy, 0, sizeof(client_message_copy));
	memset(&remote_request, 0, sizeof(remote_request));
	memset(&remote_response, 0, sizeof(remote_response));

	char remote_ip_address[256], client_ip_address[256];
	int remote_port_number, client_port_number;
	return_socket_information(client, remote_ip_address, &remote_port_number, client_ip_address, &client_port_number);

	printf("Client IP Addresss: %s Client Port Number: %d\n", client_ip_address, client_port_number);
	printf("Server IP Addresss: %s Server Port Number: %d\n", remote_ip_address, remote_port_number);

	int remote_socket;
	struct sockaddr_in sa;
	unsigned int sa_len = sizeof(sa);
	sa_len = sizeof(sa);

	remote_socket = setup_remote_socket(remote_port_number, remote_ip_address);

	if (getsockname(remote_socket, (struct sockaddr *)&sa, &sa_len) == -1)
		printf("getsockname failed\n");
	//printf("Local IP address is: %s\n", inet_ntoa(sa.sin_addr));
	//printf("Local Port number is: %d\n", (int) ntohs(sa.sin_port));

	ssize_t test_size = 0;

	errno = 0;
	int bytes_sent_to_server;
	int bytes_sent_to_client;
	unsigned char *bufptr;
	while (1) {
		request_read_size = recv(client, client_message, 1024, MSG_DONTWAIT);
		bufptr = client_message;
		if (request_read_size > 0) {
			do {
				bytes_sent_to_server = send(remote_socket, bufptr, request_read_size, 0); 
				request_read_size -= bytes_sent_to_server;
				bufptr += bytes_sent_to_server;
			} while(request_read_size > 0);
		}
		else if (request_read_size  <= 0){
			if (request_read_size == 0) {
				shutdown(remote_socket, SHUT_WR);
			}
			else if (errno != EWOULDBLOCK && errno == EAGAIN) break;
		}
		errno = 0;
		response_read_size = recv(remote_socket, remote_response, 1024,MSG_DONTWAIT);
		bufptr = remote_response;
		if (response_read_size > 0) {
			do {
				bytes_sent_to_client = send(client, bufptr, response_read_size, 0);
				response_read_size -= bytes_sent_to_client;
				bufptr += bytes_sent_to_client;
			} while(response_read_size > 0);
		}
		else if (response_read_size <= 0){
			if (response_read_size == 0) {
				shutdown(client, SHUT_WR);
				break;
			}
			else if (errno != EWOULDBLOCK && errno != EAGAIN) break;
		}
	errno = 0;
	}
close(client);
close(remote_socket);
system("iptables -t nat -D PREROUTING -p tcp -i eth0 -j DNAT --to 192.168.0.1:9999");
}
int get_valid_remote_ip(char *hostname) {

	struct addrinfo hints, *res;
	int sockfd;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	getaddrinfo(hostname, "http", &hints, &res);

	sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (sockfd == -1) {
		printf("some error occured with socket\n");
		exit(1);
	}

	if ( (connect(sockfd, res->ai_addr, res->ai_addrlen)) == -1)
	{
		printf("some error occured with  connect \n");
		exit(1);
	}


	return sockfd;
}

int setup_remote_socket(int port_number, char *ip_address) {

	//printf("Argument port_number :%d", port_number);
	//printf("Argument ip_address :%s", ip_address);

	//printf("Hello from setup_remote_socket\n");
	/* The data structure used to hold the address/port information of the remote server-side socket */
	struct sockaddr_in server;
	/* The data structure used to hold the address/port information of the local socket */
	struct sockaddr_in local_port;

	/* This will be the socket descriptor that will be returned from the socket() call */
	int sock;

	/* Socket family is INET (used for Internet sockets) */
	server.sin_family = AF_INET;
	local_port.sin_family = AF_INET;

	/* Apply the htons command to convert byte ordering of port number into Network Byte Ordering (Big Endian) */
	server.sin_port = htons(port_number);
	local_port.sin_port = htons(0);

	/* Set the IP address to be the IP address extracted from the clients original destination request */
	inet_pton(AF_INET, ip_address, &(server.sin_addr));

	/* Zero off remaining sockaddr_in structure so that it is the right size */
	memset(server.sin_zero, '\0', sizeof(server.sin_zero));

	/* Allocate the socket */
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == ERROR) {
		perror("server socket: ");
		exit(-1);
	}

	if (bind(sock, (struct sockaddr *)&local_port, sizeof(local_port)) < 0)
		printf("can't bind to port :( \n");

	struct sockaddr_in sa;
	unsigned int sa_len = sizeof(sa);
	sa_len = sizeof(sa);
	if (getsockname(sock, (struct sockaddr *)&sa, &sa_len) == -1)
		printf("getsockname failed\n");
	//printf("Local IP address is: %s\n", inet_ntoa(sa.sin_addr));
	//printf("Local Port number is: %d\n", (int) ntohs(sa.sin_port));

	char dynamic_snat_rule[256];
	sprintf(dynamic_snat_rule, "iptables -t nat -A POSTROUTING -p tcp -j SNAT --sport %d --to-source %s\n", (int) ntohs(sa.sin_port), "192.168.0.2");
	system(dynamic_snat_rule);
	if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) 
		printf("Can't connect to %s:%d\n", ip_address, port_number);
	else
		//printf("succesfully connected to remote server!!! :) \n");
	return sock;
}
/*----------------------------------------------------------------------------------------------
 * setup_socket - allocate and bind a server socket using TCP, then have it listen on the port
 *---------------------------------------------------------------------------------------------- */
int setup_socket(int port_number, int max_clients)
{
	//printf("Hello from setup_socket\n");
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
