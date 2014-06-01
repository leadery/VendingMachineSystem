/***********************************************************************
 *
 * Group name: LLL
 * Authors: Weiyan Li
 * Purpose: Builds a TCP based 3 Ways client/server application to simulates 
 * the functionality of a vending machine, which includes a thrid party to do the 
 * user authentication and deposit money.
 * It allows customer to deposit 
 * and withdraw money, buy items, and track status. Also, the code will 
 * create an audit log as it operates.
 * 
 * My program is the part of vending server, which received the commond message 
 * from Authentication Center and  respond the result of the buying process
 * to the user. 
 *
 * expected inputs: 
 * Before starting the program:
 * test: run test as argv
 * <port number1>:  the port that we open for our vending server
 * <port number2>:  the port that the user open for us
 * <ip address>: the address that the user open for us
 * start the program like : ./program3  <port number1> <ip addr> <port number2> 
 *
 * We offer an auto initialize file called ini_server.txt , which includes 
 * several items information.
 * start auto initial: ./program3 10689 < ini_server
 *
 * Example for start our program:
 * ./program3 10689 127.0.0.1 10699 < ini_server.txt
 *
 *
 * After normal starting the program, use <COMMAND> as input:
 * 
 * In STARTUP MODE:
 * ITEMS <NUMBER>
 *	<NUMBER> indicates the number of items to be loaded
 * ITEM <XYZ> <DOLLARS>d 
 * 	<XYZ> is the name of the item(1-20 uppercase letters, no space)
 * 	<DOLLARS> is the price prefixed by the letter d 
 * 
 *
 * The program associates program3_util.c , program3_util.h , common.h 
 * common.c and server.c
 *
 * The common.c contains the 5 common functions.
 * 
 * void cleanup()
 * void register_tcp_handlers()
 * void handler2(int signal);
 *
 * The server_util.c conatins buying process functions.
 * 
 * these buying functions come from our previous project
 *
 * void vending_exit_with_error(const char *msg,machine * p_machine);
 * void sprint_hex(uint8_t *data, size_t length);
 * void input_log(char *input_string, machine* p_machine);
 * void input_error(machine* p_machine);
 * int32_t cmd_items(machine *p_machine);
 * int32_t cmd_item(machine *p_machine);
 * int32_t mode_startup(machine* p_machine);
 * int32_t cmd_buy(machine *p_machine,struct ser_tran *torecv,struct respond_packet *tosend );
 *
 *
 * The program3.c contains tcp receiving and sending functions.
 *
 * void connect_user(machine* p_machine,struct ser_tran *torecv, int argc, char *argv[])
 * void prepare_generic_socket(int argc, char *argv[],int family, int flags, int type, int protocol)
 * void handle_buy(struct buy_packet *torecv,int client, machine *p_machine)
 * void send_tcp_data( int client, void *data, int datalen)
 * void receive_center(machine* p_machine)
 * void testall()
 * void handle_client(int client,machine* p_machine)
 *
 ************************************************************************/
#include "common.h"
#include "program3_util.h"

		int stop = 0;
		int sock = 0;
		struct addrinfo *listen_addr = NULL;
		char *output = NULL;
		struct ser_tran *exit_torecv = NULL;
		/*
		 * Authors:  Weiyan Lin
		 * 
		 * Purpose: It sends packet from the server
		 *
		 * Expected input: It takes respond packet from the result of cmd_buy and handle_buy
		 *
		 *
		 * Implementation details:
		 * It forms the send packet then sends it. 
		 * If the client of the user or of the Authentication Center
		 * is wrong, the send_tcp_data would 
		 * use sprint_hex() to print the data in char array
		 */
void send_tcp_data( int client, void *data, int datalen)
{
		if (client)
		{
				ssize_t bytes_sent = send(client, data, datalen, 0);
				if (errno == EPIPE)
				{
						close(client);
						vending_exit_with_error("pipe error\n",NULL);
				}
				else if (bytes_sent < 0)
				{
						close(client);
						vending_exit_with_error("send failed",NULL);
				}
		}
		else
		{
				sprint_hex(data, datalen);
		}
}




