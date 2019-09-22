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

#define HTTPPORT "80" // default http port client will be connecting to
#define HTTP200 "HTTP/1.1 200 OK\r\n\r\n"
#define MAXDATASIZE 256 // max number of bytes we can get at once

void append(char *str, char ch);

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

char** str_split(char* str, char delim)
{   
    char** splitStr  = 0;
    size_t count     = 0;
    char* strPtr     = str;
    char delimArray[2];   //I HAVE NOT IDEA WHY strok wants to take an array as argument......stackflow save my life at this point....
    delimArray[0] = delim;
    delimArray[1] = '\0';
    /* Count how many elements will be extracted. */
    while (*strPtr){
        if (delim == *strPtr){
            count++;
        }
        strPtr++;
    }
    count++;
    splitStr = malloc(sizeof(char*) * count);
    if (splitStr){
        size_t idx  = 0;
        char* token = strtok(str, delimArray);

        while (token){
            *(splitStr + idx++) = strdup(token);
            token = strtok(0, delimArray);
        }
        *(splitStr + idx) = NULL;
    }
    return splitStr;
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


	//Parse Client String
	char *firstAddress = argv[1] + 7;
	char hostName[256] = "";
	char filePath[256] = "GET ";
	char inParse[2][256];
	int slashCount = 0, i;

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

	char **split;
	split = str_split(hostName,':');
	if (split) {
		for (i=0; *(split+i); i++) {
			strcpy(inParse[i],*(split+i));
			// printf("LINK_%d: %s\n",i,*(split+i));
			// free(*(split+i));
		}
		// free(split);
	}

	char *httpOverHeader = " HTTP/1.1\nUser-Agent: Wget/1.12 (linux-gnu)\nHost: localhost:3490\nConnection: Keep-Alive";
	append(filePath, *httpOverHeader);

	while(*(httpOverHeader) != '\0'){
		append(filePath, *httpOverHeader);
		httpOverHeader++;
	}

	if(strlen(inParse[1])==0){
		strcpy(inParse[1],HTTPPORT);
	}

	printf("filePath:%s\n",filePath);
	printf("hostName:%s\n",inParse[0]);
	printf("portNo:%s, portNo len:%lu\n",inParse[1], strlen(inParse[1]));

	if ((rv = getaddrinfo(inParse[0], inParse[1], &hints, &servinfo)) != 0) {
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

	// Send File Path
	if ((numbytes = send(sockfd, filePath, MAXDATASIZE, 0)) == -1){
		printf("Send Failed");
	}
	// Recv Header File
	if ((numbytes = recv(sockfd, buf, MAXDATASIZE, 0)) == -1) {
	    perror("recv");
	    exit(1);
	}
	char header_buf[MAXDATASIZE];
	int slashN_ctr = 0, buf_cur=0;
	while (slashN_ctr<3 && buf_cur<50){
		if (buf[buf_cur]=='\n') {
			slashN_ctr++;
		}
		buf_cur++;
	}
	memcpy(header_buf,buf,buf_cur);
	// header_buf[buf_cur+1] = '\0';

	printf("-----HTTP Response Received-----\n");
	printf("%s\n",header_buf);
	printf("-----HTTP Response Finished-----\n");
	printf("\nnumbytes recv:%d, HTTP Response recv:%lu\n",numbytes,strlen(header_buf));

	char HTTPOK[] = "HTTP/1.1 200 OK";
	char* OKCheck; 
	OKCheck = strstr(header_buf, HTTPOK);
	if(strstr(header_buf, "404 Not Found") != NULL){
		printf("\n404 File Not Found\n");
		close(sockfd);
		return 0;
	}
	else if(OKCheck == NULL) {
		printf("\n400 Bad Request\n");
		close(sockfd);
		return 0;

	}

	// Create Output File
	FILE * fPtr = NULL;
	fPtr = fopen("output", "wb");
	if (!fPtr )
		printf("create file failed");
	printf("\nOutput created\n");
	printf("\nOutput Writing\n");
	int recv_bytes;
	if ((numbytes!=MAXDATASIZE)&&(numbytes!=strlen(header_buf))){
		fwrite(&buf[buf_cur], numbytes-strlen(header_buf), 1, fPtr);
		printf("Writing Bytes:%lu\n",numbytes-strlen(header_buf));
		printf("Tiny File..\n");
		recv_bytes = numbytes-strlen(header_buf);

	}else{
		fwrite(&buf[buf_cur], numbytes-strlen(header_buf), 1, fPtr);
		printf("Writing Bytes:%lu\n",numbytes-strlen(header_buf));
		recv_bytes = numbytes-strlen(header_buf);
		while(1){
			memset(buf, 0, sizeof(buf));
			if ((numbytes = recv(sockfd, buf, MAXDATASIZE, 0)) == -1) {
				printf("receive failed\n");
				break;
			}
			fwrite(buf, numbytes, 1, fPtr);
			printf("Writing Bytes:%d\n",numbytes);
			recv_bytes+=numbytes;
			if (numbytes<MAXDATASIZE) {
				break;
			}
		}
	}
	printf("\nRecv %d Bytes", recv_bytes);
	printf("\nOutput Finished\n");
	fclose(fPtr);
	send(sockfd, inParse[1], MAXDATASIZE, 0);
	close(sockfd);
	return 0;
}


void append(char *str, char ch){
    int len = strlen(str);
    str[len] = ch;
    str[len + 1] = '\0';
}