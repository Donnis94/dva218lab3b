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

char message[DATA_SIZE];
int protocol = GBN;

TransmissionInfo *transmissionInfo;
queue readQueue;
queue sentQueue;
queue ackQueue;

void parseArgs(int argc, char **argv) {
  for (int i = 0; i < argc; ++i)
  {
      // if (argc < 2) // no arguments were passed
      // {
      //     p("Please specify host with '-h <hostname>'");
      //     exit(EXIT_FAILURE);
      // }

      if (strcmp("--help", argv[i]) == 0 || strcmp("-h", argv[i]) == 0)
      {
          printf("------------------------------\n");
          printf("RTP Server\n\n");
          printf("Usage: server [option]\n");
          printf("\nOptions:");
          printf("\n-e\t: Error fault percentage (default: 0).");
          printf("\n-v\t: Verbose mode.");
          printf("\n-c\t: Set protocol to Selective repeat (default: Go-back-N)");
          printf("\n\n");
          exit(EXIT_SUCCESS);
      }

      if (strcmp("-e", argv[i]) == 0)
      {
          float error = atof(argv[i+1]);
          if (error < 0.0 || error > 1.0) {
            printf("Error must be set to a number between 0.0 and 1.0, received %.2f\n", error);
            error = 0.0;
          }

          setErrorLevel(error);
          printf("Error = %.2f\n", error);

      }

      if (strcmp("-v", argv[i]) == 0)
      {
          setVerboseLevel(1);
          printf("Verbose = TRUE\n");
      }

      if (strcmp("-c", argv[i]) == 0)
      {
          protocol = SR;
          printf("Protocol = SR\n");
      }
  }

  printf("\n");
}

