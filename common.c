/***********************************************************************
 *
 * Group name: LLL
 * Authors: Deyuan Li, Zhi Li, Weiyan Lin
 * Purpose: functions for common things in our three programs
 *
 * It consists 4 functions.
 * -void handler2(int signal)
 * It takes the signal as input, sets stop to break the while loop and 
 * close the socket
 * -void register_tcp_handlers()
 * It sets the handler for SIGINT SIGUP SIGTERM and SIGPIPE.
 * -void cleanup()
 * It frees the memory and close the socket
 * -void exit_with_error(const char *msg)
 * It takes the error string as input, and prints error message 
 * frees the memory and terminates the program
 * 
***********************************************************************/
#include "common.h"

/*
 * Authors: LLL
 * Purpose: It sets stop to break the while loop and close the 
 * socket
 *
 * Expected input: It takes the signal as input
 *
 * Implementation details:
 * Set the stop to be 1 to break the server's infinite loop
 * and close the socket if it exists
 * 
 */
void handler2(int signal)
{
	stop = 1;
	if (sock != 0)
	{
		close(sock);
	}
}

/*
 * Authors: LLL
 * Purpose: Set up the signal handler
 *
 * Implementation details:
 * Set the handler for SIGINT SIGUP SIGTERM and
 * SIGPIPE.
 * 
 */
void register_tcp_handlers()
{
	struct sigaction actinfo;
	actinfo.sa_handler = handler2;
	sigfillset(&actinfo.sa_mask); 
	actinfo.sa_flags = 0;
	sigaction(SIGINT, &actinfo, 0); 
	sigaction(SIGHUP, &actinfo, 0); 
	sigaction(SIGTERM, &actinfo, 0);
	//ignores the pipe error
	actinfo.sa_handler = SIG_IGN;
	sigaction(SIGPIPE, &actinfo, 0); 
}

/*
 * Authors: LLL
 * Purpose: It frees the memory and close the socket
 *
 * Implementation details:
 * It frees pointer pointed to sent packet and addrinfo allocated in
 * getaddrinfo() then close the socket
 */
void cleanup()
{
	if (listen_addr)
		freeaddrinfo(listen_addr);
	if (sock)
		close(sock); 
}
/*
 * Authors: LLL
 * Purpose: When there is an error, the program prints error message 
 * frees the memory and terminates
 *
 * Expected input: It takes the error string as input
 *
 * Implementation details:
 * It prints out the error message and call cleanup() to free
 * memory and close the socket
 */
void exit_with_error(const char *msg)
{
	perror(msg);
	cleanup();
	exit(1);
}
