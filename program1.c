/***********************************************************************
 *
 * Group name: LLL
 * Authors: Zhi Li
 * Purpose: Program 1 is the program at user side. It sends the user 
 * request to the authentication center. Then, it waits the vending
 * server to send the corresponding response. It performs LOGIN and
 * LOGOUT, and send DEPOSIT/BALANCE/BUY/PRICE/STATUS request.
 *
 * expected inputs: 
 * Before starting the program:
 * no argument: show usage instruction
 * test: run selftest
 * <AC ipv4 address> <AC port number> <local port number>: 
 * 						start the program
 * 
 * After starting the program, use <COMMAND> as input:
 * in login mode
 * LOGIN <USERID> <PASSWORD>
 * in request mode
 * DEPOSIT $<DOLLARS>
 * BALANCE
 * BUY <INDEX>
 * PRICE <INDEX>
 * STATUS
 * LOGOUT
 * EXIT
 * <INDEX> is the item number (A followed by an integer, e.g. A1)
 *
 * The program consists of both client and server part. The server
 * is always up and running in a separate thread.
 * It associates program1_util.c program1_util.h common.c and
 * common.h
 * This file contains the 11 functions.
 * -void input_log(FILE *log, char *input_string)
 * It takes a string and log file as input, and writes the input
 * string into the log file
 * -void input_error(FILE *log)
 * It takes log file as input, prints error message when one 
 * occurs and writes it into log file 
 * -int32_t command_exe(struct user_login* curr_user, FILE *log, 
 *  uint8_t code, uint32_t req_value, int argc, char *argv[])
 * It takes user's information, log file, packet code, user's
 * requested value and input arguments as input. It sets up the TCP 
 * connection with authentication center, then it sends data from the 
 * user side, and receives feedback from the authentication center. 
 * Afterwards, it handles the received data and shows corresponding
 * information to the user.
 * -int32_t send_request(pthread_t threadID, struct user_login* curr_user,
 *  FILE *log, int argc, char *argv[])
 * It takes server thread ID, user information, log file and input
 * argument as input, then it detects and execute the command from 
 * stdin and calls the command_exe to send the request to the
 * authentication center
 * -void handle_recv_packet(FILE *rx, FILE *log,
 *  struct respond_packet *torecv,uint8_t code)
 * It takes rx file, log file, the pointer to the struct which stores 
 * the received data and packet code as input, then it handles the 
 * received data from vending server when user send BUY/PRICE/STATUS 
 * command and it prints corresponding information to the user.
 * -void handle_tcp_client(int client, FILE *log)
 * It takes file descriptor client from accept(), log file as input, 
 * then it handles the received data from vending server when the
 * received packet is valid
 * -void receive_tcp_clients(FILE *log)
 * It takes the log file as input, then it accepts the new connection 
 * from vending server and calls handlet_tcp_client to handle the
 * received data.
 * -void *recv_response(void *threadArgs)
 * It takes a pointer to a struct of all the arguments that the thread
 * needed as input, then it sets up the connection between user side
 * and vending server. Then it call receive_tcp_clients to handles
 * all the functionality of the server in user side
 * -void user_login(pthread_t threadID,FILE *log,
 *  struct user_login *curr_user)
 * It takes server thread ID, current user information as input, then
 * it performs the user login mode. It detects and execute the command
 * from stdin. If the user request LOGIN command, then it stores the 
 * user ID and password in a local memory. If the user request EXIT
 * command, it terminates the server thread, frees the memory and exits
 * the program.
 * -void testall(int32_t argc, char* argv[])
 * It is self test function
 * -int main(int32_t argc, char* argv[])
 * If there is no argument, show usage instructions. If "test" is 
 * provided, it runs self-test.
 * If <AC ipv4 address> <AC port number> <local port number>is provided,
 * it starts the program and enters the user login mode.
 *
***********************************************************************/

#include "common.h"
#include "program1_util.h"
#include <pthread.h>
// flag to stop the infinite loop in server
int stop = 0;
// socket for the client in user side
int sock = 0;
// socket for the server in user side
int sock2 = 0;
// list of server addresses
struct addrinfo *send_addr = NULL;
// list of addresses it listens to
struct addrinfo *listen_addr = NULL;
// used to check sent data in self test
char *output = NULL;

