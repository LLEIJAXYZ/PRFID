/*
  
  Parallax RFID Read/Write: Basic Demonstration 
  
  ALL CREDIT GOES TO THE ORGINAL CODE CREATORS.
  JOE GRAND IS THE ORGINAL CREATOR.
  I HAVE ONLY MODIFIED AND ADAPTED IT FOR EASY
  USE AND FOR SIMPLICITY. -Lucas Leija     
                                                         
  Author: Joe Grand [www.grandideastudio.com] 
  Modified by: Lucas Leija [www.lleija.xyz]            
  Contact: support@parallax.com
  Lucas Leija Contact: popbag4@gmail.com
                             
  
  Program Description:
  
  This program demonstrates the basic functionality of the Parallax RFID Read/Write
  Module (#28440). Data is displayed using the Arduino Serial Monitor. 
  
  The module is compatible with EM Microelectronic EM4x50 1kbit R/W 125kHz tags.
  These tags can be used in a number of ways:
  1) Read/write up to 116 bytes of user data (publicly accessible)
  
  Please refer to the product manual for full details of module functionality 
  and capabilities.

  Revisions:
  
  1.0 (May 1, 2014): Initial release
  1.1 (Febuary 19, 2017): Lucas Leija's personal modification
  
*/

#include <SoftwareSerial.h>

#define rxPin      11  // Serial input (connects to the RFID's SOUT pin)
#define txPin      10  // Serial output (connects to the RFID's SIN pin)

#define BUFSIZE    12  // Size of receive buffer (in bytes) for incoming data from the RFID R/W Module (this should be adjusted to be larger than the expected response)

// RFID R/W Module Commands
// Number of bytes returned in ()
#define RFID_Read         0x01    // Read data from specified address, valid locations 1 to 33 (5)
#define RFID_Write    0x02    // Write data to specified address, valid locations 3 to 31 (1)
#define RFID_Login    0x03    // Login to tag with password (1)
#define RFID_SetPass      0x04    // Change tag's password from old to new (1)
#define RFID_Protect    0x05    // Enable/disable password protection (1)
#define RFID_Reset        0x06    // Reset tag (1)
#define RFID_ReadLegacy   0x0F    // Read unique ID from EM4102 read-only tag (for backwards compatibility with Parallax RFID Card Reader #28140) (12)

// Memory map/address locations for EM4x50 tag
// Each address holds/returns a 32-bit (4 byte) value
#define ADDR_Password     0   // Password (not readable)
#define ADDR_Protect    1   // Protection Word
#define ADDR_Control    2   // Control Word
// ADDR 3-31 are User EEPROM area
#define ADDR_Serial   32    // Device Serial Number
#define ADDR_DeviceID   33    // Device Identification

// Status/error return codes
#define ERR_OK      0x01    // No errors
#define ERR_LIW     0x02    // Did not find a listen window
#define ERR_NAK     0x03    // Received a NAK, could be invalid command
#define ERR_NAK_OLDPW   0x04    // Received a NAK sending old password (RFID_SetPass), could be incorrect password
#define ERR_NAK_NEWPW   0x05    // Received a NAK sending new password (RFID_SetPass)
#define ERR_LIW_NEWPW   0x06    // Did not find a listen window after setting new password (RFID_SetPass)
#define ERR_PARITY    0x07    // Parity error when reading data

// For use with RFID_ReadLegacy command
#define LEGACY_StartByte  0x0A
#define LEGACY_StopByte   0x0D


// set up a new serial port
SoftwareSerial rfidSerial =  SoftwareSerial(rxPin, txPin);

void setup()  // Set up code called once on start-up
{
  // define pin modes
  pinMode(rxPin, INPUT);
  pinMode(txPin, OUTPUT);
 
  // setup Arduino Serial Monitor
  Serial.begin(9600);
  while (!Serial);   // wait until ready
  Serial.println("\n\nParallax RFID Read/Write Module");
  
  rfidSerial.begin(9600);
  
  Serial.flush();   
}


void loop()  
{
  char idx;
  uint8_t rfidData[BUFSIZE];  


  Serial.print("Reading tag's entire memory contents:\n");
  for (idx = 1; idx <= 33; idx++) 
  {
    while (rfidRead(rfidData, idx) != 0); 
    Serial.print(idx, DEC);
    Serial.print(": ");
    PrintHex(rfidData, 4);   
    Serial.println();
    Serial.flush();
  }
  Serial.print("Tag memeory read finished.\n");
  delay(2000);
}

