#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#define MAX 5000 //maximum buffer size
#define PORT 12852
#define SA struct sockaddr

void send_commands(int client_socket);

int main(int argc, char * argv[]) {
	
	/*usage check => argv[1] should be the server's IP address*/
	if(argc != 2) {
		printf("Must include 2 arguments...\n");
		exit(1);
	}
	
	
	
	int network_socket; //endpoint to read to and write from server later
	struct sockaddr_in server_address; //will contain server's socket info
	
	
	
	/* CREATE SOCKET */
	network_socket = socket(AF_INET, SOCK_STREAM, 0);
	
	if(network_socket == -1) {
		perror("socket failed");
		exit(1);
	}
	
	
	
	/*SET THE SERVER SOCKET ADDRESS*/
	bzero(&server_address, sizeof(server_address));
	server_address.sin_family = AF_INET;                   //set the address family as Internet
	server_address.sin_addr.s_addr = inet_addr(argv[1]);   //set IP address
	server_address.sin_port = htons(PORT);                 //set PORT
	
	
	
	/* CONNECT SOCKET TO SERVER ADDRESS */
	if(connect(network_socket, (SA*)&server_address, sizeof(server_address)) != 0) {
		perror("connect failed");
		exit(1);
	}
	
	char msg[40];
	read(network_socket, msg, sizeof(msg));
	printf("%s\n", msg);
	
	
	
	/*client writes to and reads from server within this looping function*/
	send_commands(network_socket);
	
	close(network_socket);
}

/*Sends commands to the server, reads messages from the server */
void send_commands(int network_socket) {
	char buffer[MAX];
	
	for(;;) {
		bzero(buffer, sizeof(buffer));
		
		printf("~$ ");
		int i = 0;
		
		/* populate buffer with user input */
		while((buffer[i++] = getchar()) != '\n');
		
		/* write buffer contents to server */
		write(network_socket, buffer, sizeof(buffer));
		
		/* break out of for-loop when client sends "exit"; ends client program afterwards */
		if(strncmp("exit", buffer, 4) == 0) {
			printf("Client exiting...\n");
			break;
		}
		
		bzero(buffer, sizeof(buffer));
		
		/*read the data from the server and copy it into the buffer*/
		if(read(network_socket, buffer, sizeof(buffer)) <= 0) {
			perror("read failed");
			break;
		}
		
		printf("%s\n", buffer);
		
	}	
}