/*
 * Authors: Zhi Li
 * Purpose: It saves the input in the log file
 * 
 * Expected input: It takes the log file and the string to store
 * as input 
 *
 * Implementation details: The function takes the string from the
 * input and save it with proper format in the file which "log"
 * points to, and adds details later in other functions
 */
void input_log(FILE *log, char *input_string)
{
	char s[MAXLENGTH];
	//copy the original input string to a temporary string array
	//and change the last character from '\n' to ' '
	strcpy(s,input_string);
	s[strlen(s)-1]=' ';
	//write to the log
	fprintf(log,"%s",s);
	fflush(log);
}

/*
 * Authors: Zhi Li
 * Purpose: Display an error message and save it in log file
 * when an invalid input occurs
 *
 * Expected input: It takes the log file as input
 *
 * Implementation details:
 * If the input is detected invalid in other functions,
 * this function will be called. It outputs an error message
 * and save it in log file
 */
void input_error(FILE *log)
{
	char error_message[]="ERROR INVALID INPUT\n";
	printf("%s",error_message);
	fprintf(log,"%s",error_message);
	fflush(log);
}

/*
 * Authors: Zhi Li
 * 
 * Purpose: It sets up the TCP connection with authentication center, 
 * then it sends data from the user side, and receives feedback from the
 * authentication center. Afterwards, it handles the received data and
 * shows corresponding information to the user.
 *
 * Expected input: It takes user's information, log file, packet code,
 * user's requested value and input arguments as input
 *
 * Implementation details:
 * Send part:
 * It creates a TCP socket and establishes the TCP connection, then it
 * encryptes the users password, forms the send packet and sends the
 * packet. If it is in test mode, then it sets the sequence number to be 
 * 1, and calls sprint_hex to store the send data in certain format.
 * 
 * Receive part:
 * It receives the whole packet using fread function. If the packet is
 * with correct expecting code, it starts to handle it. It first checks
 * the status_code to see whether the user information is valid or not.
 * If it is not, then it warns the user to log out and re-login. If it
 * is valid, then it shows the deposit value or/and the total balance
 * the users stores in their account. If the STATUS request is made,
 * then it prints the title. It returns 0 after the execution.
 */
int32_t command_exe(struct user_login* curr_user, FILE *log, uint8_t code, 
				uint32_t req_value, int argc, char *argv[])
{
	// generate a random number for per-user request tracking number
	uint32_t seq_num = rand();			
	// initialize send packet
	uint8_t packetlen = sizeof(request);
	// the current request sent by user
	request *curr_req = malloc(packetlen); 
	memset(curr_req,0,packetlen);
	// form the packet
	curr_req->code = code;
	curr_req->seq_num = htonl(seq_num);
	memcpy(curr_req->userid, curr_user->userid, ID_LEN);
	// encrypt the password
	SHA1(curr_user->password,strlen((const char *)curr_user->password),
							curr_req->password);
	curr_req->request_value = htonl(req_value);
	// initialize receive packet
	uint32_t recvlen = sizeof(struct balance_packet);
	// the instant feedback from p2
	struct balance_packet *torecv = malloc(recvlen);
	memset(torecv, 0, recvlen);
	// initialize temporary receive buffer
	char buffer[recvlen];
	memset(buffer, 0, recvlen);
	if(strcmp(argv[1], "test") == 0)
	{
		curr_req->seq_num = htonl(1);
		sprint_hex(curr_req, packetlen);
	}
	else
	{
		// create a TCP socket and establish the connection
		setup_connection(argc, argv, AF_UNSPEC, SOCK_STREAM, 
								IPPROTO_TCP);	
		// write to sock
		FILE *tx = fdopen(sock,"w");
		// read from sock
		FILE *rx = fdopen(dup(sock),"r");
		// send the packet to the server
		size_t bytes_sent = fwrite(curr_req, 1, packetlen , tx);
		if(bytes_sent < 0)
		{
			fclose(tx);
			fclose(rx);
			client_exit_with_error("fwrite failed");
		}
		if(bytes_sent != packetlen)
		{
			fclose(tx);
			fclose(rx);
			client_exit_with_error("fwrite unexpected number of bytes");
		}
		fflush(tx);		
	
		// receive the feedback
		size_t bytes_received = fread(buffer, recvlen, 1, rx);
		if(bytes_received < 0)
		{
			printf("error\n");
			fclose(tx);
			fclose(rx);
			client_exit_with_error("fread failed");
		}
		// copy the buffer to tocv
		memcpy(&torecv->code, buffer, recvlen);
	
		// if the packet with correct expecting code
		if(torecv->code == BALANCE_CODE)
		{
			// when the user information is invalid
			if(torecv->status_code == FAILURE)
			{
				printf("INVALID USER INFO\nPLEASE LOG OUT\n");
				// output into log file
				fprintf(log,"INVALID USER INFO\n");
				fflush(log);
			}
			// when the user made a DEPOSIT request
			else if(code == USER_CODE && req_value > 0)
			{
				printf("$%d DEPOSITED ~~~ BALANCE NOW $%d\n",
						req_value,
						ntohl(torecv->balance));
				// output into log file
				fprintf(log,"BALANCE NOW $%d\n",
						ntohl(torecv->balance));
				fflush(log);
			}
			// when the user made a BALANCE request
			else if(code == USER_CODE && req_value == 0)
			{
				printf("BALANCE NOW $%d\n",
						ntohl(torecv->balance));
				// output into log file
				fprintf(log,"BALANCE NOW $%d\n",
						ntohl(torecv->balance));
				fflush(log);
			}
			// when the user made a STATUS request
			else if(code == STATUS_CODE)
			{
				printf("BALANCE NOW $%d\n",
						ntohl(torecv->balance));
				printf("ITEMS\nITEM\tNAME\tPRICE\n");
			}
		}	
	
		// free memory and close socket
		fclose(tx);
		fclose(rx);
		cleanup_client();
	}
	free(curr_req);
	free(torecv);	
	return 0;
}

