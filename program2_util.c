#include "common.h"
#include "program2_util.h"
void sprint_hex(uint8_t *data, size_t length)
{
	char myoutput[MAXLENGTH];
	char tmp[MAXLENGTH];
	assert(length < 100);
	myoutput[0] = '\0';

	size_t i;
	for (i = 0; i < length; i++)
	{
		sprintf(tmp, "%02x ", data[i]);
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
};

void prepare_generic_socket(int argc, char *argv[], 
		int family, int flags, int type, int protocol)
{
	struct addrinfo lookup_addr;
	memset(&lookup_addr, 0, sizeof(struct addrinfo));
	lookup_addr.ai_family = family; 
	lookup_addr.ai_flags = flags;
	lookup_addr.ai_socktype = type;
	lookup_addr.ai_protocol = protocol;

	if (getaddrinfo(NULL, argv[1], &lookup_addr, &listen_addr) != 0)
	{
		exit_with_error("getaddrinfo failed");
	}

	sock = socket(listen_addr->ai_family, listen_addr->ai_socktype,
			listen_addr->ai_protocol);
	if (sock < 0)
	{
		exit_with_error("socket failed");
	}
	if (bind(sock, listen_addr->ai_addr, 
				listen_addr->ai_addrlen) < 0)
	{
		exit_with_error("bind failed");
	}

}
//this function sends a generic data buffer via tcp
void send_tcp_data(FILE* rx, FILE *tx, void *data, int datalen)
{
	if (tx)
	{
		size_t bytes_sent = fwrite(data, 1, datalen, tx);
		if (errno == EPIPE)
		{
			fclose(tx);
			fclose(rx);
			exit_with_error("pipe error\n");
		}
		else if (bytes_sent < 0)
		{
			fclose(tx);
			fclose(rx);
			exit_with_error("send failed");
		}
		fflush(tx);
	}
	else
	{
		sprint_hex((uint8_t *)data, datalen);
	}
}

void receive_tcp_clients(char *argv[])
{
	while (!stop)
	{
		struct sockaddr_storage client_addr;
		socklen_t addr_len = sizeof(struct sockaddr_storage);

		int client = accept(sock, 
					(struct sockaddr *) &client_addr,
					&addr_len);
		if (stop)
		{
			break;
		}

		if (client < 0)
		{
			exit_with_error("accept failed");
		}

		handle_tcp_client(client, argv);

	}

}
