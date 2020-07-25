#include <SD.h>
#include <LiquidCrystal.h>

// Replica1+ MIDI KARAOKE - open source, MIT License - by Eric Rangell - project for Kansasfest, July 2020
// Arduino Mega + Data Logger Shield + LCD Keypad Shield + Breadboard + 16 pin Ribbon cable to Replica 1 keyboard socket

/* MFF0PLAY.ino for Arduino MEGA
 *  2020 enhancements
 *  - use LCD Keypad shield to select song and directory
 *  
 * Arduino SD Card MIDI Song Player - by Eric Rangell 
 * Adapted from open-source Arduino MIDI code.  Published in the public domain.
 *
 * Connect pin TX1 to MIDI socket pin 4, +5V to 220 ohm resistor to MIDI socket pin 5, pin 3 to GND
 * 
 * Connect DIP switches as follows:
 * +5V to all OFF positions of each switch
 * A ribbon cable connects Each ON position of each switch to the odd-numbered pins 23-37
 * Eight 470 ohm resistors from each ON position of each switch to GND
 * This allows selection of 255 directories (Directory #0 always plays silence).
 * 
 * Even numbered pins 22-36 are used to choose the starting song number within the selected directory
 * 
 * Store MIDI Format 0 files in SD directories named /001 /002 /003 etc.
 * File names must be: 001.mid, 002.mid, 003.mid, etc in the order to be played.
 * 
 * DIP switches on pins 23 through 37 (odd numbers) determine the directory number (binary with high bit on pin 37)
 * DIP swtiches are read at the beginning of the sketch, or when RESET is pressed.
 * If all DIP switches are OFF (0), then playback is stopped until a different directory number is set
 * 
 * You can wire the RESET button to a foot pedal, and make a box with switches or potentiometers to select the playlist.
 * Use your imagination and build what YOU want.
 * 
 * Set debugSong = true to debug songs that don't play.
 * For Type 1 Midi Files, program will attempt to play data from track #2.
 * It does not mix down tracks in Type 1 Midi Files, so they should be converted to Type 0.
 * 
 */

//Uses LCD Keypad Shield - has 5 buttons + reset
// select the pins used on the LCD panel
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

// define some values used by the panel and buttons
int lcd_key     = 0;
int adc_key_in  = 0;
#define btnRIGHT  0
#define btnUP     1
#define btnDOWN   2
#define btnLEFT   3
#define btnSELECT 4
#define btnNONE   5

//LCDKeypad uses A0 for Buttons
int buttonsPin = A0;

 
#define SD_BUFFER_SIZE 512 
#define HEADER_CHUNK_ID 0x4D546864  // MThd
#define TRACK_CHUNK_ID 0x4D54726B   // MTrk
#define DELTA_TIME_VALUE_MASK 0x7F
#define DELTA_TIME_END_MASK 0x80
#define DELTA_TIME_END_VALUE 0x80
#define EVENT_TYPE_MASK 0xF0
#define EVENT_CHANNEL_MASK 0x0F
#define NOTE_OFF_EVENT_TYPE 0x80
#define NOTE_ON_EVENT_TYPE 0x90
#define KEY_AFTERTOUCH_EVENT_TYPE 0xA0
#define CONTROL_CHANGE_EVENT_TYPE 0xB0
#define PROGRAM_CHANGE_EVENT_TYPE 0xC0
#define CHANNEL_AFTERTOUCH_EVENT_TYPE 0xD0
#define PITCH_WHEEL_CHANGE_EVENT_TYPE 0xE0
#define META_EVENT_TYPE 0xFF
#define SYSTEM_EVENT_TYPE 0xF0
#define META_SEQ_COMMAND 0x00
#define META_TEXT_COMMAND 0x01
#define META_COPYRIGHT_COMMAND 0x02
#define META_TRACK_NAME_COMMAND 0x03
#define META_INST_NAME_COMMAND 0x04
#define META_LYRICS_COMMAND 0x05
#define META_MARKER_COMMAND 0x06
#define META_CUE_POINT_COMMAND 0x07
#define META_CHANNEL_PREFIX_COMMAND 0x20
#define META_END_OF_TRACK_COMMAND 0x2F
#define META_SET_TEMPO_COMMAND 0x51
#define META_SMPTE_OFFSET_COMMAND 0x54
#define META_TIME_SIG_COMMAND 0x58
#define META_KEY_SIG_COMMAND 0x59
#define META_SEQ_SPECIFIC_COMMAND 0x7F