/*
 * Authors: Zhi Li
 * Purpose: It detects and execute the command from stdin and calls
 * the command_exe to send the request to the authentication center
 * 
 * Expected input: It takes server thread ID, user information, log
 * file and input argument as input
 * commands included: DEPOSIT $<DOLLARS>
 * 		      BALANCE
 *		      BUY <INDEX>
 *                    PRICE <INDEX>
 *                    STATUS
 * 		      LOGOUT
 *		      EXIT
 * 
 * Implementation details: 
 * The function first read from stdin and call the command_exe 
 * function with corresponding code and requested value if the
 * commands are DEPOSIT/BALANCE/BUY/PRICE/STATUS. The requested
 * value for DEPOSIT and BALANCE is the money, where we use 0 to
 * indicate BALANCE command. The requested value for BUY/PRICE/
 * STATUS is the index of requested item. If the command is EXIT,
 * it shuts down the server thread immediately and terminates
 * the program. If the command is LOGOUT, it clear the user info.
 * After this, it stops to read from the stdin by breaking the
 * loop. If none of the above commands are executed it will call
 * input_error() and start detecting commands with the next
 * line from stdin.
 * It returns the packet type code for later use.
 */
int32_t send_request(pthread_t threadID, struct user_login* curr_user,
				FILE *log, int argc, char *argv[])
{
	// return value which indicates the packet type
	int32_t mode;
	// store the command from the input
	char command[MAXLENGTH];
	// store the input
	char line[MAXLENGTH];
	// check the validity of the end of each argument
	// after the command
	char enter_check;
	// check the dollar sign
	char dollor_check;
	uint32_t item_index = 0;
	uint32_t result;
	uint32_t deposit = 0;
	// read from input
	while(fgets(line, MAXLENGTH, stdin)!=NULL)
	{
		// record the command immediately after the input
		input_log(log,line);
		// judge the command type
		if(sscanf(line, "%s", command)==EOF)
		{
			input_log(log,"ERROR INVALID INPUT\n");
		}
		// DEPOSIT command
		else if(strcmp("DEPOSIT",command)==0)
		{
			// check the format of the arguments
			// after the command DEPOSIT
			result=sscanf(line, "DEPOSIT %c%d%c", 
				&dollor_check, &deposit, &enter_check);
			if(result!=3||dollor_check!='$'||
				enter_check!='\n'||deposit>DEPOSIT_MAX)
			{
				input_error(log);
			}
			else
			{
				command_exe(curr_user, log, USER_CODE, 
						deposit, argc, argv);
				mode = USER_CODE;
				break;
			}
		}
		// BALANCE command
		else if(strcmp("BALANCE",command)==0)
		{
			// check the format of the arguments after
			// the command BALANCE
			result=sscanf(line, "BALANCE%c", &enter_check);
			if(result!=1||enter_check!='\n')
			{
				input_error(log);
			}
			else
			{
				// set the requested value as 0 to
				// indicate the BALANCE command
				command_exe(curr_user, log, USER_CODE, 
							0, argc, argv);
				mode = USER_CODE;
				break;
			}
		}
		// BUY command
		else if(strcmp("BUY",command)==0)
		{
			// check the format of the arguments after 
			// the command BUY
			result=sscanf(line, "BUY A%d%c", &item_index, 
							&enter_check);
			if(result!=2||enter_check!='\n'||item_index<=0)
			{
				input_error(log);
			}
			else
			{
				command_exe(curr_user, log, BUY_CODE, 
						item_index, argc, argv);
				mode = BUY_CODE;
				break;
			}
		}
		// PRICE command
		else if(strcmp("PRICE",command)==0)
		{
			// check the format of the arguments after the 
			// command PRICE
			result=sscanf(line, "PRICE A%d%c", &item_index, 
							&enter_check);
			if(result!=2||enter_check!='\n'||item_index<=0)
			{
				input_error(log);
			}
			else
			{
				command_exe(curr_user, log, PRICE_CODE,
						item_index, argc, argv);
				mode = PRICE_CODE;
				break;
				
			}
		}
		// STATUS command
		else if(strcmp("STATUS",command)==0)
		{
			// check the format of the command STATUS
			result=sscanf(line, "STATUS%c", &enter_check);
			if(result!=1||enter_check!='\n')
			{
				input_error(log);
			}
			else
			{
				// simply set the item index with 0
				command_exe(curr_user, log, STATUS_CODE,
							 0, argc, argv);
				mode = STATUS_CODE;
				break;
			}
		}
		// LOGOUT command
		else if(strcmp("LOGOUT",command)==0)
		{
			// check the format of the command LOGOUT
			result=sscanf(line, "LOGOUT%c", &enter_check);
			if(result!=1||enter_check!='\n')
			{
				input_error(log);
			}
			else
			{
				// clear the user information
				memset(curr_user, 0, 
						sizeof(struct user_login));
				mode = LOGOUT_CODE;
				break;
			}
		}
		// EXIT command
		else if(strcmp("EXIT",command)==0)
		{
			// check the format of the command EXIT
			result=sscanf(line, "EXIT%c", &enter_check);
			if(result!=1||enter_check!='\n')
			{
				input_error(log);
			}
			else
			{
				sleep(1);
				stop = 1;
				// terminate the server socket immediately
				// so that the accept function stops blocking
				shutdown(sock2,2);
				close(sock2);
				// make sure the server thread terminates			
				pthread_join(threadID,NULL);
				free(curr_user);
				fclose(log);				
				exit(0);
			}
		}
		else
		{
			input_error(log);
		}	
	}
	return mode;	
}

