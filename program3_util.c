/***********************************************************************
 *
 * Group name: LLL
 * Authors: Deyuan Li, Zhi Li, Weiyan Lin
 * Purpose: utility functions for common things in server that we used
 *
 *
 * We have 6 functions here
 * void sprint_hex(uint8_t *data, size_t length)
 * void input_error(machine* p_machine)
 * int32_t cmd_items(machine *p_machine)
 * int32_t cmd_item(machine *p_machine) 
 * int32_t mode_startup(machine* p_machine)
 * int32_t cmd_buy(machine *p_machine,struct buy_packet *torecv,struct respond_packet *tosend)
 * 
 ***********************************************************************
 */

/* utility functions for common things */
#include "program3_util.h"

/*
 * Authors: Weiyan lin
 * Purpose: exit program with the signal SIGINT ,and free all pointer 
 *   
 *
 * Expected input: It takes the wrong message and the pointer to
 * the vending machine as input.  
 * 
 * Implementation details: 
 * show the error message on the screen
 * and free all the memory we used in our program
 *
 * test by valgrind  
 * 
 *
 */

void vending_exit_with_error(const char *msg,machine *p_machine)
{
	perror(msg);
	int i;
	for (i=0; i <99; i++)
	{
		if ( p_machine->ptr_bg[i] != NULL)
		{       
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
	free(exit_torecv);	
	cleanup();
	exit(1);
}
/*
 * Authors: Weiyan lin
 * Purpose: It prints out the data in respond packet we sent byte by byte 
 * in the "output" char array  
 *
 * Expected input: It takes the respond packet , length
 * of the packet  
 * 
 * Implementation details: 
 * It reads data byte by byte in the buy packet and put them in
 * a string followed a certain format that 16 bytes in a line,
 * and each byte are stored in hex with a space in between.
 */
void sprint_hex(uint8_t *data, size_t length)
{
	char myoutput[MAXLENGTH];
	char tmp[MAXLENGTH]={'\0'};
	assert(length < 100);
	myoutput[0] = '\0';

	int i;
	for (i = 0; i < length; i++)
	{
		sprintf(tmp, "%02x ",(*data));
		data++;
		strcat(myoutput, tmp);
		if (i % 16 == 15)
		{
			strcat(myoutput, "\n");
		}
	}
	if (myoutput[strlen(myoutput)-1] != '\n')
	{
		strcat(myoutput, "\n");
	}

	output = strdup(myoutput);

	printf("the sended packet is:%s \n",output);
	free(output);			
};








/*
 * Authors: Deyuan Li
 * Purpose: It saves the input in the log file
 * 
 * Expected input: The input command from the keyboard or file
 *
 * Implementation details: The function takes the string from the
 * input and save it with proper format in the file which "log"
 * points to, and adds details later in other functions
 */
void input_log(char *input_string, machine* p_machine)
{
	char s[MAXLENGTH];
	//copy the original input string to a temporary string array
	//and change the last character from '\n' to ' '
	strcpy(s,input_string);
	s[strlen(s)-1]=' ';
	//write to the log
	fprintf(p_machine->log,"%s",s);
	fflush(p_machine->log);
}

/*
 * Authors: Deyuan Li
 * Purpose: Display an error message and save it in log file
 * when an invalid input occurs
 *
 * Expected input: It takes client info as input
 *
 * Implementation details:
 * If the input is detected invalid in other functions,
 * this function will be called. It outputs an error message
 * and save it in log file with the client information
 */
void input_error(machine* p_machine)
{
	char error_message[]="ERROR INVALID INPUT\n";
	printf("%s",error_message);
	fflush(p_machine->log);
}

/*
 * Authors: Deyuan Li
 * Purpose: It takes care of ITEMS command, read from the input
 * and initialize the number of items with the correct <NUMBER> value
 * 
 * Expected input: It takes machine info as input
 * command format: ITEMS <NUMBER>
 * <NUMBER> indicates the number of items to be loaded
 * 
 * Implementation details: The function deals with the string from stdin.
 * If the string is a valid ITEMS command, assign <NUMBER> to
 * p_machine->item_num
 * If the string is NOT a valid ITEMS command, call input_error() function.
 */
int32_t cmd_items(machine *p_machine)
{
	//store the array of chars from stdin
	char line[MAXLENGTH];
	//store the command from the input
	char command[MAXLENGTH];
	//Check the validity of each input
	char enter_check='\0';
	//item num from the input
	int32_t item_num;
	//read from stdin
	while(fgets(line, MAXLENGTH, stdin)!=NULL)
	{
		//record the command immediatly after the input
		input_log(line,p_machine);
		//Check the command
		if(sscanf(line, "%s", command)==EOF)
		{
			input_error(p_machine);
		}
		else if(strcmp(command, "ITEMS")==0)
		{
			int32_t result;
			result = sscanf(line, "ITEMS %d%c", &item_num, &enter_check);
			//Check the input format
			if(result != 2 || enter_check!='\n' || item_num <= 0)
			{
				input_error(p_machine);
			}
			//The input is in correct format
			else
			{
				p_machine->item_num=item_num;
				//write the machine status to the log
				fflush(p_machine->log);
				return 0;
			}
		}
		else
		{
			input_error(p_machine);
		}
	}
	return 1;
}

/*
 * Authors: Deyuan Li
 * Purpose: It takes care of ITEM command, read from the input
 * and initialize the parameters of each item with the correct
 * <LABEL> and <PRICE> value
 * 
 * Expected input: It takes machine info as input
 * command format: ITEM <LABEL> <PRICE>c
 * <LABEL> is the name of the item in string format
 * <PRICE> is the price of the item in integer format and should
 *         not be more than 300, prefixed with letter "c"
 * 
 * Implementation details: The function deals with the string from stdin.
 * If the string is a valid ITEM command, assign <LABEL> to <PRICE> to
 * corresponding parameters in p_machine->item[i].
 * If the string is NOT a valid ITEMS command, call input_error() function.
 *
 */
int32_t cmd_item(machine *p_machine)
{
	//store the command from stdin
	char command[MAXLENGTH];
	//store the input from stdin
	char line[MAXLENGTH];
	//<LABEL>
	char item_label[MAXLENGTH];
	//check the prefixed letter "c"
	char cent_check;
	//check the validity of the input
	char enter_check;
	//<PRICE>
	uint32_t price;
	uint32_t i=0;
	//
	//read from the input
	while((i<p_machine->item_num)&&fgets(line,MAXLENGTH,stdin)!=NULL)
	{
		//record the command immediately after the input
		input_log(line,p_machine);
		//check the validity of the command
		if(sscanf(line, "%s", command)==EOF)
		{
			input_error(p_machine);
		}
		//Got ITEM command
		else if(strcmp(command,"ITEM")==0)
		{
			int32_t result;
			result=sscanf(line, "ITEM %s%d%c%c",
					item_label, &price, &cent_check, &enter_check);
			//Check the validity of <LABEL> and <PRICE> 
			//and "c" arguments after command ITEM
			if(result!=4||cent_check!='d'||enter_check!='\n'
					||strlen(item_label)>ITEM_LABEL_MAXLEN
					||price>PRICE_MAX||price<=0)
			{
				input_error(p_machine);
			}
			//when valid, copy the arguments to the *p_machine
			else
			{
				p_machine->item[i].label=
					malloc(sizeof(char)*(strlen(item_label)+1));
				memset(p_machine->item[i].label,sizeof(char)*(strlen(item_label)+1),0);
				strcpy(p_machine->item[i].label,item_label);
				p_machine->item[i].price=price;
				p_machine->item[i].num=3;
				//write to log
				fprintf(p_machine->log, 
						"A%d %s %d %d - - - -\n",
						i+1,
						p_machine->item[i].label,
						p_machine->item[i].num,
						p_machine->item[i].price);
				fflush(p_machine->log);
				i++;	
			}
		}
		else
		{
			input_error(p_machine);
		}
	}
	if(i==p_machine->item_num)
		return 0;
	else
		return 1;
}

/*
 * Authors: Deyuan Li
 * Purpose: It initializes the info of the machine and all the items
 * 
 * Expected input: It takes machine info as input
 * commands included: ITEMS <NUMBER>
 *                    ITEM <LABEL> <PRICE>c
 * 
 * Implementation details: The function has two functions executed:
 * cmd_items(), cmd_item() which gain some of the parameters' values
 * from stdin, and in this mode, the other parameters of p_machine and
 * p_machine->item[i] are initialized with default value.
 * The function will return zero if all the initializations are
 * processed properly.
 */
int32_t mode_startup(machine* p_machine)
{
	//check if cmd_items() is properly processed
	uint8_t check_cmd_items;
	//check if cmd_item() is properly processed
	uint8_t check_cmd_item;
	//open log.txt for writting log into
	p_machine->log = fopen("log.txt", "w");
	if(p_machine->log == NULL)
	{
		printf("unable to open file\n");
		exit(1);
	}
	printf("STARTUP MODE READY\n");
	//handle ITEMS command first
	check_cmd_items=cmd_items(p_machine);
	p_machine->item=malloc(p_machine->item_num*sizeof(struct item_info));
	memset(p_machine->item,p_machine->item_num*sizeof(struct item_info),0);
		
	check_cmd_item=cmd_item(p_machine);
	//return 0 when cmd_item() and cmd_items() are processed properly
	return check_cmd_item|check_cmd_items;
}

/*
 * Authors: Weiyan Lin
 * Purpose: It do the buy function
 * 
 * Expected input: It takes machine info , buy packet , respond packet as input
 * 
 * 
 * Implementation details:
 * It gets the server infomation , buy packet to handld the buying process.
 * And then put the result into a respond packet 
 * 
 * First of all,  it make sure client have enough money 
 * and server have enough item to be sold
 *
 * If everything is ok, we would put the status, code, name_length, name into the 
 * respond packet. And then change the price of item depends on situation.
 * (in this situation status == 1)
 * Output the result into the log file. 
 * 
 * If money is not enough or item is not enough, we would do the same thing expect
 * the status code of respond packet is 2,which could let client know the buying process is field.
 */
int32_t cmd_buy(machine *p_machine,struct ser_tran *torecv,struct respond_packet *tosend)
{
	int index = torecv->item_index-1;

	int price=p_machine->item[index].price;
	//make sure client have enough money and server have enough item to be sold
	if (ntohl(torecv->money_available) >= price && p_machine->item[index].num > 0)
	{
		tosend->price=htonl(price);
		tosend->code=BUYRES_CODE;
		tosend->seq_num=torecv->seq_num;
		tosend->name_length=strlen(p_machine->item[torecv->item_index-1].label);
		tosend->status_code=1;
		char *name=&(tosend->name);
		char *label=p_machine->item[index].label;
		while(*label)
		{
				*name=*label;
				name++;
				label++;
		}
	
		p_machine->item[index].num--;
		if(p_machine->item[index].num==0)
		{	;	}	
		else if (p_machine->item[index].num==1)
		{
			p_machine->item[index].price += 25;
		}
	    else
		{
			p_machine->item[index].price += 1;
		}	
		// output into log file
		fprintf(p_machine->log,"seq_num %d BUY  %s A%d %d %d - - - -\n",
				ntohl(torecv->seq_num),
				p_machine->item[index].label,
				index+1,
				p_machine->item[index].num,
				p_machine->item[index].price);
		printf("\nin the servers log:\n");
	
		printf(" BUY  %s A%d %d %d - - - -\n",
				
				p_machine->item[index].label,
				index+1,
				p_machine->item[index].num,
				p_machine->item[index].price);

		fflush(p_machine->log);

	}
	//if we do not have item to be sold , we would notice customer
	//SOLD OUT
	else if(p_machine->item[index].num==0)
	{
		printf("ITEM A%d HAS BEEN SOLD OUT!\n",index);
		// output into log file
		fprintf(p_machine->log,"seq_num %d BUY  %s A%d %d %d - - - -\n",
				ntohl(torecv->seq_num),
				p_machine->item[index].label,
				index+1,
				p_machine->item[index].num,
				p_machine->item[index].price);
		printf("\nservers log is:\n");
		
	   	printf("\n BUY  %s A%d %d %d - - - -\n",
				
				p_machine->item[index].label,
				index+1,
				p_machine->item[index].num,
				p_machine->item[index].price);
	
		fflush(p_machine->log);

		tosend->price=htonl(price);
		if ( torecv->code ==9 )
		{
				tosend->code=4;
		}
		else
		{
			tosend->code=torecv->code+3;
		}
		tosend->seq_num=torecv->seq_num;
		tosend->name_length=strlen(p_machine->item[torecv->item_index-1].label);
		tosend->status_code=2;
		char *name=&(tosend->name);
		char *label=p_machine->item[index].label;
		while(*label)
		{
				*name=*label;
				name++;
				label++;
		}
	}
	//if we do not have enough item to be sold 
	//show price again
	else
	{
		fprintf(p_machine->log,"seq_num %d -  %s A%d %d %d - - - -\n",
				ntohl(torecv->seq_num),
				p_machine->item[index].label,
				index+1,
				p_machine->item[index].num,
				p_machine->item[index].price);
		printf("\nservers log is:\n");
		printf("  %s A%d %d %d - - - -\n",
				
				p_machine->item[index].label,
				index+1,
				p_machine->item[index].num,
				p_machine->item[index].price);

		fflush(p_machine->log);


	    	tosend->price=htonl(price);
			if (torecv->code ==BUYACK_CODE)
			{
					tosend->code=BUYRES_CODE;
			}
			else
			{
					tosend->code=torecv->code+3;
			}
		tosend->seq_num=torecv->seq_num;
		tosend->name_length=strlen(p_machine->item[torecv->item_index-1].label);
		tosend->status_code=2;
		char *name=&(tosend->name);
		char *label=p_machine->item[index].label;
		while(*label)
		{
				*name=*label;
				name++;
				label++;
		}	
	}
	return 0;
}
