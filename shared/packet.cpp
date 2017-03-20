#include "packet.h"
#include <string.h>
#include <stdlib.h>
#include <Arduino.h>

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

bool marshal_packet (uint8_t msg[PACKET_SIZE], rfpacket_t *p){
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
    
    return duplicate_packet(p);
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



//struct NODE {
//    rfpacket_t *data;
//    NODE *next;
//};

rfpacket_t outbound[OUTBOUND_SIZE];
int queueSize = 0;
int queueIndex = -1;
//NODE *head;
//NODE *tail;

void enqueue(rfpacket_t *packet) {
    queueIndex = (++queueIndex)%OUTBOUND_SIZE;
    outbound[queueIndex].source = packet->source;
    outbound[queueIndex].destination = packet->destination;
    outbound[queueIndex].uid = packet->uid;
    outbound[queueIndex].packet_type = packet->packet_type;
    outbound[queueIndex].node_type = packet->node_type;
    memcpy(&outbound[queueIndex].data[0],&packet->data[0], MAX_PAYLOAD);
    if(queueSize<OUTBOUND_SIZE)
        queueSize++;
    else
        Serial.println("Queue full");
}
rfpacket_t* dequeue() {
    if(queueSize>0) {
        int outIndex = queueIndex;
        if(queueIndex<1)
            queueIndex = OUTBOUND_SIZE;
        queueIndex--;
        queueSize--;
        return &outbound[outIndex];
    }
    else {
        Serial.println("Returning 0. Maybe check size before deque?");
        return 0;
    }
}

//void enqueue(rfpacket_t *packet) {
//    rfpacket_t *newPkt = (rfpacket_t *) malloc(sizeof (rfpacket_t));
//    
//    newPkt->source = packet->source;
//    newPkt->destination = packet->destination;
//    newPkt->uid = packet->uid;
//    newPkt->packet_type = packet->packet_type;
//    newPkt->node_type = packet->node_type;
//    memcpy(&newPkt->data[0],&packet->data[0], MAX_PAYLOAD);
//    Serial.println("Memcopied");
//    NODE *newNode = (NODE*) malloc(sizeof (NODE));
//    newNode->data = newPkt;
//    if(queueSize==0) {
//        head = newNode;
//        tail = newNode;
//    }
//    else {
//        tail->next = newNode;
//    }
//    queueSize++;
//    Serial.println("Enqued");
//}

//rfpacket_t* dequeue() {
//    if(queueSize>0) {
//        rfpacket_t *data = head->data;
//        head = head->next;
//        queueSize--;
//        free (head);
//        Serial.println("Returning data");
//        return data;
//    }
//    else {
//        Serial.println("Returning 0");
//        return 0;
//    }
//}

int getQueueSize() {
    return queueSize;
}