//Aruino pins for Replica 1 keyboard socket pins:
#define strobe  23
#define d5      25
#define d4      27
#define d6      29
#define d2      31
#define d3      33
#define d0      35
#define d1      37

/* PINS */

// change this to match your SD shield or module;
// Arduino Ethernet shield: pin 4
// Adafruit SD shields and modules: pin 10
// Sparkfun SD shield: pin 8

//For Arduino Mega, follow these instructions: https://learn.adafruit.com/adafruit-data-logger-shield/for-the-mega-and-leonardo
//(reverted to original SD libaray)

const int SSPIN = 53;
const int spi1 = 10;  // pins needed for SD.begin()
const int spi2 = 11;
const int spi3 = 12;
const int spi4 = 13;
int SDconnected = 0; //will get set to 1 if successful connection


boolean file_opened = false;
boolean last_block = false;
boolean file_closed = false;

char currentSong[18];

uint16_t bufsiz=SD_BUFFER_SIZE;
uint8_t buf1[SD_BUFFER_SIZE];
uint16_t bytesread1;
uint16_t bufIndex;
uint8_t currentByte;
uint8_t previousByte;

int format;
int trackCount;
int timeDivision;

unsigned long deltaTime;
int eventType;
int eventChannel;
int parameter1;
int parameter2;

// The number of microseconds per quarter note (i.e. the current tempo)
long microseconds = 500000;
int index = 0;
unsigned long accDelta = 0;

boolean firstNote = true;
int currFreq = -1;
unsigned long lastMillis;

int dirnum = 1;         //Directory number (000-999)
int currentSongNum = 1; //Song number (000.MID - 999.MID)

boolean debugSong = false;

/************ SDCARD STUFF ************/
Sd2Card card;
File thefile;

// store error strings in flash to save RAM
#define error(s) error_P(PSTR(s))

void error_P(const char* str) {
 PgmPrint("error: ");
 SerialPrintln_P(str);
 if (card.errorCode()) {
   PgmPrint("SD error: ");
   Serial.print(card.errorCode(), HEX);
   Serial.print(',');
   Serial.println(card.errorData(), HEX);
 }
 while(1);
}
/************ SDCARD STUFF ************/

void setPinMode (int pin, int firstpin, int secondpin)
{
    if ((pin == firstpin) || (pin == secondpin))
    {
      pinMode(pin, OUTPUT);           
    }
    else
    {
      pinMode(pin, INPUT);
    }

}

void setup() {  
  Serial.begin(9600);
  
  //  Set MIDI baud rate:
  Serial1.begin(31250);

  lcd.begin(16, 2);              // start the library
  lcd.setCursor(0,0);
  lcd.print("MFF0PLAY20200704"); 
  lcd.setCursor(0,1);
  lcd.print("Initg SD Card...");  

  Serial.print("\nInitializing SD card...");

  pinMode(SSPIN, OUTPUT);
  
  pinMode(LED_BUILTIN,OUTPUT);
  digitalWrite(LED_BUILTIN,LOW);  //LED will be lit if any error occurs
    
  if (!SD.begin(spi1, spi2, spi3, spi4)) {
    lcd.setCursor(0,1);
    lcd.print("Check SD Card!!!");
    Serial.println("initialization failed!  Check wiring, if card inserted, and pin to use for your SD shield");
    digitalWrite(LED_BUILTIN,HIGH);  
    // don't do anything more:
    while (1) ;
  } else {
    Serial.println("Wiring is correct and a card is present.");
    lcd.setCursor(0,1);
    lcd.print("SD Card Good    ");
    delay(1000);
    SDconnected = 1;
  }
  pinMode(d0,OUTPUT);
  pinMode(d1,OUTPUT);
  pinMode(d2,OUTPUT);
  pinMode(d3,OUTPUT);
  pinMode(d4,OUTPUT);
  pinMode(d5,OUTPUT);
  pinMode(d6,OUTPUT);
  pinMode(strobe,OUTPUT);
  digitalWrite(strobe,LOW);
  digitalWrite(d0,LOW);
  digitalWrite(d1,LOW);
  digitalWrite(d2,LOW);    
  digitalWrite(d3,LOW);    
  digitalWrite(d4,LOW);    
  digitalWrite(d5,LOW);    
  digitalWrite(d6,LOW);    

  Serial.println("Typing in code");

  for (int i=0; i<25; i++)
  {
    SendAscii(0x0D);
    delay(100);
  }
       
  String fileLine;
  char * fpath = "/apple1/";
  char * currfnam = "a1kara.txt";
  char * ffullpath = strcat(fpath, currfnam);
  File fFile = SD.open(ffullpath, FILE_READ);

  if (fFile) {
    while (fFile.available()) {
      fileLine = fFile.readStringUntil('\n');
      Serial.println(fileLine);
      for (int i=0; i < fileLine.length() ; i++)
      {
         SendAscii((int)fileLine[i]);
      }
      SendAscii(0x0D);
      Serial.println();
      long tgttime = millis() + 250;  //delay between lines
      while (millis() < tgttime)
      {
      }
    }
  }
  else {
        Serial.println("ERROR opening file");
  }
  fFile.close();

  delay(1000);
  for (int i=0; i<127; i++)
  {
    SendAscii(i);
  }
  SendAscii(0x0D);
  delay(1000);
  // processing resumes with loop()  
 }
 


