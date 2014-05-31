#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <arpa/inet.h>
#include <time.h>
#include <openssl/sha.h>

#define MAXLENGTH 2000
#define ITEM_LABEL_MAXLEN 20
#define DEPOSIT_MAX 10000
#define PRICE_MAX 5000
#define USER_CODE 0
#define BUY_CODE 1
#define PRICE_CODE 2
#define STATUS_CODE 3
#define BUYRES_CODE 4
#define PRICERES_CODE 5
#define STATUSRES_CODE 6
#define BALANCE_CODE 7
#define PRICECHECK_CODE 8
#define BUYACK_CODE 9
#define LOGOUT_CODE 10
#define SUCCESS 1
#define FAILURE 2
#define ID_LEN 8
#define PSWD_MAXLEN 16

extern int sock;
// flag to stop the infinite loop in server
extern int stop;
// list of server addresses
extern struct addrinfo *send_addr;
// list of listening address
extern struct addrinfo *listen_addr;

// the packet sent from user side to authentication center
// the packet includes user information and user request
struct user_request_packet
{
	//set to 0 indicating a Deposit/Balance action
	//set to 1 indicating a Buy action
	//set to 2 indicating a Price action
	//set to 3 indicating a Status action
	uint8_t code;
	//arbitrary unique value specified by the user
	uint32_t seq_num;
	//user id
	unsigned char userid[ID_LEN];
	//encrypted user password
	unsigned char password[SHA_DIGEST_LENGTH];
	//if code is 0:balance of user input money
	//if code is not 0:request item index
	uint32_t request_value;
} __attribute__((packed));
typedef struct user_request_packet request;

// the packet sent from authentication center to vending server
// the packet used to forward the user request
struct action_packet
{
	//set to 1 indicating a Buy action
	//set to 2 indicating a Price action
	//set to 3 indicating a Status action
	uint8_t code;
	//arbitrary unique value specified by the user
	uint32_t seq_num;
	//item index
	uint32_t item_index;
} __attribute__((packed));

// the packet sent from vending server to user side
// the packet contains the respond message
struct respond_packet
{
	//set to 4 indicating a Buy respond
	//set to 5 indicating a Price respond
	//set to 6 indicating a Status respond
	uint8_t code;
	//the user's seq_num in response to
	uint32_t seq_num;
	//item price
	uint32_t price;
	//item name length
	uint8_t name_length;
	//1 for success and 2 for failure
	uint8_t status_code;
	//item name
	char name;
} __attribute__((packed));

// the packet sent from authentication center to user side
// the packet contains idendity checking info and the user balance
struct balance_packet
{
	//set to 7 indicating a Balance packet
	uint8_t code;
	//the user's seq_num in response to
	uint32_t seq_num;
	//user balance
	uint32_t balance;
	//1 for success and 2 for failure
	uint8_t status_code;
} __attribute__((packed));

// the packet sent from vending server to authentication center
// the packet contains the price for the requested item
struct pricecheck_packet
{
	//set to 8 indicating a Price check packet
	uint8_t code;
	//the user's seq_num in response to
	uint32_t seq_num;
	//requested item price
	uint32_t price;
} __attribute__((packed));

// the packet sent from authentication center to vending server
// the packet indicates whether the item could be purchased or not
struct buyack_packet
{
	//set to 9 indicating a Buy acknowledgment packet
	uint8_t code;
	//the user's seq_num
	uint32_t seq_num;
	//1 for success and 2 for failure
	uint8_t status_code;
	//item index
	uint32_t index;
} __attribute__((packed));

// frees memory and close the socket
void cleanup();
// handler for signal SIGINT SIGHUP SIGTERM
void handler2(int signal);
// sets the signal handler
void register_tcp_handlers();
// print the error messsage 
// free the memory and exit the program
void exit_with_error(const char *msg);
#endif
