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

#define N 16


#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif


int fd,errcode, newfd, searchFd, client_sockets[3];
int maxfd, counter;
ssize_t n;
fd_set rfds;
socklen_t addrlen;
struct addrinfo hints,*res;
struct sockaddr_in addr;
char buffer[128] = "";
char str[100] = "";
int searchKey = -1;
int tmp;


void createServer(Server* server) {
  fd=socket(AF_INET,SOCK_STREAM,0); //TCP socket
  if (fd==-1) exit(1); //error

  memset(&hints,0,sizeof hints);
  hints.ai_family=AF_INET; //IPv4
  hints.ai_socktype=SOCK_STREAM; //TCP socket
  hints.ai_flags=AI_PASSIVE;

  errcode=getaddrinfo(NULL,server->myPort,&hints,&res);
  if((errcode)!=0)/*error*/exit(1);

  n=bind(fd,res->ai_addr,res->ai_addrlen);
  if(n==-1) /*error*/ exit(1);

  if(listen(fd,5)==-1)/*error*/exit(1);

  while(1) {
    // Set all sockets to 0
    FD_ZERO(&rfds);

    // Add the main socket to set
    FD_SET(fd, &rfds);
    maxfd = fd;

    FD_SET(STDIN_FILENO  , &rfds);

    // Add all available child sockets to set
    // If there are available sockets
    if(client_sockets[0] > 0) {
      FD_SET(client_sockets[0], &rfds);
    }
    maxfd = max(client_sockets[0], maxfd);

    // If there's an available previous connection, save it in the file descriptors array and set it
    if(server->prevConnFD > 0) {
      client_sockets[1] = server->prevConnFD;
      FD_SET(server->prevConnFD, &rfds);
    }
    maxfd = max(server->prevConnFD, maxfd);

    // If there's an available next connection, save it in the file descriptors array and set it
    if(server->nextConnFD > 0) {
      client_sockets[2] = server->nextConnFD;
      FD_SET(server->nextConnFD, &rfds);
    }
    maxfd = max(server->nextConnFD, maxfd);

    counter = select(maxfd + 1, &rfds, (fd_set*)NULL, (fd_set*)NULL, (struct timeval *)NULL);
    if(counter <=0) exit(1);
    else if(FD_ISSET(STDIN_FILENO , &rfds)) { // If there's something available to read on the keyboard
      n = read(STDIN_FILENO, buffer, 128);
      if (n > 0) {
        if(strstr(buffer, "leave") != NULL) {
          close(server->prevConnFD);
          close(server->nextConnFD);
          break;
        } else if(strstr(buffer, "find") != NULL) {
            sscanf(buffer, "%s %d", str, &searchKey);
            printf("Distances: %d %d\n", distance(searchKey, server->nextKey), distance(searchKey, server->myKey));
            if((server->nextConnFD <= 0) && (server->prevConnFD <= 0)) { //If only 1 server in the received
              printf("I'm the nearest server (my key: %d) to %d key\n", server->myKey, searchKey);
            } else if(distance(searchKey, server->nextKey) > distance(searchKey, server->myKey)) {
              //Send FND for the successor
              sprintf(str, "FND %d %d %s %s\n", searchKey, server->myKey, server->myIp, server->myPort);
              n = write(server->nextConnFD, str, strlen(str));
              if(n == -1)/*error*/exit(1);
            } else { //I am the nearest server
              printf("Server %d is the nearest server of %d\n", server->nextKey, searchKey);
            }
        } else if(strstr(buffer, "send") != NULL) {
          n = write(server->prevConnFD, "Prev\n", 6);
          n = write(server->nextConnFD, "Next\n", 6);
        }
      }
      continue;
    }

    // If something is happening in the main socket, it means there's a new connection 
    if(FD_ISSET(fd, &rfds)) {
      printf("\nGive me some connections baby\n");
      addrlen = sizeof(addr);
      if((newfd = accept(fd, (struct sockaddr*)&addr, &addrlen)) == -1) exit(1);
                 //inform user of socket number - used in send and receive commands  
      printf("New connection , socket fd is %d , ip is : %s , port : %d \n" , newfd , inet_ntoa(addr.sin_addr) , ntohs (addr.sin_port));   
      
      client_sockets[0] = newfd;
    }

    //Prevents from reading the same port 2 times (newfd and some prev or next)
    if ((client_sockets[0] == server->nextConnFD) || (client_sockets[0] == server->prevConnFD)) {
        client_sockets[0] = 0;
    }


    //Else its on some other socket
    for(int i = 0; i < 3; i++) {
      newfd = client_sockets[i];
      if(FD_ISSET(newfd, &rfds)) {
        if((n = read(newfd, buffer, 128)) == 0) {
          printf("Connection closed %d\n\n", i);
          if((server->myKey == server->doubleNextKey) && (newfd == server->nextConnFD)) {
            server->nextConnFD = -1;
          } else if(newfd == server->nextConnFD) {
            copyDoubleToNext(server);
            server->nextConnFD = connectToNextServer(server);

            sprintf(str, "SUCC %d %s %s\n", server->nextKey, server->nextIp, server->nextPort);
            n = write(server->prevConnFD, str, strlen(str));

            n = write(server->nextConnFD, "SUCCCONF\n", 10);
          } else if(newfd == server->prevConnFD) {
            server->prevConnFD = -1;
          }
          client_sockets[i] = 0;
        } else { 
          buffer[n] = '\0';
          //write(1,"received: ",11); write(1,buffer,n); // Print incoming message
          //Now check what the incoming message is asking for
          if(strstr(buffer, "NEW") != NULL) {
            if(newfd == server->nextConnFD) { // If the connection is comming from the next server
              close(server->nextConnFD);

              //Update double next to current next
              server->doubleNextKey = server->nextKey;
              strcpy(server->doubleNextIp, server->nextIp);
              strcpy(server->doubleNextPort, server->nextPort);

              //Connect to the incoming server
              sscanf(buffer, "%s %d %s %s", str, &(server->nextKey), server->nextIp, server->nextPort);
              server->nextConnFD = connectToNextServer(server);

              //Send SUCC and SUCCCONF to previous server
              sprintf(str, "SUCC %d %s %s\n", server->nextKey, server->nextIp, server->nextPort);
              n = write(server->prevConnFD, str, strlen(str));

              n = write(server->nextConnFD, "SUCCCONF\n", 10);
            } else {
              if((server->nextConnFD <= 0) && (server->prevConnFD <= 0)) { //If there's only 1 server in the ring
                // Set the double next as myself
                server->doubleNextKey = server->myKey;
                strcpy(server->doubleNextIp, server->myIp);
                strcpy(server->doubleNextPort, server->myPort);

                // Create a second connection to the incoming server to be used as next
                sscanf(buffer, "%s %d %s %s", str, &(server->nextKey), server->nextIp, server->nextPort);
                server->nextConnFD = connectToNextServer(server);
                n = write(server->nextConnFD, "SUCCCONF\n", 10);
              } else {
                n = write(server->prevConnFD, buffer, strlen(buffer));
              }
              server->prevConnFD = newfd;
              sprintf(str, "SUCC %d %s %s\n", server->nextKey, server->nextIp, server->nextPort);
              n = write(server->prevConnFD, str, strlen(str));
            }
          } else if(strstr(buffer, "SUCC ") != NULL) {
              sscanf(buffer, "%s %d %s %s", str, &(server->doubleNextKey), server->doubleNextIp, server->doubleNextPort);
          } else if(strstr(buffer, "SUCCCONF") != NULL) {
            if(server->prevConnFD == -1) {
              server->prevConnFD = newfd;
              sprintf(str, "SUCC %d %s %s\n", server->nextKey, server->nextIp, server->nextPort);
              n = write(server->prevConnFD, str, strlen(str));
            } else {
              server->prevConnFD = newfd;
            }
          } else if(strstr(buffer, "FND ") != NULL) {
              char ip[30], port[10];
              printf("%s \n", buffer);
              sscanf(buffer, "%s %d %d %s %s", str, &searchKey, &tmp, ip, port);
              if(distance(searchKey, server->nextKey) > distance(searchKey, server->myKey)) {
                //Send FND for the successor
                n = write(server->nextConnFD, buffer, strlen(buffer));
                if(n == -1)/*error*/exit(1);
            } else { //I am the nearest server
                printf("Linha 210\n");
                searchFd = connectToGivenServer(ip, port);
                sprintf(str, "KEY %d %d %s %s\n", searchKey, server->nextKey, server->nextIp, server->nextPort);
                n = write(searchFd, str, strlen(str));
                if(n == -1)/*error*/exit(1);
            }
          } else if(strstr(buffer, "KEY ") != NULL) {
            sscanf(buffer, "%s %d %d %s %s", str, &searchKey, &tmp, str, str);
            printf("Received Key from: %d\n", tmp);
          }
        }
      }

    }
    printServerData(server);
  }
  // Close newfd
  close(client_sockets[0]);

  freeaddrinfo(res); 
  close(fd);
}

