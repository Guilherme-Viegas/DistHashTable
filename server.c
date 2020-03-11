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
#include <errno.h>


#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

int fd,errcode, newfd, afd=0, second_tcp_fd = 0;
int maxfd, counter;
ssize_t n;
fd_set rfds;
socklen_t addrlen;
struct addrinfo hints,*res;
struct sockaddr_in addr;
char buffer[128];

typedef enum {idle, busy} state;


void createServer(char*port) {
  fd=socket(AF_INET,SOCK_STREAM,0);
  if (fd==-1) {
      printf("SOCKET ERROR\n");
      exit(1); //error
  }

  memset(&hints,0,sizeof hints);
  hints.ai_family=AF_INET;
  hints.ai_socktype=SOCK_STREAM;
  hints.ai_flags=AI_PASSIVE;

  errcode=getaddrinfo(NULL,port,&hints,&res);
    if((errcode)!=0) {
      printf("GETADDRINFO ERROR\n");
      /*error*/exit(1);
    }

  n = bind(fd,res->ai_addr,res->ai_addrlen);
  if(n == -1){
      printf("Error code: %d\n", errno);
      printf("BIND ERROR\n");
       /*error*/ exit(1);
    }

  if(listen(fd,5)==-1)/*error*/exit(1);

    state state_first_tcp;
    state state_second_tcp;

    state_first_tcp = idle;


  while(1) {
    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);
    maxfd = fd;

    if(state_first_tcp == busy) {
      FD_SET(afd, &rfds);
      maxfd = max(maxfd, afd);
    }
    
    if(state_second_tcp == busy) {
        FD_SET(second_tcp_fd, &rfds);
        maxfd = max(maxfd, second_tcp_fd);
    }

    counter = select(maxfd + 1, &rfds, (fd_set*)NULL, (fd_set*)NULL, (struct timeval *)NULL);
    if(counter <= 0) exit(1);

    if(FD_ISSET(fd, &rfds)) {
      addrlen = sizeof(addr);
      if((newfd = accept(fd, (struct sockaddr*)&addr, &addrlen)) == -1) exit(1);
      switch(state_first_tcp) {
        case idle:
            afd = newfd;
            state_first_tcp = busy;
            break;
        case busy:
            if(state_second_tcp == idle) {
                second_tcp_fd = newfd;
                state_second_tcp = busy;    
                break;
            }
            close(newfd);
            break;
      }
    }

    if(FD_ISSET(afd, &rfds)) {
        if((n = read(afd, buffer, 128)) != 0) {
            if(n==-1) exit(1);
            write(1,"received: ",10);write(1,buffer,n); // Print incoming message
            n=write(afd,"Server Response\n",n);
        } else {
            printf("\n\n\n\nCLLLOOSSEEE\n\n\n\n");
            close(afd);
            state_first_tcp = idle;
        }
    }

    if(FD_ISSET(second_tcp_fd, &rfds)) {
        if((n = read(second_tcp_fd, buffer, 128)) != 0) {
            if(n==-1) exit(1);
            write(1,"received: ",10);write(1,buffer,n); // Print incoming message
            n=write(second_tcp_fd,buffer,n);
        } else {
            close(second_tcp_fd);
            state_second_tcp = idle;
        }
    }

  }
  close(fd);
}
