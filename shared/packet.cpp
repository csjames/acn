#include "packet.h"
#include <string.h>

void unmarshal_packet (rfpacket_t *p, uint8_t msg[61]){
	p->packet_type = msg[0];
}

void marshal_packet (uint8_t msg[61], rfpacket_t *p){
	msg[0] = p->packet_type;

	memcpy(&msg[PACKET_SIZE-MAX_PAYLOAD],&p->data[0], MAX_PAYLOAD);
}