/*
 * Authors: Zhi Li
 * 
 * Purpose: It handles the received data from vending server when user
 * send BUY/PRICE/STATUS command and it prints corresponding information
 * to the user.
 *
 * Expected input: It takes rx file, log file, the pointer to the struct
 * which stores the received data and packet code as input
 *
 * Implementation details:
 * It uses file to receive the whole packet except for the code field
 * from the vending server. It divides the whole packet into two parts.
 * It first read all the field with certain length, which includes the 
 * length of the item name that has variable length. Then, it receives
 * the name according to its length. After got the whole packet, it 
 * handles the BUY/PRICE/STATUS respond separately.
 * For BUY:
 * It first checks the status, if it shows it is a successful transaction,
 * it shows the corresponding information for the user. If it is not,
 * then it checks the price field to see whether it is because of 
 * insufficient balance or it is because of the requested item is not
 * available.
 * For PRICE:
 * It prints out the price to the customer if the requested index is
 * valid, or it prints out a warning.
 * For STATUS:
 * It uses a while loop to print all the item information as long as it
 * receives data from the vending server.
 */
void handle_recv_packet(FILE *rx, FILE *log,
				struct respond_packet *torecv,uint8_t code)
{
	// create the buffer with maximum possible length
	uint8_t recvlen = sizeof(struct respond_packet) + ITEM_LABEL_MAXLEN;
	char buffer[recvlen];
	// read the first part
	size_t bytes_received = fread(buffer,
				sizeof(struct respond_packet)-1,1, rx);								 
	// exit if an error occurs
	if (bytes_received < 0)
	{
		fclose(rx);
		server_exit_with_error("fread failed");
	}
	if(bytes_received >0)
	{
		memcpy(&torecv->seq_num, buffer, 
					sizeof(struct respond_packet)-1);
		// when the item name length is more than 1	
		if (torecv->name_length >1)
		{
			// read the rest of the name
			bytes_received = fread(buffer, 
					torecv->name_length-1, 1, rx);
			// exit if an error occurs
			if (bytes_received < 0)
			{
				fclose(rx);
				server_exit_with_error("fread failed");
			}
			if(bytes_received >0)
			{
				// copy the other part of the packet
				memcpy(&torecv->name+1, buffer,
						torecv->name_length-1);
			}
		}
		// the packet has been received up to now
		// handle BUY respond
		if(code == BUYRES_CODE)
		{
			// null-terminate received name for later printing
			*(&torecv->name+torecv->name_length) = '\0';
			// the purchase is successful
			if(torecv->status_code == SUCCESS)
			{
				printf("ITEM %s BOUGHT FOR $%d\n",
					&torecv->name,ntohl(torecv->price));
				// output into log file
				fprintf(log,"ITEM %s BOUGHT FOR $%d\n",
					&torecv->name, ntohl(torecv->price));
				fflush(log);	
			}
			// the purchase fails
			else if(torecv->status_code == FAILURE)
			{
				// failure due to insufficient money
				if(torecv->price > 0)
				{
					char error_message[]= 
						"INSUFFICIENT BALANCE\n";
					printf("%s",error_message);
					// print the price
					printf("%s IS $%d\n", &torecv->name,
						ntohl(torecv->price));
					// output into log file
					fprintf(log,"%s%s IS $%d\n",
						error_message,&torecv->name, 
						ntohl(torecv->price));
					fflush(log);
				}
				// failure due to invalid index request
				else
				{
					char error_message[]=
						"REQUESTED ITEM IS NOT AVAILABLE\n";
					printf("%s",error_message);
				  	// output into log file
					fprintf(log,"%s",error_message);
					fflush(log);
				}
			}
		}
		// handle PRICE respond
		else if(code == PRICERES_CODE)
		{
				
			// null-terminate received name for later printing
			*(&torecv->name+torecv->name_length) = '\0';						
			// print the price if the index is valid
			if(torecv->price > 0)
			{
				printf("%s IS $%d\n", &torecv->name,
						ntohl(torecv->price));
				// output into log file
				fprintf(log,"%s IS $%d\n",&torecv->name,
						ntohl(torecv->price));
				fflush(log);
			}
			// the requested index is invalid
			else
			{
				char error_message[]=
					"REQUESTED ITEM IS NOT AVAIBLE\n";
				printf("%s",error_message);
				// output into log file
				fprintf(log,"%s",error_message);
				fflush(log);
			}
		}
		// handle status respond
		else if(code == STATUSRES_CODE)
		{
			// initiate the start index
			uint32_t item_index = 1;
			// as long as we receive the data
			while(bytes_received > 0)
			{
				// null-terminate received name for later printing
				*(&torecv->name+torecv->name_length) = '\0';
				// print out each item information
				// until there is no more items
				printf("A%d\t%s\t%u CENTS\n", item_index,
					&torecv->name,ntohl(torecv->price));
				item_index++;
				// read the next packet
				bytes_received = fread(buffer,
					sizeof(struct respond_packet), 1, rx);
				// exit if an error occurs
				if(bytes_received < 0)
				{
					fclose(rx);
					server_exit_with_error("fread failed");
				}
				if(bytes_received >0)
				{
					memcpy(&torecv->code, buffer,
						sizeof(struct respond_packet));
					if (torecv->name_length >1)
					{
						bytes_received = 
							fread(buffer, 
							torecv->name_length-1,
							1, rx);
						// exit if an error occurs
						if (bytes_received < 0)
						{									
							fclose(rx);
							server_exit_with_error("fread failed");
						}
						if(bytes_received >0)
						{
							// copy the other part of
							// the packet to torecv
							memcpy(&torecv->name+1,
								buffer,
								torecv->name_length-1);
						}
					}
				}
			}	
		}		
	}		
}

