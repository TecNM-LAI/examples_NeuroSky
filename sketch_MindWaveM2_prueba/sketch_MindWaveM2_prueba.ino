#include "BluetoothSerial.h"

#if !defined(CONFIG_BT_SPP_ENABLED)
#error Serial Bluetooth not available or not enabled. It is only available for the ESP32 chip.
#endif

BluetoothSerial SerialBT;

String MACadd = "98:07:2D:7F:5E:93";
uint8_t address[6]  = {0x98, 0x07, 0x2D, 0x7F, 0x5E, 0x93};
String name = "MindWave Mobile";
//const char *pin = "1234"; //<- standard pin would be provided by default
const char *pin = "0000";
bool connected;



#define LED 13
#define BAUDRATE 115200
#define DEBUGOUTPUT 0

#define GREENLED1  12
#define GREENLED2  13
#define GREENLED3  14
#define YELLOWLED1 25
#define YELLOWLED2 26
#define YELLOWLED3 27
#define YELLOWLED4 32
#define REDLED1    33
#define REDLED2    34
#define REDLED3    35

#define powercontrol 10

// checksum variables
byte generatedChecksum = 0;
byte checksum = 0; 
int payloadLength = 0;
byte payloadData[64] = {
  0};
byte poorQuality = 0;
byte attention = 0;
byte meditation = 0;

// system variables
long lastReceivedPacket = 0;
boolean bigPacket = false;







void setup() {
  pinMode(GREENLED1, OUTPUT);
  pinMode(GREENLED2, OUTPUT);
  pinMode(GREENLED3, OUTPUT);
  pinMode(YELLOWLED1, OUTPUT);
  pinMode(YELLOWLED2, OUTPUT);
  pinMode(YELLOWLED3, OUTPUT);
  pinMode(YELLOWLED4, OUTPUT);
  pinMode(REDLED1, OUTPUT);
  pinMode(REDLED2, OUTPUT);
  pinMode(REDLED3, OUTPUT);

  pinMode(LED, OUTPUT);



  
  Serial.begin(BAUDRATE);
  //SerialBT.setPin(pin);
  SerialBT.begin("ESP32test", true); 
  SerialBT.setPin(pin);
  Serial.println("The device started in master mode, make sure remote BT device is on!");
  
  // connect(address) is fast (upto 10 secs max), connect(name) is slow (upto 30 secs max) as it needs
  // to resolve name to address first, but it allows to connect to different devices with the same name.
  // Set CoreDebugLevel to Info to view devices bluetooth address and device names
  //connected = SerialBT.connect(name);
  connected = SerialBT.connect(address);
  
  if(connected) {
    Serial.println("Connected Succesfully!");
  } else {
    while(!SerialBT.connected(10000)) {
      Serial.println("Failed to connect. Make sure remote device is available and in range, then restart app."); 
    }
  }
  // disconnect() may take upto 10 secs max
  if (SerialBT.disconnect()) {
    Serial.println("Disconnected Succesfully!");
  }
  // this would reconnect to the name(will use address, if resolved) or address used with connect(name/address).
  SerialBT.connect();
}



////////////////////////////////
// Read data from Serial UART //
////////////////////////////////
byte ReadOneByte() {
  int ByteRead;

  while(!SerialBT.available());
  ByteRead = SerialBT.read();

#if DEBUGOUTPUT  
  Serial.print((char)ByteRead);   // echo the same byte out the USB serial (for debug purposes)
#endif

  return ByteRead;
}



