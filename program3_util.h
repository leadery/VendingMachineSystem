/* header file for 3_util.c */
#include "common.h"

#ifndef UTIL_H_3
#define UTIL_H_3

extern int stop;
extern int sock;
extern struct addrinfo *listen_addr;
extern char *output;

struct ser_tran
{
	uint8_t code;
	uint32_t seq_num;
	uint32_t money_available;
	uint32_t item_index;
}__attribute__((packed));
extern struct ser_tran *exit_torecv;

/*
* parameters of the items
* in the vending machin
* e
*/
struct item_info
{
	//The label of each item
	char *label;
	//Amount of the item
	uint32_t num;
	//Price of each item
	uint32_t price;
} __attribute__((packed));

/*
* parameters of the vending machine
*/
struct machine_info
{
	//Balance of the customer
	uint32_t balance;
	//Number of the item kind
	uint32_t item_num;
	//store the number of each kind of coins
	struct item_info *item;
	//Pointer to the log file
	FILE *log;
	//pointer to bought packet array
	struct respond_packet *ptr_bg[100];
	//ptr_bg index
	uint8_t pg_index;
} __attribute__((packed));
typedef struct machine_info machine;

void sprint_hex(uint8_t *data, size_t length);

void input_log(char *input_string, machine* p_machine);

void input_error(machine* p_machine);

int32_t cmd_items(machine *p_machine);

int32_t cmd_item(machine *p_machine);

int32_t mode_startup(machine* p_machine);

int32_t cmd_buy(machine *p_machine,struct ser_tran *torecv,struct respond_packet *tosend );

void vending_exit_with_error(const char *msg, machine *p_machine);
#endif 