void teardown() {
  rtp_h *frame = (rtp_h*)malloc(FRAME_SIZE);
  int state = ACK;


  while (1) {

    if (frame->flags == 0) {
      state = ACK;
    }


    if (state != WAIT_FINACK) {
      if (getData(transmissionInfo, frame, 1) <= 0) {
        memset(frame, 0x0, sizeof(rtp_h));
      }
    }

    switch (state) {

      case ACK:
        p("Sending ACK\n");
        makePacket(frame, transmissionInfo->s_vars.next, transmissionInfo->r_vars.next, ACK, 0);
        sendData(transmissionInfo, frame);
        state = WAIT_FINACK;
        break;

      case WAIT_FINACK:

        p("Sending FIN + ACK\n");

        makePacket(frame, transmissionInfo->s_vars.next, transmissionInfo->r_vars.next, FIN + ACK, 0);
        sendData(transmissionInfo, frame);
        enqueue(transmissionInfo, &sentQueue, *frame, SENT);
        state = WAIT_ACK;
        break;

      case WAIT_ACK:
        if (frame->flags == FIN) {
          state = ACK;
          break;
        }

        if (frame->flags == ACK) {
          p("Last ACK received, closing...\n");
          exit(EXIT_SUCCESS);
        }
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
  // int flags = fcntl(transmissionInfo->socket, F_GETFL);
  // flags |= O_NONBLOCK;
  // fcntl(transmissionInfo->socket, F_SETFL, flags);
  transmissionInfo->s_vars.window_size = WINDOW_SIZE;

  transmissionInfo->s_vars.is = randomSeq();
  transmissionInfo->s_vars.next = transmissionInfo->s_vars.is;
  transmissionInfo->s_vars.oldest = transmissionInfo->s_vars.is;

  if (transmissionInfo->socket == -1) {
    p("socket error\n");
    exit(1);
  }
	
	//bind socket to address as given by the host struct in the TCB	
	bind(transmissionInfo->socket, (struct sockaddr *) &transmissionInfo->host, sizeof(transmissionInfo->host));
}

int main(int argc, char **argv) { 
  parseArgs(argc, argv);
  pthread_t timeout_thread;
  transmissionInfo = (TransmissionInfo*)malloc(sizeof(TransmissionInfo));
  /* Create a socket */
  makeSocket();

  printf("Waiting...\n");

  initQueue(&readQueue, transmissionInfo->s_vars.window_size, RECEIVED);
  initQueue(&ackQueue, transmissionInfo->s_vars.window_size, ACKNOWLEDGEMENT);
  initQueue(&sentQueue, transmissionInfo->s_vars.window_size, SENT);
  
    // Initiate timeout thread on sentQueue
  struct timeout_arguments *args = malloc(sizeof(struct timeout_arguments));
  args->arg1 = transmissionInfo;
  args->arg2 = &sentQueue;
  if (protocol == GBN)
    pthread_create(&timeout_thread, 0, &timeout, args);
  else
    pthread_create(&timeout_thread, 0, &selectiveTimeout, args);

  initState();

  return EXIT_SUCCESS;
}

void initState() {
  int state = WAIT_SYN;
  rtp_h *frame = (rtp_h*)malloc(FRAME_SIZE);

  while (1) {

    if (getData(transmissionInfo, frame, 1) <= 0) {
      memset(frame, 0x0, sizeof(rtp_h));
    }

    switch (state)
    {

    case WAIT_SYN:
      
      if (frame->flags == SYN) {

        p("Received SYN, SEQ = %d\n", frame->seq);
        
        transmissionInfo->r_vars.is = frame->seq;
        transmissionInfo->r_vars.next = transmissionInfo->r_vars.is + 1;

        makePacket(frame, transmissionInfo->s_vars.is, transmissionInfo->r_vars.is, SYN + ACK, 0);
        p("Sending ACK, SEQ = %d, ACK = %d\n", frame->seq, frame->ack);
      
        sendData(transmissionInfo, frame);
        transmissionInfo->s_vars.next++;

        state = WAIT_ACK;
      }
      break;

    case WAIT_ACK:
      if (frame->flags == SYN)
        state = WAIT_SYN;

      if (frame->flags == ACK) {
        p("ACK Received, SEQ = %d, ACK = %d\n", frame->seq, frame->ack);
        transmissionInfo->r_vars.next = transmissionInfo->r_vars.is + 1;
        p("Expecting SEQ = %d\n", transmissionInfo->r_vars.next);
        
        state = ESTABLISHED;
        p("ESTABLISHED\n");
      }

      if (!frame->flags) {
        state = ESTABLISHED;
      }
      break;

    case ESTABLISHED:

      if (frame->flags == ACK) {
        p("ACK Received, SEQ = %d, ACK = %d\n", frame->seq, frame->ack);
        transmissionInfo->r_vars.next = frame->seq + 1;
        p("Expecting SEQ = %d\n", transmissionInfo->r_vars.next);
        
        state = ESTABLISHED;
        p("ESTABLISHED\n");
      }

      if (frame->flags == 0) {

        p("Expecting SEQ = %d\n", transmissionInfo->r_vars.next);


        if (protocol == SR && frame->seq <= transmissionInfo->r_vars.next + transmissionInfo->s_vars.window_size) {
          if (frame->seq < transmissionInfo->r_vars.next) {
            p("Old packet received, SEQ = %d, data = %s\n", frame->seq, frame->data);
            makePacket(frame, transmissionInfo->s_vars.next, frame->seq, ACK, 0);
            p("Sending ACK, SEQ = %d, ACK = %d, FLAG = %d\n", frame->seq, frame->ack, frame->flags);
            sendData(transmissionInfo, frame);
          }

          else if (frame->seq == transmissionInfo->r_vars.next) {

            pTimestamp();

            printf("Expected packet received, SEQ = %d, data = %s, CRC = %d\n", frame->seq, frame->data, frame->crc);
            strcat(message, frame->data);
            p("Data %s appended to message: %s\n", frame->data, message);

            makePacket(frame, transmissionInfo->s_vars.next, frame->seq, ACK, 0);
            p("Sending ACK, SEQ = %d, ACK = %d, FLAG = %d\n", frame->seq, frame->ack, frame->flags);
            sendData(transmissionInfo, frame);
            transmissionInfo->s_vars.next++;
            transmissionInfo->r_vars.next++;

            int index = isInQueue(&readQueue, transmissionInfo->r_vars.next);
            p("Next: %d at index = %d\n", transmissionInfo->r_vars.next, index);

            while (index >= 0) {
              transmissionInfo->r_vars.next++;
              strcat(message, readQueue.queue[index].data);
              p("Data %s appended to message: %s\n", frame->data, readQueue.queue[index].data);

              p("Next: %d", transmissionInfo->r_vars.next);
              printQueue(&readQueue);
              removeFromQueue(&readQueue, index);

              index = isInQueue(&readQueue, transmissionInfo->r_vars.next);
            }
          }

          else if (frame->seq > transmissionInfo->r_vars.next) {
            p("Future packet received, SEQ = %d, data = %s, saving in buffer...\n", frame->seq, frame->data);
            if (enqueue(transmissionInfo, &readQueue, *frame, RECEIVED) != -1) {
              printQueue(&readQueue);
              
              makePacket(frame, transmissionInfo->s_vars.next, frame->seq, ACK, 0);
              p("Sending ACK, SEQ = %d, ACK = %d, FLAG = %d\n", frame->seq, frame->ack, frame->flags);
              sendData(transmissionInfo, frame);
              transmissionInfo->s_vars.next++;
            }
          }
        }

        if (protocol == GBN) {
          // Old packet
          if (frame->seq < transmissionInfo->r_vars.next) {
            p("Old packet received, SEQ = %d, data = %s\n", frame->seq, frame->data);
            makePacket(frame, transmissionInfo->s_vars.next, frame->seq, ACK, 0);
            p("Sending ACK, SEQ = %d, ACK = %d, FLAG = %d\n", frame->seq, frame->ack, frame->flags);
            sendData(transmissionInfo, frame);
          }
          // Expected packet
          else if (frame->seq == transmissionInfo->r_vars.next) {
            pTimestamp();
            printf("Expected packet received, SEQ = %d, data = %s, CRC = %d\n", frame->seq, frame->data, frame->crc);
            // dequeue(&readQueue);
            transmissionInfo->r_vars.next++;
            strcat(message, frame->data);
            // while (readQueue.queue[0].seq == transmissionInfo->r_vars.next) {
            //   rtp_h *ackFrame = malloc(sizeof(rtp_h));
            //   makePacket(ackFrame, transmissionInfo->s_vars.next, readQueue.queue[0].seq, ACK, 0);
            //   p("Sending ACK, SEQ = %d, ACK = %d, FLAG = %d\n", ackFrame->seq, ackFrame->ack, ackFrame->flags);
            //   sendData(transmissionInfo, ackFrame, error);
            //   dequeue(&readQueue);
            //   transmissionInfo->s_vars.next++;
            //   transmissionInfo->r_vars.next++;
            //   strcat(message, frame->data);
            //   free(ackFrame);
            // }

            // if (isQueueEmpty(&readQueue)) {
            //   p("Queue is empty\n");
            // }

            // if (isQueueFull(&readQueue)) {
            //   p("Queue is full");
            // }
            
            makePacket(frame, transmissionInfo->s_vars.next, frame->seq, ACK, 0);
            p("Sending ACK, SEQ = %d, ACK = %d, FLAG = %d\n", frame->seq, frame->ack, frame->flags);
            sendData(transmissionInfo, frame);
            transmissionInfo->s_vars.next++;
          }
          // Future
          else if (frame->seq > transmissionInfo->r_vars.next) {
            p("Future packet received, SEQ = %d, data = %s\n", frame->seq, frame->data);
            // enqueue(transmissionInfo, &readQueue, *frame, RECEIVED);
            // printQueue(&readQueue);
          }
        }

      }

      // If it's not a data packet
      if (frame->flags == FIN) {
        printf("\n\nMessage: %s\n\n", message);
        teardown();
      }

      break;
    
    default:
      break;
    }

    usleep(10000);
  }
}