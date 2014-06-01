/***********************************************************************
 *
 * Group name: LLL
 * Authors: Deyuan Li
 * Purpose: Builds a TCP based client/server application to simulates 
 * the functionality of a authentication center. It allows buyer to deposit,
 * inquire balance, buy items, inquire the price, track status, and it will
 * communicate with the vending server to perform the corresponding request 
 * 
 * expected inputs: 
 * Before starting the program:
 * no argument: show usage instruction
 * test: run selftest
 * <port number1> <ipv4 address> <port number2>:
 *	start the program
 * 
 *
 * The program client_util.c client_util.h and common.h
 * This file contains the 7 functions.
 * - void user_info_init()
 *   It reads from the file which contains the user's information.
 *   Encrypte each password and create a hash map with all the user's ID, 
 *   encrypted password, and balance.
 * - int8_t check_code(uint8_t code)
 *   It takes code of each packet from the user as input. If the code is
 *   not valid, it will return -1 indicating the main function to discard
 *   the packet.
 * - void set_balance_packet(struct balance_packet* user_balance,
 *		request user_request, uint8_t status, std::string userid_str)
 *   It takes pointer to struct balance_packet, user's request packet,
 *   status, and the user ID of corresponding user as input.
 *   It sets the balance_packet will the value of other inputs.
 * - void handle_tcp_client(int client, char *argv[])
 *   It takes file descriptor client, vending server's IP address and
 *   port number argv[] as input. It receives packet from client. If it
 *   is a BALANCE or DEPOSIT packet, it will send the user's information
 *   back to the user. If it is a BUY/PRICE/STATUS packet, it will
 *   forward the request to the vending server using forward_request()
 *   function.
 * - int8_t forward_request(char* server_ip, char* server_port, 
 *		request user_packet_rcvd, std::string userid_str)
 *   It takes the server_ip, server_port, the request packet from the
 *   user, user id as input. If the request packet from user is a
 *   PRICE/STATUS packet, it forwards the request to the vending server.
 *   If the request packet from user is a BUY packet, it communicate withi
 *   the vending server to fultill the BUY request.
 * - void testall()
 *   It is self test function
 * - int main(int argc, char *argv[])
 *   If there is no argument, show usage instructions. If "test" is 
 *   provided, it runs self-test.
 *   If <port number1> <ipv4 address> <port number2> are provided,
 *   it starts the program and enters the transaction mode.
 * 
 ************************************************************************/

#include "common.h"
#include "program2_util.h"
// flag to stop the infinite loop in server
int stop = 0;
int sock = 0;
// list of listening address
struct addrinfo *listen_addr = NULL;
char *output = NULL;
// hash map of the user information
user_map user_all;

/*
 * Authors: Deyuan Li
 * Purpose: It initialize the inforamtion of all users
 *
 * Implementation details:
 * It reads from the file which contains the user's information.
 * Encrypte each password and create a hash map with all the user's ID, 
 * encrypted password, and balance.
 */
void user_info_init()
{
	// open the file storing all users' userid, password, balance
	// in plaint text
	FILE *userfile = fopen("user_info.txt","r");
	printf("Loading user info......");
	// Terminate the program if the file could not be opened
	if(userfile == NULL)
	{
		printf("Unable to open file\n");
		exit(1);
	}
	// Used to store each line of the file
	char line[MAXLENGTH];
	// Store one user's information temporarily
	user data;
	char name[10];
	char password[20];
	// Read the file line by line to write the users' information
	// into memory
	while(fgets(line, MAXLENGTH, userfile) != NULL)
	{
		sscanf(line, "%s%s%d",name,password,&data.balance);
		data.userid = std::string(name);
		// Use SHA1() from openssl/sha.h to encrypt the password
		SHA1((unsigned char*)password, strlen(password), data.password);
		user_all[data.userid] = data;
	}
	fclose(userfile);
	printf("Done\n");
}

/*
 * Authors: Deyuan Li
 * Purpose: It checks the validity of the code from the request packet
 *
 * Implementation details:
 * It compares the received code with the known code value and returns
 * the result
 */
