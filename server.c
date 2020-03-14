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
#include "server.h"


#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif


int fd,errcode, newfd, client_sockets[5];
int maxfd, counter;
ssize_t n;
fd_set rfds;
socklen_t addrlen;
struct addrinfo hints,*res;
struct sockaddr_in addr;
char buffer[128] = "";



void createServer(Server* server) {
  fd=socket(AF_INET,SOCK_STREAM,0);
  if (fd==-1) {
      printf("SOCKET ERROR\n");
      exit(1); //error
  }

  memset(&hints,0,sizeof hints);
  hints.ai_family=AF_INET;
  hints.ai_socktype=SOCK_STREAM;
  hints.ai_flags=AI_PASSIVE;

  errcode=getaddrinfo(NULL,server->myPort,&hints,&res);
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


  while(1) {
    // Clear all sockets to set
    FD_ZERO(&rfds);

    // Add the main socket to set
    FD_SET(fd, &rfds);
    maxfd = fd;

    // Add all available child sockets to set
    for (int i = 0; i < 5; i++) {
      newfd = client_sockets[i];
      // If there are available sockets
      if(newfd > 0) {
        FD_SET(newfd, &rfds);
      }
      // Get the highest file descriptor number
      if(newfd > maxfd) {
        maxfd = newfd;
      }
    }

    
    counter = select(maxfd + 1, &rfds, (fd_set*)NULL, (fd_set*)NULL, (struct timeval *)NULL);
    if(counter <= 0) exit(1);

    // If something is happening in the main socket, it means there's a new connection 
    if(FD_ISSET(fd, &rfds)) {
      addrlen = sizeof(addr);
      if((newfd = accept(fd, (struct sockaddr*)&addr, &addrlen)) == -1) exit(1);
                 //inform user of socket number - used in send and receive commands  
      printf("New connection , socket fd is %d , ip is : %s , port : %d \n" , newfd , inet_ntoa(addr.sin_addr) , ntohs (addr.sin_port));   

      for(int i = 0; i < 5; i++) {
        if(client_sockets[i] == 0) {
          client_sockets[i] = newfd;
          break;
        }
        if(i == 4) {
          close(newfd);
        }
      }
    }

    //Else its on some other socket
    for(int i = 0; i < 5; i++) { 
      newfd = client_sockets[i];
      if(FD_ISSET(newfd, &rfds)) {
        if((n = read(newfd, buffer, 128)) == 0) {
          close(newfd);
          printf("Connection closed %d\n\n", i);
          client_sockets[i] = 0;
        } else {
          buffer[n] = '\0';
          write(1,"received: ",11); write(1,buffer,n); // Print incoming message
          //Now check what the incoming message is asking for
          if(strstr(buffer, "NEW") != NULL) { //A new server is trying to enter the ring  
            serverIsEntering(buffer, &newfd, server); 
          }

          n = write(newfd,"Server Response\n",17);
          if(n==-1)/*error*/exit(1);
        }
      }
    }
  }
  freeaddrinfo(res); 
  close(fd);
}

//For handling the entry of a new server to the ring
void serverIsEntering(char buffer[128], int *newfd, Server *server) {  //TODO A MENSAGEM NEW PODE TER 2 SIGNIFICADOS DIFERENTES!!!
  int newServerKey = 0;
  char newServerIp[16] = "";
  char newServerPort[10] = "";
  decodeMessage(buffer, &newServerKey, newServerIp, newServerPort);

  //TODO ENVIAR MENSAGEM NEW 8 8.IP 8.TCP AO 5(VER SLIDES) 
  //ENVIAR UM SUCC 12 12.IP 12.TCP AO 8 PARA QUE O 8 GUARDE AS INFOS SOBRE O SEU SUCESSOR DO SUCESSOR

  //Sending the infos to entering server about my successor <SUCC 12 12.IP 12.TCP>
  //If there is no SUCCESSOR then send <SUCC -1>
  char str[100] = "";
  if(server->doubleNextKey == -1) { //Does not have successor
    n = write(*newfd, "SUCC -1", 7);
  }else {
    sprintf(str, "SUCC %d %s %s\n", server->doubleNextKey, server->doubleNextIp, server->doubleNextPort);
    n = write(*newfd,str, strlen(str));
  }
  if(n==-1)/*error*/exit(1);

  //Send the NEW command to it's predecessor, if there isn't then send to the one trying to enter the ring
  strcpy(str, "");
  if(server->preKey == -1) { //There is no predecessor
    sprintf(str, "NEW %d %s %s\n", server->myKey, server->myIp, server->myPort);
    n = write(*newfd, str, strlen(str));
  } else { //Send NEW to predecessor
    sprintf(str, "NEW %d %s %s\n", newServerKey, newServerIp, newServerPort);
    n = write(server->prevConnFD, str, strlen(str));
  }
  if(n==-1)/*error*/exit(1);

}

int connectToNextServer(Server* server) { // sentry 5 10 127.0.0.1 8005 and send message NEW 8 8.IP 8.TCP
  fd = socket(AF_INET,SOCK_STREAM,0); //TCP socket
  if (fd == -1) exit(1); //error
  
  memset(&hints,0,sizeof hints);
  hints.ai_family=AF_INET; //IPv4
  hints.ai_socktype=SOCK_STREAM; //TCP socket
  
  errcode = getaddrinfo(server->nextIp, server->nextPort,&hints,&res);
  if(errcode != 0) {
    printf("GET ADDRESS INFO ERROR\n");
    exit(1);
  }
  
  n = connect(fd,res->ai_addr,res->ai_addrlen);
  if(n == -1) {
    printf("CONNECT ERROR\n");
    exit(1);
  }
  return fd;
}



//Returns key, ip and tcp from a message string like "NEW i i.IP i.TCP"
void decodeMessage(char* message, int* key, char ip[16], char port[10]) {
  // Returns first token 
  char* token = strtok(message, " ");
  int i = 0; 

  // Keep printing tokens while one of the 
  // delimiters present in str[]. 
  while (token != NULL) { 
      if(i==1) *key = atoi(token);
      else if(i==2) strcpy(ip, token);
      else if(i==3) strcpy(port, token);

      token = strtok(NULL, " "); 
      i++;
  } 
}