char rfidRead(uint8_t *data, uint8_t address)
{
  char offset;  
  
  rfidFlush();
  
  rfidSerial.print("!RW");       
  rfidSerial.write(RFID_Read);   
  rfidSerial.write(address);     
    
  while (rfidSerial.available() != 5);  
  
  if (rfidSerial.read() == ERR_OK)  
  {
    for (offset = 0; offset < 4; offset++)
      data[offset] = rfidSerial.read(); 
       
    return 0; 
  }
  
  return -1; // return error
}


char rfidWrite(uint8_t address, uint8_t *data)
{
  char offset;  // offset into buffer
  
  rfidFlush();  // empties any buffered incoming serial data
  
  rfidSerial.print("!RW");        // header
  rfidSerial.write(RFID_Write);   // command
  rfidSerial.write(address);      // address
  for (offset = 0; offset < 4; offset++)
    rfidSerial.write(data[offset]);     // data (4 bytes) 
  
  while (rfidSerial.available() == 0); // wait until data is received from RFID module

  if (rfidSerial.read() == ERR_OK) // if our status byte is OK
    return 0; // return good
  
  return -1; // return error
}


char rfidLogin(uint8_t *password)
{
  char offset;  // offset into buffer
  
  rfidFlush();  // empties any buffered incoming serial data
  
  rfidSerial.print("!RW");        // header
  rfidSerial.write(RFID_Login);   // command
  for (offset = 0; offset < 4; offset++)
    rfidSerial.write(password[offset]);     // password (4 bytes) 
  
  while (rfidSerial.available() == 0); // wait until data is received from RFID module

  if (rfidSerial.read() == ERR_OK) // if our status byte is OK
    return 0; // return good
  
  return -1; // return error
}


char rfidSetPass(uint8_t *currentpass, uint8_t *newpass)
{
  char offset;  // offset into buffer
  
  rfidFlush();  // empties any buffered incoming serial data
  
  rfidSerial.print("!RW");          // header
  rfidSerial.write(RFID_SetPass);   // command
  for (offset = 0; offset < 4; offset++)
    rfidSerial.write(currentpass[offset]);  // current password (4 bytes) 
  for (offset = 0; offset < 4; offset++)
    rfidSerial.write(newpass[offset]);      // new password (4 bytes) 
    
  while (rfidSerial.available() == 0); // wait until data is received from RFID module

  if (rfidSerial.read() == ERR_OK) // if our status byte is OK
    return 0; // return good
  
  return -1; // return error
}


char rfidProtect(boolean enable)
{
  rfidFlush();  // empties any buffered incoming serial data
  
  rfidSerial.print("!RW");          // header
  rfidSerial.write(RFID_Protect);   // command  
  if (enable == false)              // flag
    rfidSerial.write((byte)0);
  else
    rfidSerial.write((byte)1);
  
  while (rfidSerial.available() == 0); // wait until data is received from RFID module

  if (rfidSerial.read() == ERR_OK) // if our status byte is OK
    return 0; // return good
  
  return -1; // return error    
}


char rfidReset()
{
  rfidFlush();  // empties any buffered incoming serial data
  
  rfidSerial.print("!RW");        // header
  rfidSerial.write(RFID_Reset);   // command  
  
  while (rfidSerial.available() == 0); // wait until data is received from RFID module

  if (rfidSerial.read() == ERR_OK) // if our status byte is OK
    return 0; // return good
  
  return -1; // return error    
}


char rfidReadLegacy(uint8_t *data)
{
  char offset;  // offset into buffer
  
  rfidFlush();  // empties any buffered incoming serial data
  
  rfidSerial.print("!RW");             // header
  rfidSerial.write(RFID_ReadLegacy);   // command
    
  while (rfidSerial.available() != 12);  // wait until data is received from RFID module
  
  if (rfidSerial.read() == LEGACY_StartByte)  // if our start byte is OK
  {
    for (offset = 0; offset < 10; offset++)
      data[offset] = rfidSerial.read();  // get the remaining data
       
    data[offset] = 0;  // null terminate the string of bytes we just received   
    return 0; // return good
  }
  
  return -1; // return error
}


void rfidFlush()  // Empties any buffered incoming serial data
{
    while (rfidSerial.available() > 0)
      rfidSerial.read();
}


// from http://forum.arduino.cc/index.php?topic=38107#msg282343
void PrintHex(uint8_t *data, uint8_t length) // prints 8-bit data in hex
{
  char tmp[length*2+1];
  byte first ;
  int j=0;
  for (uint8_t i=0; i<length; i++) 
  {
    first = (data[i] >> 4) | 48;
    if (first > 57) tmp[j] = first + (byte)39;
    else tmp[j] = first ;
    j++;

    first = (data[i] & 0x0F) | 48;
    if (first > 57) tmp[j] = first + (byte)39; 
    else tmp[j] = first;
    j++;
  }
  tmp[length*2] = 0;
  Serial.print(tmp);
}


  