int8_t check_code(uint8_t code)
{
	switch(code)
	{
		case USER_CODE:
		case BUY_CODE:
		case PRICE_CODE:
		case STATUS_CODE: return 0;
		// The code doesn't match the value of any known code, return -1
		default: return -1;
	}
}

/*
 * Authors: Deyuan Li
 * Purpose: Set the value of each variable of *user_balance
 *
 * Implementation details:
 * The function takes user_balance as input and set its variables with
 * other input
 */
void set_balance_packet(struct balance_packet* user_balance, 
		request user_request, uint8_t status, std::string userid_str)
{
	user_balance->code = BALANCE_CODE;
	user_balance->seq_num = htonl(user_request.seq_num);
	// The request packet is valid
	if(status == 1)
		user_balance->balance = htonl(user_all[userid_str].balance);
	// Got an invalide request
	else
		user_balance->balance = 0;
	user_balance->status_code = status;
}


/*
 * Authors: Deyuan Li
 * Purpose: This function processes the request packet from the user
 * and send corresponding packet to the user and the pending server
 *
 * Implementation details:
 * It receives packet from client. If it is a BALANCE or DEPOSIT
 * packet, it will send the user's information back to the user. If it is a
 * BUY packet, it will send a balance_packet to the user indicating the
 * request has been accepted successffly and forward the user's request to
 * the vending server using forward_request() function.
 */
void handle_tcp_client(int client, char *argv[])
{	
	FILE *rx = fdopen(client, "r");
	FILE *tx = fdopen(dup(client), "w");
	uint8_t code_rcvd = 255;
	printf("Receiving packet from user......\n");
	// receive the code of the requst packet from the user
	size_t bytes_received = fread(&code_rcvd, sizeof(uint8_t), 1, rx);
	// receive failed
	if (bytes_received < 0)
	{
		fclose(tx);
		fclose(rx);
		exit_with_error("fread failed");
	}
	// receive succeeded, check the validity of the code
	else if(check_code(code_rcvd) == 0)
	{
		request user_packet_rcvd;
		//paste the code to the receiving packet
		user_packet_rcvd.code = code_rcvd;
		//receive the rest of the request packet
		bytes_received = fread(&user_packet_rcvd.seq_num, 
				sizeof(request)-1, 1, rx);
		//receive failed
		if(bytes_received < 0)
		{
			fclose(tx);
			fclose(rx);
			exit_with_error("fread failed");
		}
		// change the byte order of received seq_num and request_value 
		else
		{
			user_packet_rcvd.seq_num = ntohl(user_packet_rcvd.seq_num);
			user_packet_rcvd.request_value = 
				ntohl(user_packet_rcvd.request_value);
		}
		// extract the userid from the request packet
		struct balance_packet user_balance;
		char userid_temp[ID_LEN+1];
		strncpy(userid_temp, (const char*)user_packet_rcvd.userid, ID_LEN);
		userid_temp[ID_LEN] = '\0';
		std::string userid_str = std::string(userid_temp);
		// check if the userid and the password are matched
		if(strncmp((const char*)user_packet_rcvd.password,
					(const char*)user_all[userid_str].password,
					SHA_DIGEST_LENGTH) != 0)
		{
			printf("Invalid id or password.\n");
			// send a balance_packet to the user with status == 2
			// indicating the request packet is invalid
			set_balance_packet(&user_balance,
					user_packet_rcvd, 2, userid_str);
			send_tcp_data(rx, tx, &user_balance, 
					sizeof(struct balance_packet));
		}
		else
		{
			// BALANCE or DEPOSIT request
			if(user_packet_rcvd.code == USER_CODE)
			{
				printf("USER_CODE detected!\n");
				// BALANCE request
				if(user_packet_rcvd.request_value == 0)
				{
					// send the balance_packet back to user
					set_balance_packet(&user_balance, 
							user_packet_rcvd, 1, userid_str);
					send_tcp_data(rx, tx, &user_balance,
							sizeof(struct balance_packet));
				}
				// DEPOSIT request
				else
				{
					// Deposit
					user_all[userid_str].balance +=
						user_packet_rcvd.request_value;
					printf("Deposit success! Balance: %u\n",
							user_all[userid_str].balance);
					// send the balance_packet back to user
					set_balance_packet(&user_balance, 
							user_packet_rcvd, 1, userid_str);
					send_tcp_data(rx, tx, &user_balance, 
							sizeof(struct balance_packet));
				}
			}
			// BUY request
			else if(user_packet_rcvd.code == BUY_CODE)
			{
				printf("BUY_CODE detected!\n");
				// forward the BUY request to the vending server
				set_balance_packet(&user_balance,
						user_packet_rcvd, 1, userid_str);
				send_tcp_data(rx, tx, &user_balance,
						sizeof(struct balance_packet));
				forward_request(argv[2],argv[3],user_packet_rcvd,
						userid_str);
			}
			// PRICE/STATUS request
			else if(user_packet_rcvd.code == PRICE_CODE ||
					user_packet_rcvd.code == STATUS_CODE)
			{
				printf("PRICE/STATUS_CODE detected!\n");
				// forward the PRICE/STATUS request to the vending server
				set_balance_packet(&user_balance,
						user_packet_rcvd, 1, userid_str);
				send_tcp_data(rx, tx, &user_balance,
						sizeof(struct balance_packet));
				forward_request(argv[2], argv[3], user_packet_rcvd,
						userid_str);
			}
		}
	}
	//invalid packet code
	else 
	{
		printf("Invalid packet!\n");
	}
	fclose(tx);
	fclose(rx);
}