void SendAscii(int asciinum)
{
  int testval = asciinum;

  int bit8 = (testval >= 128);
  testval = testval - 128 * bit8;

  int bit7 = (testval >= 64);
  testval = testval - 64 * bit7;
  digitalWrite(d6, bit7);

  int bit6 = (testval >= 32);
  testval = testval - 32 * bit6;
  digitalWrite(d5, bit6);

  int bit5 = (testval >= 16);
  testval = testval - 16 * bit5;
  digitalWrite(d4, bit5);

  int bit4 = (testval >= 8);
  testval = testval - 8 * bit4;
  digitalWrite(d3, bit4);

  int bit3 = (testval >= 4);
  testval = testval - 4 * bit3;
  digitalWrite(d2, bit3);

  int bit2 = (testval >= 2);
  testval = testval - 2 * bit2;
  digitalWrite(d1, bit2);

  int bit1 = (testval >= 1);
  testval = testval - 1 * bit1;
  digitalWrite(d0, bit1);

  digitalWrite(strobe,HIGH);
  long tgttime = micros() + 10; // strobe pulse is 10 microseconds
  while (micros() < tgttime)
  { 
  }
  digitalWrite(strobe,LOW);
  tgttime = millis()+15; // delay between characters - configured for Replica 1+
  while (millis() < tgttime)
  { 
  }
}


void SendAllNotesOff()
{
  int savedebug = debugSong;
  debugSong = false;
  for (int channel = 0; channel < 16; channel++)
  {
    midiShortMsg(0xB0+channel,120,0); // all sounds off
    delay(1);
    midiShortMsg(0xB0+channel,123,0); // all notes off
    delay(1);
    for (int note = 00; note < 128; note++) 
    {
      midiShortMsg(0x90+channel, note, 0x00);  
      delay(1); 
    }
  }
  debugSong = savedebug;
}

void logs(String thestring) {
  if(!debugSong)
    return;
  
  Serial.println(thestring);
}

void logi(String label, int data) {
  if(!debugSong)
    return;
  
  Serial.print(label);
  Serial.print(": ");
  Serial.println(data);
}

void logl(String label, long data) {
  if(!debugSong)
    return;
  
  Serial.print(label);
  Serial.print(": ");
  Serial.println(data,HEX);
}


void logDivision(boolean major) {
  if(!debugSong)
    return;
  
  if(major) {
    Serial.println("===========================");    
  }
  else {
    Serial.println("----------------------");
  }
}


int readInt()
{
  return readByte() << 8 | readByte();
}


long readLong() {
  return (long) readByte() << 24 | (long) readByte() << 16 | (long)readByte() << 8 | (long)readByte();
}


byte getLastByte()
{
  return currentByte;
}

byte readByte()
{
  ReadMidiByte();
  return currentByte;
}


void ReadMidiByte()
{
  if (!file_opened)
  {
     if (thefile = SD.open(currentSong,FILE_READ))
     {
        logs("Opened file");
        file_opened = true;
        ReadNextBlock();
     }
  }
  previousByte = currentByte;
  currentByte = buf1[bufIndex];
  bufIndex++;
  if (bufIndex >= bytesread1)
  {
       if (last_block)
       {
          thefile.close();   
          file_closed = true;
          logs("\nDone");         
       }
       else
       {
           ReadNextBlock();
       }     
  }
}

