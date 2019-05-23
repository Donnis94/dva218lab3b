/* File: client.c
 * Trying out socket communication between processes using the Internet protocol family.
 * Usage: client [host name], that is, if a server is running on 'lab1-6.idt.mdh.se'
 * then type 'client lab1-6.idt.mdh.se' and follow the on-screen instructions.
 * 
 * Author Donatello Piancazzo dpo16001 and Oskar Berglund obd16004 Alexander Andersson Tholin atn17004 2019/05/02
 * 
 * 
 */

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include <fcntl.h>

#include "rtp.h"

#define hostNameLength 50
#define messageLength 256

TransmissionInfo *transmissionInfo;

void mainMenu(){
  int choice;
  printf("Choose what to do:\n1.Recieve package\n2.Quit connection to server\nInput: ");
  scanf("%d",&choice);
  fflush(stdout);
  switch(choice){

    case 1:
      break;
    
    case 2:
      printf("\n\nPreparing to close...\n");
      teardown();
      break;
  }
  
}

void teardown() {
  rtp_h *frame = (rtp_h*)malloc(FRAME_SIZE);
  frame->flags = FIN;

  int state = FIN;

  while (1) {

    if (state == WAIT_FINACK) {
      getData(transmissionInfo, frame);
    }

    switch (state) {

    case FIN:
      printf("Sending FIN\n");
      makePacket(frame, transmissionInfo->s_vars.seq, FIN, 0);
      sendData(transmissionInfo, frame);
      state = WAIT_FINACK;
      break;

    case WAIT_FINACK:
      if (frame->flags == FIN + ACK) {
        printf("Received FIN + ACK\n");
        state = ACK;
      }
      break;

    case ACK:
      printf("Sending ACK\n");
      makePacket(frame, transmissionInfo->s_vars.seq, ACK, 0);
      sendData(transmissionInfo, frame);
      exit(EXIT_SUCCESS);
    break;
    
    default:
      return;
      break;
    }
  }
}

int makeSocket(char* hostName) {	
	struct hostent *hostInfo;
	hostInfo = gethostbyname(hostName);
	transmissionInfo->dest.sin_family = AF_INET;
	transmissionInfo->dest.sin_port = htons(PORT);
	transmissionInfo->dest.sin_addr = *(struct in_addr *)hostInfo->h_addr_list[0];
	
	// Create socket
	transmissionInfo->socket = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, 0);

  transmissionInfo->s_vars.seq = 0;

  if (transmissionInfo->socket == -1) {
      printf("socket error\n");
      exit(1);
  }

  int flags = (flags & O_NONBLOCK);
  fcntl(transmissionInfo->socket, F_SETFL, flags);

	return 0;
}

int main (int argc, const char *argv[]){
  char hostName[hostNameLength];
  transmissionInfo = (TransmissionInfo*)malloc(sizeof(TransmissionInfo));

  /* Check arguments */
  if(argv[1] == NULL) {
      perror("Usage: client [host name]\n");
      exit(EXIT_FAILURE);
  }
  else {
      strncpy(hostName, argv[1], hostNameLength);
      hostName[hostNameLength - 1] = '\0';
  }

  makeSocket(hostName);

  initState();

  return EXIT_SUCCESS;
}

void initState() {
  int state = INIT;
  rtp_h *frame = (rtp_h*)malloc(FRAME_SIZE);

  while (1) {

    if (state != INIT && state != ESTABLISHED) {
      int res = getData(transmissionInfo, frame);

      if (res == -1) {
        perror("Error: ");
        printf("\n");
      }
    }
    

    switch (state)
    {
    case INIT:
      makePacket(frame, getSeq(transmissionInfo), SYN, 0);
      sendData(transmissionInfo, frame);
      printf("Sending SYN, SEQ = %d...\n", getSeq(transmissionInfo));
      state = WAIT_SYNACK;

      break;

    case WAIT_SYNACK:
      if (frame->flags == SYN+ACK) {

        printf("Received SYN+ACK, SEQ = %d\n", frame->seq);
        makePacket(frame, frame->seq, ACK, 0);
        sendData(transmissionInfo, frame);
        printf("Sending ACK for SEQ = %d...\n", frame->seq);
        frame->flags = 0;
        state = ESTABLISHED;
      }
    break;

    case ESTABLISHED:

      printf("\n\n");
      mainMenu();
      printf("\n\n");

      char * buffer = (char*)malloc(DATA_SIZE);
      strncpy(buffer, "guten tag\0", DATA_SIZE);

      if (frame->flags == ACK) {
        printf("Received ACK, SEQ = %d\n", frame->seq);
      }

      incrementSeq(transmissionInfo);

      makePacket(frame, getSeq(transmissionInfo), 0, buffer);
      sendData(transmissionInfo, frame);
      printf("Sent packet, SEQ = %d, data = %s\n", getSeq(transmissionInfo), frame->data);
      break;
    
    default:
      // if (state == WAIT_ACK) {
      //   sendData(transmissionInfo, frame);
      // }
      break;
    }
  }
}