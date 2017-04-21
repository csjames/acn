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
    p->node_type = msg[1];
    p->origin = msg[4];
    
    uint16_t tUid =(((uint16_t)msg[2]) << 8) | ((uint16_t)msg[3]);
    Serial.print("UID: ");
    Serial.println(tUid);
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
    msg[1] = p->node_type;
    msg[4] = p->origin;
    Serial.println("ORIGIN");
    Serial.println(p->origin);
    
    if(newUid) {
        p->uid = packetCount;
        packetCount++;
    }
    
    msg[2] = (p->uid >> 8);
    msg[3] = p->uid;
    
	memcpy(&msg[PACKET_SIZE-MAX_PAYLOAD],&p->data[0], MAX_PAYLOAD);
    
    return duplicate_packet(p);
}

//bool re_marshal_packet (uint8_t msg[PACKET_SIZE], rfpacket_t *p){
//    msg[0] = p->packet_type;
//    msg[1] = p->node_type;
//    msg[4] = p->origin;
//    
//    msg[2] = (p->uid >> 8);
//    msg[3] = p->uid;
//    Serial.println("marshal uid: ");
//    Serial.println(p->uid);
//    memcpy(&msg[PACKET_SIZE-MAX_PAYLOAD],&p->data[0], MAX_PAYLOAD);
//    
//    return duplicate_packet(p);
//}

bool duplicate_packet(rfheader_t *inc) {
    for(int i = 0; i<HISTORY_SIZE; i++) {
        if(history[i].uid == inc->uid &&
           history[i].packet_type == inc->packet_type &&
           history[i].destination == inc->destination &&
           history[i].origin == inc->origin)
            return true;
    }
    Serial.println("No duplicate found for: ");
    Serial.println(inc->uid);
    Serial.println(inc->packet_type);
    Serial.println(inc->destination);
    Serial.println(inc->origin);
    return false;
}

void initPacket(unsigned long y) {
    //srand(y);
}



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
    outbound[queueIndex].origin = packet->origin;
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
            Serial.println("Packet already exists.");
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
            repeat[i].node_type = packet->node_type;
            repeat[i].origin = packet->origin;
            memcpy(&repeat[i].data[0],&packet->data[0], MAX_PAYLOAD);
        }
        if(repeatSize<REPEAT_SIZE) {
            repeatSize++;
            return true;
        }
        else {
            Serial.println("Repeat Queue full");
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
        Serial.println("Returning 0. Maybe check size before deque?");
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
