#ifndef _SERVER_H
#define _SERVER_H
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

typedef struct server {
  char myIp[30];
  char myPort[10];
  int myKey;

  char nextIp[30];
  char nextPort[10];
  int nextKey;

  char doubleNextIp[30];
  char doubleNextPort[10];
  int doubleNextKey;

  int prevConnFD; // Holds the TCP connection to previous server
  int nextConnFD; // Holds the TCP connection to next server
}Server;

typedef struct udpData {
  struct addrinfo *res;
  int fd;
}UdpData;


void createServer(Server*, int);
int connectToNextServer(Server*);
void serverIsEntering(char[128], int *, Server*);
void printServerData(Server*);
void copyDoubleToNext(Server *);
int distance(int, int);
int connectToGivenServer(char[30], char[10]);
UdpData* connectToUdpServer(char ip[30], char port[10]);
void startKeySearch(Server *, int, int, struct sockaddr*, socklen_t, int, int *);
void copyNextToDouble(Server *server);
void copySelfToDouble(Server *server);
void cleanServer(Server * server);
void tcpWrite(int, char *);
int tpcRead(int, char *);

#endif