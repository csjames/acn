#include <stdint.h>

#define MAX_PAYLOAD 		 27
#define PACKET_SIZE			 32
#define HISTORY_SIZE         10

#define ACK_PKT 			'A'
#define BCAST_PKT 			'B'
#define UNICAST_PKT 		'U'

#define LP_NODE             'L'
#define RPT_NODE            'R'
#define GTW_NODE            'G'

#define OUTBOUND_SIZE        5

#define RETRY_DELAY          250
#define REPEAT_DELAY         1500
#define RETRIES              3

#define REPEAT_SIZE          5

#define ACK_ADDRESS          255
#define BROADCAST_ADDRESS    254

typedef struct {
    uint8_t origin;
    uint8_t source;
    uint8_t destination;
    uint8_t tries;
    uint32_t lastTry;
    uint16_t uid;
    char packet_type;
    char node_type;
} rfheader_t;

typedef struct rfpacket_t : public rfheader_t {
	uint8_t data[MAX_PAYLOAD];
};

bool unmarshal_packet (rfpacket_t *p, uint8_t msg[PACKET_SIZE]);

bool marshal_packet (uint8_t msg[PACKET_SIZE], rfpacket_t *p, bool newUid);

void initPacket(unsigned long y);

bool duplicate_packet(rfheader_t *inc);

void enqueue(rfpacket_t *packet);

rfpacket_t* dequeue();

int getQueueSize();

bool enqueueRepeat(rfpacket_t *packet, uint32_t receiveTime);

rfpacket_t* dequeRepeat(uint32_t curTime);

int getEligibleRepeatSize(uint32_t curTime);

bool handleACK(uint16_t uid, uint8_t from, uint8_t to);

uint8_t* broadcastAckPacket(rfpacket_t *repeatPacket);