void loop() {


  // Look for sync bytes
  if(ReadOneByte() == 170) {
    if(ReadOneByte() == 170) {

      payloadLength = ReadOneByte();
      if(payloadLength > 169)                      //Payload length can not be greater than 169
          return;

      generatedChecksum = 0;        
      for(int i = 0; i < payloadLength; i++) {  
        payloadData[i] = ReadOneByte();            //Read payload into memory
        generatedChecksum += payloadData[i];
      }   

      checksum = ReadOneByte();                      //Read checksum byte from stream      
      generatedChecksum = 255 - generatedChecksum;   //Take one's compliment of generated checksum

        if(checksum == generatedChecksum) {    

        poorQuality = 200;
        attention = 0;
        meditation = 0;

        for(int i = 0; i < payloadLength; i++) {    // Parse the payload
          switch (payloadData[i]) {
          case 2:
            i++;            
            poorQuality = payloadData[i];
            bigPacket = true;            
            break;
          case 4:
            i++;
            attention = payloadData[i];                        
            break;
          case 5:
            i++;
            meditation = payloadData[i];
            break;
          case 0x80:
            i = i + 3;
            break;
          case 0x83:
            i = i + 25;      
            break;
          default:
            break;
          } // switch
        } // for loop

#if !DEBUGOUTPUT

        // *** Add your code here ***

        if(bigPacket) {
          if(poorQuality == 0)
            digitalWrite(LED, HIGH);
          else
            digitalWrite(LED, LOW);
          Serial.print("PoorQuality: ");
          Serial.print(poorQuality, DEC);
          Serial.print(" Attention: ");
          Serial.print(attention, DEC);
          Serial.print(" Time since last packet: ");
          Serial.print(millis() - lastReceivedPacket, DEC);
          lastReceivedPacket = millis();
          Serial.print("\n");

          switch(attention / 10) {
          case 0:
            digitalWrite(GREENLED1, HIGH);
            digitalWrite(GREENLED2, LOW);
            digitalWrite(GREENLED3, LOW);
            digitalWrite(YELLOWLED1, LOW);
            digitalWrite(YELLOWLED2, LOW);
            digitalWrite(YELLOWLED3, LOW);
            digitalWrite(YELLOWLED4, LOW);
            digitalWrite(REDLED1, LOW);
            digitalWrite(REDLED2, LOW);
            digitalWrite(REDLED3, LOW);           
            break;
          case 1:
            digitalWrite(GREENLED1, HIGH);
            digitalWrite(GREENLED2, HIGH);
            digitalWrite(GREENLED3, LOW);
            digitalWrite(YELLOWLED1, LOW);
            digitalWrite(YELLOWLED2, LOW);
            digitalWrite(YELLOWLED3, LOW);
            digitalWrite(YELLOWLED4, LOW);
            digitalWrite(REDLED1, LOW);
            digitalWrite(REDLED2, LOW);
            digitalWrite(REDLED3, LOW);
            break;
          case 2:
            digitalWrite(GREENLED1, HIGH);
            digitalWrite(GREENLED2, HIGH);
            digitalWrite(GREENLED3, HIGH);
            digitalWrite(YELLOWLED1, LOW);
            digitalWrite(YELLOWLED2, LOW);
            digitalWrite(YELLOWLED3, LOW);
            digitalWrite(YELLOWLED4, LOW);
            digitalWrite(REDLED1, LOW);
            digitalWrite(REDLED2, LOW);
            digitalWrite(REDLED3, LOW);
            break;
          case 3:              
            digitalWrite(GREENLED1, HIGH);
            digitalWrite(GREENLED2, HIGH);
            digitalWrite(GREENLED3, HIGH);              
            digitalWrite(YELLOWLED1, HIGH);
            digitalWrite(YELLOWLED2, LOW);
            digitalWrite(YELLOWLED3, LOW);
            digitalWrite(YELLOWLED4, LOW);
            digitalWrite(REDLED1, LOW);
            digitalWrite(REDLED2, LOW);
            digitalWrite(REDLED3, LOW);             
            break;
          case 4:
            digitalWrite(GREENLED1, HIGH);
            digitalWrite(GREENLED2, HIGH);
            digitalWrite(GREENLED3, HIGH);              
            digitalWrite(YELLOWLED1, HIGH);
            digitalWrite(YELLOWLED2, HIGH);
            digitalWrite(YELLOWLED3, LOW);
            digitalWrite(YELLOWLED4, LOW);
            digitalWrite(REDLED1, LOW);
            digitalWrite(REDLED2, LOW);
            digitalWrite(REDLED3, LOW);              
            break;
          case 5:
            digitalWrite(GREENLED1, HIGH);
            digitalWrite(GREENLED2, HIGH);
            digitalWrite(GREENLED3, HIGH);              
            digitalWrite(YELLOWLED1, HIGH);
            digitalWrite(YELLOWLED2, HIGH);
            digitalWrite(YELLOWLED3, HIGH);
            digitalWrite(YELLOWLED4, LOW);
            digitalWrite(REDLED1, LOW);
            digitalWrite(REDLED2, LOW);
            digitalWrite(REDLED3, LOW);               
            break;
          case 6:              
            digitalWrite(GREENLED1, HIGH);
            digitalWrite(GREENLED2, HIGH);
            digitalWrite(GREENLED3, HIGH);              
            digitalWrite(YELLOWLED1, HIGH);
            digitalWrite(YELLOWLED2, HIGH);
            digitalWrite(YELLOWLED3, HIGH);
            digitalWrite(YELLOWLED4, HIGH);
            digitalWrite(REDLED1, LOW);
            digitalWrite(REDLED2, LOW);
            digitalWrite(REDLED3, LOW);              
            break;
          case 7:
            digitalWrite(GREENLED1, HIGH);
            digitalWrite(GREENLED2, HIGH);
            digitalWrite(GREENLED3, HIGH);              
            digitalWrite(YELLOWLED1, HIGH);
            digitalWrite(YELLOWLED2, HIGH);
            digitalWrite(YELLOWLED3, HIGH);
            digitalWrite(YELLOWLED4, HIGH);
            digitalWrite(REDLED1, HIGH);
            digitalWrite(REDLED2, LOW);
            digitalWrite(REDLED3, LOW);              
            break;    
          case 8:
            digitalWrite(GREENLED1, HIGH);
            digitalWrite(GREENLED2, HIGH);
            digitalWrite(GREENLED3, HIGH);              
            digitalWrite(YELLOWLED1, HIGH);
            digitalWrite(YELLOWLED2, HIGH);
            digitalWrite(YELLOWLED3, HIGH);
            digitalWrite(YELLOWLED4, HIGH);
            digitalWrite(REDLED1, HIGH);
            digitalWrite(REDLED2, HIGH);
            digitalWrite(REDLED3, LOW);
            break;
          case 9:
            digitalWrite(GREENLED1, HIGH);
            digitalWrite(GREENLED2, HIGH);
            digitalWrite(GREENLED3, HIGH);              
            digitalWrite(YELLOWLED1, HIGH);
            digitalWrite(YELLOWLED2, HIGH);
            digitalWrite(YELLOWLED3, HIGH);
            digitalWrite(YELLOWLED4, HIGH);
            digitalWrite(REDLED1, HIGH);
            digitalWrite(REDLED2, HIGH); 
            digitalWrite(REDLED3, HIGH);
            break;
          case 10:
            digitalWrite(GREENLED1, HIGH);
            digitalWrite(GREENLED2, HIGH);
            digitalWrite(GREENLED3, HIGH);              
            digitalWrite(YELLOWLED1, HIGH);
            digitalWrite(YELLOWLED2, HIGH);
            digitalWrite(YELLOWLED3, HIGH);
            digitalWrite(YELLOWLED4, HIGH);
            digitalWrite(REDLED1, HIGH);
            digitalWrite(REDLED2, HIGH); 
            digitalWrite(REDLED3, HIGH);
            break;           
          }                     
        }
#endif        
        bigPacket = false;        
      }
      else {
        // Checksum Error
      }  // end if else for checksum
    } // end if read 0xAA byte
  } // end if read 0xAA byte
}
