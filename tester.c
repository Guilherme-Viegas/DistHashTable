#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>
#include <string.h>
#include <math.h>
#include <sys/select.h>
#define PORT "58006"

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

int fd,errcode;
ssize_t n;
socklen_t addrlen;
struct addrinfo hints,*res;
struct sockaddr_in addr;
char buffer[128];

void main() {
    fd=socket(AF_INET,SOCK_STREAM,0); //TCP socket
    if (fd==-1) exit(1); //error

    memset(&hints,0,sizeof hints);
    hints.ai_family=AF_INET; //IPv4
    hints.ai_socktype=SOCK_STREAM; //TCP socket

    errcode=getaddrinfo("127.0.0.1",PORT,&hints,&res);
    if(n!=0)/*error*/exit(1);

    n=connect(fd,res->ai_addr,res->ai_addrlen);
    if(n==-1)/*error*/exit(1);

    while(1) {
        char user_input[128];
        fgets(user_input, 100 , stdin);
        n=write(fd,user_input,128);
        if(n==-1)/*error*/exit(1);

        n=read(fd,buffer,128);
        if(n==-1)/*error*/exit(1);

        write(1,"Teste: ",6); write(1,buffer,n);
    }

    freeaddrinfo(res);
    close(fd);
}