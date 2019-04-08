// Arduino to Atari 1020 printer
// October 11, 2014
// @paulrickards
// @Aaron Rouhi (
// Code currently does not work

// References:
// http://www.atarimax.com/jindroush.atari.org/asio.html
// http://www.atariarchives.org/dere/chapt08.php
// http://www.atariarchives.org/cfn/12/02/0022.php
// http://forum.arduino.cc/index.php/topic,19177.0.html
// http://web.archive.org/web/20090405180711/
// http://www.stephens-home.com/sio2usb/
// http://atariage.com/forums/topic/182392-sio2usb-with-a-ftdi-basic-breakout-board/page-2
// http://www.atarimax.com/sio2pc/documentation/chapter1.html
// http://atariage.com/forums/topic/170228-accessing-multiple-printers/

// NOTE: Please note that pinout of SIO is from computer perspective so
// DATA IN is data sent to computer. This means Data In should be connected
// to RX on Arduino

/*************************** PIN CONNETION GUIDE ****************************/
// Atari SIO Data In pin 3    = Arduino 0 (RX)
// Atari SIO Ground pin 4     = Arduino ground (GND)
// Atari SIO Data Out pin 5   = Arduino 1 (TX)
// Atari SIO Command pin 7    = Arduino 2
// Atari SIO +5v/READY pin 10 = Arduino 3
/****************************************************************************/

// From Atari 1020 Field Service Manual
// Atari SIO Pin 7 (COMMAND) Goes to zero when a command frame is being sent
// Atari SIO Pin 10 (+5v/READY) Indicates that the computer is turned on and ready.
//                              Restricted use as a +5 volt supply

// Atari printer device IDs
//850 -- P2: (0x41)
//1025 - P3: (0x42)
//1020 - P4: (0x43)
//1027 - P5: (0x44)
//1029 - P6: (0x45)

const byte ATARI_CMD_WRITE      = 0x57;
const byte ATARI_CMD_GET_STATUS = 0x53;
const byte ATARI_CMD_ACK        = 0x41;
const byte ATARI_CMD_NAK        = 0x4E;
const byte ATARI_CMD_ERROR      = 0x45;
const byte ATARI_CMD_COMPLETE   = 0x43;
const byte ATARI_LF             = 0x9B;

const byte P1 = 0x40;
const byte P2 = 0x41;
const byte P3 = 0x42;
const byte P4 = 0x43;
const byte P5 = 0x44;
const byte P6 = 0x45;

const int ATARI_CMD_PIN   = 2;
const int ATARI_READY_PIN = 3;
const int LED             = 13;

byte atariDevice = P4; // Set the P: address here

// formatting char sequences 
const char formatEsc[] = "\x1B\x1B";
const byte format80cps = 0x13;
const byte format40cps = 0x0E;
const byte format20cps = 0x10;
const String formatGraphic = "\x1B\x1B\x07";

// setup procedure
void setup() {
  initPrinter();
}

void loop() {
  // test data >>>

  //atariData("\x1B\x1B\x07 S10");
  //atariData("A");
  //atariData("TEXT 50");
  
  //atariPrint("\x1B\x1B\x07 Q1*S50"); 

  // testing graphic 
  atariPrint(formatGraphic);
  atariPrint("H"); 
  atariPrint("M240,-200"); 
  atariPrint("I"); 
  
  float d[11][2];
  float s = 0;
   
  for (int i=0; i <= 10; i++){
    s = i * 3.14159265/5.5;
    d[i][0]=sin(s);
    d[i][1]=cos(s);
  } 
    atariPrint(sprintf("D%f,%f",d[0][0], d[0][1])); 
  
  // testing text 
  atariPrint("*S5*PPEW PEW!"); 
  atariPrint("M0,0");
  atariPrint("*S15*PATARI"); 
  atariPrint("H");   
  atariPrint("A");       
  
  //endless loop
  while(true) {}
}

// function to initialize printer
void initPrinter(){
  Serial.begin(19200);
  pinMode(ATARI_CMD_PIN, OUTPUT);
  pinMode(ATARI_READY_PIN, OUTPUT);
  pinMode(LED, OUTPUT);

  digitalWrite(ATARI_CMD_PIN, HIGH);   // LOW when a command is sent
  digitalWrite(ATARI_READY_PIN, HIGH); // +5V/READY
  digitalWrite(LED, LOW); 
}

// function to print to atari 1020
void atariPrint(String data){
  byte chksum = 0; 
  char msg[40] = "";

  // send a write command until successfull
  while( true ) {
    
    // send a write command  
    atariCommand(atariDevice, ATARI_CMD_WRITE, 0x4E, 0x4E);
    
      // check for ACK
    if (!waitFor(ATARI_CMD_ACK))
      continue;
    
    // mandatory wait after command  
    delayMicroseconds(1300);
  
    // add LF to data string
    data = data + (char)ATARI_LF;
    data.toCharArray(msg,40);  

    // calculate checksum
    chksum = checksum((byte*)&msg, 40);
  
    // send data byte by byte
    for (int i=0; i < 40; i++){
       Serial.write(msg[i]);
    } 
  
    // send checksum
    Serial.write(chksum);

    // printer must respond with ACK
    if (!waitFor(ATARI_CMD_ACK))
      continue;      
  
    // wait until operation is complete
    if (!waitFor(ATARI_CMD_COMPLETE))
      continue;
      
    // everything is good, exit the loop
    break;
  }
  
  //wait before sending another frame
  delay(100);
}

// function to check for message from printer
bool waitFor(byte message) {
  byte incomingByte = 0;
  
  // wait until data is available (printer must respond with a message)
  while (!Serial.available());

  // read
  incomingByte = Serial.read();
  
  // recevied the message, we're good, carry on
  if (incomingByte ==message){  
    return true;
  }
  else {  
    // blink the LED to indicate an error message
    while (20) {
      digitalWrite(LED, HIGH);
      delay(100);
      digitalWrite(LED, LOW);
      delay(100);     
    }
    return false;
  }      
}

// function to send an Atari Command frame of 4 bytes + checksum byte while pulling COMMAND
// pin LOW.
void atariCommand(byte deviceID, byte command, byte aux1, byte aux2) {
  byte chksum = 0; 
  
  // build command frame
  byte cmdFrame[4] = { deviceID, command, aux1, aux2 }; 
  
  // compute checksum
  chksum = checksum((byte*)&cmdFrame, 4); 
  
  // pull COMMAND pin low to start
  digitalWrite(ATARI_CMD_PIN, LOW);
  
  // Between 750 and 1600 usec to first character of command frame
  delayMicroseconds(900); 

  // write 5 bytes
  Serial.write(deviceID);
  Serial.write(command);
  Serial.write(aux1);
  Serial.write(aux2);
  Serial.write(chksum); 
  
  // Between 650 and 950 usec after last character of command frame
  delayMicroseconds(850); 
  
  // COMMAND pin high to end
  digitalWrite(ATARI_CMD_PIN, HIGH); 
}

// checksum calculation for SIO frame
byte checksum(byte* chunk, int length) {
  int chkSum = 0;
  for (int i = 0; i < length; i++) {
    chkSum = ((chkSum + chunk[i]) >> 8) + ((chkSum + chunk[i]) & 0xff);
  }
  return (byte)chkSum;
}

