/*
* RCI Project - Distributed Hashtable - João Maria Janeiro and Guilherme Viegas
* Key values ​​equal to -1 mean that it does not contain information about that server
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "server.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>
#include <math.h>
#include <sys/select.h>
#include <errno.h>

#define N 16



int checkIpPortValid(int, char **);
int valid_digit(char *);
int checkIp(char *);
void flush_stdin();

char aux[100];
char c;
int n;
char buff[100];

struct addrinfo *res;
UdpData* udp_main;


int main(int argc, char *argv[]) {
    if(!checkIpPortValid(argc, argv)) exit(0); // Check if the dkt program was summoned with 3 arguments
    Server * myServer = (Server*)malloc(sizeof(Server));
    strcpy(myServer->myIp, "127.0.0.1");
    strcpy(myServer->myPort, argv[2]);
      

    // Show the user interface
    printf("\n\nAvailable commands:\n\n new i \n sentry i succi succi.IP succi.TCP\n entry i succi succie.IP succi.TCP\n leave\n exit\n");
    
    strcpy(buff, "\0");
    fgets(buff, 100 , stdin);
    const char delim[2] = " ";
    if(strcmp(strtok(strdup(buff), delim), "new") == 0) { // If its the "new" command then create a ring with that server
        sscanf(buff, "%s %d", aux, &(myServer->myKey)); // Get the server key
        if(myServer->myKey > N) {
            printf("The key must be smaller than the ring size. Ring size is currently set to 16\n");
            do {
                printf("Please choose another key (must be between 1-16)\n");
                printf("New Key: ");
                scanf("%d", &(myServer->myKey));
            } while(myServer->myKey > N);
        }
        createServer(myServer, 0);
    } else  if(strcmp(strtok(strdup(buff), delim), "sentry") == 0) { // If its the sentry command open a connection with nextServer 
        sscanf(buff, "%s %d %d %s %s", aux, &(myServer->myKey), &(myServer->nextKey), myServer->nextIp, myServer->nextPort); // Get the successor details
        if(myServer->myKey > N) {
            printf("The key must be smaller than the ring size. Ring size is currently set to 16\n");
            do {
                printf("Please choose another key (must be between 1-16)\n");
                printf("New Key: ");
                scanf("%d", &(myServer->myKey));
            } while(myServer->myKey > N);
        } 
        printf("Trying to enter\n");
        myServer->nextConnFD = connectToNextServer(myServer); // Set the next server as the given server and establish a connection
        
        sprintf(buff, "NEW %d %s %s\n", myServer->myKey, myServer->myIp, myServer->myPort);
        int n = write(myServer->nextConnFD, buff, strlen(buff)); // Give the successor your details
        if(n == -1)/*error*/exit(1);
                        
        createServer(myServer, 1); //Now that the entry connections are established and stable it's time enter in listening mode
    } else if(strcmp(strtok(strdup(buff), delim), "entry") == 0) {
        char connectIp[100], connectPort[100];
        int connectKey;
        sscanf(buff, "%s %d %d %s %s", aux, &(myServer->myKey), &connectKey, connectIp, connectPort); // Get the successor details
        if(myServer->myKey > N) {
            printf("The key must be smaller than the ring size. Ring size is currently set to 16\n");
            do {
                printf("Please choose another key (must be between 1-16)\n");
                printf("New Key: ");
                scanf("%d", &(myServer->myKey));
            } while(myServer->myKey > N);
        }
        udp_main = connectToUdpServer(connectIp, connectPort);

        sprintf(buff, "EFND %d\n", myServer->myKey);
        n=sendto(udp_main->fd,buff,strlen(buff),0, udp_main->res->ai_addr, udp_main->res->ai_addrlen);
        if(n==-1) /*error*/ exit(1);

        n=recvfrom(udp_main->fd,buff,128,0, (struct sockaddr*)&(udp_main->res->ai_addr),&(udp_main->res->ai_addrlen));
        if(n==-1) /*error*/ exit(1);


        sscanf(buff, "%s %d %d %s %s", aux, &(myServer->myKey), &(myServer->nextKey), myServer->nextIp, myServer->nextPort); // Get the successor details
        printf("Trying to enter\n");
        myServer->nextConnFD = connectToNextServer(myServer); // Set the next server as the given server and establish a connection
        
        sprintf(buff, "NEW %d %s %s\n", myServer->myKey, myServer->myIp, myServer->myPort);
        int n = write(myServer->nextConnFD, buff, strlen(buff)); // Give the successor your details
        if(n == -1)/*error*/exit(1);
                        
        createServer(myServer, 1); //Now that the entry connections are established and stable it's time enter in listening mode

        freeaddrinfo(udp_main->res);
        free(udp_main);
        close(udp_main->fd);
    } else if(strstr(buff, "exit") != NULL) {
        free(myServer);
        exit(0);
    }
    free(myServer);
    return 0;
}

int checkIpPortValid(int _argc, char *_argv[]) {
    if(_argc !=  3) {
        printf("Number of Arguments must be 3\n");
        exit(0);
    }
    if(!checkIp(_argv[1])) {
        exit(0);
    }
    return 1;
}

/* return 1 if string contain only digits, else return 0 */
int valid_digit(char *ip_str) { 
    while (*ip_str) { 
        if (*ip_str >= '0' && *ip_str <= '9') 
            ++ip_str; 
        else
            return 0; 
    } 
    return 1; 
}

/*Check the Ip Address format*/
int checkIp(char * ip) {
    char* ptr;
    int i, num, dots = 0;


    ptr = strtok(ip, ".");

    if (ptr == NULL) 
        return 0; 
  
    while (ptr) { 
        if (!valid_digit(ptr)) {
            printf("Not Valid Digit\n");
            return 0; 
        }
  
        num = atoi(ptr); 
  
        if (num >= 0 && num <= 255) {
            ptr = strtok(NULL, "."); 
            if (ptr != NULL) 
                ++dots; 
        } else {
            printf("Strange Format my boy");
            return 0; 
        }
    } 
  
    /* valid IP string must contain 3 dots */
    if (dots != 3) {
        printf("Number of dots must be of 3 %d\n", dots);
        return 0; 
    }
    return 1; 
}