/*
 * Authors: Zhi Li
 * 
 * Purpose: It handles the received data from vending server when the
 * received packet is valid
 *
 * Expected input: It takes file descriptor client from accept(), log
 * file as input
 *
 * Implementation details:
 * It uses file to receive the first byte which is the code field
 * from the vending server. If the code is what the user side expects, 
 * then it starts to handle the whole packet by calling 
 * handle_recv_packet function. If code is invalid, it frees the
 * memory and close the rx file.
 *
 */
void handle_tcp_client(int client, FILE *log)
{
	
	FILE *rx = fdopen(client, "r");
	// stores the received data
	struct respond_packet *torecv = NULL;
	uint8_t recvlen = sizeof(struct respond_packet) 
					+ ITEM_LABEL_MAXLEN;
	torecv = malloc(recvlen);
	memset(torecv, 0, recvlen);
	// temporary memory to store the received data
	char buffer[recvlen];
	memset(buffer, 0, recvlen);
	// read the first byte which indicates the code
	size_t bytes_received = fread(buffer,1, 1, rx);
	// exit if an error occurs
	if (bytes_received < 0)
	{
		fclose(rx);
		server_exit_with_error("fread failed");
	}
	if(bytes_received > 0)
	{
		memcpy(&torecv->code, buffer, 1);
		// when the code is valid
		// then it starts to handle the whole packet
		if(torecv->code == BUYRES_CODE||
			torecv->code == PRICERES_CODE||
			torecv->code == STATUSRES_CODE)
		{
			handle_recv_packet(rx,log,torecv,torecv->code);	
		}
			
	}
	free(torecv);
	fclose(rx);
}

