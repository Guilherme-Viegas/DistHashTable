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


int fd,errcode, newfd, client_sockets[3];
int maxfd, counter;
ssize_t n;
fd_set rfds;
socklen_t addrlen;
struct addrinfo hints,*res;
struct sockaddr_in addr;
char buffer[128] = "";
char str[100] = "";


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
    
    printf("\nHERE\n");


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
    // Get the highest file descriptor number
    if(client_sockets[0] > maxfd) {
      maxfd = client_sockets[0];
    }

    if(server->prevConnFD > 0) {
      client_sockets[1] = server->prevConnFD;
      FD_SET(server->prevConnFD, &rfds);

      if(server->prevConnFD > maxfd) {
        maxfd = server->prevConnFD;
      }
    }

    if(server->nextConnFD > 0) {
      client_sockets[2] = server->nextConnFD;
      FD_SET(server->nextConnFD, &rfds);

      if(server->nextConnFD > maxfd) {
        maxfd = server->nextConnFD;
      }
    }   

    printf("Linha 104\n");
    counter = select(maxfd + 1, &rfds, (fd_set*)NULL, (fd_set*)NULL, (struct timeval *)NULL);
    if(counter <= 0) {
      //TODO meter o erro com perror
      exit(1);
    }
    else if (FD_ISSET(STDIN_FILENO , &rfds)) { // Read from keyboard
      n = read(STDIN_FILENO, buffer, 128);
      if (n > 0) {
        if(strstr(buffer, "leave") != NULL) {
          break;
        } else if(strstr(buffer, "send") != NULL) {
          n = write(server->prevConnFD, "Prev\n", 6);
          n = write(server->nextConnFD, "Next\n", 6);
        }
      }
      continue;
    }
    printf("Linha 107\n"); 

    // If something is happening in the main socket, it means there's a new connection 
    if(FD_ISSET(fd, &rfds)) {
      printf("\nGive me some connections baby!\n");
      addrlen = sizeof(addr);
      if((newfd = accept(fd, (struct sockaddr*)&addr, &addrlen)) == -1) exit(1);
                 //inform user of socket number - used in send and receive commands  
      printf("New connection , socket fd is %d , ip is : %s , port : %d \n" , newfd , inet_ntoa(addr.sin_addr) , ntohs (addr.sin_port));   

        if(client_sockets[0] == 0) {
          client_sockets[0] = newfd;
        } else {
          close(newfd);
        }
      
    }

      if ((client_sockets[0] == server->nextConnFD) || (client_sockets[0] == server->prevConnFD)) {
        client_sockets[0] = 0;
      }

    //Else its on some other socket
    for(int i = 0; i < 3; i++) { 
      newfd = client_sockets[i];
      printf("Reached for %d %d\n", i, client_sockets[i]);
      if(i == 2) {
        if(client_sockets[1] == client_sockets[2]) {
          break;
        }
      }
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
          } else if(strstr(buffer, "SUCC ") != NULL) {
              if((server->prevConnFD == 0) ) {
                write(1,"\nReceived SUCC: ",17); write(1,buffer,n);
                sscanf(buffer, "%s %d %s %s", str, &(server->doubleNextKey), server->doubleNextIp, server->doubleNextPort); // Get the double successor details and update   
                server->prevConnFD = server->nextConnFD;
              } else if(newfd == server->nextConnFD) {
                sscanf(buffer, "%s %d %s %s", str, &(server->doubleNextKey), server->doubleNextIp, server->doubleNextPort); // Get the successor details
              }
          } else if(strcmp(buffer, "SUCCCONF\n") == 0) {
            server->prevConnFD = newfd;
            //n = write(server->prevConnFD, "Sucessfully connected\n", 23);
            n = write(1, "Sucessfully connected\n", 23);
          }
        }
        printServerData(server);
      }
      printf("End for %d %d\n", i, client_sockets[i]);
    }
    printf("Out of the for\n");
  }

  for(int i=0; i<3; i++) {
    close(client_sockets[i]);
  }

  freeaddrinfo(res); 
  close(fd);
}

