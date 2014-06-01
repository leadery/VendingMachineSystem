#ifndef _SERVERFILE_HANDLE_
#define _SERVERFILE_HANDLE_

#include <stdio.h>
#include <stdint.h>
#include <algorithm>
#include <unordered_map>
#include "common.h"
extern char *output;

struct user_info
{
	std::string userid;
	unsigned char password[SHA_DIGEST_LENGTH];
	uint32_t balance;
};
typedef struct user_info user;

typedef std::unordered_map<std::string, user> user_map;

extern user_map user_all;

int8_t check_code(uint8_t code);

void send_user_info();

void forward_user_request();

void send_tcp_data(FILE* rx, FILE *tx, void *data, int datalen);

void handle_tcp_client(int client, char* argv[]);

int8_t forward_request(char*, char*, request user_action, std::string userid_str);

void sprint_hex(uint8_t *data, size_t length);

void prepare_generic_socket(int argc, char *argv[], 
		int family, int flags, int type, int protocol);

void send_tcp_data(FILE* rx, FILE *tx, void *data, int datalen);

void receive_tcp_clients(char *argv[]);
#endif