void ReadNextBlock()
{
        bytesread1 = thefile.read(buf1,bufsiz);
        bufIndex = 0;
        if (bytesread1 < bufsiz)
        {
           logs("Last Block");
           last_block = true;
        }
        else
        {
          logi("BUF1 Bytes read",bytesread1);
        }
}

void processByte(uint8_t b)
{
  if (debugSong)
  {
    Serial.print(b,HEX);
    Serial.print(" ");
  }
}

void processChunk() {
  boolean valid = true;
  
  long chunkID = readLong();
  long size = readLong();
  
  logDivision(true);
  logi("Chunk ID", chunkID);
  logl("Chunk Size", size);
  
  if(chunkID == HEADER_CHUNK_ID) {
    processHeader(size);
    
    logi("File format", getFileFormat());
    logi("Track count", getTrackCount());
    logi("Time division", getTimeDivision());
  }
  else if(chunkID == TRACK_CHUNK_ID) {
    processTrack(size);
  }
}


/*
 * Parses useful information out of the MIDI file header.
 */
void processHeader(long size) {
  // size should always be 6
  // do we want to bother checking?
  
  format = readInt();
  trackCount = readInt();
  timeDivision = readInt();
  
  //logs("Processed header info.");
}

int getFileFormat() {
  return format;
}

int getTrackCount() {
  return trackCount;
}

int getTimeDivision() {
  return timeDivision;
}

/*
 * Loops through a track of the MIDI file, processing the data as it goes.
 */

void processTrack(long size) {
  long counter = 0;

  while(counter < size) {
    //logl("Track counter", counter);
    counter += processEvent();
    counter = checkButtons(counter,size);
  }
}

long checkButtons(long acounter, long asize) 
{
    lcd_key = read_LCD_buttons();  // read the buttons
    int bouncedly = 250;
 
    switch (lcd_key)               // depending on which button was pushed, we perform an action
    {     
      case btnNONE:
      {
        break;
      }
      case btnSELECT:
      {
        lcd.setCursor(15,0);
        lcd.print("S");
        SendAllNotesOff();
        lcd.setCursor(15,0);
        lcd.print(" ");
        lcd.setCursor(0,0);
        lcd.print("U/D:mode R:opts ");
        lcd.setCursor(0,1);
        lcd.print("L:rewind S:done ");
        
        bool selectMode = true;
        
        while(selectMode) 
        {
          lcd_key = read_LCD_buttons();  // read the buttons
          delay(bouncedly);
          switch(lcd_key)
          {
            case btnSELECT:
            {
                lcd.setCursor(15,0);
                lcd.print("S");
                selectMode = false;
                break;
            }
            case btnLEFT:
            {
                lcd.setCursor(15,0);
                lcd.print("L");
                acounter = 0;
                selectMode = false;
                break;
            }
          }
              
        }
        break;
      }
      case btnRIGHT:
      {
        lcd.setCursor(15,0);
        lcd.print("R");
        lcd.setCursor(13,1);
        lcd.print(currentSongNum);
        delay(bouncedly);
        
        acounter = asize;  // currentSongNum will be automatically incremented

        lcd.setCursor(15,0);
        lcd.print(">");
        lcd.setCursor(13,1);
        lcd.print(currentSongNum + 1);
        
        int held = 1;
        while (held)
        {
          lcd_key = read_LCD_buttons();  // read the buttons
          switch(lcd_key)
          {
            case btnRIGHT:
            {
                currentSongNum++;
                lcd.setCursor(13,1);
                lcd.print(currentSongNum + 1);
                delay(bouncedly);
                break;
            }
            case btnNONE:
            {
                held = 0;
                break;
            }
          } // switch
        } // while

        break;
      }
      case btnLEFT:
      {
        lcd.setCursor(15,0);
        lcd.print("L");
        lcd.setCursor(13,1);
        lcd.print(currentSongNum - 1);
        delay(bouncedly);
        
        currentSongNum--;
        currentSongNum--;
        if (currentSongNum < 0)
        {
          currentSongNum = 0;
        }
        acounter = asize;

        lcd.setCursor(15,0);
        lcd.print("<");
        lcd.setCursor(13,1);
        lcd.print(currentSongNum);

        int held = 1;
        while (held)
        {
          lcd_key = read_LCD_buttons();  // read the buttons
          switch(lcd_key)
          {
            case btnDOWN:
            {
                currentSongNum--;
                if (currentSongNum < 0)
                {
                    currentSongNum = 0;
                }
                lcd.setCursor(13,1);
                lcd.print(dirnum);
                delay(bouncedly);
                break;
            }
            case btnNONE:
            {
                held = 0;
                break;
            }
          } // switch
        } // while
        
        break;
      }
      
      case btnUP:
      {
        lcd.setCursor(15,0);
        lcd.print("U");
        lcd.setCursor(13,1);
        lcd.print(dirnum);
        delay(bouncedly);

        dirnum++;
        currentSongNum = 0;
        acounter = asize;
        
        lcd.setCursor(15,0);
        lcd.print(">");
        lcd.setCursor(13,1);
        lcd.print(dirnum);
        
        int held = 1;
        while (held)
        {
          lcd_key = read_LCD_buttons();  // read the buttons
          switch(lcd_key)
          {
            case btnUP:
            {
                dirnum++;
                lcd.setCursor(13,1);
                lcd.print(dirnum);
                delay(bouncedly);
                break;
            }
            case btnNONE:
            {
                held = 0;
                break;
            }
          } // switch
        } // while
        break;
      }
      
      case btnDOWN:
      {
        lcd.setCursor(15,0);
        lcd.print("D");
        delay(bouncedly);
        lcd.setCursor(13,1);
        lcd.print(dirnum);
        
        dirnum--;
        if (dirnum < 0)
        {
          dirnum = 0;
        }
        currentSongNum = 0;
        acounter = asize;

        lcd.setCursor(15,0);
        lcd.print("<");
        lcd.setCursor(13,1);
        lcd.print(dirnum);

        int held = 1;
        while (held)
        {
          lcd_key = read_LCD_buttons();  // read the buttons
          switch(lcd_key)
          {
            case btnDOWN:
            {
                dirnum--;
                if (dirnum < 0)
                {
                    dirnum = 0;
                }
                lcd.setCursor(13,1);
                lcd.print(dirnum);
                delay(bouncedly);
                break;
            }
            case btnNONE:
            {
                held = 0;
                break;
            }
          } // switch
        } // while

        break;
      }

    } //switch
    return acounter;
}

