#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void ERROR(const char *msg)
{
    printf("%s\n",msg);
    exit(0);
}

int main(int argc, char *argv[])
{
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    char buffer[256];
    if (argc < 4) {
       fprintf(stderr,"usage %s hostname port filename\n", argv[0]);
       exit(0);
    }
    portno = atoi(argv[2]);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        ERROR("ERROR opening socket");
    server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(sockfd,(struct sockaddr*)&serv_addr,sizeof(serv_addr)) < 0) 
        ERROR("ERROR connecting");
/*
    printf("Please enter the message: ");
    bzero(buffer,256);
    fgets(buffer,255,stdin);
 */
	FILE* fp = fopen(argv[3], "r");
	if ( fp )
	{
			// Determine file length
			fseek( fp, 0L, SEEK_END );
			int nFileLen = ftell(fp);
			fseek( fp, 0L, SEEK_SET );

			char * buffer = (char *)malloc(nFileLen+1);
			if ( fread(buffer, nFileLen,1,fp ) == 1 )
			{
					buffer[nFileLen] = '\0';
			}
			fclose(fp);
			n = write(sockfd,&nFileLen,4);
			if (n < 0) 
					ERROR("ERROR writing to socket");
			n = write(sockfd,buffer,nFileLen);
			if (n < 0) 
					ERROR("ERROR writing to socket");
			bzero(buffer,256);
			n = read(sockfd,buffer,255);
			if (n < 0) 
					ERROR("ERROR reading from socket");
			close(sockfd);
			printf("%s\n",buffer);
//			free(buffer);
			return 0;
	}
	printf("open file:%s error. %m",argv[3]);
}