//For handling the entry of a new server to the ring
void serverIsEntering(char buffer[128], int *newfd, Server *server) {
  char str[100];
  char newIp[30];
  char newPort[10];
  int newKey = 0;
  sscanf(buffer, "%s %d %s %s", str, &newKey, newIp, newPort);

  if(*newfd == server->nextConnFD) { //Se o comando NEW vier do sucessor...
      //Atualiza o meu duplo sucessor para ser o que é o meu atual sucessor
      server->doubleNextKey = server->nextKey;
      strcpy(server->doubleNextIp, server->nextIp);
      strcpy(server->doubleNextPort, server->nextPort);

      //Atualiza o seu sucessor para ser o servidor entrante
      server->nextKey = newKey;
      strcpy(server->nextIp, newIp);
      strcpy(server->nextPort, newPort);
      if(server->nextConnFD != server->prevConnFD) {
        close(server->nextConnFD);
      }
      
      server->nextConnFD = connectToNextServer(server); //Tem de alterar a conexão com o sucessor para se ligar ao servidor entrante
      //Envia SUCCCONF ao servidor entrante
      n = write(server->nextConnFD, "SUCCCONF\n", 10);  if(n==-1)/*error*/exit(1);

      //Envia SUCC ao meu predecessor
      sprintf(str, "SUCC %d %s %s\n", server->nextKey, server->nextIp, server->nextPort); //Envia SUCC ao seu predecessor (3) para ele atualizar o seu duplo sucessor
      n = write(server->prevConnFD, str, strlen(str)); if(n==-1)/*error*/exit(1);
  }else { //Então é porque é um novo servidor a enviar o comando NEW
    if(server->nextConnFD == 0) { //Não tem sucessor, logo sou o unico servidor do anel
      server->nextConnFD = *newfd; // Set the incoming request as the next server
      server->prevConnFD = *newfd;
      sscanf(buffer, "%s %d %s %s", str, &(server->nextKey), server->nextIp, server->nextPort); // Get the successor details
      // Set the double next info as your own
      strcpy(server->doubleNextIp, server->myIp);
      strcpy(server->doubleNextPort, server->myPort);
      server->doubleNextKey = server->myKey;

      //n = write(*newfd,buffer, strlen(buffer)); if(n==-1)/*error*/exit(1);
    } else { 
        if(server->doubleNextKey == server->myKey) { //Se eu for o meu proprio duplo sucessor entao existem apenas 2 servidores no anel[tenho de mudar o meu proprio duplo]
          server->doubleNextKey = newKey;
          strcpy(server->doubleNextIp, newIp);
          strcpy(server->doubleNextPort, newPort);
        }
        n = write(server->prevConnFD, buffer, strlen(buffer)); if(n==-1)/*error*/exit(1); //Enviar NEW ao seu predecessor atual (5)
        printf("Linha 184:%ld %s \n",n, buffer);
        //Atualiza o seu predecessor
        server->prevConnFD = *newfd;
        server->prevKey = newKey;
        strcpy(server->prevIp, newIp);
        strcpy(server->prevPort, newPort);
    }
      //Envia SUCC ao servidor entrante
      sprintf(str, "SUCC %d %s %s\n", server->nextKey, server->nextIp, server->nextPort);
      n = write(*newfd,str, strlen(str)); if(n==-1)/*error*/exit(1);
  }

}

int connectToNextServer(Server* server) {
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
    printf("Error code: %d\n", errno);
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

void printServerData(Server* server) {
  printf("\n\n myKey: %d\nnextKey: %d\ndoubleNextKey: %d\nprevKey: %d\nnextConnection: %d\nprevConnection: %d\n\n", server->myKey, server->nextKey, server->doubleNextKey, server->prevKey, server->nextConnFD, server->prevConnFD);
}