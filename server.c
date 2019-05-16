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
#include <errno.h>

#include "rtp.h"

#define PORT 5555
#define MAXMSG 512

void makeSocket(TransimssionInfo *ti) {
	ti->host.sin_family = AF_INET;
	ti->host.sin_port = htons(PORT);
	ti->host.sin_addr.s_addr = htonl(INADDR_ANY);
	
	//make a socket
	ti->socket = socket (AF_INET, SOCK_DGRAM, 0);

  if (ti->socket == -1) {
    printf("socket error\n");
    exit(1);
  }
	
	//bind socket to address as given by the host struct in the TCB	
	bind(ti->socket, (struct sockaddr *) &ti->host, sizeof(ti->host));
}

int main(int argc, const char *argv[]) {
    TransimssionInfo transmissionInfo;
  
 
    /* Create a socket */
    makeSocket(&transmissionInfo);
  
    initState(&transmissionInfo);

    return EXIT_SUCCESS;
}

void initState(TransimssionInfo *transmissionInfo) {
  int state = WAIT_SYN;
  rtp_h *frame = (rtp_h*)malloc(FRAME_SIZE);

  while (1) {

    int res = getData(transmissionInfo, frame);

    printf("%d", res);

    if (res == -1) {
      perror("Error: ");
      printf("\n");
    }

    switch (state)
    {
    case INIT:
      /* code */
      break;

    case WAIT_SYN:
      
      if (frame->flags == SYN) {
        frame->flags = SYN+ACK;
        sendData(transmissionInfo, frame);
        printf("Received SYN");
        state = WAIT_SYNACK;
      }

      
      break;

    case WAIT_SYNACK:
      /* code */
      break;

    case WAIT_ACK:
      /* code */
      break;
    
    default:
      break;
    }

    sleep(1);
  }
}