/**
  Sets the printmode (0x1B 0x21 n == COMMAND PRINT_MODE mode)
  BARCODE:
    COMMAND_BARCODE COMMAND_BARCODE_PRINT BARCODE_MODE_UPCA barcode 0x00

  MISURE BARCODE (altezza / 256)
     COMMAND_BARCODE COMMAND_BARCODE_HEIGHT height
     COMMAND_BARCODE COMMAND_BARCODE_WIDTH WIDTH

  BOLD
    COMMAND DOUBLEPRINT 0x01 (ON) 0x00 (OFF)

  UNDERLINE
    COMMAND UNDERLINE 0x01 (ON) 0x00 (OFF)

*/

const byte COMMAND = 0x1B;
const byte LF = 0x10;
const byte LINEFEED_RATE = 0x33;
const byte FULLCUT = 0x69; //[0x1B;0x69];
const byte PARTIALCUT = 0x6D; //[0x1B;0x6D];
const byte PRINT_MODE = 0x21;
const byte DOUBLEPRINT = 0x47;
const byte UNDERLINE = 0x2D;
const byte RESET = 0x40;
const byte COMMAND_IMAGE = 0x2A;
const byte COMMAND_FLIPCHARS = 0x7B;
const byte COMMAND_ROTATECHARS = 0x56;
const byte COMMAND_BARCODE = 0x1D;
const byte COMMAND_BARCODE_PRINT = 0x6B;
const byte COMMAND_BARCODE_WIDTH = 0x77;
const byte COMMAND_BARCODE_HEIGHT = 0x68;
const byte COMMAND_BARCODE_TEXTPOSITION = 0x48;
const byte COMMAND_BARCODE_FONT = 0x66;

const byte ON = 0x01;
const byte OFF = 0x00;

//CBMBARCODES
const byte BARCODE_WIDTH_NARROW = 0x02;
const byte BARCODE_WIDTH_MEDIUM = 0x03;
const byte BARCODE_WIDTH_WIDE = 0x04;
const byte BARCODE_TEXT_NONE = 0x00;
const byte BARCODE_TEXT_ABOVE = 0x01;
const byte BARCODE_TEXT_BELOW = 0x02;
const byte BARCODE_TEXT_BOTH = 0x03;
const byte BARCODE_MODE_UPCA = 0x00;
const byte BARCODE_MODE_UPCE = 0x01;
const byte BARCODE_MODE_JAN13AEN = 0x02;
const byte BARCODE_MODE_JAN8EAN = 0x03;
const byte BARCODE_MODE_CODE39 = 0x04;
const byte BARCODE_MODE_ITF = 0x05;
const byte BARCODE_MODE_CODEABAR = 0x06;
const byte BARCODE_MODE_CODE128 = 0x07;


#include <SoftwareSerial.h>

#define rxPin 2
#define txPin 3

SoftwareSerial printer =  SoftwareSerial(rxPin, txPin);

void setup() {

  // serial
  pinMode(rxPin, INPUT);
  pinMode(txPin, OUTPUT);

  printer.begin(9600);
  //Serial.begin(9600);

  printer.write(COMMAND);
  printer.write(FULLCUT);
  
  printer.println("");
    
  //BOLD  
  printer.write(COMMAND);
  printer.write(DOUBLEPRINT);
  printer.write(ON);
  printer.println("The standard Lorem Ipsum passage, used since the 1500s");
  printer.write(COMMAND);
  printer.write(DOUBLEPRINT);
  printer.write(OFF);

  //UNDERLINE
  printer.write(COMMAND);
  printer.write(UNDERLINE);
  printer.write(ON);
  printer.println("http://www.lipsum.com/");
  printer.write(COMMAND);
  printer.write(UNDERLINE);
  printer.write(OFF);
  
  //NORMAL
  printer.println("Lorem ipsum dolor sit amet, consectetur adipisicing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum");
  printer.println("");
  printer.println("");
   
  //BARCODE 
  printer.write(COMMAND_BARCODE);
  printer.write(COMMAND_BARCODE_PRINT);
  printer.write(BARCODE_MODE_UPCA);
  printer.println(1234567890);
  printer.write(OFF);
  printer.println("");
  printer.println("");
  
  //IMAGE
  printer.write(COMMAND);
  printer.write(COMMAND_IMAGE);
  printer.println("");
  printer.println("");


  printer.println("");  
  printer.println("");
  printer.println("");
}

void loop() {
//  stateCheck();
}
