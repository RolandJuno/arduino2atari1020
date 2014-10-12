// Arduino to Atari 1020 printer
// October 11, 2014
// @paulrickards
// Code currently does not work

// http://www.atarimax.com/jindroush.atari.org/asio.html
// http://www.atariarchives.org/dere/chapt08.php
// http://www.atariarchives.org/cfn/12/02/0022.php
// http://forum.arduino.cc/index.php/topic,19177.0.html
// http://web.archive.org/web/20090405180711/http://www.stephens-home.com/sio2usb/
// http://atariage.com/forums/topic/182392-sio2usb-with-a-ftdi-basic-breakout-board/page-2
// http://www.atarimax.com/sio2pc/documentation/chapter1.html

/*************************** PIN CONNETION GUIDE ****************************/
// Pinout from http://www.whizzosoftware.com/sio2arduino/
// Atari SIO Data In pin 3    = Arduino 1 (TX)
// Atari SIO Ground pin 4     = Arduino ground (GND)
// Atari SIO Data Out pin 5   = Arduino 0 (RX)
// Atari SIO Command pin 7    = Arduino 2
// Atari SIO +5v/READY pin 10 = Arduino 3
/****************************************************************************/

// From Atari 1020 Field Service Manual
// Atari SIO Pin 7 (COMMAND) Goes to zero when a command frame is being sent
// Atari SIO Pin 10 (+5v/READY) Indicates that the computer is turned on and ready.
//                              Restricted use as a +5 volt supply

// Atari printer device IDs
// from http://atariage.com/forums/topic/170228-accessing-multiple-printers/
//850 -- P2: (0x41)
//1025 - P3: (0x42)
//1020 - P4: (0x43)
//1027 - P5: (0x44)
//1029 - P6: (0x45)

#define ATARI_CMD_WRITE 0x57
#define ATARI_CMD_GET_STATUS 0x53
#define ATARI_CMD_ACK 0x43
#define ATARI_CMD_NAK 0x4E
#define ATARI_CMD_ERROR 0x45
#define ATARI_LF 0x9B

#define P1 0x40
#define P2 0x41
#define P3 0x42
#define P4 0x43
#define P5 0x44
#define P6 0x45

#define ATARI_CMD_PIN 2
#define ATARI_READY_PIN 3
#define LED 13

int atariDevice = P4; // Set the P: address here
int incomingByte = 0;
byte chksum = 0;
char msg[40];

void setup() {
  Serial.begin(19200);

  pinMode(ATARI_CMD_PIN, OUTPUT);
  pinMode(ATARI_READY_PIN, OUTPUT);
  pinMode(LED, OUTPUT);

  digitalWrite(ATARI_CMD_PIN, HIGH);   // LOW when a command is sent
  digitalWrite(ATARI_READY_PIN, HIGH); // +5V/READY
  digitalWrite(LED, LOW);
}

void loop() {
  delay(2000);
  bool ack = false; // for future loop control

  delayMicroseconds(1300);
  // bytes 3 and 4 are guesses for the 1020 printer
  atariCommand(atariDevice, ATARI_CMD_WRITE, 0x00, 0x00);
  delay(16);

  // Check to see if printer responds
  if (Serial.available() > 0) {
    incomingByte = Serial.read();
    // Recevied an ACK, we're good, carry on
    if(incomingByte == ATARI_CMD_ACK) { 
      ack = true; 
    }
    // If we received a response of NAK or ERR, halt and flash LED
    // Never receives a response AFAIK
    if (incomingByte == ATARI_CMD_NAK || incomingByte == ATARI_CMD_ERROR) {
      while (1) { 
        digitalWrite(LED, HIGH);
        delay(250);
        digitalWrite(LED, LOW);
        delay(250);
      }
    }
  }

  // Data frame
  delayMicroseconds(1300);
  // Create 40 byte payload message, padded with spaces
  // A reference to the Atari 850 printer noted this was needed. Not sure if
  // the Atari 1020 needs this too.
  // End with ATARI_LF character
  sprintf(msg, "%-39s%c", "AHello, World!", ATARI_LF);
  chksum = checksum((byte*)&msg, 40);
  Serial.print(msg);
  Serial.write(chksum);
  Serial.flush();

  digitalWrite(LED, HIGH);
  delay(250);
  digitalWrite(LED, LOW);
  delay(250);

  delay(2000); // Wait 2 secs and try again
}

// Function to send an Atari Command frame of 4 bytes + checksum byte while pulling COMMAND
// pin LOW.
void atariCommand(byte deviceID, byte command, byte aux1, byte aux2) {
  byte cmdFrame[4] = { deviceID, command, aux1, aux2 }; // build command frame
  chksum = checksum((byte*)&cmdFrame, 4); // compute checksum
  digitalWrite(ATARI_CMD_PIN, LOW); // pull COMMAND pin low
  delayMicroseconds(1175); // Between 750 and 1600 usec to first character of command frame

  Serial.write((byte)deviceID);
  Serial.write((byte)command);
  Serial.write((byte)aux1);
  Serial.write((byte)aux2);
  Serial.write((byte)chksum); // write 5 bytes

  delayMicroseconds(800); // Between 650 and 950 usec after last character of command frame
  digitalWrite(ATARI_CMD_PIN, HIGH); // COMMAND pin high
  Serial.flush(); // not sure if this is needed
}

// Checksum function from http://www.whizzosoftware.com/sio2arduino/
byte checksum(byte* chunk, int length) {
  int chkSum = 0;
  for(int i=0; i < length; i++) {
    chkSum = ((chkSum+chunk[i])>>8) + ((chkSum+chunk[i])&0xff);
  }
  return (byte)chkSum;
}
