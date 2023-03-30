// This #include statement was automatically added by the Particle IDE.
#include <Keypad_Particle.h>

// This #include statement was automatically added by the Particle IDE.
#include <Adafruit_SSD1306_RK.h>

// This #include statement was automatically added by the Particle IDE.
//#include <Adafruit_SSD1306.h>

/**
 * ----------------------------------------------------------------------------
 * This is a MFRC522 library example; see https://github.com/miguelbalboa/rfid
 * for further details and other examples.
 *
 * NOTE: The library file MFRC522.h has a lot of useful info. Please read it.
 *
 * Released into the public domain.
 * ----------------------------------------------------------------------------
 * This sample shows how to read and write data blocks on a MIFARE Classic PICC
 * (= card/tag).
 *
 * BEWARE: Data will be written to the PICC, in sector #1 (blocks #4 to #7).
 *
 *
 * Typical pin layout used:
 * -----------------------------------------------------------------------------------------
 *             MFRC522      Arduino       Arduino   Arduino    Arduino          Arduino
 *             Reader/PCD   Uno/101       Mega      Nano v3    Leonardo/Micro   Pro Micro
 * Signal      Pin          Pin           Pin       Pin        Pin              Pin
 * -----------------------------------------------------------------------------------------
 * RST/Reset   RST          9             5         D9         RESET/ICSP-5     RST
 * SPI SS      SDA(SS)      10            53        D10        10               10
 * SPI MOSI    MOSI         11 / ICSP-4   51        D11        ICSP-4           16
 * SPI MISO    MISO         12 / ICSP-1   50        D12        ICSP-1           14
 * SPI SCK     SCK          13 / ICSP-3   52        D13        ICSP-3           15
 *
 */

#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include "Particle.h"

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

constexpr uint8_t RST_PIN =8;     // Configurable, see typical pin layout above
constexpr uint8_t SS_PIN = 7;     // Configurable, see typical pin layout above
int led = D2; 
int led2 = A5;

MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance.

MFRC522::MIFARE_Key key;

// Variables to manage time
unsigned long seconds = 0;
unsigned long  previous = 0;
char buffer[10];
char buffer1[10];
char buffer2[15];
char buffer3[10];
char code[4];
bool scanned = false;
bool codeIn = false;

const byte ROWS = 4; 
const byte COLS = 3; 

char hexaKeys[ROWS][COLS] = {
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},
  {'*', '0', '#'}
};

byte rowPins[ROWS] = {D3, D4, D5, D6}; 
byte colPins[COLS] = {A2, A3, A4}; 

Keypad customKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);

/**
 * Initialize.
 */
void setup() {
    pinMode(led, OUTPUT);
    pinMode(led2, OUTPUT);
    Serial.begin(9600); // Initialize serial communications with the PC
    while (!Serial);    // Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)
    SPI.begin();        // Init SPI bus
    mfrc522.PCD_Init(); // Init MFRC522 card

    // Prepare the key (used both as key A and as key B)
    // using FFFFFFFFFFFFh which is the default at chip delivery from the factory
    for (byte i = 0; i < 6; i++) {
        key.keyByte[i] = 0xFF;
    }

    Serial.println(F("Scan a MIFARE Classic PICC to demonstrate read and write."));
    Serial.print(F("Using key (for A and B):"));
    dump_byte_array(key.keyByte, MFRC522::MF_KEY_SIZE);
    Serial.println();

    Serial.println(F("BEWARE: Data will be written to the PICC, in sector #1"));
    
    // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  // Clear the buffer
  display.clearDisplay();
  display.display();
}

/**
 * Main loop.
 */