/*
 * Authors: Weiyan Lin
 * 
 * Purpose: It handle the ser_tran , which we translate it from the 
 * Authentication Center message ,  and use cmd_buy to do the buying 
 * process and use send_tcp_data to send result to the user
 *
 * Expected input: It takes the client of the user , information of the machine ,
 * and the ser_tran packet as input
 *
 * our receive_client would translate the message from
 * Authentication Center to the ser_tran packet, which can be 
 * handle easily by handle_buy and cmd_buy
 *
 * Implementation details:
 * 
 * It would show the packet we received on the server,
 * then use the index information  from the ser_tran packet
 * to find the target item
 * 
 * If the item index is in our machine, we would continue buying process
 * else  we would return a specific respond packet to notice Certification
 * Center 
 *
 * Continuing our buying process, we would check the seq_num from buy packet,
 * If this seq_num exists in our machine, we would send respond packet from the memory
 * directly to the user. 
 * Else we would continue our buying process with cmd_buy().
 *
 * Finially, we got a handled respond packet. Then we would use send_tcp_data function
 * to send our respond packet
 */
void handle_buy(struct ser_tran *torecv, 
				int	sock, machine *p_machine)
{
		// show what we got from the Authentication Center
		printf("code: %u\n", torecv->code);
		printf("index: A%u\n", torecv->item_index);
		uint32_t send_length;
		uint8_t i;
		//get the seq_num
		uint32_t seq_num = torecv->seq_num;
		uint32_t name_length;
		uint32_t index=p_machine->pg_index;
		struct respond_packet *tosend = NULL;
		//check if the item_index exists
		if(torecv->item_index <= p_machine->item_num && torecv->item_index >0 )
		{
				//check if the seq_num exists
				if (sock > 0)
				{
						for (i=0; i <99; i++)
						{
								if ( p_machine->ptr_bg[i] != NULL)
								{       
										//if seq_num exists, get the address from pointer array
										//then send the data directly
										if (p_machine->ptr_bg[i]->seq_num == seq_num)
										{
												name_length=p_machine->ptr_bg[i]->name_length;
												send_length = sizeof(struct respond_packet)+name_length-1;
												send_tcp_data(sock, p_machine->ptr_bg[i],send_length);
												return;
										}
								}

						}
				}
				//make sure the length of our respond because item's name is different
				name_length = strlen(p_machine->item[torecv->item_index-1].label);
				send_length = sizeof(struct respond_packet)+name_length-1;
				//malloc for respond packet
				tosend = malloc(send_length);
				cmd_buy(p_machine,torecv,tosend);
				//input our respond packet in to a pointer array 
				// so we could check the seq_num and know whether we go the seq_num before
				p_machine->ptr_bg[index] = tosend;
				p_machine->pg_index+=1;
				if(p_machine->pg_index > 99)
						p_machine->pg_index=0;
		}
		else
		{
				// if we got something wrong about the title
				// we would make a specific respond packet, which name is '\0' and
				// status is 2 to the user 
				send_length = sizeof(struct respond_packet);
				tosend = malloc(send_length);
				tosend->seq_num = torecv->seq_num;
				if (torecv->code == BUYACK_CODE)
				{
						tosend->code = BUYRES_CODE;
				}
				else
				{
						tosend->code = torecv->code+3;
				}
				tosend->price = 0;
				tosend->name_length = 0;
				tosend->status_code = 2;
				tosend->name = '\0';
		}
		send_tcp_data(sock, tosend, send_length);
		if(sock == 0 )
		{
				free(tosend);
		}
}


/*
 * Authors: Weiyan Lin
 * 
 * Purpose: It receives data from the Authentication Center
 * then call handle_buy() 
 * function to handle the buy packet.
 *
 * Expected input: It takes file descriptor data from accept(), 
 * machine info as input
 *
 * Implementation details:
 * It receives the first byte of the first byte from the 
 * Authentication Center, if the packet is
 * action_packet, we woulc check the code of this packet.
 *
 * CODE == BUYCODE   =>    send pricecheck_packet with price to 
 * the Authentication Center and wait the buyack_packet 
 *
 * CODE == PRICE_CODE or STATUS CODE => 
 * translate action_packet to ser_tran and finish the handle_client function
 *
 * if the the packet is buyack_packet 
 * we translate the buyack_packet to a ser_tran packet and then finish 
 * handle_client part  
 *  
 */
