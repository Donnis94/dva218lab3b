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

#include "rtp.h"

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
    int i;
    struct sockaddr_in dest_addr;
    socklen_t size;
  
 
    /* Create a socket and set it up to accept connections */
    sock = makeSocket(PORT);

    if ( bind(sock, (const struct sockaddr *)&dest_addr,  
            sizeof(dest_addr)) < 0 )
    { 
        perror("bind failed"); 
        exit(EXIT_FAILURE); 
    }

    rtp_h *data = malloc(sizeof(rtp_h));
    data->flags = WAIT_SYN;
    sendto(sock,&data,sizeof(data),0,(struct sockaddr *)&dest_addr,sizeof(dest_addr));

    return EXIT_SUCCESS;
}