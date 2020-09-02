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
#include <sys/time.h>
#include <sys/select.h>
#include <openssl/md5.h>
#include <stdarg.h>
#include <fcntl.h>

#define PORT 5555
#define DST "127.0.0.1"

#define WINDOW_SIZE 5

#define FRAME_SIZE 544
#define DATA_SIZE 512

#define INIT 0
#define WAIT_SYN 1
#define WAIT_SYNACK 2
#define WAIT_ACK 3
#define WAIT_FINACK 4
#define CLOSED 99
#define DONE 999

#define ESTABLISHED 1337

#define SYN 420
#define ACK 421
#define FIN 422

#define GBN 333
#define SR 334

enum QueueType {
    SENT,
    RECEIVED,
    ACKNOWLEDGEMENT
};

// A packet being sent
typedef struct rtp_header {
    long stime;
    long time;
    int flags;
    int id;
    int ack;
    int seq;
    int windowsize;
    int crc;
    char data[DATA_SIZE];
} rtp_h;

typedef struct
{
	rtp_h *queue;           // All of the items in the queue
	int size;               // The max size of the queue
	int count;              // The current size of the queue
    enum QueueType type;    // The type of queue
} queue;

typedef struct Variables {
    int is;             // initial sequence
    int oldest;         // oldest unacked packet
    int next;           // next expected (if frame->flags < next => old packet)
    int window_size;
} Variables;

typedef struct {
    struct sockaddr_in host;
    struct sockaddr_in dest;
    struct Variables s_vars; // Sent variables (initial, oldest etc.)
    struct Variables r_vars; // Received variables (initial, oldest etc.)
    int socket;
} TransmissionInfo;

// The struct sent to the timeout queue
struct timeout_arguments {
    TransmissionInfo *arg1;
    queue *arg2;
}timeout_args;

void setVerboseLevel(int level);
void setErrorLevel(float level);
void p(char * format, ...);
int randomSeq();
int checksum(char *data);
int checksumFrame(rtp_h *frame);

int checkChecksum(char *data, int crc);
int checkChecksumFrame(rtp_h *frame, int crc);

void makePacket(rtp_h *frame, int seq, int ack, int flag, char* data);
int getData(TransmissionInfo *ti, rtp_h *frame, int timeout);
int sendData(TransmissionInfo *ti, rtp_h *frame);
int sendLostData(TransmissionInfo *ti, rtp_h *frame);
void initState();
void teardown();

void initQueue(queue* q, int len, enum QueueType type);
int enqueue(TransmissionInfo *transmissionInfo, queue *q, rtp_h frame, enum QueueType type);
void printQueue(queue *q);
void dequeue(queue *q);
void removeFromQueue(queue *q, int index);
void clearQueue(queue *q);
int isInQueue(queue* q, int seq);
int isQueueFull(queue *q);
int isQueueEmpty(queue *q);
void selectiveState();
void *timeout(void *args);
void *selectiveTimeout(void *args);
void pTimestamp();

// void incrementSeq(TransmissionInfo *transmissionInfo);
// int getSeq(TransmissionInfo *transmissionInfo);