/*
 * Authors: Deyuan Li
 * Purpose: This function forwards the request from the user to the vending
 *			server
 *
 * Implementation details:
 * It takes the server_ip, server_port, the request packet from the user, user id
 * as input. If the request packet from user is a PRICE/STATUS packet, it sends a
 * action_packet to the vending server. If it receives the packet from the user
 * indicating a BUY request, it will send a corresponding action_packet to the
 * vending server and expects to receive pricecheck_packet from it. 
 * After receiving the pricecheck_packet, it will send a
 * buyack_packet to the vending server to indicate whether the purchase has been
 * proceeded.
 */
int8_t forward_request(char* server_ip, char* server_port, 
		request user_packet_rcvd, std::string userid_str)
{
	// initialize the connection with the vending server
	struct addrinfo lookup_addr;
	memset(&lookup_addr, 0, sizeof(struct addrinfo));
	lookup_addr.ai_family = AF_UNSPEC;
	lookup_addr.ai_socktype = SOCK_STREAM;
	lookup_addr.ai_protocol = IPPROTO_TCP;

	struct addrinfo *send_addr;
	if (getaddrinfo(server_ip, server_port, 
				&lookup_addr, &send_addr) != 0)
	{
		perror("getaddrinfo failed");
		return 1;
	}

	int sock = socket(send_addr->ai_family, send_addr->ai_socktype,
			send_addr->ai_protocol);
	if (sock < 0)
	{
		perror("socket failed");
		return 1;
	}

	if (connect(sock, send_addr->ai_addr, send_addr->ai_addrlen) < 0)
	{
		perror("connect failed");
		return 1;
	}
	FILE *rx = fdopen(sock, "r");
	FILE *tx = fdopen(dup(sock), "w");
	// set the value of the action_packet
	struct action_packet user_action;
	user_action.code = user_packet_rcvd.code;
	user_action.seq_num = htonl(user_packet_rcvd.seq_num);
	user_action.item_index = htonl(user_packet_rcvd.request_value);
	// forward the user's request
	send_tcp_data(rx, tx, &user_action, sizeof(struct action_packet));
	// BUY request, it expects a pricecheck_packet from the vending server
	if(user_action.code == BUY_CODE)
	{
		// receive and check the code of the packet from the
		// vending server
		struct pricecheck_packet item_price;
		size_t bytes_received = fread(&item_price, 
				sizeof(struct pricecheck_packet), 1, rx);
		if(bytes_received < 0)
		{
			fclose(tx);
			fclose(rx);
			exit_with_error("fread from server failed");
		}
		// The code is valid and compare the item price
		// and user balance
		if(item_price.code == PRICECHECK_CODE)
		{
			// Change seq_num and price into host byte order
			// for calculation with user balance
			item_price.seq_num = ntohl(item_price.seq_num);
			item_price.price = ntohl(item_price.price);
			struct buyack_packet buy_stat;
			buy_stat.code = BUYACK_CODE;
			// change seq_num into network byte order
			// for sending to vending server
			buy_stat.seq_num = htonl(item_price.seq_num);
			buy_stat.index = user_action.item_index;
			printf("Sending buy ack......");
			// User balance is enough for the required item
			// BUY request is handled successfully
			if(item_price.price <= user_all[userid_str].balance)
			{
				user_all[userid_str].balance -= item_price.price;
				buy_stat.status_code = 1;
			}
			// Insufficient money
			else
				buy_stat.status_code = 2;
			send_tcp_data(rx, tx, &buy_stat, 
					sizeof(struct buyack_packet));
			printf("Done!\n");
		}
		else
		{
			printf("Invalid code: %d\n", item_price.code);
		}
	}
	fclose(tx);
	fclose(rx);
	freeaddrinfo(send_addr);
	close(sock); 
	return 0;
}

