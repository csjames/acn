#include "packet.h"
#include <string.h>
#include <stdlib.h>
#include <Arduino.h>

rfheader_t history[HISTORY_SIZE];

int hIndex = 0;
int ackIndex = 0;
uint16_t packetCount = 0;

//void push(rfpacket_t *packet);
//rfpacket_t dequeue();

bool unmarshal_packet (rfpacket_t *p, uint8_t msg[PACKET_SIZE]){
	p->packet_type = msg[0];
    p->origin = msg[4];
    
    uint16_t tUid =(((uint16_t)msg[2]) << 8) | ((uint16_t)msg[3]);
    p->uid = tUid;
    
    bool duplicate = duplicate_packet(p);
    if(!duplicate) {
        history[hIndex] = *p;
        hIndex = (++hIndex) % HISTORY_SIZE;
    }
    
    memcpy(&p->data[0], &msg[PACKET_SIZE-MAX_PAYLOAD], MAX_PAYLOAD);
    
    return duplicate;
}

bool marshal_packet (uint8_t msg[PACKET_SIZE], rfpacket_t *p, bool newUid){
	msg[0] = p->packet_type;
    msg[4] = p->origin;
    
    if(newUid) {
        p->uid = packetCount;
        packetCount++;
    }
    
    msg[2] = (p->uid >> 8);
    msg[3] = p->uid;
    
	memcpy(&msg[PACKET_SIZE-MAX_PAYLOAD],&p->data[0], MAX_PAYLOAD);
    
    return duplicate_packet(p);
}

bool duplicate_packet(rfheader_t *inc) {
    for(int i = 0; i<HISTORY_SIZE; i++) {
        if(history[i].uid == inc->uid &&
           history[i].packet_type == inc->packet_type &&
           history[i].destination == inc->destination &&
           history[i].origin == inc->origin)
            return true;
    }
    return false;
}

void initPacket(unsigned long y) {
}

rfpacket_t outbound[OUTBOUND_SIZE];
uint8_t queueSize = 0;
int16_t queueIndex = -1;
//NODE *head;
//NODE *tail;

void enqueue(rfpacket_t *packet) {
    queueIndex = (++queueIndex)%OUTBOUND_SIZE;
    outbound[queueIndex].source = packet->source;
    outbound[queueIndex].destination = packet->destination;
    outbound[queueIndex].uid = packet->uid;
    outbound[queueIndex].packet_type = packet->packet_type;
    outbound[queueIndex].origin = packet->origin;
    memcpy(&outbound[queueIndex].data[0],&packet->data[0], MAX_PAYLOAD);
    if(queueSize<OUTBOUND_SIZE)
        queueSize++;
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
        return 0;
    }
}
int getQueueSize() {
    return queueSize;
}

rfpacket_t repeat[REPEAT_SIZE];
uint32_t repeatTime[REPEAT_SIZE] = {0};
int repeatSize = 0;

bool enqueueRepeat(rfpacket_t *packet, uint32_t receiveTime) {
    //check if packet already exists
    for(int i=0; i<REPEAT_SIZE; i++) {
        if(repeat[i].uid == packet->uid &&
           repeat[i].destination == packet->destination &&
           repeat[i].origin == packet->origin) {
            return false;
        }
    }
    
    for(int i=0; i<REPEAT_SIZE; i++) {
        if(repeatTime[i]==0){
            //slot is empty
            repeatTime[i] = receiveTime;
            repeat[i].source = packet->source;
            repeat[i].destination = packet->destination;
            repeat[i].uid = packet->uid;
            repeat[i].packet_type = packet->packet_type;
            repeat[i].origin = packet->origin;
            memcpy(&repeat[i].data[0],&packet->data[0], MAX_PAYLOAD);
        }
        if(repeatSize<REPEAT_SIZE) {
            repeatSize++;
            return true;
        }
        else {
            return false;
        }
    }
}

rfpacket_t* dequeRepeat(uint32_t curTime) {
    if(repeatSize>0) {
        int oldestTimeIndex = 0;
        for(int i = 1; i<REPEAT_SIZE; i++) {
            if(repeatTime[i]<repeatTime[0] && repeatTime[i]!=0) {
                oldestTimeIndex = i;
            }
        }
        if(repeatTime[oldestTimeIndex] + REPEAT_DELAY < curTime) {
            repeatTime[oldestTimeIndex] = 0;
            repeatSize--;
            return &repeat[oldestTimeIndex];
        }
    }
    else {
        return 0;
    }
}

int getEligibleRepeatSize(uint32_t curTime) {
    int eligibleSize = 0;
    if(repeatSize>0)
        for(int i = 0; i < REPEAT_SIZE; i++) {
            if(repeatTime[i]!=0 && repeatTime[i] + REPEAT_DELAY < curTime) {
                eligibleSize++;
            }
        }
    return eligibleSize;
}

bool handleACK(uint16_t uid, uint8_t from, uint8_t destination) {
    for(int i = 0; i < REPEAT_SIZE; i++) {
        if(repeat[i].uid == uid &&
           repeat[i].destination == from &&
           repeat[i].origin == destination) {
            repeatTime[i] = 0;
            return true;
        }
    }
    return false;
}

uint8_t* broadcastAckPacket(rfpacket_t *repeatPacket) {
  uint8_t bcastAck[4];
    
  bcastAck[0] = repeatPacket->destination;
  bcastAck[1] = repeatPacket->origin;
  bcastAck[2] = (repeatPacket->uid >> 8);
  bcastAck[3] = repeatPacket->uid;

  return bcastAck;
}