void handle_client(int client,machine* p_machine,struct  ser_tran *torecv)
{
		// set two struct to receive the packets from the Authentication Center
		char buffer[30];
		struct action_packet act_buffer;
		struct buyack_packet buychk_buffer;
		// associate *rx with Authentication Center
		FILE *rx = fdopen(client, "r");
		// receive the first byte of the first packet(password packet)
		size_t bufs_received= fread(buffer, 1, 1, rx);
		// if receive failed, exit with an error
		if(bufs_received <0)
		{
				fclose(rx);
				vending_exit_with_error("recv failed",p_machine);
		}
		// check the first CODE of the packet
		else if (buffer[0]==1)
		{
				//it deal with the buy action
				//got the last data from the rx	
				fread(buffer+1,sizeof(struct action_packet)-1,1,rx);
				memcpy(&act_buffer,buffer, sizeof(struct action_packet));
				struct pricecheck_packet prichk;			
				prichk.code=PRICECHECK_CODE;
				prichk.price=ntohl(p_machine->item[ntohl(act_buffer.item_index)-1].price);
				prichk.seq_num=act_buffer.seq_num;

				//send price to the Authentication Center
				send_tcp_data(client,&prichk,sizeof(struct pricecheck_packet));	


				//get the buyack from the Authentication Center
				fread(buffer,sizeof(struct buyack_packet),1,rx);	
				memcpy(&buychk_buffer,buffer, sizeof(struct buyack_packet));
				torecv->code=buychk_buffer.code;
				torecv->seq_num=buychk_buffer.seq_num;
				torecv->item_index=ntohl(buychk_buffer.index);
				//if status is success, we would put enough money to 
				//the ser_tran packet, so our buying process would be success
				if ( buychk_buffer.status_code==1)
				{
						torecv->money_available=p_machine->item[ntohl(buychk_buffer.index)-1].price;
				}
				//if status is wrong , we would put 0 money to the ser_tran packet
				//so the status would be fail and the user would get the price of 
				//the item that they want
				else
				{
						torecv->money_available=0;
				}


		}
		else 
		{
				//if CODE is PRICE_CODE or STATUS_CODE 
				//we would copy the CODE to ser_tran 
				//and set the money_available is 0.
				fread(buffer+1,sizeof(struct action_packet)-1,1,rx);
				memcpy(&act_buffer,buffer, sizeof(struct action_packet));

				torecv->code=act_buffer.code;
				torecv->seq_num=act_buffer.seq_num;
				torecv->item_index=ntohl(act_buffer.item_index);
				torecv->money_available=0;			

		}
		//close the connection
		fclose(rx);
}	


/*
 * Authors: Weiyan Lin
 *
 * Purpose: It receives a version-independent TCP socket and establishes 
 * the connection with the Authentication Center. 
 *
 * Expected input: It takes the machine information and an empty ser_tran 
 * to get the information from Authentication Center
 *
 *
 * Implementation details:
 * 
 * Function continually accept the sock from Authentication Center to establish
 * the connection between the Authentication Center and the vending_server.
 * Once connection established, the receive_center function would use 
 * handle_client function to handle the Authentication Center.
 *
 *
 */
void receive_center(machine* p_machine,struct ser_tran *torecv)
{

		struct sockaddr_storage client_addr;

		socklen_t addr_len = sizeof(struct sockaddr_storage);

		//get the connection request from the Authentication Center	

		int client = accept(sock, 
						(struct sockaddr *) &client_addr,
						&addr_len);
		//if connect fail, exit and show the error message
		if (client < 0)
		{
				vending_exit_with_error("accept failed",p_machine);
		}

		// connected successfully, and then we would use handle_client
		// to get the information 	
		handle_client(client, p_machine,torecv);


}


/*
 * Authors: Weiyan Lin
 *
 * Purpose: It makes a version-independent TCP socket and establishes 
 * the connection with the User
 * then it would use handle_function to do the buying process. 
 *
 * Expected input: It takes the machine informationi, argc ,argv,
 * which contains the ip address and port information of the user, 
 * and a ser_tran packet,
 * which contains the translated information from Authentication Center
 *  
 *
 * Implementation details:
 * 
 * connect_user function would initial the connection 
 * between the Authentication Center and the vending_server.
 * Once connection established, the receive_center function would use 
 * handle_buy function to handle the buying process.
 *
 *
 */
