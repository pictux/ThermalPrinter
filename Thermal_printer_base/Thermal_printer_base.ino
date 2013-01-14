#include <SoftwareSerial.h>

#define rxPin 2
#define txPin 3

SoftwareSerial printer =  SoftwareSerial(rxPin, txPin);

const byte command = 0x1B;
const byte fullcut = 0x69;
const byte partialcut = 0x6D;

void setup() {

  // serial
  pinMode(rxPin, INPUT);
  pinMode(txPin, OUTPUT);
  printer.begin(9600);
  //Serial.begin(9600);
  
  printer.println("Ce la siamo proprio meritata, una Heineken!!!");
  printer.println("");
  printer.println("");
  printer.println("");
  printer.write(command);
  printer.write(partialcut);
}

void loop() {

}
