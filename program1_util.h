#include "common.h"

#ifndef PGM1_UTIL_H
#define PGM1_UTIL_H

// socket for the client in user side
extern int sock;
// flag to stop the infinite loop in server
extern int stop;
// socket for the server in user side
extern int sock2;
// list of server addresses
extern struct addrinfo *send_addr;
// list of listening address
extern struct addrinfo *listen_addr;
// used to check sent data in self test
extern char *output;

// stores the current user information
struct user_login
{	
	//stores the user id
	unsigned char userid[ID_LEN];
	//stores the user password
	unsigned char password[PSWD_MAXLEN];	
}__attribute__((packed));

// For receive thread
struct ThreadArgs
{
	// stores user's information
	struct user_login* curr_user;
	// log file
	FILE* log;
	// input parameter
	int argc;
	// port number
	char* port;
}__attribute__((packed));

// connection setup
void setup_connection(int argc, char *argv[], int family, 
					int type, int protocol);
// frees memory and close the socket
void cleanup_client();
void cleanup_server();
// exits the program with error message
void client_exit_with_error(char *msg);
void server_exit_with_error(char *msg);
// prepares the tcp server socket
void prepare_generic_socket(int argc, char *argv, 
		int family, int flags, int type, int protocol);
// prints the data in certain format
void sprint_hex(request *data, size_t length);

#endif 