void loop() {
    // Look for new cards
    if ( ! mfrc522.PICC_IsNewCardPresent()){
        digitalWrite(led, LOW);
        digitalWrite(led2, LOW);
        return;
    }
        

    // Select one of the cards
    if ( ! mfrc522.PICC_ReadCardSerial()){
        digitalWrite(led, LOW);
        digitalWrite(led2, LOW);
        return;
    }
        
    digitalWrite(led, HIGH);
    digitalWrite(led2, LOW);
    sprintf(buffer, "SCANNING");
    
     display.clearDisplay();

      display.setTextSize(2); // Draw 2X-scale text
      display.setTextColor(WHITE);
      display.setCursor(0, 0);
      // Convert the unsigned interger to a char array
      display.println(buffer);
      display.display();      // Show initial text
      display.startscrollright(0x00, 0x0F);
      delay(2000);
      display.stopscroll();
      display.clearDisplay();
      //display.display();
    
    // Show some details of the PICC (that is: the tag/card)
    Serial.print(F("Card UID:"));
    dump_byte_array(mfrc522.uid.uidByte, mfrc522.uid.size);
    Serial.println();
    Serial.print(F("PICC type: "));
    MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
    Serial.println(mfrc522.PICC_GetTypeName(piccType));

    // Check for compatibility
    if (    piccType != MFRC522::PICC_TYPE_MIFARE_MINI
        &&  piccType != MFRC522::PICC_TYPE_MIFARE_1K
        &&  piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
        Serial.println(F("This sample only works with MIFARE Classic cards."));
        return;
    }

    // In this sample we use the second sector,
    // that is: sector #1, covering block #4 up to and including block #7
    byte sector         = 1;
    byte blockAddr      = 4;
    byte dataBlock[]    = {
        0x01, 0x02, 0x03, 0x04, //  1,  2,   3,  4,
        0x05, 0x06, 0x07, 0x08, //  5,  6,   7,  8,
        0x09, 0x0a, 0xff, 0x0b, //  9, 10, 255, 11,
        0x0c, 0x0d, 0x0e, 0x0f  // 12, 13, 14, 15
    };
    byte trailerBlock   = 7;
    MFRC522::StatusCode status;
    byte buffer[18];
    byte size = sizeof(buffer);

    // Authenticate using key A
    Serial.println(F("Authenticating using key A..."));
    status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("PCD_Authenticate() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
        
        sprintf(buffer3, "AUTH FAIL");
        display.setCursor(0, 0);
      // Convert the unsigned interger to a char array
      display.println(buffer3);
      display.display();      // Show initial text
      display.startscrollright(0x00, 0x0F);
      delay(500);
      display.stopscroll();
      display.clearDisplay();
      
        return;
    }

    // Show the whole sector as it currently is
    Serial.println(F("Current data in sector:"));
    mfrc522.PICC_DumpMifareClassicSectorToSerial(&(mfrc522.uid), &key, sector);
    Serial.println();

    // Read data from the block
    Serial.print(F("Reading data from block ")); Serial.print(blockAddr);
    Serial.println(F(" ..."));
    status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(blockAddr, buffer, &size);
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("MIFARE_Read() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
    }
    Serial.print(F("Data in block ")); Serial.print(blockAddr); Serial.println(F(":"));
    dump_byte_array(buffer, 16); Serial.println();
    Serial.println();

    // Authenticate using key B
    Serial.println(F("Authenticating again using key B..."));
    status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_B, trailerBlock, &key, &(mfrc522.uid));
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("PCD_Authenticate() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
        return;
    }

    // Write data to the block
    Serial.print(F("Writing data into block ")); Serial.print(blockAddr);
    Serial.println(F(" ..."));
    dump_byte_array(dataBlock, 16); Serial.println();
    status = (MFRC522::StatusCode) mfrc522.MIFARE_Write(blockAddr, dataBlock, 16);
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("MIFARE_Write() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
    }
    Serial.println();

    // Read data from the block (again, should now be what we have written)
    Serial.print(F("Reading data from block ")); Serial.print(blockAddr);
    Serial.println(F(" ..."));
    status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(blockAddr, buffer, &size);
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("MIFARE_Read() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
    }
    Serial.print(F("Data in block ")); Serial.print(blockAddr); Serial.println(F(":"));
    dump_byte_array(buffer, 16); Serial.println();

    // Check that data in block is what we have written
    // by counting the number of bytes that are equal
    Serial.println(F("Checking result..."));
    byte count = 0;
    for (byte i = 0; i < 16; i++) {
        // Compare buffer (= what we've read) with dataBlock (= what we've written)
        if (buffer[i] == dataBlock[i])
            count++;
    }
    Serial.print(F("Number of bytes that match = ")); Serial.println(count);
    if (count == 16) {
        sprintf(buffer2, "INPUT CODE");
        display.setCursor(0, 0);
        display.println(buffer2);
        display.display(); 
        Serial.println(buffer2);
        display.startscrollright(0x00, 0x0F);
        delay(100);
        display.stopscroll();
        display.clearDisplay();
        Serial.println(F("Success, Input Code"));
        
        scanned = true; 
        char customKey1 = NO_KEY;
        int counter = 0;
        while (counter < 4){
            customKey1 = customKeypad.getKey();
            
            if(customKey1 != NO_KEY && counter < 4){
                code[counter] = customKey1;
                counter++;
                
            }
            /*
            if (customKey1 == '7' && scanned == true){
                sprintf(buffer1, "7");
                display.println(buffer1);
                display.display(); 
                Serial.println(customKey1);
                delay(2000);
                display.clearDisplay();
                display.display();
                if(){
                    if(){
                        if(){
                            
                        }
                    }
                }
                codeIn = true; 
            }
            if(customKey1 != '7'){
                customKey1 = 'x';
            }
            */
        }
        
        if(counter == 4 && code[0] == '7' && code[1] == '4' && code[2] == '3' && code[3] == '9'){
            //display.setTextSize(2); // Draw 2X-scale text
            //display.setTextColor(WHITE);
            Particle.publish("WELCOME");
            sprintf(buffer1, "WELCOME");
            display.setCursor(0, 0);
            display.println(buffer1);
            display.display(); 
            Serial.println(buffer1);
            display.startscrollright(0x00, 0x0F);
            delay(3000);
            display.stopscroll();
            display.clearDisplay();
            display.display();
            digitalWrite(led2, HIGH);
            codeIn = true;
        }
        else{
            Particle.publish("INVALID");
            sprintf(buffer1, "INVALID");
            counter = 0;
            display.setTextSize(2); // Draw 2X-scale text
            display.setTextColor(WHITE);
            display.setCursor(0, 0);
            display.println(buffer1);
            display.display(); 
            Serial.println(buffer1);
            display.startscrollright(0x00, 0x0F);
            delay(3000);
            display.stopscroll();
            display.clearDisplay();
            display.display();
        }
        
    } 
    else {
        Serial.println(F("Failure, no match :-("));
        Serial.println(F("  perhaps the write didn't work properly..."));
    }
    Serial.println();

    // Dump the sector data
    //Serial.println(F("Current data in sector:"));
    //mfrc522.PICC_DumpMifareClassicSectorToSerial(&(mfrc522.uid), &key, sector);
    //Serial.println();
    
    if(codeIn){
        // Halt PICC
        mfrc522.PICC_HaltA();
        // Stop encryption on PCD
        mfrc522.PCD_StopCrypto1();
    }
    
    
  
  display.clearDisplay();
  display.display();

}

/**
 * Helper routine to dump a byte array as hex values to Serial.
 */
void dump_byte_array(byte *buffer, byte bufferSize) {
    for (byte i = 0; i < bufferSize; i++) {
        Serial.print(buffer[i] < 0x10 ? " 0" : " ");
        Serial.print(buffer[i], HEX);
    }
}
