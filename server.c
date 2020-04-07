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


int fd,errcode, newfd, searchFd, client_sockets[3], udpFd, udpFdTemp;
int maxfd, counter;
ssize_t n;
fd_set rfds;
socklen_t addrlen, udpAddrlen;
struct addrinfo hints,*res;
struct sockaddr_in addr, udpAddr;
char buffer[128] = "";
char str[100] = "";
int searchKey = -1;
int tmp;
int searchFlag = 0;
UdpData* udp;



void createServer(Server* server, int _onGoingOperation) {
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

  // UDP code
  udpFd=socket(AF_INET,SOCK_DGRAM,0); //UDP socket
  if (udpFd==-1) exit(1); //error

  memset(&hints,0,sizeof hints);
  hints.ai_family=AF_INET; //IPv4
  hints.ai_socktype=SOCK_DGRAM; //UDP socket
  hints.ai_flags=AI_PASSIVE;

  errcode=getaddrinfo(NULL,server->myPort,&hints,&res);
  if((errcode)!=0)/*error*/exit(1);

  n = bind(udpFd,res->ai_addr,res->ai_addrlen);
  if(n==-1) /*error*/ exit(1);

  int ongoingOperation = _onGoingOperation;
  int serverInRing = 1;

  while(1) {
    // Set all sockets to 0
    FD_ZERO(&rfds);

    // We only check the main sockets if the server is in the ring, 
    // if a server is not in the ring it cannot receive connections
    if(serverInRing == 1) {
      // Add the main socket to set
      FD_SET(fd, &rfds);
      maxfd = fd;

      // Add main UDP socket to set
      FD_SET(udpFd, &rfds);
      maxfd = max(fd, udpFd);
    }

    FD_SET(STDIN_FILENO  , &rfds);
    maxfd = max(STDIN_FILENO, maxfd);
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
    } else {
      client_sockets[1] = -1;
    }
    maxfd = max(server->prevConnFD, maxfd);

    // If there's an available next connection, save it in the file descriptors array and set it
    if(server->nextConnFD > 0) {
      client_sockets[2] = server->nextConnFD;
      FD_SET(server->nextConnFD, &rfds);
    } else {
      client_sockets[2] = -1;
    }
    maxfd = max(server->nextConnFD, maxfd);

    if(ongoingOperation == 0) { //If there's no operation going on at the moment, print the menu
      if(serverInRing == 1) 
        printf("\n show\n find k\n leave\n");
      else
        printf("\nAvailable commands:\n\n new i \n sentry i succi succi.IP succi.TCP\n entry i succi succie.IP succi.TCP\n exit\n");
    }

    counter = select(maxfd + 1, &rfds, (fd_set*)NULL, (fd_set*)NULL, (struct timeval *)NULL);
    if(counter <=0) exit(1);
    else if(FD_ISSET(STDIN_FILENO , &rfds)) { // If there's something available to read on the keyboard
      fgets(buffer, 100 , stdin);
        if(serverInRing == 1) {
          if(strstr(buffer, "leave") != NULL) {
            if((server->nextConnFD > 0) && (server->prevConnFD > 0)) {
              close(server->prevConnFD);
              close(server->nextConnFD);
            }
            serverInRing = 0;
            cleanServer(server);
            for(int i = 0; i < 3; i++) {
              client_sockets[i] = 0;
            }
          } else if(strstr(buffer, "find") != NULL) {
              sscanf(buffer, "%s %d", str, &searchKey);
              ongoingOperation = 1;
              startKeySearch(server, searchKey, 0, NULL, 0, 0, &ongoingOperation);
          } else if(strstr(buffer, "send") != NULL) {
            tcpWrite(server->prevConnFD, "Prev\n");
            tcpWrite(server->nextConnFD, "Next\n");
          } else if(strstr(buffer, "show") != NULL) {
            printServerData(server);
          }
        } else {
            if(strstr(buffer, "new") != NULL) {
              sscanf(buffer, "%s %d", str, &(server->myKey));
              serverInRing = 1;
            } else if(strstr(buffer, "sentry ")) {
              sscanf(buffer, "%s %d %d %s %s", str, &(server->myKey), &(server->nextKey), server->nextIp, server->nextPort); // Get the successor details
              server->nextConnFD = connectToNextServer(server); // Set the next server as the given server and establish a connection
              
              sprintf(buffer, "NEW %d %s %s\n", server->myKey, server->myIp, server->myPort);
              tcpWrite(server->nextConnFD, buffer); // Give the successor your details

              serverInRing = 1;
              ongoingOperation = 1;
            } else if(strstr(buffer, "entry ")) {
              char connectIp[100], connectPort[100];
              int connectKey;
              sscanf(buffer, "%s %d %d %s %s", str, &(server->myKey), &connectKey, connectIp, connectPort); // Get the successor details
              udp = connectToUdpServer(connectIp, connectPort);

              sprintf(buffer, "EFND %d\n", server->myKey);
              n=sendto(udp->fd,buffer,strlen(buffer),0, udp->res->ai_addr, udp->res->ai_addrlen);
              if(n==-1) /*error*/ exit(1);

              n=recvfrom(udp->fd,buffer,128,0, (struct sockaddr*)&(udp->res->ai_addr),&(udp->res->ai_addrlen));
              if(n==-1) /*error*/ exit(1);


              sscanf(buffer, "%s %d %d %s %s", str, &(server->myKey), &(server->nextKey), server->nextIp, server->nextPort); // Get the successor details
              server->nextConnFD = connectToNextServer(server); // Set the next server as the given server and establish a connection
              
              sprintf(buffer, "NEW %d %s %s\n", server->myKey, server->myIp, server->myPort);
              tcpWrite(server->nextConnFD, buffer);  // Give the successor your details

              serverInRing = 1;
              ongoingOperation = 1;
            } else if(strstr(buffer, "exit")) {
              break;
            }
        }
      continue;
    }

    // If something is happening in the main socket, it means there's a new connection 
    if(FD_ISSET(fd, &rfds)) {
      addrlen = sizeof(addr);
      if((newfd = accept(fd, (struct sockaddr*)&addr, &addrlen)) == -1) exit(1);
                 //inform user of socket number - used in send and receive commands  
      printf("New connection , socket fd is %d , ip is : %s , port : %d \n" , newfd , inet_ntoa(addr.sin_addr) , ntohs (addr.sin_port));   
      
      client_sockets[0] = newfd;
    }

    //If is receiving an udp connection, so it starts a key search
    if(FD_ISSET(udpFd, &rfds)) {
      udpAddrlen = sizeof(udpAddr);
      n=recvfrom(udpFd,buffer,128,0, (struct sockaddr*)&udpAddr,&udpAddrlen);
      if(n==-1)/*error*/exit(1);
      sscanf(buffer, "%s %d", str, &searchKey);
      startKeySearch(server, searchKey, 1, (struct sockaddr*)&udpAddr,udpAddrlen, udpFd, &ongoingOperation);
      searchFlag = 1;
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
          if((server->myKey == server->doubleNextKey) && (newfd == server->nextConnFD)) { //If there are only 2 servers in the ring...
            close(server->nextConnFD);
            server->nextConnFD = -1;
            int auxKey = server->myKey;
            cleanServer(server);
            server->myKey = auxKey;
          } else if(newfd == server->nextConnFD) {
            close(server->nextConnFD);
            copyDoubleToNext(server);
            server->nextConnFD = connectToNextServer(server);

            sprintf(str, "SUCC %d %s %s\n", server->nextKey, server->nextIp, server->nextPort);
            tcpWrite(server->prevConnFD, str);

            tcpWrite(server->nextConnFD, "SUCCCONF\n");
          } else if(newfd == server->prevConnFD) {
            close(server->prevConnFD);
            server->prevConnFD = -1;
          }
          client_sockets[i] = 0;
        } else { 
          buffer[n] = '\0';
          write(1, "Received: ", 11); write(1, buffer, sizeof(buffer));
          //Now check what the incoming message is asking for
          if(strstr(buffer, "NEW") != NULL) {
            if(newfd == server->nextConnFD) { // If the connection is comming from the next server
              close(server->nextConnFD);

              //Update double next to current next
              copyNextToDouble(server);

              //Connect to the incoming server
              sscanf(buffer, "%s %d %s %s", str, &(server->nextKey), server->nextIp, server->nextPort);
              server->nextConnFD = connectToNextServer(server);

              //Send SUCC and SUCCCONF to previous and next server
              sprintf(str, "SUCC %d %s %s\n", server->nextKey, server->nextIp, server->nextPort);
              tcpWrite(server->prevConnFD, str);
              
              tcpWrite(server->nextConnFD, "SUCCCONF\n");
            } else {
              if((server->nextConnFD <= 0) && (server->prevConnFD <= 0)) { //If there's only 1 server in the ring
                // Set the double next as myself
                copySelfToDouble(server);

                // Create a second connection to the incoming server to be used as next
                sscanf(buffer, "%s %d %s %s", str, &(server->nextKey), server->nextIp, server->nextPort);
                server->nextConnFD = connectToNextServer(server);
                tcpWrite(server->nextConnFD, "SUCCCONF\n");
              } else {
                tcpWrite(server->prevConnFD, buffer);
              }
              server->prevConnFD = newfd;
              sprintf(str, "SUCC %d %s %s\n", server->nextKey, server->nextIp, server->nextPort);
              tcpWrite(server->prevConnFD, str);
            }
          } else if(strstr(buffer, "SUCC ") != NULL) {
              sscanf(buffer, "%s %d %s %s", str, &(server->doubleNextKey), server->doubleNextIp, server->doubleNextPort);
          } else if(strstr(buffer, "SUCCCONF") != NULL) {
            if(server->prevConnFD == -1) {
              server->prevConnFD = newfd;
              sprintf(str, "SUCC %d %s %s\n", server->nextKey, server->nextIp, server->nextPort);
              tcpWrite(server->prevConnFD, str);
            } else {
              server->prevConnFD = newfd;
            }
            ongoingOperation = 0;
          } else if(strstr(buffer, "FND ") != NULL) {
              char ip[30], port[10];
              sscanf(buffer, "%s %d %d %s %s", str, &searchKey, &tmp, ip, port);
              if(distance(searchKey, server->nextKey) > distance(searchKey, server->myKey)) {
                //Send FND for the successor
                tcpWrite(server->nextConnFD, buffer);
            } else { //I am the nearest server
                searchFd = connectToGivenServer(ip, port);
                sprintf(str, "KEY %d %d %s %s\n", searchKey, server->nextKey, server->nextIp, server->nextPort);
                tcpWrite(searchFd, str);
            }
          } else if(strstr(buffer, "KEY ") != NULL) {
            int connectKey;
            char connectIp[30], connectPort[10];
            sscanf(buffer, "%s %d %d %s %s", str, &searchKey, &connectKey, connectIp, connectPort);

            sprintf(buffer, "EKEY %d %d %s %s\n", searchKey, connectKey, connectIp, connectPort);
            n = sendto(udpFd,buffer,strlen(buffer),0, (struct sockaddr*)&udpAddr, udpAddrlen);
            ongoingOperation = 0;
          }
        }
      }

    }
    printServerData(server);
  }
  // Close newfd
  close(client_sockets[0]);

  free(udp);
  freeaddrinfo(res); 
  close(udpFd);
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

