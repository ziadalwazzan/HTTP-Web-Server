/* 
* tcpechosrv.c - A concurrent TCP echo server using threads
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>      /* for fgets */
#include <strings.h>     /* for bzero, bcopy */
#include <unistd.h>      /* for read, write */
#include <sys/socket.h>  /* for socket use */
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define MAXLINE  8192  /* max text line length */
#define MAXBUF   8192  /* max I/O buffer size */
#define LISTENQ  1024  /* second argument to listen() */

int open_listenfd(int port);
void echo(int connfd);
void *thread(void *vargp);

int main(int argc, char **argv) 
{
    int listenfd, *connfdp, port, clientlen=sizeof(struct sockaddr_in);
    struct sockaddr_in clientaddr;
    pthread_t tid; 

    if (argc != 2) {
	fprintf(stderr, "usage: %s <port>\n", argv[0]);
	exit(0);
    }
    port = atoi(argv[1]);

    listenfd = open_listenfd(port);
    while (1) {
	connfdp = malloc(sizeof(int));
	*connfdp = accept(listenfd, (struct sockaddr*)&clientaddr,(socklen_t *) &clientlen);
	pthread_create(&tid, NULL, thread, connfdp);
    }
}

/* thread routine */
void * thread(void * vargp) 
{  
    int connfd = *((int *)vargp);
    pthread_detach(pthread_self()); 
    free(vargp);
    echo(connfd);
    close(connfd);
    return NULL;
}

/*
 * echo - read and echo text lines until client closes connection
 */
void echo(int connfd) 
{
    size_t n; 
    char buf[MAXLINE]; 
    char httpmsg[1024]="HTTP/1.1 200 Document Follows\r\nContent-Type:text/html\r\nContent-Length:32\r\n\r\n<html><h1>Hello CSCI4273 Course!</h1>"; 
    int bytes_sent = 0;
    int bytes_read = 0;
    long int fsize;
    n = read(connfd, buf, MAXLINE);
    printf("server received the following request:\n%s\n",buf);
    
    //Tokenize client request
    char method[15];
    char url[1024];
    char version[15];
    sscanf(buf, "%s %s %s", method, url, version);
    FILE * filename;
    //printf("\nCONSOLE LOG > tokenized REQUEST: %s %s %s\n", method, url, version);
    char name[1024] = "www";
    if (strcmp(url, "/") == 0) //Default
    {
        bzero(name, sizeof(name));
        strcat(name, "www/index.html");
    }
    else
    {
        strcat(name, url);
    }

    /////////////////////////////////////////////  GET  //////////////////////////////////////////////////////
    
    if (strcmp(method, "GET") == 0)
    {
        //check file type
        char *type = strrchr(name, '.');
        if (type != NULL)
        {
            if (strcmp(type, ".html") == 0)
            {
                type = "text/html";
            }
            else if (strcmp(type, ".txt") == 0)
            {
                type = "text/plain";
            }
            else if (strcmp(type, ".png") == 0)
            {
                type = "image/png";
            }
            else if (strcmp(type, ".gif") == 0)
            {
                type = "image/gif";
            }
            else if (strcmp(type, ".jpg") == 0)
            {
                type = "image/jpg";
            }
            else if (strcmp(type, ".css") == 0)
            {
                type = "text/css";
            }
            else if (strcmp(type, ".js") == 0)
            {
                type = "application/javascript";
            }
        }
        else
        {
            type = "text/html";
            sprintf(name, "%s/index.html", name);
        }

        printf("\nCONSOLE LOG > TYPE: %s\n", type);
        printf("\nCONSOLE LOG > FILENAME: %s\n", name);
        printf("\nCONSOLE LOG > HTML Version: %s\n", version);
        filename = fopen(name, "rb");

        if (filename == NULL || (strcmp(version, "HTTP/1.1")!= 0 && strcmp(version, "HTTP/1.0")!= 0) ) //in case of file not existing
        {
            bzero(httpmsg, sizeof(httpmsg));
            sprintf(httpmsg, "%s 500 Internal Server Error\n", version);
            write(connfd, httpmsg,strlen(httpmsg));
            printf("CONSOLE LOG > File not found\n");
        }
        else
        {
            // calculating the size of the file
            fseek(filename, 0, SEEK_END);
            fsize = ftell(filename);
            fseek(filename, 0, SEEK_SET); //seek back to beginning
            printf("\nCONSOLE LOG > SIZE OF FILE: %ld\n", fsize);
            
            //empty buffers -> construct response header -> send header followed by document
            bzero(buf, sizeof(buf));
            bzero(httpmsg, sizeof(httpmsg));
            sprintf(httpmsg, "%s 200 Document Follows\r\nContent-Type:%s\r\nContent-Length:%ld\r\n\r\n", version, type, fsize);
            write(connfd, httpmsg,strlen(httpmsg));
            printf("server returning a http message with the following content:\n%s\n",httpmsg);
            while(bytes_sent < fsize){
                bytes_read = fread(buf, 1, MAXBUF, filename);
                bytes_sent += bytes_read;
                printf("%s", buf);
                write(connfd, buf,bytes_read);
                bzero(buf, sizeof(buf));
            }
            printf("\nCONSOLE LOG > BYTES READ: %d\n", bytes_sent);
        }
    }

    /////////////////////////////////////////////  POST  //////////////////////////////////////////////////////
    if (strcmp(method, "POST") == 0)
    {
        //tokenize request to get POST data
        const char s[2] = "\r\n";
        char *post_data = strtok(buf, s);
        post_data = strtok(NULL, s); //unwanted token
        post_data = strtok(NULL, s); //unwanted token
        post_data = strtok(NULL, s);
        filename = fopen(name, "rb");

        //calculating the size of the file
        fseek(filename, 0, SEEK_END);
        fsize = ftell(filename);
        fseek(filename, 0, SEEK_SET); //seek back to beginning
        printf("\nCONSOLE LOG > SIZE OF FILE: %ld\n", fsize);

        bzero(httpmsg, sizeof(httpmsg));
        sprintf(httpmsg, "%s 200 Document Follows\r\nContent-Type: text/html\r\nContent-Length:%ld\r\n\r\n<pre><h1>%s</pre></h1>", version, fsize, post_data);
        write(connfd, httpmsg,strlen(httpmsg));
        printf("server returning a http message with the following content:\n%s\n",httpmsg);
        //send POSTDATA
        while(bytes_sent < fsize){
            bytes_read = fread(buf, 1, MAXBUF, filename);
            bytes_sent += bytes_read;
            printf("%s", buf);
            write(connfd, buf,bytes_read);
            bzero(buf, sizeof(buf));
        }
    }
    
}

/* 
 * open_listenfd - open and return a listening socket on port
 * Returns -1 in case of failure 
 */
int open_listenfd(int port) 
{
    int listenfd, optval=1;
    struct sockaddr_in serveraddr;
  
    /* Create a socket descriptor */
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        return -1;

    /* Eliminates "Address already in use" error from bind. */
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, 
                   (const void *)&optval , sizeof(int)) < 0)
        return -1;

    /* listenfd will be an endpoint for all requests to port
       on any IP address for this host */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET; 
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    serveraddr.sin_port = htons((unsigned short)port); 
    if (bind(listenfd, (struct sockaddr*)&serveraddr, sizeof(serveraddr)) < 0)
        return -1;

    /* Make it a listening socket ready to accept connection requests */
    if (listen(listenfd, LISTENQ) < 0)
        return -1;
    return listenfd;
} /* end open_listenfd */

