#include <stdint.h>

#define MAX_PAYLOAD 		57
#define PACKET_SIZE			61
#define HISTORY_SIZE        16

#define ACK_PKT 			'A'
#define BCAST_PKT 			'B'
#define UNICAST_PKT 		'U'

#define LP_NODE             'L'
#define RPT_NODE            'R'
#define GTW_NODE            'G'

typedef struct {
    uint8_t source;
    uint8_t destination;
    uint16_t uid;
    char packet_type;
    char node_type;
} rfheader_t;

typedef struct rfpacket_t : public rfheader_t {
    //rfheader_t header;
	uint8_t data[MAX_PAYLOAD];
};

bool unmarshal_packet (rfpacket_t *p, uint8_t msg[61]);

void marshal_packet (uint8_t msg[61], rfpacket_t *p);

void initPacket(unsigned long y);

bool duplicate_packet(rfheader_t *inc);
