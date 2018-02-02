// Liam Dungan - cs214 Systems Programming - Spring 2017

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "libnetfiles.h"

//Really should have left more comments...

typedef struct {
    char hostname[64];
    CONNECTION_MODE fMode;
} SERVER_CONN;

SERVER_CONN clientConn;



int getSocket( const char * hostname ){
    int sockfd = 0;

    struct sockaddr_in serv_addr;
    struct hostent *server = NULL;  
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        fprintf(stderr,"libnetfiles: socket() failed, errno= %d\n", errno);
		return FAIL;
    }


    server = gethostbyname(hostname);//Get address of server inputed name
    if (server == NULL) {
       	errno = 0;
        h_errno = HOST_NOT_FOUND;
        printf("stderr,libnetfiles: host not found, h_errno");
		return FAIL;
    }
    //initialize 
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(PORT);
    if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0) 
    {
        fprintf(stderr,"libnetfiles: cannot connect to %s, h_errno= %d\n", hostname, h_errno);
    return FAIL;
    }

    return sockfd;
}




int netserverinit(char *hostname, int filemode){
    int n = 0;
    int sockfd = -1;
    char buffer[BUFFER_SIZE] = "";

    errno =  0;
    h_errno = 0;

    strcpy(clientConn.hostname, buffer);
	clientConn.fMode = INVALID_FILE;

    switch (filemode) {
        case UNRESTRICTED:
        case EXCLUSIVE:
        case TRANSACTION:
            break;

        default:
            h_errno = INVALID_FILE;
            fprintf(stderr, "netserverinit: invalid file connection mode\n");
            return FAIL;
            break;
    }

    if (hostname == NULL || strlen(hostname) == 0) {
        errno = EINVAL;  
        return FAIL;
    }

    sockfd = getSocket(hostname);
    if (sockfd < 0 ) {
        errno = 0;
       	h_errno = HOST_NOT_FOUND;
        return FAIL;
    }
    
    bzero(buffer, BUFFER_SIZE);
    sprintf(buffer, "%d,0,0,0", 1);

    n = write(sockfd, buffer, strlen(buffer));
    if ( n < 0 ) {

       	h_errno = ECOMM;  
        printf("Failed to write to SERVER_CONN");
        return FAIL;
    }

    bzero(buffer, BUFFER_SIZE);
    n = read(sockfd, buffer, BUFFER_SIZE-1);
    if ( n < 0 ) {
        h_errno = ECOMM;  
        printf("Failed to read from socket\n");
        if ( sockfd != 0 ) close(sockfd);
        return FAIL;
    }

    close(sockfd);  

    sscanf(buffer, "%d,", &n);
    if (n == 0){ 
        strcpy(clientConn.hostname, hostname);
        clientConn.fMode = (CONNECTION_MODE)filemode;
    }
    return n;
}


/* This method takes a pathname and a permission flag, then
returns a file descriptor or -1 if there was an error 
flags : O_RDONLY, O_WRONLY, or O_RDWR */
int netopen(const char *pathname, int flags)
{
    int netFd  = -1;
    int sockfd = -1;
    int n     = 0;
    char buffer[BUFFER_SIZE] = "";

    errno = 0;
    h_errno = 0;

    if (pathname == NULL || strlen(pathname)<1) {
        printf("Pathname is Invalid\n");
        errno = EINVAL;  
        return FAIL;
    } 

    if(flags != O_RDONLY && flags != O_WRONLY && flags != O_RDWR){
    	printf("Invalid flags argument\n");
    	errno = EINVAL;
    	return FAIL;
    }

    sockfd = getSocket(clientConn.hostname);
    if (sockfd < 0) {
        errno = 0;
        h_errno = HOST_NOT_FOUND;
        fprintf(stderr, "netopen: host not found, %s\n", clientConn.hostname);
        return FAIL;
    }
 
    bzero(buffer, BUFFER_SIZE);
    sprintf(buffer, "%d,%d,%d,%s", NET_OPEN, clientConn.fMode, flags, pathname);


    n = write(sockfd, buffer, strlen(buffer));
    if ( n < 0 ) {
        fprintf(stderr, "netopen: failed to write cmd to clientConn.  n= %d\n", n);
        return FAIL;
    }


    bzero(buffer, BUFFER_SIZE);
    n = read(sockfd, buffer, BUFFER_SIZE-1);
    if ( n < 0 ) {
        if ( sockfd != 0 ) close(sockfd);
        return FAIL;
    }

    close(sockfd);

    sscanf(buffer, "%d,%d,%d,%d", &n, &netFd, &errno, &h_errno);
    if ( n == FAIL ) {
        printf("netopen: SERVER_CONN returns FAILURE, errno= %d (%s), h_errno=%d\n", errno, strerror(errno), h_errno);
        return FAIL;
    }

    return netFd;
}




