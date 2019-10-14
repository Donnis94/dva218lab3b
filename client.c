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

#define messageLength 256

char hostName[50];
char message[DATA_SIZE];
int split = 1;
int error = 0;
int protocol = GBN;

TransmissionInfo *transmissionInfo;
queue sentQueue;
queue ackQueue;

void parseArgs(int argc, char **argv) {
  for (int i = 0; i < argc; ++i)
  {
      if (argc < 2) // no arguments were passed
      {
          printf("Please specify host with '-h <hostname>'");
          exit(EXIT_FAILURE);
      }

      if (strcmp("--help", argv[i]) == 0)
      {
          printf("------------------------------\n");
          printf("RTP Implementation\n\n");
          printf("Usage: client -h <ip> -m <message> [OPTION]\n");
          printf("\n");
          printf("\n-h=HOSTNAME\n\tSpecify a hostname to connect to, eg. localhost\n");
          printf("\n-m=MESSAGE\n\tSpecify a message to send. The message will be sent in splits of 1 by default. To change this, specify -s <split>. To send a message with spaces, use quotes (\"\").\n");
          printf("\n-e=ERROR\n\tError fault percentage.\n");
          printf("\n-s=SPLIT\n\tHow many characters should be sent in a single packet.\n");
          exit(EXIT_SUCCESS);
      }

      if (strcmp("-h", argv[i]) == 0)
      {
          strcpy(hostName, argv[i + 1]);
      }
      if (strcmp("-m", argv[i]) == 0)
      {
          strcpy(message, argv[i + 1]);
      }
      if (strcmp("-s", argv[i]) == 0)
      {
          split = atoi(argv[i+1]);
      }
      if (strcmp("-e", argv[i]) == 0)
      {
          error = atoi(argv[i+1]);
      }
      if (strcmp("-c", argv[i]) == 0)
      {
          protocol = SR;
      }
  }
}

// void mainMenu(rtp_h *frame){
//   int choice;
//   printf("Choose what to do:\n");
//   printf("1. Send packet with arbitrary data:\n");
//   printf("2. Lose packet with arbitrary data:\n");
//   printf("3. Send packet with failed checksum:\n");
//   printf("4. Quit connection:\n");
//   scanf("%d",&choice);
//   fflush(stdout);

//   char * buffer = (char*)malloc(DATA_SIZE);
//   strncpy(buffer, "guten tag\0", DATA_SIZE);

//   switch(choice){

//   case 1:

//     for (int i = 0; i < 5; i++) {
//       char *charr = malloc(2);
//       strncpy(charr, buffer + i, 1);
//       charr[1] = '\0';
//       makePacket(frame, transmissionInfo->s_vars.next, 0, 0, charr);
//       sendData(transmissionInfo, frame);
//       printf("Sent packet, SEQ = %d, data = %s, CRC = %d\n", transmissionInfo->s_vars.next, frame->data, frame->crc);
//       enqueue(transmissionInfo, &sentQueue, *frame, SENT);
//     }
//     // // incrementSeq(transmissionInfo);
//     // if (!isQueueFull(&sentQueue)) {
//     //   makePacket(frame, transmissionInfo->s_vars.next, 0, 0, buffer);
//     //   sendData(transmissionInfo, frame);
//     //   printf("Sent packet, SEQ = %d, data = %s, CRC = %d\n", transmissionInfo->s_vars.next, frame->data, frame->crc);
//     //   enqueue(transmissionInfo, &sentQueue, *frame, SENT);
//     // }

//     break;

//   case 2:

//     // incrementSeq(transmissionInfo);
//     if (!isQueueFull(&sentQueue)) {
//       makePacket(frame, transmissionInfo->s_vars.next, 0, 0, buffer);
//       sendLostData(transmissionInfo, frame);
//       printf("Sent packet, SEQ = %d, data = %s, CRC = %d\n", transmissionInfo->s_vars.next, frame->data, frame->crc);
//       enqueue(transmissionInfo, &sentQueue, *frame, SENT);
//     }

//     break;

//   case 3:

//     // incrementSeq(transmissionInfo);
//     if (!isQueueFull(&sentQueue)) {
//       makePacket(frame, transmissionInfo->s_vars.next, 0, 0, buffer);
//       frame->crc = 1337;
//       sendData(transmissionInfo, frame);
//       printf("Sent packet, SEQ = %d, data = %s, CRC = %d\n", transmissionInfo->s_vars.next, frame->data, frame->crc);
//       enqueue(transmissionInfo, &sentQueue, *frame, SENT);
//     }

//     break;
  
//   case 4:
//     printf("\n\nPreparing to close...\n");
//     teardown();
//     break;
//   }
// }

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

  int flags = fcntl(transmissionInfo->socket, F_GETFL, 0);
  if (flags == -1) {
    printf("Error setting socket to non-block");
    exit(EXIT_FAILURE);
  }
  flags = (flags | O_NONBLOCK);
  return (fcntl(transmissionInfo->socket, F_SETFL, flags) == 0) ? 0 : -1;
}

