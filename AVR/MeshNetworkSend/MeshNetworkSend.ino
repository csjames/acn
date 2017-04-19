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

  radio.initialize(FREQUENCY, nodeid, NETWORKID);
  radio.promiscuous(true);
  char buff[50];
  sprintf(buff, "\nListening at %d Mhz...", FREQUENCY == RF69_433MHZ ? 433 : FREQUENCY == RF69_868MHZ ? 868 : 915);
  Serial.println(buff);
}

byte ackCount = 0;
uint32_t pktCount = 0;

uint8_t currentIndex = 0;

uint32_t lastTimer;
void loop() {
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil(',');
    if (input.length() > 0) {
      Serial.println(input);
      if (currentIndex > 0)
        outPkt->data[currentIndex - 1] = input.toInt();
      else {
        Serial.println(input.toInt());
        outPkt->destination = input.toInt();
      }
      currentIndex++;
    }
  }

  if (currentIndex == 4) {
    currentIndex = 0;
    outPkt->packet_type = UNICAST_PKT;
    outPkt->source = nodeid;
    outPkt->origin = nodeid;
    outPkt->tries = 0;
    outPkt->lastTry = 0;

    Serial.print("Destination: ");
    char buf[12];
    itoa(outPkt->destination, buf, 10);
    Serial.println(buf);

    enqueue(outPkt);
    Serial.println("Enque :)");
  }

  long tm = millis();
  if (nodeid == 4 && tm - lastTimer >= 3000 ) {
    lastTimer = tm;
    outPkt->destination = 1;
    outPkt->packet_type = UNICAST_PKT;
    outPkt->source = nodeid;
    outPkt->origin = nodeid;
    outPkt->data[0] = random(255);
    outPkt->data[1] = random(255);
    outPkt->data[2] = random(255);
    Serial.println("Enq pkt");
    enqueue(outPkt);
    uint8_t msg[PACKET_SIZE];
  }

  if (radio.receiveDone()) //Bug may be here case: pause while sending
  {
    if (radio.TARGETID == 255) {
      Serial.println("ACK Received through broadcast");
   
      uint16_t uid;
      uint8_t to, from;
      if (radio.DATALEN < 4)
        Serial.println("Ack received is too small to contain UID");
      else {
        to = radio.DATA[0];
        from = radio.DATA[1];
        Serial.print("Ack to : ");
        Serial.println(to);
        Serial.print("Ack from : ");
        Serial.println(from); 
        uid = ((uint16_t) radio.DATA[2]) << 8 | ((uint16_t) radio.DATA[3]);
        Serial.print("ACK UID: ");
        Serial.println(uid);
        handleACK(uid, from, to);
      }
      return;
    }

    pktCount++;

    uint8_t inBuffer[PACKET_SIZE] = {};

    for (byte i = 0; i < radio.DATALEN; i++) {
      inBuffer[i] = (uint8_t) radio.DATA[i];
    }

    bool notForUs;
    uint8_t targetID = radio.TARGETID;
    uint8_t senderID = radio.SENDERID;
    if (radio.ACK_REQUESTED && targetID == nodeid) {
      char uid[2];
      uid[0] = inBuffer[2];
      uid[1] = inBuffer[3];
      radio.sendACK(uid, 2);
      notForUs = false;
    }
    else if (targetID != nodeid) {
      notForUs = true;
    }

    bool packetSeen = unmarshal_packet(inPkt, inBuffer);
    inPkt->source = senderID;
    inPkt->destination = targetID;

    Serial.println("Packet received.");
    Serial.print("From: ");
    Serial.print(inPkt->source);
    Serial.print(" To: ");
    Serial.println(inPkt->destination);
    Serial.print("["); Serial.print(inPkt->uid); Serial.println("]");
    Serial.print("Seen Packet: ");
    Serial.println(packetSeen);
    Serial.print("Not for us: ");
    Serial.println(notForUs);
    //printPacket(inPkt);

    // is packet for us
    if (inPkt->packet_type == UNICAST_PKT && inPkt->destination == nodeid) {
      //change our own colour
      if (!packetSeen) {
        changeColour(inPkt);
      }
    }
    else if (inPkt->packet_type == UNICAST_PKT) {
      //repeat
      if(!packetSeen)
        enqueueRepeat(inPkt, tm + random(100)); // offset to avoid collision.
      
    }

    // if its an ack // change color to outpkt
    if (inPkt->packet_type == ACK_PKT && inPkt->destination == nodeid) {
      Serial.println("Ack received :)");
      //changeColour(outPkt);
    }

    //Serial.print("   [RX_RSSI:");Serial.print(radio.RSSI);Serial.print("]");
  }

  uint8_t msg[PACKET_SIZE];
  
  if (getQueueSize() > 0)
  {
    rfpacket_t *sendPkt = dequeue();
    
    if ((millis() - sendPkt->lastTry) > RETRY_DELAY && marshal_packet(msg, sendPkt, true) == false) {
      Serial.print("Sending Deque Packet: ");
      //printPacket(sendPkt);

      bool success = radio.sendWithRetry(sendPkt->destination, (const void*)(msg), sizeof(msg));
      if (!success) {
        if (sendPkt->tries < RETRIES) {
          sendPkt->lastTry = millis();
          enqueue(sendPkt);
          Serial.println("Failed sending packet");
          sendPkt->tries++;
        }
      } else {

        Serial.println("Unicast Success");
        broadcastAck(sendPkt);
      }
    }
  }

  //check repeat queue
  if(getEligibleRepeatSize(tm)) {
    rfpacket_t *repeatPacket = dequeRepeat(tm);

    if (marshal_packet(msg, repeatPacket, false) == false ) {
      bool success = radio.sendWithRetry(repeatPacket->destination, (const void*)(msg), sizeof(msg));
      if (!success) {
        Serial.println("Repeat Fail - node is probs straight unreachable");
      } else {
        Serial.println("Repeat Success");

        broadcastAck(repeatPacket);
        
      }
    }
  }
}

void broadcastAck(rfpacket_t *repeatPacket) {
  byte bcastAck[4];
    
  bcastAck[0] = repeatPacket->destination;
  bcastAck[1] = repeatPacket->origin;
  bcastAck[2] = (repeatPacket->uid >> 8);
  bcastAck[3] = repeatPacket->uid;

  radio.send(255, bcastAck, 4);
  delay(25);
  radio.send(255, bcastAck, 4);
}

void changeColour(rfpacket_t *pck) {

  analogWrite(RED, 255 - pck->data[0]);
  analogWrite(GREEN, 255 - pck->data[1]);
  analogWrite(BLUE, 255 - pck->data[2]);
}
void Blink(byte PIN, int DELAY_MS)
{
  pinMode(PIN, OUTPUT);
  digitalWrite(PIN, HIGH);
  delay(DELAY_MS);
  digitalWrite(PIN, LOW);
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
  for (uint8_t i = 0; i < MAX_PAYLOAD; i++) {
    Serial.print(pkt->data[i]);
    Serial.print(" ");
  }
  Serial.println();
}

void readConfig() {
  nodeid = EEPROM.read(addressIndex);

  Serial.print("Read nodeID: ");
  char buf[12];
  Serial.println(itoa(nodeid, buf, 10));

  Serial.print("Read key: ");
  for (int i = 0; i < keyLength; i ++) {
    char x = EEPROM.read(keyStart + i);
    Serial.print(x);
    key[i] = x;
  }
  Serial.println();
}