void connect_user(machine* p_machine,struct ser_tran *torecv, int argc, char *argv[])
{

		// initial the connect with the user 
		// criteria for address match
		struct addrinfo lookup_addr;
		// zero out the structure
		memset(&lookup_addr, 0, sizeof(struct addrinfo));
		// set address family
		lookup_addr.ai_family = AF_UNSPEC;
		// set sockets type
		lookup_addr.ai_socktype = SOCK_STREAM;
		// set protocol type
		lookup_addr.ai_protocol = IPPROTO_TCP;
		// get addresses
		struct addrinfo *send_addr;
		if (getaddrinfo(argv[2], argv[3], &lookup_addr, &send_addr) != 0)
		{
				vending_exit_with_error("getaddrinfo failed",p_machine);
		}

		int sock_s = -1;
		// Create a reliable, stream socket using TCP
		sock_s = socket(send_addr->ai_family, send_addr->ai_socktype,send_addr->ai_protocol);
		// Socket creation failed; try next address
		if (sock_s < 0)
		{
				vending_exit_with_error("setup socket failed, unable to connect",p_machine);
		}
		// Establish the connection
		if(connect(sock_s, send_addr->ai_addr,send_addr->ai_addrlen) <0)
		{
				vending_exit_with_error("connect failed",p_machine);
		}

		//handle the do buying do process depend on the message we get
		//if it is a status message
		//we would use a loop to send all the item information to 
		//the user
		if (torecv->code==STATUS_CODE)
		{
				int index=p_machine->item_num;
				torecv->item_index=1;
				int i=0;
				for(i=0;i<index;i++)
				{	
						handle_buy(torecv,sock_s,p_machine); 			
						torecv->item_index+=1;
						torecv->seq_num+=1;
				}
		}
		// if it is not a status message
		// we only need to run the program one time
		else
		{
				handle_buy(torecv,sock_s,p_machine);
		}
		free(send_addr);
		//close connection
		close(sock_s);
		sleep(2);
}



/*
 * Authors: Weiyan Lin
 * Purpose: It creates a version-independent TCP socket
 *
 * Expected input: It takes the in put arguments,address family,socket
 * type and protocol type as input
 *
 * Implementation details:
 * It creates a version-indepedent TCP server. 
 * It help user set the IP protocols of the server.
 * If it can not the the right port for TCP/ip ,it would exit with wrong add info.
 * If it can not open a socket through port, it would exit with socket failed info.
 * 
 * If it can not bind the listen address with the socket , it would exit with 
 * bind error info.
 *
 * If evereything is ok, it would not show any error information.
 */
void prepare_generic_socket(int argc, char *argv[], 
				int family, int flags, int type, int protocol)
{
		struct addrinfo lookup_addr;
		memset(&lookup_addr, 0, sizeof(struct addrinfo));
		lookup_addr.ai_family = family; 
		lookup_addr.ai_flags = flags;
		lookup_addr.ai_socktype = type;
		lookup_addr.ai_protocol = protocol;
		printf("%s\n",argv[0]);
		printf("%s\n",argv[1]);
		printf("%s\n",argv[2]);
		printf("%s\n",argv[3]);
		if (getaddrinfo(NULL, argv[1], &lookup_addr, &listen_addr) != 0)
		{
				vending_exit_with_error("getaddrinfo 2  failed",NULL);
		}

		sock = socket(listen_addr->ai_family, listen_addr->ai_socktype,
						listen_addr->ai_protocol);
		if (sock < 0)
		{
				vending_exit_with_error("socket failed",NULL);
		}
		if (bind(sock, listen_addr->ai_addr, 
								listen_addr->ai_addrlen) < 0)
		{
				vending_exit_with_error("bind failed",NULL);
		}

}



/*
 * Authors: Weiyan Lin
 * Purpose: It tests how the server handle 
 * the received packet, what happend in server and how data send.
 *
 * Expected input: It takes argument test as input
 * 
 * Implementation details: 
 * It tests handle_buy(), cmd_buy() functions
 * to see how the server deal with
 * received packet, what happend in server and how data send.
 * 
 *
 * the process in the testall() is:
 * input the buy packet ---> 
 * which seq 10 balance 100 and index 1
 * server log:
 * seq_num 10 BUY  APPLE A1 2 2 - - - -
 * then show the packet we send with analysis
 *
 * input the buy packet ---> 
 * which seq 20 balance 100 and index 1
 * server log:
 * seq_num 20 BUY  APPLE A1 1 27 - - - -
 * then show the packet we send with analysis
 *
 * input the buy packet ---> 
 * which seq 30 balance 100 and index 1
 * server log:
 * seq_num 30 BUY  APPLE A1 0 27 - - - -
 * then show the packet we send with analysis
 */

