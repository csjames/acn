#include <RFM69.h>         
#include <RFM69_ATC.h>        
#include <SPI.h>   
#include <MemoryFree.h>  
#include "packet.h"
#include <EEPROM.h>

//#define NODEID        1 //unique for each node on same network
#define NETWORKID     100  //the same on all nodes that talk to each other

#define FREQUENCY     RF69_868MHZ

#define SERIAL_BAUD   57600

#define keyStart 0
#define keyLength 16
#define addressIndex keyStart+keyLength

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

char nodeid;
char key[16];

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

  initPacket(0);
  outPkt = &out;
  inPkt = &in;

  outPkt->source = nodeid;
  outPkt->destination = 1;
  
  Serial.begin(SERIAL_BAUD);
  delay(10);
  
  readConfig();
  
  radio.initialize(FREQUENCY,nodeid,NETWORKID);
  radio.promiscuous(true);
  char buff[50];
  sprintf(buff, "\nListening at %d Mhz...", FREQUENCY==RF69_433MHZ ? 433 : FREQUENCY==RF69_868MHZ ? 868 : 915);
  Serial.println(buff);
}

byte ackCount=0;
uint32_t pktCount = 0;

uint8_t currentIndex = 0;

uint32_t lastTimer;
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
    outPkt->source = nodeid;
    outPkt->origin = nodeid;
    
    Serial.print("Destination: ");
    char buf[12];
    itoa(outPkt->destination, buf, 10);
    Serial.println(buf);

    enqueue(outPkt);
    Serial.println("Enque :)");
  }

  long tm = millis();
  if(tm - lastTimer >= 3000 ) {
    lastTimer = tm;
    outPkt->destination = 1;
    outPkt->packet_type = UNICAST_PKT;
    outPkt->source = nodeid;
    outPkt->origin = nodeid;
    outPkt->data[0] = tm%3000==0? 0:255;
    outPkt->data[1] = tm%4000==0? 0:255;
    outPkt->data[2] = tm%5000==0? 0:255;
    Serial.println("Enq pkt");
//    enqueue(outPkt);
    uint8_t msg[PACKET_SIZE];
    if(marshal_packet(msg, outPkt)) {
      Serial.print("Sending Packet: ");
      Serial.println(outPkt->packet_type);
      //printPacket(sendPkt);
      radio.send(outPkt->destination, (const void*)(msg), sizeof(msg));
      //Serial.println(freeMemory());
      //Blink(LED,3);
    }
  }
  
  if (radio.receiveDone()) //Bug may be here case: pause while sending
  {
    pktCount++;

    uint8_t inBuffer[PACKET_SIZE] = {};
    
    for (byte i = 0; i < radio.DATALEN; i++){
      inBuffer[i] = (uint8_t) radio.DATA[i];
    }

    bool packetSeen = unmarshal_packet(inPkt, inBuffer);
    inPkt->source = radio.SENDERID;
    inPkt->destination = radio.TARGETID;

    Serial.println("Packet received.");
    Serial.print("From: ");
    Serial.print(radio.SENDERID, DEC);
    Serial.print(" To: ");
    Serial.println(radio.TARGETID, DEC);
    Serial.print("[");Serial.print(inPkt->uid);Serial.println("]");
    Serial.print("Seen Packet: ");
    Serial.println(packetSeen);
    //printPacket(inPkt);

    if (inPkt->packet_type == BCAST_PKT) {
      changeColour(inPkt);
      if(!packetSeen) {
        outPkt->packet_type = ACK_PKT;
        outPkt->destination = inPkt->source;
        outPkt->source = nodeid;
        outPkt->origin = inPkt->origin;
        memcpy(&outPkt->data[0], &inPkt->data[0], MAX_PAYLOAD);
        Serial.println("Should repeat broadcast");
        enqueue(outPkt);
      }
    }

    // is packet for us
    if(inPkt->packet_type == UNICAST_PKT && inPkt->destination == nodeid) {
      //change our own colour
      if(!packetSeen) {
        changeColour(inPkt);
        outPkt->packet_type = ACK_PKT;
        outPkt->destination = inPkt->source;
        outPkt->source = nodeid;
        outPkt->origin = nodeid;
        memcpy(&outPkt->data[0], &inPkt->data[0], MAX_PAYLOAD);
        Serial.println("Should send ack");
        enqueue(outPkt);
      }
    }
    else if(inPkt->packet_type == UNICAST_PKT) {
      //repeat
      if(!packetSeen) {
        outPkt->packet_type = inPkt->packet_type;
        outPkt->destination = inPkt->destination;
        outPkt->source = inPkt->source;
        outPkt->origin = inPkt->origin;
        outPkt->uid = inPkt->uid;
        memcpy(&outPkt->data[0], &inPkt->data[0], MAX_PAYLOAD);
        enqueue(outPkt);
      }
    }

    // if its an ack // change color to outpkt
    if(inPkt->packet_type == ACK_PKT && inPkt->destination == nodeid) {
      Serial.println("Ack received :)");
      //changeColour(outPkt);
    }
    
    //Serial.print("   [RX_RSSI:");Serial.print(radio.RSSI);Serial.print("]");
  }
  
  if (getQueueSize() > 0)
  {
    rfpacket_t *sendPkt = dequeue();
    uint8_t msg[PACKET_SIZE];
    if(marshal_packet(msg, sendPkt)) {
      Serial.print("Sending Deque Packet: ");
      Serial.println(sendPkt->packet_type);
      //printPacket(sendPkt);
      radio.send(sendPkt->destination, (const void*)(msg), sizeof(msg));
      //Serial.println(freeMemory());
      //Blink(LED,3);
    }
  }
}

void changeColour(rfpacket_t *pck) {

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

void readConfig() {
  nodeid = EEPROM.read(addressIndex);

  Serial.print("Read nodeID: ");
  char buf[12];
  Serial.println(itoa(nodeid,buf,10));

  Serial.print("Read key: ");
  for (int i = 0; i < keyLength; i ++) {
    char x = EEPROM.read(keyStart+i);
    Serial.print(x);
    key[i] = x;
  }
  Serial.println();

  
  
}

