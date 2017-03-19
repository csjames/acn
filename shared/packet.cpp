#include "packet.h"
#include <string.h>
#include <stdlib.h>

rfheader_t history[HISTORY_SIZE];
int hIndex = 0;

//void push(rfpacket_t *packet);
//rfpacket_t dequeue();

bool unmarshal_packet (rfpacket_t *p, uint8_t msg[PACKET_SIZE]){
	p->packet_type = msg[0];
    p->node_type = msg[1];
    
    p->uid = (msg[2] << 8) | msg[3];
    
    memcpy(&p->data[0], &msg[PACKET_SIZE-MAX_PAYLOAD], MAX_PAYLOAD);
    
    return duplicate_packet(p);
}

void marshal_packet (uint8_t msg[PACKET_SIZE], rfpacket_t *p){
	msg[0] = p->packet_type;
    msg[1] = p->node_type;
    
    p->uid = rand()%65536;
    msg[2] = (p->uid >> 8);
    msg[3] = p->uid;
    
    if(!duplicate_packet(p)) {
        history[hIndex] = *p;
        hIndex = (++hIndex) % HISTORY_SIZE;
    }
    
	memcpy(&msg[PACKET_SIZE-MAX_PAYLOAD],&p->data[0], MAX_PAYLOAD);
}

bool duplicate_packet(rfheader_t *inc) {
    for(int i = 0; i<HISTORY_SIZE; i++) {
        if(history[i].uid == inc->uid &&
           history[i].packet_type == inc->packet_type &&
           history[i].destination == inc->destination)
            return true;
    }
    return false;
}

void initPacket(unsigned long y) {
    srand(y);
}



struct NODE {
    rfpacket_t *data;
    NODE *next;
};

int queueSize;
NODE *head;
NODE *tail;

void push(rfpacket_t *packet) {
    NODE *newNode = (NODE*) malloc(sizeof (NODE));
    if(queueSize==0) {
        head = newNode;
        tail = newNode;
    }
    else {
        tail->next = newNode;
    }
    queueSize++;
}

rfpacket_t* dequeue() {
    if(queueSize>0) {
        rfpacket_t *data = head->data;
        head = head->next;
        queueSize--;
        free (head);
        return data;
    }
    else
        return 0;
}
