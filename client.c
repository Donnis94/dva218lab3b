/* File: client.c
 * Trying out socket communication between processes using the Internet protocol family.
 * Usage: client [host name], that is, if a server is running on 'lab1-6.idt.mdh.se'
 * then type 'client lab1-6.idt.mdh.se' and follow the on-screen instructions.
 * 
 * Author Donatello Piancazzo dpo16001 and Oskar Berglund obd16004 Alexander Andersson Tholin atn17004 2019/05/02
 * 
 * 
 */

#include "rtp.h"

#define hostNameLength 50
#define messageLength 256

TransmissionInfo *transmissionInfo;
queue sentQueue;
queue ackQueue;

void mainMenu(rtp_h *frame){
  int choice;
  printf("Choose what to do:\n");
  printf("1. Send packet with arbitrary data:\n");
  printf("2. Lose packet with arbitrary data:\n");
  printf("3. Send packet with failed checksum:\n");
  printf("4. Quit connection:\n");
  scanf("%d",&choice);
  fflush(stdout);

  char * buffer = (char*)malloc(DATA_SIZE);
  strncpy(buffer, "guten tag\0", DATA_SIZE);

  switch(choice){

    case 1:


      // incrementSeq(transmissionInfo);
      if (!isQueueFull(&sentQueue)) {
        makePacket(frame, transmissionInfo->s_vars.next, 0, 0, buffer);
        sendData(transmissionInfo, frame);
        printf("Sent packet, SEQ = %d, data = %s\n", transmissionInfo->s_vars.next, frame->data);
        enqueue(transmissionInfo, &sentQueue, *frame, SENT);
      }
      break;

    case 2:

      // incrementSeq(transmissionInfo);
      if (!isQueueFull(&sentQueue)) {
        makePacket(frame, transmissionInfo->s_vars.next, 0, 0, buffer);
        sendLostData(transmissionInfo, frame);
        printf("Sent packet, SEQ = %d, data = %s\n", transmissionInfo->s_vars.next, frame->data);
        enqueue(transmissionInfo, &sentQueue, *frame, SENT);
      }
      break;
    
    case 4:
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
      getData(transmissionInfo, frame, 1);
    }

    switch (state) {

    case FIN:
      printf("Sending FIN\n");
      makePacket(frame, transmissionInfo->s_vars.next, 0, FIN, 0);
      sendData(transmissionInfo, frame);
      transmissionInfo->s_vars.next++;
      state = WAIT_FINACK;
      break;

    case WAIT_FINACK:
      if (frame->flags == FIN + ACK) {
        printf("Received FIN + ACK\n");
        state = ACK;
        printf("Sending ACK\n");
        makePacket(frame, 0, frame->ack, ACK, 0);
        sendData(transmissionInfo, frame);
        transmissionInfo->s_vars.next++;

        exit(EXIT_SUCCESS);
      }
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
	transmissionInfo->socket = socket(AF_INET, SOCK_DGRAM, 0);
  transmissionInfo->s_vars.window_size = WINDOW_SIZE;

  transmissionInfo->s_vars.is = randomSeq();
  transmissionInfo->s_vars.next = transmissionInfo->s_vars.is;
  transmissionInfo->s_vars.oldest = transmissionInfo->s_vars.is;

  if (transmissionInfo->socket == -1) {
      printf("socket error\n");
      exit(1);
  }

	return 0;
}

int main (int argc, const char *argv[]){
  pthread_t timeout_thread;
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

  initQueue(&sentQueue, transmissionInfo->s_vars.window_size);
  initQueue(&ackQueue, transmissionInfo->s_vars.window_size);

  // Initiate timeout thread on sentQueue
  struct timeout_arguments *args = malloc(sizeof(struct timeout_arguments));
  args->arg1 = transmissionInfo;
  args->arg2 = &sentQueue;

  pthread_create(&timeout_thread, 0, &timeout, args);

  initState();

  return EXIT_SUCCESS;
}

void initState() {
  int state = INIT;
  rtp_h *frame = malloc(FRAME_SIZE);

  while (1) {  

    switch (state)
    {
    case INIT:
      makePacket(frame, transmissionInfo->s_vars.is, 0, SYN, 0);
      sendData(transmissionInfo, frame);
      enqueue(transmissionInfo, &sentQueue, *frame, SENT);

      printf("Sending SYN, SEQ = %d\n", frame->seq);
      state = WAIT_SYNACK;
      getData(transmissionInfo, frame, 1);
      break;

    case WAIT_SYNACK:
      if (frame->flags == SYN+ACK) {
        if (frame->ack == transmissionInfo->s_vars.is) {
          // An ack has been received for our SYN.
          // Dequeue packet
          dequeue(&sentQueue);
          // Increment oldest received
          transmissionInfo->s_vars.oldest++;
          transmissionInfo->r_vars.is = transmissionInfo->r_vars.next = frame->seq;

          printf("Received SYN+ACK for SEQ = %d\n", frame->ack);

          // ACK the sequence number
          makePacket(frame, 0, transmissionInfo->r_vars.is, ACK, 0);
          sendData(transmissionInfo, frame);
   
          printf("Sending ACK = %d...\n", frame->ack);
          frame->flags = 0;
          state = ESTABLISHED;
        }        
      }
    break;

    case ESTABLISHED:      

      if (frame->flags == ACK) {
        // Received an ACK, increment oldest.
        transmissionInfo->r_vars.oldest++;
        printf("Received ACK for packet SEQ = %d\n", frame->ack);
        dequeue(&sentQueue);
				dequeue(&ackQueue);
				transmissionInfo->s_vars.oldest++;
      }

      printf("\n\n");

      mainMenu(frame);
      printf("\n\n");

      getData(transmissionInfo, frame, 1);
      
      break;
    
    default:
      // if (state == WAIT_ACK) {
      //   sendData(transmissionInfo, frame);
      // }
      break;
    }

    usleep(100000);

  }
}