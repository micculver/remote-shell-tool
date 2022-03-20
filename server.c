#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX 5000 //maximum buffer size
#define PORT 12852
#define SA struct sockaddr
#define MAX_ARGS 10
#define ARG_LENGTH 15 //maximum size of single argument

#define TRUE  1
#define FALSE 0

void respond(int client_socket);
void split_string(char array[], int args_count, char * new_array[args_count]);
int count_words(char array[]);

int main() {
	int opt = TRUE;
	
	int server_socket; //server's socket fd
	int client_socket[5]; //array of clients' socket fds
	int max_clients = 2; //set up max amount of clients that can be connected simultaneously
	int client_count = 0; //keep track of amount of clients currently connected to
	
	struct sockaddr_in server_address; //server address
	struct sockaddr_in client_address; //client address
	
	bzero(&server_address, sizeof(server_address));
	bzero(&client_address, sizeof(client_address));
	
	
	
	/* CREATE SERVER SOCKET */
	server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if(server_socket == -1) {
		perror("socket failed");
		exit(1);
	}
	
	
	
	/* allow server socket to handle multiple clients simultaneously */
	if(setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0) {
		perror("setsockopt failed");
		exit(1);
	}
	
	
	
	/*Set IP and PORT of server_address*/
	server_address.sin_family = AF_INET;                 //set address family = Internet
	server_address.sin_addr.s_addr = htonl(INADDR_ANY);   //set IP address; INADDR_ANY => use when IP is unknown
	server_address.sin_port = htons(PORT);               //set PORT
	
	
	
	
	/* BIND SERVER'S SOCKET TO ITS LOCAL IP ADDRESS AND SPECIFIED PORT */
	if((bind(server_socket, (SA*)&server_address, sizeof(server_address))) != 0) {
		perror("bind failed\n");
		exit(1);
	}
	


	printf("Listening at port %d...\n", PORT);
	
	/* spawn child processes to connect to and handle each client*/
	for(int i = 0; i < max_clients - 1; i++)
		fork();
	
	int len;
	
	/* infinite for-loop until server is killed */
	for(;;) {
		/* LISTEN FOR REQUESTS */
		if((listen(server_socket, 5)) != 0) {
			perror("listen failed\n");
			exit(1);
		}
		
		
		
		len = sizeof(client_address);
		int index; //keeps track of individual client connection
		
		/* ACCEPT CONNECTION WITH REQUESTING CLIENT'S SOCKET */
		client_socket[client_count] = accept(server_socket, (SA*)&client_address, &len);
		
		if(client_socket[client_count] < 0) {
			printf("accept failed\n");
			exit(1);
		}
		else {
			printf("Accepted a client\n");
			
			index = client_count;
			client_count++;
		}
		
		/* send welcome message to client */
		char msg[] = "Successfully connected to the server!";
		write(client_socket[index], msg, sizeof(msg));
		
		
		
		/*  contains loop to handle client requests */
		respond(client_socket[index]);
	}
}

void respond(int client_socket) {
	char buffer[MAX];
	
	bzero(buffer, sizeof(buffer));
	
	int i = 0;
	
	int read_value;
	
	/* while server has soemthing to read from client*/
	while((read_value = read(client_socket, buffer, sizeof(buffer))) > 0) {

		/* number of elements of command MUST be count_words(buffer)+1 */
		int args_count = count_words(buffer);
		char * command[args_count+1];
		
		
		
		/* command is populated with arguments from buffer */
		split_string(buffer, args_count, command);
		
		
		
		/* print to server terminal */
		printf("Client attempting to execute command: ");
		for(int i = 0; i < args_count; i++) {
			printf("%s ", command[i]);
		}
		printf("\n");
		
		
		
		/* gracefully disconnect when client sends "exit" */
		if(strncmp("exit", buffer, 4) == 0) {
			printf("Client disconnected...\n");
			break;
		}
		
		
		
		/* EXECUTE CLIENT'S COMMAND */
		int pid = fork();
		
		if(pid < 0) {
			perror("fork failed");
			exit(1);
		}
		else if(pid == 0) {
			/* child process executes the command */
			int stdout = dup(1);
			
			dup2(client_socket, 1); //redirect output to client socket
			
			if(execvp(command[0], command) == -1) {
				perror("execvp failed");
				char msg[] = "Unable to execute\n";
				write(client_socket, msg, sizeof(msg));
			}
			else 
				/* successful execution */
				write(client_socket, buffer, sizeof(buffer));
			
			dup2(stdout, 1); //restore output back to server terminal
		}
		else {
			/* parent process */
			waitpid(pid, NULL, 0); //collects child process before resuming execution
		}
	}
		
	if(read_value <= 0) {
		printf("Client process was killed...\n");
	}	
}

/* populate new_array with delimited sections of array */
void split_string(char array[], int args_count, char * new_array[args_count]) {
	char * word = strtok(array, " ");
	
	int index = 0;
	
	while(word != NULL) {
		new_array[index] = word;
		word = strtok(NULL, " ");
		index++;
	}
	
	/* new_array will be passed into execvp, so it needs to end with NULL */
	new_array[index] = NULL;
}

/* count number of "island" strings delimited by space(s)*/
int count_words(char array[]) {
	if(strcmp(array, "") == 0) {
		return 0;
	}
	
	int count = 0; //keep track of "island" strings
	int index = 0; //keep track of index of array
	
	while(1) {
		if(array[index] == ' ') {
			if(index == 0 || array[index-1] == ' ') {
				while(array[++index] == ' ' && array[index+1] != '\0');
			}
			else {
				count++;
				index++;
			}
		}
		else {
			if(array[index] == '\n' ) {
				array[index] = '\0'; //need to change from '\n' to '\0' to make execvp work
				if(array[index-1] != ' ') {
					count++;
				}
				
				break;
			}
			
			index++;
		}
	}
	
	return count;
}