/* This method takes a file descriptor, connects to the server
and closes the file. If closing fails, returns -1. If successful returns 0. */
extern int netclose(int fd){ 
    if(fd >= 0){
        errno = EBADF;
        return FAILURE;
    }
    int netFd  = -1;
    int sockfd = -1;
    int n     = 0;
    char buffer[BUFFER_SIZE] = "";

    errno = 0;
    h_errno = 0;

    sockfd = getSocket(clientConn.hostname);
    if (sockfd < 0) {
        errno = 0;
        h_errno = HOST_NOT_FOUND;
        fprintf(stderr, "netopen: host not found, %s\n", clientConn.hostname);
        return FAIL;
    }

    bzero(buffer, BUFFER_SIZE);
    sprintf(buffer, "%d,%d", NET_CLOSE, fd);

    n = write(sockfd, buffer, strlen(buffer));

    if ( n < 0 ) {

        h_errno = ECOMM;  
        printf("Failed to write to SERVER_CONN");
        return FAIL;
    }

    bzero(buffer, BUFFER_SIZE);
    n = read(sockfd, buffer, BUFFER_SIZE-1);
    if ( n < 0 ) {
        h_errno = ECOMM;  
        printf("Failed to read from socket\n");
        if ( sockfd != 0 ) close(sockfd);
        return FAIL;
    }
    close(sockfd); 
    sscanf(buffer, "%d,%d,%d", &n, &errno, &h_errno);  

    return n;  
}




extern ssize_t netread(int fildes, void *buf, size_t nbyte){
    int netFd  = -1;
    int sockfd = -1;
    int n     = 0;
    char * stringBuf = (char *)buf;
    char buffer[BUFFER_SIZE];
    errno = 0;
    h_errno = 0;
    if(fildes >= 0){
        errno = EBADF;
        return FAILURE;
    }

     errno = 0;
    h_errno = 0;

    sockfd = getSocket(clientConn.hostname);
    if (sockfd < 0) {
        errno = 0;
        h_errno = HOST_NOT_FOUND;
        fprintf(stderr, "NETOPEN: host was not found, %s\n", clientConn.hostname);
        return FAIL;
    }

    bzero(buffer, BUFFER_SIZE);
    sprintf(buffer, "%u,%d,%d", NET_READ, fildes, (int)nbyte);

    n = write(sockfd, buffer, strlen(buffer));

    if ( n < 0 ) {

        h_errno = ECOMM;
        printf("Failed to write to SERVER_CONN");
        return FAIL;
    }

    bzero(buffer, BUFFER_SIZE);
    n = read(sockfd, buffer, BUFFER_SIZE-1); 
    if ( n < 0 ) {
        h_errno = ECOMM;  
        printf("Failed to read from socket\n");
        if ( sockfd != 0 ) close(sockfd);
        return FAIL;
    }

    close(sockfd); 

    sscanf(buffer, "%d,%d,%d", &n, &netFd, &h_errno);  
    strncpy(stringBuf, buffer+strlen(buffer)-nbyte, nbyte);

    if(n == FAIL) return FAIL;

    return netFd; 
}





extern ssize_t netwrite(int fildes, const void *buf, size_t nbyte){

    int netFd  = -1;
    int sockfd = -1;
    int n     = 0;
    char * stringBuf = (char *)buf;
    char buffer[BUFFER_SIZE];
    errno = 0;
    h_errno = 0;

    if(fildes >= 0){
        errno = EBADF;
        return FAILURE;
    }

    errno = 0;
    h_errno = 0;

    sockfd = getSocket(clientConn.hostname);
    if (sockfd < 0) {
        errno = 0;
        h_errno = HOST_NOT_FOUND;
        fprintf(stderr, "NETOPEN: host was not found, %s\n", clientConn.hostname);
        return FAIL;
    }

    bzero(buffer, BUFFER_SIZE);
    sprintf(buffer, "%u,%d,%d,%d,%s", NET_WRITE, fildes, (int)nbyte, strlen(stringBuf), stringBuf);

    n = write(sockfd, buffer, strlen(buffer));

    if ( n < 0 ) {

        h_errno = ECOMM;  
        printf("Failed to write to SERVER_CONN");
        return FAIL;
    }

    bzero(buffer, BUFFER_SIZE);
    n = read(sockfd, buffer, BUFFER_SIZE-1); 
    if ( n < 0 ) {
        h_errno = ECOMM;  
        printf("Failed to read from socket\n");
        if ( sockfd != 0 ) close(sockfd);
        return FAIL;
    }

    close(sockfd); 

    sscanf(buffer, "%d,%d,%d", &n, &netFd, &errno);  
    if(n == FAIL) return FAIL;

    return netFd; 

}
