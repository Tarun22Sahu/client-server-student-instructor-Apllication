/*
** server.c -- a stream socket server demo
*/

#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#define MAXDATASIZE 100 // max number of bytes we can get at once 

#define PORT "3490"  // the port users will be connecting to

#define BACKLOG 10	 // how many pending connections queue will hold
#define NO_OF_USER 21 // 20 student + 1 instructor
void sigchld_handler(int s)
{
	(void)s; // quiet unused variable warning

	// waitpid() might overwrite errno, so we save and restore it:
	int saved_errno = errno;

	while(waitpid(-1, NULL, WNOHANG) > 0);

	errno = saved_errno;
}


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

const char* getfield(char* line, int num)
{
    const char* tok;
    for (tok = strtok(line, ";");
            tok && *tok;
            tok = strtok(NULL, ";\n"))
    {
        if (!--num)
            return tok;
    }
    return NULL;
}

bool verify_credentials(char *const username, char *const password){
	FILE* cred_data = fopen("user_pass.csv", "r");
	char line[NO_OF_USER];
	while (fgets(line, NO_OF_USER, cred_data))
    {
        char* tmp = strdup(line);
        if(username == getfield(tmp, 1) && password == getfield(tmp, 2)){
			return true;
		}
        free(tmp);
    }
	return false;
}

void display_all_marks(int client_fd){
	FILE* marks = fopen("student_marks", "r");
	char line[NO_OF_USER];
	char *sub_number;
	int sub_number_int;
	send(client_fd, "Name", 12, 0);
	send(client_fd, "\t", 1, 0);
	for(int i=0;i<5;i++){
		send(client_fd, "SUB", 3, 0);
		sub_number_int = i+1;
		send(client_fd, (void *)&sub_number_int, 3, 0);
		send(client_fd, "\t", 1, 0);
	}
	
	while (fgets(line, NO_OF_USER, marks))
    {
        char* tmp = strdup(line);
        send(client_fd, (void *)getfield(tmp, 1), 12, 0);
		send(client_fd, "\t", 1, 0);
		for(int i=0;i<5;i++){
			send(client_fd, (void *)getfield(tmp, i+2), 3, 0);
			send(client_fd, "\t", 1, 0);
		}
		send(client_fd, "\n", 1, 0);
        free(tmp);
    }
}

int main(void)
{
	int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	struct sigaction sa;
	int yes=1;
	char s[INET6_ADDRSTRLEN];
	int rv;
	char buf[100];

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
				sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: bind");
			continue;
		}

		break;
	}

	freeaddrinfo(servinfo); // all done with this structure

	if (p == NULL)  {
		fprintf(stderr, "server: failed to bind\n");
		exit(1);
	}

	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}

	sa.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}

	printf("server: waiting for connections...\n");

	while(1) {  // main accept() loop
		sin_size = sizeof their_addr;
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
		if (new_fd == -1) {
			perror("accept");
			continue;
		}

		inet_ntop(their_addr.ss_family,
			get_in_addr((struct sockaddr *)&their_addr),
			s, sizeof s);
		printf("server: got connection from %s\n", s);

		if (!fork()) { // this is the child process
			close(sockfd); // child doesn't need the listener
			while(1){
				if (send(new_fd, "hellp", 5, 0) == -1)
					perror("send");
				//printf("sent");
				sleep(2);
			}
			
			
			// while(1){
				
			// 	if(recv(new_fd, p) > 0){

			// 	}
			// }
			close(new_fd);
			exit(0);
		}
		close(new_fd);  // parent doesn't need this
	}

	return 0;
}

