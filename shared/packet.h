#include <stdint.h>

#define MAX_PAYLOAD 		60
#define PACKET_SIZE			61

#define ACK_PKT 			'A'
#define BCAST_PKT 			'B'
#define UNICAST_PKT 		'U'


typedef struct {
	uint8_t packet_type;
	uint8_t data[MAX_PAYLOAD];
} rfpacket_t;


void unmarshal_packet (rfpacket_t *p, uint8_t msg[61]);

void marshal_packet (uint8_t msg[61], rfpacket_t *p);