#define PORT 5555
#define DST "127.0.0.1"

#define FRAME_SIZE 544

#define INIT 0
#define WAIT_SYN 1
#define WAIT_SYNACK 2
#define WAIT_ACK 3

#define ESTABLISHED 1337

#define SYN 420
#define ACK 421
#define FIN 422

typedef struct {
    int flags;
    int id;
    int seq;
    int windowsize;
    int crc;
    char *data;
} rtp_h;

typedef struct
{
	rtp_h *queue;
	int size;
	int count;
} queue;

typedef struct {
    struct sockaddr_in host;
    struct sockaddr_in dest;
    int socket;
} TransmissionInfo;

int getData(TransmissionInfo *ti, rtp_h *frame);
int sendData(TransmissionInfo *ti, rtp_h *frame);
void initState();