/*
 * Authors: Zhi Li
 *
 * Purpose: It accepts the new connection from vending server and calls
 * handlet_tcp_client to handle the received data.
 *
 * Expected input: It takes the log file as input
 *
 * Implementation details:
 * when the stop sign is 0, it waits to accept the new connection
 * from vending server in a while loop. Once the connection is
 * established, it calls handlet_tcp_client function to handle the
 * received data.
 *
 */
void receive_tcp_clients(FILE *log)
{
	while (!stop)
	{
		struct sockaddr_storage client_addr;
		socklen_t addr_len = sizeof(struct sockaddr_storage);
		// wait for vending server to connect
		int client = accept(sock2, 
					(struct sockaddr *) &client_addr,
					&addr_len);
		// when stop turns to 1 it breaks the while loop
		if (stop)
		{
			break;
		}
		if (client < 0)
		{
			server_exit_with_error("accept failed");
		}
		handle_tcp_client(client, log);
	}
}

/*
 * Authors: Zhi Li
 *
 * Purpose: It is the server thread function that set up the
 * connection between user side and vending server. Then it 
 * call receive_tcp_clients to handles all the functionality
 * of the server in user side
 *
 * Expected input: It takes a pointer to a struct of all the 
 * arguments that the thread needed as input
 *
 * Implementation details:
 * It first sets the signal handler, then builds the TCP connection
 * between vending server and the user side. Then, it calls the
 * receive_tcp_clients function to handle the server.
 * It returns a void pointer. This function is used to handle server
 * thread.
 */
void *recv_response(void *threadArgs)
{
	FILE *log = ((struct ThreadArgs *)threadArgs)->log;
	// sets the signal handler
	register_tcp_handlers();
	// sets up the socket and binds the port with address
	prepare_generic_socket(((struct ThreadArgs *)threadArgs)->argc,
			((struct ThreadArgs*)threadArgs)->port, AF_INET, 
			AI_PASSIVE, SOCK_STREAM, IPPROTO_TCP);
	if (listen(sock2, 1) < 0)
	{
		server_exit_with_error("listen failed");
	}
	// frees the memory
	free(threadArgs);
	// handle the server
	receive_tcp_clients(log);
	// server clean up
	cleanup_server();
	return (NULL);
}

