/***********************************************************************
 *
 * Group name: LLL
 * Authors: Zhi Li
 * Purpose: utility functions for user side that used in or similiar to
 * previous homeworks
 *
 * It consists 7 functions.
 * -void prepare_generic_socket(int argc, char *argv, 
 *		int family, int flags, int type, int protocol)
 * It takes the input arguments, address family, socket type, flag and
 * protocol type as input, creates a TCP socket to receive the data from
 * vending server
 * -void setup_connection(int argc, char *argv[], int family, 
 *					int type, int protocol);
 * It takes the input arguments, address family, socket type and protocol
 * type as input, creates a TCP socket to send data to the authentication 
 * center and establishes the connection. 
 * -void cleanup_client()
 * It frees the memory and close the socket in client
 * -void cleanup_server()
 * It frees the memory and close the socket in server
 * -void client_exit_with_error(char *msg)
 * It terminates the program with the error message at client side
 * -void server_exit_with_error(char *msg)
 * It terminates the program with the error message at server side
 * - void sprint_hex(request *data, size_t length)
 * It takes the request packet we sent and the length of the packet as
 * input, then prints the data in request packet we sent byte by byte 
 * in the "output" char array for unit testing
 *
***********************************************************************/
#include "program1_util.h"

/*
 * Authors: Zhi Li
 * Purpose: It creates a TCP socket to receive the data from vending
 * server
 *
 * Expected input: It takes the input arguments, address family, socket
 * type, flag and protocol type as input
 *
 * Implementation details:
 * It creates a TCP socket. It first sets the criteria for address 
 * it listens to and gets the corresponding list of address. Then it 
 * establishes the connection. If the socket creation failed, or if
 * it can not bind the listen address with the socket, the program 
 * exit with error message.
 *
 */
void prepare_generic_socket(int argc, char *argv, 
		int family, int flags, int type, int protocol)
{
	// tell the system what kind(s) of address info we want
	// criteria for address match
	struct addrinfo lookup_addr;
	// zero out the structure
	memset(&lookup_addr, 0, sizeof(struct addrinfo));
	lookup_addr.ai_family = family; 
	lookup_addr.ai_flags = flags;
	lookup_addr.ai_socktype = type;
	lookup_addr.ai_protocol = protocol;
	//get address
	if (getaddrinfo(NULL, argv, &lookup_addr, &listen_addr) != 0)
	{
		server_exit_with_error("getaddrinfo failed");
	}
	// Create a reliable, stream socket using TCP
	sock2 = socket(listen_addr->ai_family, listen_addr->ai_socktype,
			listen_addr->ai_protocol);
	if (sock2 < 0)
	{
		server_exit_with_error("socket failed");
	}
	// bind listening address
	if (bind(sock2, listen_addr->ai_addr, 
				listen_addr->ai_addrlen) < 0)
	{
		server_exit_with_error("bind failed");
	}
}

/*
 * Authors: Zhi Li
 * Purpose: It creates a TCP socket to send data to the authentication 
 * center and establishes the connection. 
 *
 * Expected input: It takes the input arguments, address family, socket
 * type and protocol type as input
 *
 * Implementation details:
 * It creates a TCP socket. It first sets the criteria for address 
 * match and gets the corresponding server address. Then it establishes 
 * the connection. If the socket creation or connection failed,
 * exit with error message.
 */
void setup_connection(int argc, char *argv[], int family,
		int type, int protocol)
{
	// tell the system what kind(s) of address info we want
	// criteria for address match
	struct addrinfo lookup_addr;
	// zero out the structure
	memset(&lookup_addr, 0, sizeof(struct addrinfo));
	// set address family
	lookup_addr.ai_family = family;
	// set sockets type
	lookup_addr.ai_socktype = type;
	// set protocol type
	lookup_addr.ai_protocol = protocol;
	// get addresses
	if (getaddrinfo(argv[1], argv[2], &lookup_addr, &send_addr) != 0)
	{
		client_exit_with_error("getaddrinfo failed");
	}
	// Create a reliable, stream socket using TCP
	sock = socket(send_addr->ai_family, send_addr->ai_socktype,
				send_addr->ai_protocol);
	if (sock < 0)
	{
		client_exit_with_error("socket failed");
	}
	// Establish the connection
	if (connect(sock, send_addr->ai_addr, send_addr->ai_addrlen) < 0)
	{
		client_exit_with_error("connect failed");
	}
}

/*
 * Authors: Zhi Li
 * Purpose: It frees the memory and close the socket in client
 *
 * Implementation details:
 * It frees pointer pointed to addrinfo allocated in getaddrinfo()
 * then close the socket in client
 */
void cleanup_client()
{
	if(send_addr)
		// free addrinfo allocated in getaddrinfo()
		freeaddrinfo(send_addr);
	if(sock)
		// close the socket
		close(sock);
}

/*
 * Authors: Zhi Li
 * Purpose: It frees the memory and close the socket in server
 *
 * Implementation details:
 * It frees pointer pointed to addrinfo allocated in getaddrinfo()
 * then close the socket in server
 */
void cleanup_server()
{
	if(listen_addr)
		// free listen address
		freeaddrinfo(listen_addr);
	if(sock2)
		// close the socket
		close(sock2);
}

/*
 * Authors: Zhi Li
 * Purpose: It terminates the program with the error message
 * at client side
 *
 * Expected input: It takes the error string as input
 *
 * Implementation details:
 * It prints error message, frees the memory, close 
 * the socket at client and exit the program
 */
void client_exit_with_error(char *msg)
{
	perror(msg);
	cleanup_client();
	exit(1);
}

/*
 * Authors: Zhi Li
 * Purpose: It terminates the program with the error message
 * at server side
 *
 * Expected input: It takes the error string as input
 *
 * Implementation details:
 * It prints error message, frees the memory, close 
 * the socket at server and exit the program
 */
void server_exit_with_error(char *msg)
{
	perror(msg);
	cleanup_server();
	exit(1);
}

/*
 * Authors: Zhi Li
 * Purpose: It prints the data in request packet we sent byte by byte 
 * in the "output" char array for unit testing
 *
 * Expected input: It takes the request packet we sent and the length
 * of the packet as input
 * 
 * Implementation details: 
 * It reads data byte by byte in the request packet and put them in
 * a string followed a certain format that 16 bytes in a line,
 * and each byte are stored in hex with a space in between.
 */
void sprint_hex(request *data, size_t length)
{
	char myoutput[MAXLENGTH];
	char tmp[MAXLENGTH];
	myoutput[0] = '\0';

	int i;
	// get data byte by byte and store them with certain
        // format in a temporary string
	for (i = 0; i < length; i++)
	{
		sprintf(tmp, "%02x ", *(&data->code+i));
		strcat(myoutput, tmp);
		if (i % 16 == 15)
		{
			strcat(myoutput, "\n");
		}
	}
	// add a new line in the end
	if (myoutput[strlen(myoutput)-1] != '\n')
	{
		strcat(myoutput, "\n");
	}

	output = strdup(myoutput);
}