// read the buttons
int read_LCD_buttons()
{
 adc_key_in = analogRead(0);      // read the value from the sensor
 // my buttons when read are centered at these valies: 0, 144, 329, 504, 741
 // we add approx 50 to those values and check to see if we are close
 if (adc_key_in > 1000) return btnNONE; // We make this the 1st option for speed reasons since it will be the most likely result
 if (adc_key_in < 50)   return btnRIGHT; 
 if (adc_key_in < 195)  return btnUP;
 if (adc_key_in < 380)  return btnDOWN;
 if (adc_key_in < 555)  return btnLEFT;
 if (adc_key_in < 790)  return btnSELECT;  
 return btnNONE;  // when all others fail, return this...
}


/*
 * Reads an event type code from the currently open file, and handles it accordingly.
 */
int processEvent() {
  //logDivision(false);
  
  int counter = 0;
  deltaTime = 0;
  
  int b;
  
  do {
    b = readByte();
    counter++;
    
    deltaTime = (deltaTime << 7) | (b & DELTA_TIME_VALUE_MASK);
  } while((b & DELTA_TIME_END_MASK) == DELTA_TIME_END_VALUE);
  
  //logi("Delta", deltaTime);
  
  b = readByte();
    counter++;

  boolean runningMode = true;
  // New events will always have a most significant bit of 1
  // Otherwise, we operate in 'running mode', whereby the last
  // event command is assumed and only the parameters follow
  if(b >= 128) {
    eventType = (b & EVENT_TYPE_MASK) >> 4;
    eventChannel = b & EVENT_CHANNEL_MASK;
    runningMode = false;
  }
  
  //logi("Event type", eventType);
  //logi("Event channel", eventChannel);
  
  // handle meta-events and track events separately
  if(eventType == (META_EVENT_TYPE & EVENT_TYPE_MASK) >> 4
     && eventChannel == (META_EVENT_TYPE & EVENT_CHANNEL_MASK)) {
    counter += processMetaEvent();
  }
  else {
    counter += processTrackEvent(runningMode, b);
  }
  
  return counter;
}