/*
 * Authors: Zhi Li
 *
 * Purpose: It performs the user login mode. It detects and execute
 * the command from stdin. If the user request LOGIN command, then 
 * it stores the user ID and password in a local memory. If the user
 * request EXIT command, it terminates the server thread, frees the
 * memory and exits the program.
 *
 * Expected input: It takes server thread ID, current user information
 * as input.
 * commands included: LOGIN <USER ID> <PASSWORD>
 *		      EXIT
 *
 * Implementation details: 
 * The function first read from stdin if the commands are LOGIN/
 * EXIT. If the command if LOGIN, it stores the input user info in
 * a struct which is used to send later. If the command is EXIT,
 * it shuts down the server thread immediately and terminates
 * the program. After this, it stops to read from the stdin by 
 * breaking the loop. If none of the above commands are executed it 
 * will call input_error() and start detecting commands with the next
 * line from stdin.
 *
 */
void user_login(pthread_t threadID,FILE *log, struct user_login *curr_user)
{
	printf("USER LOGIN MODE\n");
	//store the command from the input
	char command[MAXLENGTH];
	//store the input
	char line[MAXLENGTH];
	//check the validity of the end of each argument after the command
	char enter_check;
	unsigned char userid[MAXLENGTH];
	unsigned char password[MAXLENGTH];
	uint32_t result;
	while(fgets(line, MAXLENGTH, stdin)!=NULL)
	{
		//record the command immediately after the input
		input_log(log,line);
		//judge the command type
		if(sscanf(line, "%s", command)==EOF)
		{
			input_log(log,"ERROR INVALID INPUT\n");
		}
		//LOGIN command
		else if(strcmp("LOGIN",command)==0)
		{
			//check the format of the arguments after the
			//command LOGIN
			result=sscanf(line, "LOGIN %s%s%c", userid, 
						password, &enter_check);
			if(result!=3||enter_check!='\n'||
				strlen((const char*)userid)!=ID_LEN||
				strlen((const char*)password)>PSWD_MAXLEN)
			{
				input_error(log);
			}
			else
			{
				// stores user info in the curr_user
				memcpy(curr_user->userid, userid, ID_LEN);
				memcpy(curr_user->password,password,
							PSWD_MAXLEN);
				printf("PLEASE ENTER YOUR REQUEST\n");
				break;
			}
		}
		else if(strcmp("EXIT",command)==0)
		{
			//check the format of the command EXIT
			result=sscanf(line, "EXIT%c", &enter_check);
			if(result!=1||enter_check!='\n')
			{
				input_error(log);
			}
			else
			{
				sleep(1);
				stop = 1;
				// terminate the server socket immediately
				// so that the accept function stops blocking
				shutdown(sock2,2);
				close(sock2);
				// make sure the server thread terminates
				pthread_join(threadID,NULL);
				free(curr_user);
				fclose(log);				
				exit(0);
			}
		}
		else
		{
			input_error(log);
		}	
	}
}

/*
 * Authors: Zhi Li
 * Purpose: It tests how the client deal with the send data
 *
 * Expected input: It takes input arguments as input
 * 
 * Implementation details: 
 * It tests command_exe() function to see how the client
 * handles the send data. 
 *
 * the process in the testall() is:
 * set the user id as ALICELEE
 * set the user password as 123456
 * set the request code to be 2 (PRICE PACKET)
 * set the item index to be 5
 * then check the sending bytes
 */
