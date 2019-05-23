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
#include <fcntl.h>

#include "rtp.h"

#define PORT 5555
#define MAXMSG 512

TransmissionInfo *transmissionInfo;

void teardown() {
  rtp_h *frame = (rtp_h*)malloc(FRAME_SIZE);
  int state = FIN;

  while (1) {

    if (state != FIN) {
      getData(transmissionInfo, frame);
    }

    switch (state) {

    case FIN:
      printf("Sending FIN + ACK\n");
      makePacket(frame, transmissionInfo->s_vars.seq, FIN + ACK, 0);
      sendData(transmissionInfo, frame);
      state = WAIT_ACK;
      break;

    case WAIT_ACK:
      if (frame->flags == ACK) {
        printf("ACK received, closing...\n");
        exit(EXIT_SUCCESS);
      }
      break;
    
    default:
      return;
      break;
    }
  }
}


void makeSocket() {
	transmissionInfo->host.sin_family = AF_INET;
	transmissionInfo->host.sin_port = htons(PORT);
	transmissionInfo->host.sin_addr.s_addr = htonl(INADDR_ANY);
	
	//make a socket
	transmissionInfo->socket = socket (AF_INET, SOCK_DGRAM, 0);

  transmissionInfo->s_vars.seq = 0;

  if (transmissionInfo->socket == -1) {
    printf("socket error\n");
    exit(1);
  }

  int flags = (flags & O_NONBLOCK);
  fcntl(transmissionInfo->socket, F_SETFL, flags);
	
	//bind socket to address as given by the host struct in the TCB	
	bind(transmissionInfo->socket, (struct sockaddr *) &transmissionInfo->host, sizeof(transmissionInfo->host));
}

int main(int argc, const char *argv[]) { 
    transmissionInfo = (TransmissionInfo*)malloc(sizeof(TransmissionInfo));
    /* Create a socket */
    makeSocket();
  
    initState();

    return EXIT_SUCCESS;
}

void initState() {
  int state = WAIT_SYN;
  rtp_h *frame = (rtp_h*)malloc(FRAME_SIZE);

  while (1) {

    int res = getData(transmissionInfo, frame);

    if (res == -1) {
        printf("fak");
    }

    switch (state)
    {

    case WAIT_SYN:
      
      if (frame->flags == SYN) {
        makePacket(frame, transmissionInfo->s_vars.seq, SYN + ACK, 0);
        sendData(transmissionInfo, frame);
        printf("Received SYN\n");
        state = WAIT_ACK;
      }
      break;

    case WAIT_ACK:
      if (frame->flags == ACK) {
        printf("ACK Received\n");
        state = ESTABLISHED;
      }
      break;

    case ESTABLISHED:
      if (frame->flags == FIN) {
        printf("\n\nFIN received, preparing to close...\n");
        teardown();
      }

      printf("here is paket %d big\n", res);
      printf("Packet received: SEQ = %d, data = %s\n", frame->seq, frame->data);

      break;
    
    default:
      break;
    }
  }
}