int connectToNextServer(Server* server) {
  int tmpFd;
  tmpFd = socket(AF_INET,SOCK_STREAM,0); //TCP socket
  if (tmpFd == -1) exit(1); //error
  
  memset(&hints,0,sizeof hints);
  hints.ai_family=AF_INET; //IPv4
  hints.ai_socktype=SOCK_STREAM; //TCP socket
  
  errcode = getaddrinfo(server->nextIp, server->nextPort,&hints,&res);
  if(errcode != 0) {
    printf("GET ADDRESS INFO ERROR\n");
    exit(1);
  }
  
  n = connect(tmpFd,res->ai_addr,res->ai_addrlen);
  if(n == -1) {
    printf("Error code: %d\n", errno);
    printf("CONNECT ERROR\n");
    exit(1);
  }
  return tmpFd;
}

int connectToGivenServer(char ip[30], char port[10]) {
  int tmpFd;
  tmpFd = socket(AF_INET,SOCK_STREAM,0); //TCP socket
  if (tmpFd == -1) exit(1); //error
  
  memset(&hints,0,sizeof hints);
  hints.ai_family=AF_INET; //IPv4
  hints.ai_socktype=SOCK_STREAM; //TCP socket
  
  errcode = getaddrinfo(ip, port,&hints,&res);
  if(errcode != 0) {
    printf("GET ADDRESS INFO ERROR\n");
    exit(1);
  }
  
  n = connect(tmpFd,res->ai_addr,res->ai_addrlen);
  if(n == -1) {
    printf("Error code: %d\n", errno);
    printf("CONNECT ERROR\n");
    exit(1);
  }
  return tmpFd;
}

void printServerData(Server* server) {
  printf("\n\n myKey: %d\nnextKey: %d\ndoubleNextKey: %d\nnextConnection: %d\nprevConnection: %d\n\n", server->myKey, server->nextKey, server->doubleNextKey, server->nextConnFD, server->prevConnFD);
}

void copyDoubleToNext(Server * server) {
  server->nextKey = server->doubleNextKey;
  strcpy(server->nextIp, server->doubleNextIp);
  strcpy(server->nextPort, server->doubleNextPort);
}

int distance(int k, int l) {
  int count = l - k;
  if(count < 0) {
    count += N;
    return count%N;
  } else {
    return count%N;
  }
}