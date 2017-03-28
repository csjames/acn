// flasher :)
#include <EEPROM.h>

#define keyStart 0
#define keyLength 16
#define addressIndex keyStart+keyLength

#define ADDRESS 1
#define KEY         "wedidntwantaes:("

#define p(x) Serial.println(x)

char keyRB[16] = {0};
char addressRB = 0;
void setup() {

  Serial.begin(57600);
  
  p("Writing out...");
  p("");
  p("Key: "); p(KEY);
  p("");
  p("Address: "); p(ADDRESS);
  p("");

  // put your setup code here, to run once:
  int i;

  for (i = 0; i < keyLength; i ++) {
     EEPROM.update(keyStart+i, KEY[keyStart+i]);
  }

  p("Wrote 16 byte key to EEPROM"); 
  
  EEPROM.update(addressIndex, ADDRESS);

  p("Wrote 1 byte address to EEPROM"); 

  p("");

  p("Reading back..");
  p("");

  p("Key: ");
  
  for (i = 0; i < keyLength; i ++) {
    char x = EEPROM.read(keyStart+i);
    Serial.print(x);
    keyRB[i] = x;
    if (keyRB[i] != KEY[i]) {
      p("");
      p("ERROR Key does not match");
      return;
    }
  }
  p("");

  char k = EEPROM.read(addressIndex);

  if (k != ADDRESS) {
    p("");
    p("ERROR Address does not match");
    return;
  }
  p("");
  p("Address: ");
  p(atoi(k));

  p("");
  
  p("Key matches");
  p("Address matches");


  p();
  p();

  p("SUCCESS"); 
  while (1) ;
}

void loop() {
  // put your main code here, to run repeatedly:
  
}
