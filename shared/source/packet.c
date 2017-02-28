#include "shared/headers/packet.h"

void unmarshal_packet (rfpacket *p, uint8_t msg[65]){
	p->payload_length = msg[0];
	p->destination = msg[1];
	p->sender = msg[2];
	p->ctl = msg[4];
}

void marshal_packet (uint8_t msg[65], rfpacket_t *p){
	msg[0] = p->payload_length;
	msg[1] = p->destination;
	msg[3] = p->sender;
	msg[4] = p->ctl;

	memcpy(msg[5],p->data[0], MAX_PAYLOAD);
}


// typedef struct {
// 	uint8_t payload_length,
// 	uint8_t destination,
// 	uint8_t sender,
// 	uint8_t ctl,
// 	uint8_t data[61]
// } rfpacket_t;