int main (int argc, char **argv){
  parseArgs(argc, argv);
  pthread_t timeout_thread;
  transmissionInfo = (TransmissionInfo*)malloc(sizeof(TransmissionInfo));

  makeSocket(hostName);

  initQueue(&sentQueue, transmissionInfo->s_vars.window_size);
  initQueue(&ackQueue, transmissionInfo->s_vars.window_size);

  // Initiate timeout thread on sentQueue
  struct timeout_arguments *args = malloc(sizeof(struct timeout_arguments));
  args->arg1 = transmissionInfo;
  args->arg2 = &sentQueue;
  args->arg3 = &ackQueue;
  if (protocol == GBN)
    pthread_create(&timeout_thread, 0, &timeout, args);
  else
    pthread_create(&timeout_thread, 0, &selectiveTimeout, args);

  initState();

  return EXIT_SUCCESS;
}

void initState() {
  int messageIndex = 0;
  int mLen = strlen(message);
  int state = INIT;
  rtp_h *frame = malloc(FRAME_SIZE);

  while (1) {  

    if (getData(transmissionInfo, frame, 1) <= 0) {
      memset(frame, 0x0, sizeof(rtp_h));
    }

    switch (state) {

    case INIT:
      makePacket(frame, transmissionInfo->s_vars.is, 0, SYN, 0);
      sendData(transmissionInfo, frame);
      enqueue(transmissionInfo, &sentQueue, *frame, SENT);

      transmissionInfo->r_vars.is = frame->seq;
      transmissionInfo->r_vars.next = transmissionInfo->r_vars.is + 1;

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
        printf("Received ACK for packet SEQ = %d\n", frame->ack);
        enqueue(transmissionInfo, &ackQueue, *frame, ACKNOWLEDGEMENT);

        if (protocol == GBN) {
          if (frame->ack == sentQueue.queue[0].seq) {
            transmissionInfo->r_vars.oldest++;
            dequeue(&sentQueue);
            transmissionInfo->s_vars.oldest++;
          }
        }

        else if(protocol == SR) {
          // ACK
          
          // CASE 1: FIRST
          //int sentIndex = isInQueue(&sentQueue, frame->ack);

          if (frame->ack == transmissionInfo->s_vars.oldest) {
            // oldest sent ACK
            dequeue(&sentQueue);
            transmissionInfo->s_vars.oldest++;

            int oldestIndex = isInQueue(&ackQueue, frame->ack);

            removeFromQueue(&ackQueue, oldestIndex);
            transmissionInfo->r_vars.oldest++;
          }

          int oldestIndex = isInQueue(&ackQueue, transmissionInfo->s_vars.oldest);

          while (oldestIndex != -1) {
            dequeue(&sentQueue);
            removeFromQueue(&ackQueue, oldestIndex);
            transmissionInfo->s_vars.oldest++;
            oldestIndex = isInQueue(&ackQueue, transmissionInfo->s_vars.oldest);
          }

          // while(ackIndex != -1) {
          //   if (sentIndex != -1) {
          //     removeFromQueue(&sentQueue, sentIndex);
          //   }s
          //   removeFromQueue(&ackQueue, ackIndex);
          //   // removeFromQueue(&sentQueue, sentIndex);
          //   ackIndex = isInQueue(&ackQueue, transmissionInfo->r_vars.next);
          //   // sentIndex = isInQueue(&sentQueue, transmissionInfo->r_vars.oldest);
          // }

          // if (ackIndex != -1 && sentIndex != -1) {
          //   removeFromQueue(&ackQueue, ackIndex);
          //   removeFromQueue(&sentQueue, sentIndex);
          // }
        }
      }

      char *charr = calloc(DATA_SIZE, 1);
      while (messageIndex < mLen && !isQueueFull(&sentQueue)) {
        memset(charr, 0x0, DATA_SIZE);
        if (mLen - messageIndex > split) {
          strncpy(charr, message + messageIndex, split);
          charr[split] = '\0';
        } else {
          strncpy(charr, message + messageIndex, mLen - messageIndex);
          charr[mLen - messageIndex] = '\0';
        }
        makePacket(frame, transmissionInfo->s_vars.next, 0, 0, charr);
        // Testing packet loss on third character in our message
        if (messageIndex == 2) {
          sendLostData(transmissionInfo, frame);
        } else {

          sendData(transmissionInfo, frame);
        }
        messageIndex += split;
        printf("Sent packet, SEQ = %d, data = %s, CRC = %d\n", transmissionInfo->s_vars.next, frame->data, frame->crc);
        enqueue(transmissionInfo, &sentQueue, *frame, SENT);
      }

      if (messageIndex >= mLen && isQueueEmpty(&sentQueue)) {
        state = DONE;
      }
      
      break;

    case DONE:
      if (frame->flags == ACK) {
        // Received an ACK, increment oldest.
        transmissionInfo->r_vars.oldest++;
        printf("Received ACK for packet SEQ = %d\n", frame->ack);
        dequeue(&sentQueue);
				transmissionInfo->s_vars.oldest++;
      }

      if (isQueueEmpty(&sentQueue)) {
        teardown();
      }

      getData(transmissionInfo, frame, 1);
      break;
    
    default:
      // if (state == WAIT_ACK) {
      //   sendData(transmissionInfo, frame);
      // }
      break;
    }

    usleep(10000);

  }
}