void copyNextToDouble(Server *server) {
  server->doubleNextKey = server->nextKey;
  strcpy(server->doubleNextIp, server->nextIp);
  strcpy(server->doubleNextPort, server->nextPort);
}

void copySelfToDouble(Server *server) {
  server->doubleNextKey = server->myKey;
  strcpy(server->doubleNextIp, server->myIp);
  strcpy(server->doubleNextPort, server->myPort);
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

UdpData* connectToUdpServer(char ip[30], char port[10]) {
  UdpData *udp = (UdpData*)malloc(sizeof(UdpData));
  udp->fd = socket(AF_INET,SOCK_DGRAM,0); //TCP socket
  if (udp->fd == -1) exit(1); //error
  
  memset(&hints,0,sizeof hints);
  hints.ai_family=AF_INET; //IPv4
  hints.ai_socktype=SOCK_DGRAM; //TCP socket
  
  errcode = getaddrinfo(ip, port,&hints,&(udp->res));
  if(errcode != 0) {
    printf("GET ADDRESS INFO ERROR\n");
    exit(1);
  }
  
  return udp;
}

void startKeySearch(Server * server, int searchKey, int entry, struct sockaddr* udpAddr, socklen_t udpAddrlen, int fd, int *_onGoingOperation) {
  if((server->nextConnFD <= 0) && (server->prevConnFD <= 0)) { //If only 1 server in the ring
    printf("I'm the nearest server (my key: %d) to %d key\n", server->myKey, searchKey);
    if(entry == 1) {
      sprintf(buffer, "EKEY %d %d %s %s\n", searchKey, server->myKey, server->myIp, server->myPort);
      n = sendto(fd,buffer,strlen(buffer),0, udpAddr, udpAddrlen);
    }
    *_onGoingOperation = 0;
  } else if(distance(searchKey, server->nextKey) > distance(searchKey, server->myKey)) {
    //Send FND for the successor
    sprintf(str, "FND %d %d %s %s\n", searchKey, server->myKey, server->myIp, server->myPort);
    n = write(server->nextConnFD, str, strlen(str));
    if(n == -1)/*error*/exit(1);
  } else { //I am the nearest server
    printf("Server %d is the nearest server of %d\n", server->nextKey, searchKey);
    if(entry == 1) {
      sprintf(buffer, "EKEY %d %d %s %s\n", searchKey, server->nextKey, server->nextIp, server->nextPort);
      n = sendto(fd,buffer,strlen(buffer),0, udpAddr, udpAddrlen);
    }
  }
}

void cleanServer(Server * server) {
  server->myKey = 0;

  strcpy(server->nextIp, "");
  strcpy(server->nextPort, "");
  server->nextKey = 0;

  strcpy(server->doubleNextIp, "");
  strcpy(server->doubleNextPort, "");
  server->doubleNextKey = 0;

  server->prevConnFD = 0;
  server->nextConnFD = 0;
}

void tcpWrite(int toWriteFd, char * strToWrite) {
  int nbytes, nleft, nwritten;
  nbytes = strlen(strToWrite);
  nleft = nbytes;
  while (nleft>0) {
    nwritten = write(toWriteFd, strToWrite, nleft);
    if(nwritten <= 0) {
      printf("\n\n MASSIVE ERROR \n\n");
    }
    nleft-=nwritten;
    strToWrite += nwritten;
  }
}

int tpcRead(int toReadFd, char * bufferToRead) {
  int nread, nleft;
  nleft = strlen(bufferToRead);
  while(nleft > 0) {
    nread = read(toReadFd, bufferToRead, strlen(bufferToRead));
    if (nread == -1) return -1;
    if(nread == 0) return 0;
    nleft -= nread;
    bufferToRead += nread;
  }
}