/*
 * Reads a meta-event command and size from the file, performing the appropriate action
 * for the command.  Currently, only tempo changes are handled
 */
int processMetaEvent() {
  int command = readByte();
  int size = readByte();
  
  //logi("Meta event length", size);
  
  for(int i = 0; i < size; i++) {
    byte data = readByte();
    
    switch(command) {
      case META_SET_TEMPO_COMMAND:
      {
        processTempoEvent(i, data);
        break;
      }
      case META_LYRICS_COMMAND:
      {
        processLyricEvent (i, data);
        break;
      }
    }
  }
  
  return size + 2;
}

void processLyricEvent(int paramIndex, byte param) {
  char lyric = (char)param;
  if (lyric == '/') //PG Music files use / for end of line
  {
    SendAscii(0x0D); // Want to read forward in lyric buffer and print next line of lyrics, CR, and use bouncing ball character as midi lyrics are received
  }
  else if (lyric == '\\') // PG Music files use \ for end of paragraph
  {
    SendAscii(0x0D);  //Want to redraw screen with next paragraph in top portion of screen
    SendAscii(0x0d);
  }
  else
  {
    SendAscii(param);
  }
}

/*
 * Reads a track event from the file, either as a full event or in running mode (to
 * be determined automatically), and takes appropriate playback action.
 */
int processTrackEvent(boolean runningMode, int lastByte) {
  int count = 0;
  
  if(runningMode) {
    parameter1 = getLastByte();
  }
  else {
    parameter1 = readByte(); 
    count++;
  }
  
  //logi("Parameter 1", parameter1);
  
  int eventShift = eventType << 4;

  parameter2 = -2;  
  switch(eventShift) {
    case NOTE_OFF_EVENT_TYPE:
    case NOTE_ON_EVENT_TYPE:
    case KEY_AFTERTOUCH_EVENT_TYPE:
    case CONTROL_CHANGE_EVENT_TYPE:
    case PITCH_WHEEL_CHANGE_EVENT_TYPE:
    default:
      parameter2 = readByte();
      count++;

      //logi("Parameter 2", parameter2);
      
      break;
    case PROGRAM_CHANGE_EVENT_TYPE:
    case CHANNEL_AFTERTOUCH_EVENT_TYPE:
      parameter2 = -1;
      break;
  }
  
  if (parameter2 >= 0)
  {
    processMidiEvent(deltaTime, eventType*16+eventChannel, parameter1, parameter2);
  }
  else if (parameter2 == -1)
  {
    process2ByteMidiEvent(deltaTime, eventType*16+eventChannel, parameter1);
  }
  else {
    addDelta(deltaTime);
  }
  
  return count;
}


/*
 * Handles a tempo event with the given values.
 */
void processTempoEvent(int paramIndex, byte param) {
  byte bits = 16 - 8*paramIndex;
  microseconds = (microseconds & ~((long) 0xFF << bits)) | ((long) param << bits);
  
  //Serial.print("TEMPO:");
  //Serial.println(microseconds);
}
  
long getMicrosecondsPerQuarterNote() {
  return microseconds;
}

void addDelta(unsigned long delta) {
  accDelta = accDelta + delta;
}

void resetDelta() {
  accDelta = 0;
}

void processMidiEvent(unsigned long delta, int channel, int note, int velocity) {
  addDelta(delta);
  
  playback(channel, note, velocity, accDelta);
  index++;
  
  resetDelta();
}

void process2ByteMidiEvent(unsigned long delta, int channel, int value) {
  addDelta(delta);
  
  playback(channel, value, -1, accDelta);
  index++;
  
  resetDelta();
}


void playback(int channel, int note, int velocity, unsigned long delta) {
  unsigned long deltaMillis = (delta * getMicrosecondsPerQuarterNote()) / (((long) getTimeDivision()) * 1000);
  
  if(firstNote) {
    firstNote = false;
  }
  else {
    unsigned long currMillis = millis();
    
    int vel2use = 255;
    
    while(currMillis < lastMillis + deltaMillis)
    {
      //delay(lastMillis - currMillis + deltaMillis);
      int dly2use = 1;
      
      //Prevent display from interfering with timing of music
      if (lastMillis + deltaMillis - currMillis < 12)
      {
        dly2use= 0;
      }
      currMillis = millis();  
    }      
  }

  if (velocity < 0)
  {
      midi2ByteMsg (channel, note);
  }
  else
  {  
      midiShortMsg (channel, note, velocity);
  } 
  lastMillis = millis();
}