/*
 * Authors: Deyuan Li
 * Purpose: It tests initialization of the user information. Functions
 *          in handle_tcp_client are working properly.
 *
 * Expected input: It takes argument test as input
 * 
 * Implementation details: 
 * It tests user_info_init(), check_code(), set_balance_packet() functions
 * to see how the client handle request from the user and deal with the
 * received packet, and how the sending packet to the vending server is
 * formed.
 *
 * the process in the testall() is:
 * run user_info_init();
 * set a test user information and encrypt its password,
 * and check if the user information is matched with the test user in the 
 * hash map.
 * 
 * form a test request packet with code = USER_CODE;
 * check if check_code() can return 1 after checking the correct code.
 * 
 * form a test balance_packet and use set_balance_packet() to set its value.
 * Check the whether the test balance_packet's variable has been set
 * properly.
 */
void testall()
{
	// TEST initialization of the user information
	user_info_init();
	std::string user_test="test_user";
	const char *password_test="12345678";
	unsigned char password_hash[20];
	SHA1((unsigned char*)password_test, strlen(password_test),
			password_hash);
	assert(strncmp((const char*)(user_all[user_test].password),
				(const char*)password_hash,20)==0);
	// TEST check_code()
	request request_test;
	request_test.code = USER_CODE;
	assert(check_code(request_test.code) == 0);
	request_test.seq_num = 13579;
	strncpy((char *)request_test.userid,"test_user",strlen("test_user"));
	strncpy((char *)request_test.password,(const char*)password_hash,20);
	// TEST set_balance_packet()
	struct balance_packet balance_test;
	set_balance_packet(&balance_test, request_test, 1, user_test);
	assert(balance_test.balance == htonl(123));
}



/*
 * Authors: Deyuan Li
 * Purpose: It checks the arguments and run the program or
 *	    test or show instruction
 * 
 * Expected input: 1)no arguments
 *			show instruction
 *	           2)test
 *			run self-test
 *		   3)[port number1] [ipv4 address] [port number2]
 *			port number1 is the local port number;
 *			ipv4 address is the IP address of the
 *			vending server; port number2 is the port
 *			number of the vending server
 *
 * Implementation details: Check the arguments are provided.
 * Run the corresponding mode.
 */
int main(int argc, char *argv[])
{
	// print the instruction
	if(argc == 1)
	{
		printf("usage: %s             "
				"\n\t\tshow instructions\n", argv[0]);
		printf("   or: %s test        "
				"\n\t\trun the automatic testing\n", argv[0]);
		printf("   or: %s [port number1] [ipv4 address] [port number2]"
				"\n\t\t[port number1]: local port number"
				"\n\t\t[ipv4 address]: IP address of vending server"
				"\n\t\t[port number2]: port number of vending server\n",
				argv[0]);
	}
	else
	{
		//run the test
		if(argc >= 2 && strcmp(argv[1], "test") == 0)
		{
			testall();
		}
		//run the program
		else
		{	
			user_info_init();

			register_tcp_handlers();

			prepare_generic_socket(argc, argv, AF_INET, 
					AI_PASSIVE, SOCK_STREAM, IPPROTO_TCP);

			if (listen(sock, 1) < 0)
			{
				exit_with_error("listen failed");
			}

			receive_tcp_clients(argv);

			cleanup();
		}
	}
	return 0;
}
