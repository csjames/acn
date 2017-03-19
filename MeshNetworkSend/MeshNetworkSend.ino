#include <RFM69.h>         
#include <RFM69_ATC.h>        
#include <SPI.h>     
#include "packet.h"

#define NODEID        1 //unique for each node on same network
#define NETWORKID     100  //the same on all nodes that talk to each other

#define FREQUENCY     RF69_868MHZ

#define SERIAL_BAUD   57600

#define LED           9 // Moteinos have LEDs on D9

RFM69 radio;

//#define FLIPPED

#ifdef FLIPPED
  #define RED A0
  #define GREEN A2
  #define BLUE A3
  #define POWER A1
#else
  #define RED A3
  #define GREEN A1
  #define BLUE A0
  #define POWER A2
#endif

rfpacket_t *inPkt;
rfpacket_t *outPkt;

rfpacket_t out;
rfpacket_t in;
  
void setup() {
  pinMode(RED, OUTPUT);
  pinMode(GREEN, OUTPUT);
  pinMode(BLUE, OUTPUT);
  pinMode(POWER, OUTPUT);
  digitalWrite(POWER, HIGH);

  analogWrite(RED, 0);
  analogWrite(GREEN, 255);
  analogWrite(BLUE, 255);

  initPacket(millis());
  outPkt = &out;
  inPkt = &in;

  outPkt->source = NODEID;
  outPkt->destination = 1;
  
  Serial.begin(SERIAL_BAUD);
  delay(10);
  radio.initialize(FREQUENCY,NODEID,NETWORKID);
  radio.promiscuous(true);
  char buff[50];
  sprintf(buff, "\nListening at %d Mhz...", FREQUENCY==RF69_433MHZ ? 433 : FREQUENCY==RF69_868MHZ ? 868 : 915);
  Serial.println(buff);
}

byte ackCount=0;
uint32_t packetCount = 0;

uint8_t messageToSend = 0;
uint8_t currentIndex = 0;

void loop() {
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil(',');
    if (input.length() > 0){
      Serial.println(input);
      if(currentIndex>0)
        outPkt->data[currentIndex-1] = input.toInt();
       else {
        Serial.println(input.toInt());
        outPkt->destination = input.toInt();
       }
      currentIndex++;
    }
  }

  if(currentIndex==4) {
    currentIndex = 0;
    outPkt->packet_type = UNICAST_PKT;
    outPkt->source = NODEID;
    
    Serial.print("Destination: ");
    char buf[12];
    itoa(outPkt->destination, buf, 10);
    Serial.println(buf);
  
    Serial.println("Sending Packet: ");
    printPacket(outPkt);
    
    messageToSend = 1;
  }

  if (radio.receiveDone() && messageToSend!=1)
  {
    Serial.print(++packetCount);
    Serial.print('[');Serial.print(radio.SENDERID, DEC);Serial.print("] ");
    Serial.print("to [");Serial.print(radio.TARGETID, DEC);Serial.print("] ");


    uint8_t inBuffer[PACKET_SIZE] = {};
    
    for (byte i = 0; i < radio.DATALEN; i++){
      Serial.print((char)radio.DATA[i]);
      inBuffer[i] = (uint8_t) radio.DATA[i];
    }

    bool packetSeen = unmarshal_packet(inPkt, inBuffer);
    inPkt->source = radio.SENDERID;
    inPkt->destination = radio.TARGETID;

    printPacket(inPkt);

    if (inPkt->packet_type == BCAST_PKT) {
      changeColour(inPkt);
      Blink(LED, 3);
    }

    // is packet for us, or someone else
    if(inPkt->packet_type == UNICAST_PKT && inPkt->destination == NODEID) {
      //change our own colour
      changeColour(inPkt);
      outPkt->packet_type = ACK_PKT;
      outPkt->destination = inPkt->source;
      outPkt->source = NODEID;
      memcpy(&outPkt->data[0], &inPkt->data[0], MAX_PAYLOAD);
      Serial.println("Should send ack");
      printPacket(outPkt);
      messageToSend = 1;
    }
    else if(inPkt->packet_type == UNICAST_PKT) {
      //repeat
      if(!packetSeen) {
        outPkt->packet_type = inPkt->packet_type;
        outPkt->destination = inPkt->destination;
        outPkt->source = inPkt->source;
        outPkt->uid = inPkt->uid;
        memcpy(&outPkt->data[0], &inPkt->data[0], MAX_PAYLOAD);
        messageToSend = 1;
      }
    }

    // if its an ack // change color to outpkt
    if(inPkt->packet_type == ACK_PKT && inPkt->destination == NODEID) {
      changeColour(outPkt);
    }
    
    Serial.print("   [RX_RSSI:");Serial.print(radio.RSSI);Serial.print("]");
    
    Blink(LED,3);
  }

  if (messageToSend == 1)
  {
    uint8_t msg[PACKET_SIZE];
    marshal_packet(msg, outPkt);
    
    radio.send(outPkt->destination, (const void*)(msg), sizeof(msg));
    Serial.println();
    Blink(LED,3);
    messageToSend = 0;
  }
}

void changeColour(rfpacket_t *pck) {
    Blink(LED,3);

    analogWrite(RED, 255-pck->data[0]);
    analogWrite(GREEN, 255-pck->data[1]);
    analogWrite(BLUE, 255-pck->data[2]);
}
void Blink(byte PIN, int DELAY_MS)
{
  pinMode(PIN, OUTPUT);
  digitalWrite(PIN,HIGH);
  delay(DELAY_MS);
  digitalWrite(PIN,LOW);
}

void printPacket(rfpacket_t *pkt) {
  char buf[12];
  Serial.print("From: ");
  Serial.println(pkt->source);
  Serial.print("To: ");
  itoa(pkt->destination, buf, 10);
  Serial.println(buf);
  Serial.print("Packet Type: ");
  Serial.println(pkt->packet_type);
    Serial.println("Packet Contents: ");
    for(uint8_t i = 0; i < MAX_PAYLOAD; i++) {
      Serial.print(pkt->data[i]);
      Serial.print(" ");
    }
    Serial.println();
}