void testall()
{	
		//initial start up
		machine *p_machine_test=malloc(sizeof(machine));
		memset(p_machine_test,0,sizeof(machine));
		p_machine_test->log = fopen("log.txt", "w");	
		p_machine_test->item_num=3;
		int32_t i;
		p_machine_test->item=
				malloc(p_machine_test->item_num*sizeof(struct item_info));
		p_machine_test->item[0].label="APPLE";
		p_machine_test->item[1].label="BANANA";
		p_machine_test->item[2].label="CARROT";

		for (i=0;i<3;i++)
		{
				p_machine_test->item[i].num=3;
		}

		p_machine_test->item[0].price=1;
		p_machine_test->item[1].price=22;
		p_machine_test->item[2].price=10;
		//start up finished

		printf("INITIAL MACHINE FINISHED\n");	
		//for testall we only test the handle_client function
		//for register , prepare_socket , 
		//we leave it to test in really communication or makefile test	

		printf("buy A1:\n");
		printf("the torecv detail:\n");
		//	struct	sockaddr_storage *client_addr=NULL;
		struct  ser_tran torecv;
		torecv.code=1;
		torecv.seq_num=htonl(10);
		torecv.item_index=1;
		torecv.money_available=htonl(100);	
		int client = 0;
		handle_buy(&torecv,client,p_machine_test);
		printf("what you should see: 04 00 00 00 0a 00 00 00 01 05 01 41 50 50 4c 45\n");
		printf("code: 04 \nseq: 00 00 00 0a \nprice:00 00 00 01 \nname length: 05 \nstatus code: \
						01 \nname: 41 50 50 4c 45\n");

		printf("\nbuy A1 again:\n");
		printf("the torecv detail:\n");
		torecv.code=1;
		torecv.seq_num=htonl(20);
		torecv.item_index=1;
		torecv.money_available=htonl(100);	
		handle_buy(&torecv,client,p_machine_test);
		printf("what you should see: 04 00 00 00 14 00 00 00 02 05 01 41 50 50 4c 45\n");
		printf("code: 04 \nseq: 00 00 00 0a \nprice:00 00 00 02 \nname length: 05 \nstatus code: \
						01 \nname: 41 50 50 4c 45\n"); 

				printf("\nbuy A1 again:\n");
		printf("the torecv detail:\n");
		torecv.code=1;
		torecv.seq_num=htonl(30);
		torecv.item_index=1;
		torecv.money_available=htonl(100);	
		handle_buy(&torecv,client,p_machine_test);
		printf("what you should see is: 04 00 00 00 1e 00 00 00 1b 05 01 41 50 50 4c 45\n");
		printf("code: 04 \nseq: 00 00 00 1e \nprice:00 00 00 1b \nname length: 05 \nstatus code: \
						01 \nname: 41 50 50 4c 45\n");


		/*	
			for(i=0;i<p_machine_test->item_num;i++)
			{
			free(p_machine_test->item[i].label);
			}	*/
		free(p_machine_test->item);	
		fclose(p_machine_test->log);	

		free(p_machine_test);
		printf("\nTest has passed!\n");
}

/*
 * Authors: Weiyan Lin
 * Purpose: It checks the arguments and run the program or
 *	    test 
 * 
 * Expected input: 1)[port number]
 *		     enter transaction mode and input command
 *		     using keyboard
 *		   2)[port number] < [file]
 *	             enter transaction mode and use file as
 *	             input command
 *
 e* Implementation details: Check the arguments are provided.
 * Run the corresponding mode.
 */
int main(int argc, char *argv[])
{

		if (argc >= 2 && strcmp(argv[1], "test") == 0)
		{
				testall();
				return 0;
		}
		machine *p_machine=malloc(sizeof(machine));
		memset(p_machine,0,sizeof(machine));
		mode_startup(p_machine);
		register_tcp_handlers();
		prepare_generic_socket(argc, argv, AF_INET, AI_PASSIVE, SOCK_STREAM, IPPROTO_TCP);
		if (listen(sock, 1) < 0)
		{
				vending_exit_with_error("listen failed",p_machine);
		}
		while(!stop)
		{

				struct ser_tran *torecv=malloc(sizeof(struct ser_tran));
				exit_torecv = torecv;
				memset(torecv,0,sizeof(struct ser_tran));
				receive_center(p_machine, torecv);
				connect_user(p_machine, torecv,argc,argv);
				sleep(2);
				free(torecv);		
		}

		//	free all the p_machine information
		int i;
		for (i=0; i <99; i++)
		{
				if ( p_machine->ptr_bg[i] != NULL)
				{       
						//if seq_num exists, get the address from pointer array
						free(p_machine->ptr_bg[i]);
				}

		}

		for(i=0;i<p_machine->item_num;i++)
		{
				free(p_machine->item[i].label);
		}	
		free(p_machine->item);	
		fclose(p_machine->log);	
		free(p_machine);
		cleanup();
		return 0;
}
