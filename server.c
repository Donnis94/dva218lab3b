/* File: server.c
 * Trying out socket communication between processes using the Internet protocol family.
 * 
 * Author Donatello Piancazzo dpo16001 and Oskar Berglund obd16004 Alexander Andersson Tholin atn17004 2019/05/02
 * 
 * 
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/times.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>

#define PORT 5555
#define MAXMSG 512

int makeSocket(unsigned short int port) {
  int sock;
  struct sockaddr_in name;

  /* Create a socket. */
  sock = socket(PF_INET, SOCK_STREAM, 0);
  if(sock < 0) {
    perror("Could not create a socket\n");
    exit(EXIT_FAILURE);
  }
  /* Give the socket a name. */
  /* Socket address format set to AF_INET for Internet use. */
  name.sin_family = AF_INET;
  /* Set port number. The function htons converts from host byte order to network byte order.*/
  name.sin_port = htons(port);
  /* Set the Internet address of the host the function is called from. */
  /* The function htonl converts INADDR_ANY from host byte order to network byte order. */
  /* (htonl does the same thing as htons but the former converts a long integer whereas
   * htons converts a short.) 
   */
  name.sin_addr.s_addr = htonl(INADDR_ANY);
  /* Assign an address to the socket by calling bind. */
  if(bind(sock, (struct sockaddr *)&name, sizeof(name)) < 0) {
    perror("Could not bind a name to the socket\n");
    exit(EXIT_FAILURE);
  }
  return(sock);
}
int main(int argc, const char *argv[]) {
     int sock;
  int clientSocket;
  int i;
  fd_set activeFdSet, readFdSet; /* Used by select */
  struct sockaddr_in clientName;
  socklen_t size;
  
 
  /* Create a socket and set it up to accept connections */
  sock = makeSocket(PORT);
  /* Listen for connection requests from clients */
  if(listen(sock,1) < 0) {
    perror("Could not listen for connections\n");
    exit(EXIT_FAILURE);
  }
  /* Initialize the set of active sockets */
  FD_ZERO(&activeFdSet);
  FD_SET(sock, &activeFdSet);
  
  printf("\n[waiting for connections...]\n");
  while(1) {
    /* Block until input arrives on one or more active sockets
       FD_SETSIZE is a constant with value = 1024 */
    readFdSet = activeFdSet;
    if(select(FD_SETSIZE, &readFdSet, NULL, NULL, NULL) < 0) {
      perror("Select failed\n");
      exit(EXIT_FAILURE);
    }
    /* Service all the sockets with input pending */
    for(i = 0; i < FD_SETSIZE; ++i) 
      if(FD_ISSET(i, &readFdSet)) {
	    if(i == sock) {
        /* Connection request on original socket */
            size = sizeof(struct sockaddr_in);
        /* Accept the connection request from a client. */
            clientSocket = accept(sock, (struct sockaddr *)&clientName, (socklen_t *)&size);
            char* str = inet_ntoa(clientName.sin_addr); 
            if(clientSocket < 0) {
                perror("Could not accept connection\n");
                exit(EXIT_FAILURE);
            }
            else{
                printf("Server: Connect from client %s, port %d\n", 
                inet_ntoa(clientName.sin_addr), 
                ntohs(clientName.sin_port));
            }
        }
      }
    }
    return EXIT_SUCCESS;
}