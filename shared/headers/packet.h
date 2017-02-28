
#define MAX_PAYLOAD 61

typedef struct {
	uint8_t payload_length,
	uint8_t destination,
	uint8_t sender,
	uint8_t ctl,
	uint8_t data[61]
} rfpacket_t;

void unmarshal_packet (rfpacket *p, uint8_t msg[65]);

void marshal_packet (uint8_t msg[65], rfpacket_t *p);