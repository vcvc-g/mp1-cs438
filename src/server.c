/*
** server.c -- a stream socket server demo
*/

#include <stdio.h>
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

#define PORT "3490"  // the port users will be connecting to
#define MAXDATASIZE 100 
#define BACKLOG 10	 // how many pending connections queue will hold
#define HTTP200 "HTTP/1.1 200 OK\r\n\r\n"
#define HTTP400 "HTTP/1.1 400 Bad Request\r\n\r\n"
#define HTTP404 "HTTP/1.1 404 Not Found\r\n\r\n"

void sigchld_handler(int s)
{
	printf("%d",s);
	while(waitpid(-1, NULL, WNOHANG) > 0);
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{
	int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	struct sigaction sa;
	int yes=1;
	char s[INET6_ADDRSTRLEN];
	int rv;
	
	char wget_buf[MAXDATASIZE];
	char file_buf[MAXDATASIZE];
	int numfread; 
	FILE *fd;

	if (argc != 2) {
	    fprintf(stderr,"usage: server portno\n");
	    exit(1);
	}

	char *portno = argv[1];

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, portno, &hints, &servinfo)) != 0) {
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

	if (p == NULL)  {
		fprintf(stderr, "server: failed to bind\n");
		return 2;
	}

	freeaddrinfo(servinfo); // all done with this structure

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
			if (send(new_fd, "Hello, world!", 13, 0) == -1)
				perror("send failed");
			
			if (recv(new_fd, wget_buf, MAXDATASIZE-1, 0) == -1){ // if recv error
				if (send(new_fd, HTTP400, strlen(HTTP400), 0) == -1)
					perror("send failed");
			}
			else{ // parse get structure
				// printf("%s",wget_buf);
				char filePath[256] = ".";
				int i,j,ctr;
				char getParse[15][50];
				j=0; ctr=0;
				for (i=0; i<=strlen(wget_buf); i++) {

					if (wget_buf[i]==' '||wget_buf[i]=='\0') {
						getParse[ctr][j] = '\0';
						ctr++;
						j = 0;
					}else{
						getParse[ctr][j] = wget_buf[i];
						j++;
					}
				}

				strcat(filePath,getParse[1]);
				printf("\nFile Name Received: %s\n",filePath); 
				fd = fopen(filePath, "r"); 
				// fd = fopen(wget_buf, "r"); 
				// printf("\nFile Name Received: %s\n", wget_buf); 	

				if (fd == NULL) { //invalid file
					printf("\nFile open failed!\n"); 
					if (send(new_fd, HTTP404, strlen(HTTP404), 0) == -1)
						perror("send failed");
				}
				else{
					printf("\nFile Successfully opened!\n");
					strncpy(file_buf,HTTP200,(sizeof file_buf)-1);
					send(new_fd, file_buf, strlen(file_buf), 0);
					do {
						memset(file_buf,0,sizeof(file_buf));
						numfread = fread(file_buf, sizeof(char), MAXDATASIZE, fd);
						// file_buf[numfread] = '\0';
						if (send(new_fd, file_buf, numfread, 0) == -1) {
							perror("send failed");
							fclose(fd);
						}
							
					} while (numfread == MAXDATASIZE-1);
					fclose(fd);
				}
			}

			close(new_fd);
			exit(0);
		}
		close(new_fd);  // parent doesn't need this
	}

	return 0;
}