void midiShortMsg(int cmd, int pitch, int velocity) {  
  if (debugSong)
  {
    Serial.print(cmd,HEX);
    Serial.print(" ");
    Serial.print(pitch,HEX);
    Serial.print(" ");
    Serial.print(velocity,HEX);
    Serial.print(" ");
  }
  else
  {
    Serial1.write(cmd);
    Serial1.write(pitch);
    Serial1.write(velocity);
  }
}

void midi2ByteMsg(int cmd, int value) {
  Serial1.write(cmd);
  Serial1.write(value);
}

void loop()
{   
  lcd.setCursor(0,0);
  lcd.print("Mode: Loop Dir  "); 

 if (debugSong)
 {
     Serial.print("TOP OF LOOP: dirnum=");
     Serial.println(dirnum);
 }
 SendAllNotesOff();
 int currentDirNum = dirnum;

 if (currentDirNum > 0)
 {   
     currentSong[0] = '/';
     currentSong[3] = currentDirNum % 10 + '0';
     int tempDirNum  = currentDirNum / 10;
     currentSong[2] = tempDirNum % 10 + '0';
     tempDirNum  = tempDirNum / 10;
     currentSong[1] = tempDirNum % 10 + '0';
     currentSong[4] = '/';  
     
     currentSong[7] = currentSongNum % 10 + '0';
     int tempSongNum = currentSongNum / 10;
     currentSong[6] = tempSongNum % 10 + '0';
     tempSongNum = tempSongNum / 10;
     currentSong[5] = tempSongNum % 10 + '0';
     
     currentSong[8] = '.';
     currentSong[9] = 'M';
     currentSong[10] = 'I';
     currentSong[11] = 'D';
     currentSong[12] = '\0';
     
     if (debugSong)
     {
       Serial.print("Current Song #: ");
       Serial.println(currentSong);
     }

     lcd.setCursor(0,1);
     lcd.print(currentSong);
     lcd.print("    ");

     bool file_was_found = false;
     // test for existence of file - if it opens, close it - it will be reopened when chunks processed
     if (thefile = SD.open(currentSong,FILE_READ))
     {
       thefile.close();
       file_was_found = true;
       Serial.println("FILE FOUND ");
     }
     else
     {
       if (debugSong)
       {
           Serial.println("FILE NOT FOUND ");       
       }
       lcd.setCursor(13,1);
       lcd.print("NTF");
       file_closed = 1;
       checkButtons(0,1);
     }
     
     //Phase processing
     while (!file_closed)
     {
        processChunk(); // header chunk
        
        if(getFileFormat() == 0) {
          logs("MIDI file format = 0");
          int trackCount = getTrackCount();
          logi("Track Count=",trackCount);

          // Uncomment to debug song buffer contents
          
          if (debugSong)
          {
            file_closed = true;
            for (int i=0; i<SD_BUFFER_SIZE; i++)
            {
              int b=buf1[i];
              Serial.print(b,HEX);
              Serial.print(" ");
              if (((i+1)/16)==((i+1)%16))
              {
                Serial.println();
              }
            }
          }
          else
          {
            for(int i = 0; i < getTrackCount(); i++) 
            {
              processChunk();
            }
            file_closed = true;
          }          
        }
        else 
        { 
          logs("MIDI file not format 0.");
        }
     } // while not file_closed

     if (file_was_found)
     {
        currentSongNum++; // will play next song in playlist
     }
     else
     {
        currentSongNum = 1; // will repeat playlist from first song
     }
     if (debugSong)
     {
        Serial.print("currentSongNum=");
        Serial.println(currentSongNum);
     }
     resetGlobalVars();
        
   } // if currentDirNum > 0
 } // loop()


void resetGlobalVars()
{  
  file_opened = false;
  last_block = false;
  file_closed = false;
  bufsiz=SD_BUFFER_SIZE;
  bytesread1 = 0;
  bufIndex = 0;
  currentByte = 0;
  previousByte = 0;

  format = 0;
  trackCount = 0;
  timeDivision = 0;

  deltaTime = 0;
  eventType = 0;
  eventChannel = 0;
  parameter1 = 0;
  parameter2 = 0;

  microseconds = 500000;
  index = 0;
  accDelta = 0;

  firstNote = true;
  currFreq = -1;
  lastMillis = 0;  
}


