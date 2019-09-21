/*
** client.c -- a stream socket client demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <arpa/inet.h>

#define PORT "3490" // the port client will be connecting to
//#define PORT "80" // the http port client will be connecting to

#define MAXDATASIZE 100 // max number of bytes we can get at once

void append(char *str, char ch);

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
	int sockfd, numbytes;
	char buf[MAXDATASIZE];
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];

	if (argc != 2) {
	    fprintf(stderr,"usage: client hostname\n");
	    exit(1);
	}

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;


	//***************************************************parse cliend string**************************************************************************************//
	char *firstAddress = argv[1] + 7;
	char hostName[256] = "";
	char filePath[256] = "GET ";
	int slashCount = 0;
	while(*(firstAddress) != '\0'){
			if(*firstAddress == '/' )
					slashCount++;
			if(slashCount != 1){
					append(hostName, *firstAddress);
			}
			else if(slashCount == 1){
					append(filePath, *firstAddress);
			}
		firstAddress++;
	}
	char *httpOverHeader = " HTTP/1.1\nUser-Agent: Wget/1.12 (linux-gnu)\nHost: localhost:3490\nConnection: Keep-Alive";
	append(filePath, *httpOverHeader);

	while(*(httpOverHeader) != '\0'){
					append(filePath, *httpOverHeader);
				httpOverHeader++;
	}
	//printf("hostname: %s\n", hostName);
	//printf("%s\n", filePath);

	if ((rv = getaddrinfo(hostName, PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and connect to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("client: connect");
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		return 2;
	}

	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
			s, sizeof s);
	printf("client: connecting to %s\n", s);

	freeaddrinfo(servinfo); // all done with this structure
  //**************************************************************************send file path*******************************************************************************

	if ((numbytes = send(sockfd, filePath, MAXDATASIZE-1, 0)) == -1){   // why minus -1 ?
		printf("Send Failed");
	}
	if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
	    perror("recv");
	    exit(1);
	}

	buf[numbytes] = '\0';

	printf("client: received '%s'\n",buf);

	//*********************************************************************check header file*********************************************************************************
	// memset(buf, '0', sizeof(buf));
	// if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
	// 	printf("ERROR: NO HEADER FILE");
	// }
	//
	// if((strcmp(buf, "HTTP/1.1 200 NotFound") == 0)){
	// 	printf("File Not Found\n");
	// }
	// else if((strcmp(buf, "HTTP/1.1 200 OK")) != 0){
	// 		printf("%d, \n", strcmp(buf, "HTTP/1.1 200 OK") );
	// 		printf("404 Bad Request\n");
	// 	}
	//*********************************************************************create output file*********************************************************************************

	FILE * fPtr = NULL;
	numbytes = 0;
	int rc;
	//char fileBuf[MAXDATASIZE];
	int counter = 0;

	fPtr = fopen("output", "wb");
	if (!fPtr )
		printf("create file failed");

	while(1){
		memset(buf, 0, sizeof(buf));

		counter = 0;
		if ((numbytes = recv(sockfd, buf, MAXDATASIZE -1, 0)) == -1) {
			printf("receive failed\n");
			break;
		}
		while(counter != numbytes ){
			counter++;
		}

		if ((rc = fputs(buf, fPtr)) == -1) {
			printf("writing to file failed\n");
			break;
		}
		if(counter != (MAXDATASIZE-1)){
			break;
		}
	}

	fclose(fPtr);
	close(sockfd);
	return 0;
}


void append(char *str, char ch){
    int len = strlen(str);
	//printf("%d\n", len);
    str[len] = ch;
    str[len + 1] = '\0';
}