void testall(int32_t argc, char* argv[])
{
	FILE *log_test;
	unsigned char userid[ID_LEN] = "ALICELEE";
	unsigned char password[6] = "123456";
	log_test = fopen("log_program1_test.txt", "w");
	struct user_login *curr_user_test = 
				malloc(sizeof(struct user_login));
	memset(curr_user_test,0,sizeof(struct user_login));
	memcpy(&curr_user_test->userid,userid,ID_LEN);
	memcpy(&curr_user_test->password,password,6);
	assert(command_exe(curr_user_test, log_test, 2, 5, argc, argv)==0);
	assert(strcmp(output, "02 00 00 00 01 41 4c 49 43 45 4c 45 45 7c 4a 8d \n"
			"09 ca 37 62 af 61 e5 95 20 94 3d c2 64 94 f8 94 \n"
			"1b 00 00 00 05 \n") == 0);
	free(output);
	free(curr_user_test);	
	fclose(log_test);
	printf("Test has passed!\n");
	
	
	
}
/*
 * Authors: Zhi Li
 * Purpose: It starts the whole program at user side by checking
 *          the input arguments to run the program or test or
 *	    show instruction
 * 
 * Expected input: 1)no arguments---show instruction
 *	           2)[AC ipv4 address] [AC port number] [local
 *		     port number]
 *		     enter login mode and input command using 
 *		     keyboard
 *		   3)[AC ipv4 address] [AC port number] [local
 *		     port number] < [file]
 *	             enter login mode and use file as input 
 *	             command
 *		   4)test run self test
 *
 * Implementation details: 
 * If there is no argument, it shows the instruction.
 * If the input argument is test, it runs the self test.
 * If the address and port number is entered, it first creates
 * the server thread which makes server is always up and running in
 * behind. Then it enters the login mode. After the user login,
 * it could continuous ask user to send request until the user
 * log out. After user log out, it go back to the login mode.
 * It returns 0 after the program terminates without error.
 */
int main(int32_t argc, char* argv[])
{
	//first check arguments
	//if no arguments are given
	//show the usage instructions instead of running
	if(argc == 1)
	{
		printf("usage: %s             "
				"show instructions\n", argv[0]);
		printf("   or: %s test        "
				"run the automatic testing\n", argv[0]);
		printf("   or: %s [AC ipv4 address] [AC port number] "
			"[local port number] start the program and use "
			"keyboard as input\n", argv[0]);
		printf("   or: %s [AC ipv4 address] [AC port number] "
			"[local port number] < [file ..]  start the "
			"program and use file as input\n", argv[0]);
		printf("After starting the program, use <COMMAND> as input\n"
			"<COMMAND>:\n"
			"   LOGIN <USERID> <PASSWORD>\n"
			"   DEPOSIT $<DOLLARS>\n"
			"   BALANCE\n"
			"   BUY <INDEX>\n"
			"   PRICE <INDEX>\n"
			"   STATUS\n"
			"   LOGOUT\n"
			"   EXIT\n"
			"      <USERID> is the user id\n"
			"      <PASSWORD> is the user password\n"
			"      <DOLLARS> is the money user wants to deposit\n"
			"      <INDEX> is the item number"
			"(A followed by an integer, e.g. A1)\n");
	}
	else
	{
		//run the self test
		if(argc >= 2 && strcmp(argv[1], "test") == 0)
		{
			testall(argc,argv);
		}
		//run the program
		else
		{
			//initialize random number generator
			srand((unsigned)time(NULL));
			//Pointer to the log file
			FILE *log;
			log = fopen("log_program1.txt", "w");
			int32_t mode;
			if(log == NULL)
			{
				printf("unable to open file\n");
				exit(1);
			}
			//initialize the user state
			struct user_login *curr_user = 
				malloc(sizeof(struct user_login));
			memset(curr_user,0,sizeof(struct user_login));
			// thread ID
			pthread_t threadID;
			struct ThreadArgs *threadArgs = 
				malloc(sizeof(struct ThreadArgs));
			// initiate the thread input parameters
			threadArgs->curr_user = curr_user;
			threadArgs->log = log;
			threadArgs->argc = argc;
			threadArgs->port = argv[3];
			// Create the server thread
			int32_t returnValue = pthread_create(&threadID, 
					NULL, recv_response,threadArgs);
			if(returnValue != 0)
			{
				exit_with_error("pthread_create() failed");
			}
			// user login
			user_login(threadID,log,curr_user);
			// keep asking the user request until user log out
			while(!stop)
			{
				mode = send_request(threadID,curr_user,
							log, argc, argv);
				if(mode == LOGOUT_CODE)
				{	
					// after log out re-enter login mode
					user_login(threadID,log,curr_user);
				}
			}			
			//free memory
			free(curr_user);
			free(threadArgs);
			//close log file
			fclose(log);
			//close the server
			cleanup_server();
		}
	}	
	return 0;
}
