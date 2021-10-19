/*
 Name:    Demo_3.ino
 Created: 30-Mar-21 2:25:55 AM
 Author:  BumbleBee

 Description: Implementing the Device with Midi library (for MIDI HID Compliance)
*/

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <MIDI.h>
#include <Wire.h>
#include <EEPROM.h>
#include <OneButton.h>
#include <Encoder.h>
#include <stdio.h>

//COMMON VARIABLES
#define MAX_PATCHES 92        //max 92 for 16 pads
#define MAX_PADS 16
#define MAX_SETTINGS 4
#define MAX_GENERAL_SETTINGS 2

#define PIN_MASTER_VOLUME A0
#define ENCODER_STEPS 20

#define SCROLL_SPEED 50   // (1 is fastest) default 1500
#define PIN_SELECT 4    //pin on Arduino
#define PIN_MINUS 6     //pin on Arduino
#define PIN_PLUS 5      //pin on Arduino

//END OF COMMON VARIABLES

#define NOTE_DURATION 10 //cycles for note is 2^NOTE_DURATION so 10->1024 cycles 

#define MIDI_CMD_NOTE_ON 0x90
#define MIDI_CMD_NOTE_OFF 0x80
#define MIDI_CMD_PITCH_BEND 0xE0

//MIDI
MIDI_CREATE_DEFAULT_INSTANCE();

 //EEPROM
#define EEPROM_chipAddress 0x50
#define EEPROM_LPATCH_ADDRESS 0x1761
#define EEPROM_GENERAL_ADDRESS_OFFSET 0x1750

//Volume

Encoder knob1(2, 3);
long oldPosition = -999;  //knob default position

volatile int minus_tmp = 0; //global variables for interrrupts
volatile int plus_tmp = 0;

OneButton btnSelect(PIN_SELECT, true);
OneButton btnMinus(PIN_MINUS, false);
OneButton btnPlus(PIN_PLUS, false);


#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

//OLED
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define LOGO_HEIGHT   16
#define LOGO_WIDTH    16

int mini[MAX_PADS][MAX_SETTINGS];
int general[MAX_GENERAL_SETTINGS];

int settings_minmax[2][MAX_SETTINGS] = {
  //page1   page2 page3 page4
  {0,   0,    0,  0}, //min (17000 for do not print)
  {127, 127,  100,100}    //max (17000 for do not print)
};

#define GENERAL_CHANNEL 0


int general_minmax[2][MAX_GENERAL_SETTINGS] = {
  //pitchbend channel
  {1,   2},
  {16380, 16}
};

#define MAX_LIST_PADS MAX_PADS
/*char* list_of_pads[MAX_LIST_PADS] =
{
  "PAD 1",  "PAD 2",  "PAD 3",  "PAD 4",
  "PAD 5",  "PAD 6" 
};
*/


#define MAX_LIST_SETTINGS_GENERAL 2
char* list_of_settings_general[MAX_LIST_SETTINGS_GENERAL] =
{
  "PITCHBEND",
  "CHANNEL OUT"
};
#define PAGE_PITCHBEND 0

#define SETTING_NOTE 0
#define SETTING_VELOCITY 1
#define SETTING_SENSITIVITY 2
#define SETTING_THRESHOLD 3

#define MAX_LIST_SETTINGS_SETTINGS MAX_SETTINGS
char* list_of_settings_settings[MAX_LIST_SETTINGS_SETTINGS] = {
  "NOTE",
  "VELOCITY",
  "SENSITIVITY",
  "THRESHOLD" };


#define MAX_LIST_SETTINGS MAX_PATCHES+1
/*har* list_of_settings[MAX_LIST_SETTINGS] =
{
  "GENERAL",
  "PATCH 1",  "PATCH 2",  "PATCH 3",  "PATCH 4",  "PATCH 5",  "PATCH 6",  "PATCH 7",  "PATCH 8",  "PATCH 9",  "PATCH 10",
  "PATCH 11", "PATCH 12", "PATCH 13", "PATCH 14", "PATCH 15", "PATCH 16", "PATCH 17", "PATCH 18", "PATCH 19", "PATCH 20",
  "PATCH 21", "PATCH 22", "PATCH 23", "PATCH 24", "PATCH 25", "PATCH 26", "PATCH 27", "PATCH 28", "PATCH 29", "PATCH 30",
  "PATCH 31", "PATCH 32", "PATCH 33", "PATCH 34", "PATCH 35", "PATCH 36", "PATCH 37", "PATCH 38", "PATCH 39", "PATCH 40",
  "PATCH 41", "PATCH 42", "PATCH 43", "PATCH 44", "PATCH 45", "PATCH 46", "PATCH 47", "PATCH 48", "PATCH 49", "PATCH 50",
  "PATCH 51", "PATCH 52", "PATCH 53", "PATCH 54", "PATCH 55", "PATCH 56", "PATCH 57", "PATCH 58", "PATCH 59", "PATCH 60",
  "PATCH 61", "PATCH 62", "PATCH 63", "PATCH 64", "PATCH 65", "PATCH 66", "PATCH 67", "PATCH 68", "PATCH 69", "PATCH 70",
  "PATCH 71", "PATCH 72", "PATCH 73", "PATCH 74", "PATCH 75", "PATCH 76", "PATCH 77", "PATCH 78", "PATCH 79", "PATCH 80",
  "PATCH 81", "PATCH 82", "PATCH 83", "PATCH 84", "PATCH 85", "PATCH 86", "PATCH 87", "PATCH 88", "PATCH 89", "PATCH 90",
  "PATCH 91", "PATCH 92"};
*/

#define BMP_NOTE_HEIGHT 16
#define BMP_NOTE_WIDTH 16
static const uint8_t PROGMEM bmp_note[] =
{
  B00000000, B00000000,
  B00000000, B00000011,
  B00000011, B11111111,
  B01111111, B11111110,
  B01111111, B11111110,
  B00111000, B00000100,
  B00011000, B00001100,
  B00011000, B00001000,
  B00010000, B00001000,
  B00110000, B00011000,
  B00110000, B00111100,
  B01111000, B01011100,
  B10111000, B01111100,
  B11011000, B00111000,
  B01111000, B00000000,
  B00000000, B00000000 };


#define BMP_GEAR_HEIGHT 16
#define BMP_GEAR_WIDTH 16
static const uint8_t PROGMEM bmp_gear[] =
{
  B11100001, B10000111,
  B11110011, B11001111,
  B11111111, B11111111,
  B01111111, B11111110,
  B00111111, B11111100,
  B00111110, B01111100,
  B01111100, B00111110,
  B11111000, B00011111, //mid
  B11111100, B00111111,
  B01111110, B01111110,
  B00111111, B11111100,
  B00111111, B11111100,
  B01111111, B11111110,
  B11111111, B11111111,
  B11110011, B11001111,
  B11100001, B10000111 };


//Variables
static byte last_status = 0; //for MIDI
int prev_pb; //for pitchbend
int Lpatch; //for lastpatch from eeprom
//Forward Declarations

//to decide which level and page to display
void load_screen(int level, int sel_page, int sel_patch, int sel_pad);

//to update pad settings on change of sel_patch
void update_pad_settings();


/*
EEPROM Handlers:
read_setting_from_EEPROM
load_patch_from_EEPROM <- will be useful when patches are selected (for playing)
update_setting_in_EEPROM <- for updating settings after editmode
*/

/*MIDI Handlers
*
*send_system_message
*send_channel_message
*get_databyte
*
*/
void send_system_message(
  int num_data_bytes,
  byte* data)
{
  // send the right number of data bytes
  for (int i = 0; i < num_data_bytes; i++)
  {
    Serial.write(data[i]);
  }

}

void send_channel_message(
  byte command,
  byte channel,
  int num_data_bytes,
  byte* data)
{
  // We are assuming that all of the inputs are in a valid range:
  //  Acceptable commands are 0x80 to 0xF0.
  // Channels must be between 1 and 16.
  // num_data_bytes should be either 1 or 2
  // data should be an array of 1 or 2 bytes, with each element constrained 
  // to the 0x0 to 0x7f range.
  //
  // More realistic code might validate each input and return an error if out of range

  byte first;

  // Combine MS-nybble of command with channel nybble.
  first = (command & 0xf0) | ((channel - 1) & 0x0f);

  if (first != last_status)
  {
    Serial.write(first);
    last_status = first;
  }

  // Then send the right number of data bytes
  for (int i = 0; i < num_data_bytes; i++)
  {
    Serial.write(data[i]);
  }
}

//Pitchbend not in class 
void extpitchBend(int pb) {
  float pitchbend = pb / 8191;
  if (pitchbend > 1) pitchbend = 1;
  if (pitchbend < 0) pitchbend = 0;

  MIDI.sendPitchBend(pitchbend, general[GENERAL_CHANNEL]);
  /*
  if (pb != prev_pb) {
    prev_pb = pb;

    byte extpitchbend[2] = { pb & 0x7F,
                  pb >> 7
    };//see pitchbendhelp.png

    //Serial.print("extPitchBend");
    send_channel_message(MIDI_CMD_PITCH_BEND, general[GENERAL_CHANNEL], sizeof(extpitchbend), extpitchbend);
  }
  */
}

byte get_databyte(
  int patch,
  int pad,
  int setting,
  int databyte) {


  /*
  patch:    0 - MAX_PATCHES - 1
  pad:    1 - MAX_PADS
  setting:  0 - MAX_SETTINGS - 1
  databyte: 0 - 1 (The databyte's index)
  */

  if (databyte > 1) return 0x6A; //ERRORCODE

  int tmp_setting = mini[pad - 1][setting];
  //int tmp_setting = mini[pad-1][setting];

  /*
  Serial.println("DEBUG-get_databyte");
  Serial.print("patch:");
  Serial.println(patch);
  Serial.print("pad:");
  Serial.println(pad);
  Serial.print("setting:");
  Serial.println(setting);
  Serial.print("databyte:");
  Serial.println(databyte);
  Serial.print("tmp_setting:");
  Serial.println(tmp_setting);
  Serial.print("tmp_setting & 127:");
  Serial.println(tmp_setting & 127);
  */

  byte tmp = 0x69; //ERRORCODE
  if (databyte == 0) {
    tmp = tmp_setting & 127;
  }
  else if (databyte == 1) {
    tmp = (tmp_setting >> 7) & 127;
  }
  return tmp;
}


//CLASSES
class storage {
public:

  //LAST PATCH < TESTED
  byte get_lastpatch() {

    int address = EEPROM_LPATCH_ADDRESS;

    Wire.beginTransmission(EEPROM_chipAddress);
    Wire.write((int)address >> 8);
    Wire.write((int)address & 0xFF);
    Wire.endTransmission();

    Wire.requestFrom(EEPROM_chipAddress, 1);
    if (Wire.available()) {
      return Wire.read();
    }
  }

  void update_lastpatch(byte new_patch) {

    byte value = new_patch & 0xFF; //converts the input to a byte

    /*debug
    Serial.print("*[");
    Serial.print(patch);
    Serial.print(",");
    Serial.print(pad);
    Serial.print(",");
    Serial.print(setting);
    Serial.print("]:");
    Serial.println(setting + pad * (MAX_SETTINGS)+patch * (MAX_PADS * MAX_SETTINGS));
    */

    int address = EEPROM_LPATCH_ADDRESS;
    //update to eeprom
    //EEPROM.update(address, value);

    Wire.beginTransmission(EEPROM_chipAddress);

    Wire.write((int)address >> 8);
    Wire.write((int)address & 0xFF);
    Wire.write(new_patch & 0xFF);

    Wire.endTransmission();

    delay(15); //minimum 10ms
  }




  //PATCH SETTINGS
  int load_patchsettings(int patch) {

    //validating input
    if (patch > MAX_PATCHES) return 1;

    int address = patch * (MAX_PADS * MAX_SETTINGS);

    for (int j = 0; j < MAX_PADS; j++) {
      for (int k = 0; k < MAX_SETTINGS; k++) {
        //Serial.print("Loading address:");
        //Serial.println(address);
        Wire.beginTransmission(EEPROM_chipAddress);
        Wire.write((int)address >> 8);
        Wire.write((int)address & 0xFF);
        Wire.endTransmission();

        Wire.requestFrom(EEPROM_chipAddress, 1);
        if (Wire.available()) {
          mini[j][k] = Wire.read();
        }
        //mini[j][k] = master[patch][j][k];
        address += 1;
      }

    }

    return 0;
  }

  int update_patchsetting_in_EEPROM(int patch, int pad, int setting, int data) {

    //validating input
    if (patch > MAX_PATCHES)  return 1;
    if (pad > MAX_PADS)     return 2;
    if (setting > MAX_SETTINGS) return 3;
    if (data > 127)       return 4;


    byte value = data & 0xFF; //converts the input to a byte

    /*debug
    Serial.print("*[");
    Serial.print(patch);
    Serial.print(",");
    Serial.print(pad);
    Serial.print(",");
    Serial.print(setting);
    Serial.print("]:");
    Serial.println(setting + pad * (MAX_SETTINGS)+patch * (MAX_PADS * MAX_SETTINGS));
    */

    int address = setting + pad * (MAX_SETTINGS)+patch * (MAX_PADS * MAX_SETTINGS);
    //update to eeprom
    //EEPROM.update(address, value);

    Wire.beginTransmission(EEPROM_chipAddress);

    Wire.write((int)address >> 8);
    Wire.write((int)address & 0xFF);
    Wire.write(data & 0xFF);

    Wire.endTransmission();

    delay(15); //minimum 10ms
    return 0;
  }




  //GENERAL SETTINGS < TESTED
  void load_to_general_from_EEPROM() {

    int address = EEPROM_GENERAL_ADDRESS_OFFSET;
    int tmp;
    byte byte1;

    for (int j = 0; j < MAX_GENERAL_SETTINGS; j++) {

      /*//debug
      Serial.print("EEPROM reading address:");
      Serial.println(address);
      */
      Wire.beginTransmission(EEPROM_chipAddress);
      Wire.write((int)address >> 8);
      Wire.write((int)address & 0xFF);
      Wire.endTransmission();

      Wire.requestFrom(EEPROM_chipAddress, 1);
      if (Wire.available()) {
        byte1 = Wire.read();
      }


      if (j == PAGE_PITCHBEND) {
        byte byte2;

        address = address + 1;                    //set address to get next byte

        Wire.beginTransmission(EEPROM_chipAddress);   //get next byte
        Wire.write((int)address >> 8);
        Wire.write((int)address & 0xFF);
        Wire.endTransmission();

        Wire.requestFrom(EEPROM_chipAddress, 1);
        if (Wire.available()) {
          byte2 = Wire.read();
        }

        general[j] = (((unsigned)byte1 & 0xFF) << 8) | (byte2 & 0xFF); //UNCHECKED: combine bytes to make an integer
        /*
        Serial.print("pb_byte1:");
        Serial.print(byte1);
        Serial.print(",pb_byte2:");
        Serial.print(byte2);
        Serial.print(": combined pb:");
        Serial.println(general[j]);*/
      }
      else {
        general[j] = byte1;
        /*
        Serial.print(" | Value:");
        Serial.println(general[j]);*/
      }

      address++;
    }
  }

  void update_general_in_EEPROM(int setting, int data) {

    //validating input
    if (setting > MAX_GENERAL_SETTINGS)   return;
    if (data > 16381) return;

    int address = EEPROM_GENERAL_ADDRESS_OFFSET + checkpb(setting);

    /*
    Serial.print("EEPROM reading address:");
    Serial.println(address);
    */

    //Handles storing the MSB of integer (genius work)
    if (setting == PAGE_PITCHBEND) {
      int valueMSB = data >> 8;         //get MSB value
      /*
      Serial.print("value written valueMSB:");
      Serial.println(valueMSB);
      */
      Wire.beginTransmission(EEPROM_chipAddress);

      Wire.write((int)address >> 8);        //write the MSB value to setting's address
      Wire.write((int)address & 0xFF);
      Wire.write(data >> 8);
      Wire.endTransmission();
      delay(15); //minimum 10ms

      address = address + 1;
      //increments the address for the LSB to write its value
      /*
      Serial.print("EEPROM reading address:");
      Serial.println(address);
      */
    }

    byte valueLSB = data & 0xFF; //converts the input to a byte
    /*
    Serial.print("value written valueLSB:");
    Serial.println(valueLSB);
    */
    Wire.beginTransmission(EEPROM_chipAddress);

    Wire.write((int)address >> 8);
    Wire.write((int)address & 0xFF);
    Wire.write(data & 0xFF);

    Wire.endTransmission();
    delay(15); //minimum 10ms
  }

  int checkpb(int setting) {
    //adjusts the setting to point to correct address cause pitchbend takes 2 bytes
    if (setting > PAGE_PITCHBEND) {
      return setting + 1;
    }
    return setting;
  }
};
storage ROM;

class UpdateSetting {
public:
  int tmp;
  int max;
  int min;

  void update(int sel_patch, int sel_pad, int sel_setting) {
    //update to eeprom
    ROM.update_patchsetting_in_EEPROM(sel_patch, sel_pad, sel_setting, tmp);

    //update to master
    //master[sel_patch][sel_pad][sel_setting] = tmp;
    mini[sel_pad][sel_setting] = tmp;
  }
  void load(int sel_patch, int sel_pad, int sel_setting) {
    //tmp = master[sel_patch][sel_pad][sel_setting];
    tmp = mini[sel_pad][sel_setting];

    min = settings_minmax[0][sel_setting];
    max = settings_minmax[1][sel_setting];
    /*
    Serial.print("tmp:");
    Serial.print(tmp);
    Serial.print("Min:");
    Serial.print(min);
    Serial.print("Max:");
    Serial.println(max);*/
  }

  int subtract(int n) {
    tmp = tmp - n;

    if (tmp < min) tmp = min;

    return tmp;
  }

  int add(int n) {
    tmp = tmp + n;

    if (tmp > max) tmp = max;

    return tmp;
  }
};
UpdateSetting value;

class UpdateGeneral {
public:
  int tmp;
  int max;
  int min;
  int increment;

  void update(int setting) {
    //update to eeprom
    ROM.update_general_in_EEPROM(setting, tmp);

    //update to general
    general[setting] = tmp;
  }
  void load(int sel_setting) {
    tmp = general[sel_setting];

    min = general_minmax[0][sel_setting];
    max = general_minmax[1][sel_setting];

    //default increment
    increment = 1;

    //increment for large settings (pitchbend)
    if (max > 16000) { //500 is arbitrary
      increment = (max - min) / ENCODER_STEPS;
    }

    //Serial.print("tmp:");
    //Serial.print(tmp);
    //Serial.print("Min:");
    //Serial.print(min);
    //Serial.print("Max:");
    //Serial.print(max);
    //Serial.print("Increment:");
    //Serial.println(increment);
  }

  void subtract() {

    if (tmp <= increment) {
      tmp = min;
      return;
    }

    tmp = tmp - increment;
  }

  void add() {

    if (tmp >= max - increment) {
      tmp = max;
      return;
    }

    tmp = tmp + increment;
  }
};
UpdateGeneral gvalue;

class Navigation {
public:
  int level;
  int page;
  int edittype; //plus or minus ( not required 
  int pageMax[8]; //stores max no of pages
  bool editmode;  //editable or not

  int sel_patch;
  int sel_pad;
  int sel_setting; //this is basically page in current system

  void initPageMax() {
    pageMax[0] = { 2 };
    pageMax[1] = { MAX_LIST_SETTINGS };
    pageMax[2] = { MAX_LIST_PADS };
    pageMax[3] = { MAX_LIST_SETTINGS_SETTINGS };
    pageMax[4] = { MAX_LIST_SETTINGS_SETTINGS };
    pageMax[5] = { MAX_PATCHES };
    pageMax[6] = { MAX_LIST_SETTINGS_GENERAL };
    pageMax[7] = { MAX_LIST_SETTINGS_GENERAL };

  };

  void updatePage() {
    if (page < 0) page = 0;
    if (page > pageMax[level] - 1) page = pageMax[level] - 1;
  }

  void updatedata() {

    int prev_patch = sel_patch;

    switch (level) {

    case 5: { //Play page
      sel_patch = page;
      break;
    }
    case 1: {
      if (page != 0) sel_patch = page - 1; //cause first page is general
      break;
    }
    case 2: {
      sel_pad = page;
      break;
    }
    case 3: {
      sel_setting = page;

    }
    }

    if (prev_patch != sel_patch) {
      update_pad_settings();
    }
    /*
    Serial.println("Updating Data...");
    Serial.print("patch:");
    Serial.print(sel_patch);
    Serial.print("pad:");
    Serial.print(sel_pad);
    Serial.print("setting:");
    Serial.println(sel_setting);*/
  }

  //Pressed Select
  /* Keep this in mind
  level 0 - Home
  level 1 - Main Settings/Select Patch
  level 2 - Select Pad
  level 3 - Pad Settings
  level 4 - Edit Pad Settings
  level 5 - Play
  level 6 - General Settings
  level 7 - Edit General Settings
  */
  void nextLevel() {
    updatePage();
    updatedata();
    /*
    Serial.print("level");
    Serial.print(level);
    Serial.print("page");
    Serial.println(page);
    */
    if (level == 0 && page == 0) {
      level = 5;
      page = 0; //reset page
    }
    else if (level == 1 && page == 0) {
      level = 6;
      page = 0; //reset page
    }
    else if (level == 3) {
      level++;
    }
    else if (level == 4) {
      value.update(sel_patch, sel_pad, page);
      updatedata();
      previousLevel();
    }
    else if (level == 6) {
      level++;
    }
    else if (level == 7) { //
      gvalue.update(page);
      previousLevel();
    }
    else if (level == 5) { //Play 
    } //ends of treeload_screen(level,page,edittype);
    else {
      level++;
      page = 0; //reset page
    }
    /*
    Serial.print("Level:");
    Serial.print(level);
    Serial.print("page");
    Serial.println(page);
    */
    updateEditmode();

    load_screen(level, page, sel_patch, sel_pad);
  }

  //Pressed Back
  void previousLevel() {

    updatePage();

    /*
    Serial.print("level");
    Serial.print(level);
    Serial.print("page");
    Serial.println(page);
    */

    if (level == 6) {
      level = 1;
      page = 0;
    }
    else if (level == 5)
    {
      level = 0;
    }
    else if (level == 0)
    {
    }
    else
    {
      level--;
    }
    /*
    Serial.print("Level:");
    Serial.print(level);
    Serial.print("page");
    Serial.println(page);
    */
    updateEditmode();
    load_screen(level, page, sel_patch, sel_pad);
  }

  void updateEditmode() {
    editmode = false;

    if (level == 4) {
      editmode = true;
      value.load(sel_patch, sel_pad, sel_setting);
    }
    else if (level == 7) {
      editmode = true;
      gvalue.load(page);
      //load general settinig (another instance of updateValue class?
    }
  }

  //Pressed ^
  void up() {
    /*
    Serial.print("up.editmode:");
    Serial.println(editmode);
    Serial.print("up.Level:");
    Serial.println(level);
    */
    if (!editmode)
    {
      page++;
      updatePage();
    }
    else {
      if (level == 4) { //add 1 to pad setting
        value.add(1);
        value.update(sel_patch, sel_pad, page); //update to master
      }
      if (level == 7) {//add 1 to general setting
        gvalue.add();
        if (page == PAGE_PITCHBEND) {
          extpitchBend(gvalue.tmp);
        }
        gvalue.update(page); //update to master
      }
      update_pad_settings(); //update the note on pad classes
    }
    updatedata();
    load_screen(level, page, sel_patch, sel_pad);
  }

  //Pressed \/
  void down() {
    if (!editmode)
    {
      page--;
      updatePage();   //FIXED: WITHOUT this, the user can turn the wheel infinttely and will need to 
              //turn infinitely back to return to last page
              //FIXED: WITHOUT this, midi outputs pitchbend values. root cause unknown
    }
    else {
      if (level == 4) { //subtract 1 from pad setting
        value.subtract(1);
        value.update(sel_patch, sel_pad, page);
      }
      if (level == 7) {//edit general 

        gvalue.subtract();
        if (page == PAGE_PITCHBEND) {
          extpitchBend(gvalue.tmp);
        }
        gvalue.update(page);
      }
      update_pad_settings(); //update notes in pad classes
    }
    updatedata();
    load_screen(level, page, sel_patch, sel_pad);
  }

};
Navigation current;

class Pad {
public:

  int pin = 99;

  int pad;      // from initialization
  byte bnote;     //from master
  int maxVel;     //from master (max 127)
  int sensitivity;  //from master
  int threshold;    //from master
  int pitchbend;    //from general

  int velocity;   //calculated
  int maskTime;
  int scanTime;
  int piezoValue;

  long time_end;
  long time_hit;

  int noteCycles = 0;

  bool flag_hit = false;
  bool flag_pressed = false;

  /*---FUNCTIONS---*/
  int countBits(unsigned int number)
  {
    unsigned int count = 0;
    while (number)
    {
      count++;
      number >>= 1;
    }
    return count;
  }

  void pitchBend(int pb) {
    float pitchbend = pb / 8191;
    if (pitchbend > 1) pitchbend = 1;
    if (pitchbend < 0) pitchbend = 0;

    MIDI.sendPitchBend(pitchbend, general[GENERAL_CHANNEL]);
    /*
    byte pitchbend[2] = { pb & 0x7F ,
                pb >> 7
    };//see pitchbendhelp.png
    send_channel_message(MIDI_CMD_PITCH_BEND, general[GENERAL_CHANNEL], sizeof(pitchbend), pitchbend);*/
  }
  void noteOn() {

    /*
    Serial.print("pad");
    Serial.print(pad);
    Serial.print(".bnote:");
    Serial.println(bnote);

    Serial.print("pad");
    Serial.print(pad);
    Serial.print(".velocity:");
    Serial.println(velocity);

    Serial.print("pad");
    Serial.print(pad);
    Serial.print(".(byte)velocity:");
    Serial.println((byte)velocity);
    */

    MIDI.sendNoteOn(bnote, velocity, general[GENERAL_CHANNEL]);
    /*
    if ((byte)velocity == 0) return;

    byte noteon[2] = { bnote,     //note byte
              (byte)velocity }; //velocity
    send_channel_message(MIDI_CMD_NOTE_ON, general[GENERAL_CHANNEL], sizeof(noteon), noteon);
    */
  }
  void noteOff() {
    /*
    Serial.print("pad");
    Serial.print(pad);
    Serial.print(".bnote:");
    Serial.println(bnote);

    Serial.print("pad");
    Serial.print(pad);
    Serial.print(".velocity:");
    Serial.println(velocity);

    Serial.print("pad");
    Serial.print(pad);
    Serial.print(".(byte)velocity:");
    Serial.println((byte)velocity);
    */

    MIDI.sendNoteOff(bnote, velocity, general[GENERAL_CHANNEL]);
    /*
    if ((byte)velocity == 0) return;

    byte noteoff[2] = { bnote,        //note byte
              (byte)velocity }; //velocity byte
    send_channel_message(MIDI_CMD_NOTE_OFF, general[GENERAL_CHANNEL], sizeof(noteoff), noteoff);
    */
  }

  //POT Handlers
  unsigned char adjusted_for_master_volume() {
    /*
    Serial.print("Adjusting Volume for pad:");
    Serial.println(pad);
    */
    //get a percentage from pot 
    int raw = map(analogRead(PIN_MASTER_VOLUME), 0, 1023, 0, 100);

    //gets velocity*percentage
    byte new_vel = (byte)((int)velocity * raw / 100);

    //DEBUG
    /*
    Serial.print("raw");
    Serial.print(raw);
    Serial.print("data:");
    Serial.println(new_vel);
    */
    return new_vel;
  }

  void init_pad(byte pad_no, int pin_to_read) {
    pad = pad_no;
    pin = pin_to_read;
    pinMode(pin, INPUT);

    maskTime = 150;//110; //tested min 110; 
    scanTime = 15;//35; =<- this reduces delay in a 1-1ms
    update_settings();
  }

  void update_settings() {

    //Get patch from eeprom
    ROM.load_patchsettings(current.sel_patch);

    bnote = mini[pad][SETTING_NOTE] & 127;
    maxVel = mini[pad][SETTING_VELOCITY];
    sensitivity = mini[pad][SETTING_SENSITIVITY];
    threshold = mini[pad][SETTING_THRESHOLD];
    pitchbend = general[PAGE_PITCHBEND];

    /*LEGACY MASTER
    bnote = master[current.sel_patch][pad][SETTING_NOTE] & 127;
    maxVel = master[current.sel_patch][pad][SETTING_VELOCITY];
    sensitivity = master[current.sel_patch][pad][SETTING_SENSITIVITY];
    threshold = master[current.sel_patch][pad][SETTING_THRESHOLD];
    pitchbend = general[GENERAL_PITCHBEND];
    */
    noteCycles = 0;
    //TODO: FOR PB < 8200
    //if (maxPB < 8191) pb_step = (8191 - maxPB) * 100 / noteDuration; //the five is for the %5 in tick

    /*
    Serial.print("pad");
    Serial.print(pad);
    Serial.print(".note:");
    Serial.println(bnote);
    Serial.print("pad");
    Serial.print(pad);
    Serial.print(".maxVel:");
    Serial.println(maxVel);

    Serial.print("pad");
    Serial.print(pad);
    Serial.print(".sensitivity:");
    Serial.println(sensitivity);
    Serial.print("pad");
    Serial.print(pad);
    Serial.print(".threshold:");
    Serial.println(threshold);
    Serial.print("pad");
    Serial.print(pad);
    Serial.print(".maxPB:");
    Serial.println(maxPB);
    Serial.print("pad");
    Serial.print(pad);
    Serial.print(".pb_step:");
    Serial.println(pb_step);*/
  }

  byte mapVel(int velocityRaw, int threshold, int sensRaw, int maxVel)
  {
    /*
    'threshold-----------------------------------------velocityRaw---------------------sensitivity'
                              \/

    '1------------------------------------------------newVelocity------------------------------127' //FROM RYO KOSAKI


    /--------------------------------------------------------------------------------------------------------------
    ------------------------------------------------MY PROPOSAL---------------------------------------------------
    --------------------------------------------------------------------------------------------------------------

    'threshold---------------------------------------- - velocityRaw----------------------------------- - sensitivity'
                              \/
    '1------------------------------------------------newVelocity------------------------------master[x][x][velocity]'
    */
    //reasoning:
      //ryokosaka doesnt have a max vel so its set to 127 by default, since we have a max vel, thats gonna be the lmiit.
      //ryo's implemntation of threshold is great. im gonna use that idea as well.
      //ryo's implementation of sensitivity is also spot on. gonna just ctrl c ctrl v that too.
      //Threshold: the min hit speed to cause the min sound
      //Sensitiviity: the max value needed to cause the max sound

    //to ensure the output res doesnt exceed 127
    if (velocityRaw > sensRaw) velocityRaw = sensRaw;


    int res = map(velocityRaw, threshold, sensRaw, 0, maxVel); //map the value in linear range 1/127 //MINE


    //rounds off velocity to 4 possible levels:0%,25%,50%,75%,(100%?)
    int bits_to_shift = countBits(res) - 2;

    if (bits_to_shift < 2) return 0; // < 1 should be min

    res = (res >> bits_to_shift) << bits_to_shift;

    /*FOR DEBUG
    Serial.println("Mapping:");
    Serial.print("map(");
    Serial.print(velocityRaw);
    Serial.print(",");
    Serial.print(threshold);
    Serial.print(",");
    Serial.print(sensRaw);
    Serial.print(",1,");
    Serial.print(maxVel);
    Serial.print(") = ");
    Serial.println(res);
    */

    //if (res < 1) res = 1; not checking this cause I know it wont be met.
    if (res > 127) res = 127; //just in case

    /*
    Serial.print("pad");
    Serial.print(pad);
    Serial.print(".res:");
    Serial.println((byte)(res & 127));*/
    return (byte)res;
  }

  void tick()// byte sens, byte thre, byte scanTime, byte maskTime) //scan time and masktime to be set with the initiliaztion
  {

    //during note
    if (flag_pressed) {
      //note off 
      if (noteCycles++ >> NOTE_DURATION) {
        /*
        Serial.print("flag_pressed::");
        Serial.print("pad");
        Serial.print(pad);
        Serial.print(".noteOff");
        */

        flag_pressed = false;
        noteCycles = 0;

        //note off
        noteOff();
      }
    }
    else if (flag_hit == true) {

      if (millis() - time_end < maskTime)
      {
        //pad was stuck within mask time (probably vibration)
        flag_hit = false;
        return;
      }

      int piezoValue = analogRead(pin);

      if (piezoValue > velocity) {
        //update raw velocity
        velocity = piezoValue;

        /*
        Serial.print("pad");
        Serial.print(pad);
        Serial.print(".velocity:");
        Serial.println(velocity);
        */
      }

      if (millis() - time_hit > scanTime) {
        flag_hit = false;
        flag_pressed = true;

        //send note on
        /*
        Serial.println("SENDING NOTE ON");
        Serial.print("pad");
        Serial.print(pad);
        Serial.println(".mapVel_Start");
        */

        velocity = mapVel(velocity, threshold, sensitivity, maxVel);

        /*
        Serial.print("pad");
        Serial.print(pad);
        Serial.println(".mapVel_End");/
        */
        velocity = adjusted_for_master_volume();
        noteOn();

        time_end = millis();

        pitchBend(pitchbend);
        delayMicroseconds(100);
      }
    }
    else {

      if (analogRead(pin) > threshold)
      {
        /*
        Serial.print("pad");
        Serial.print(pad);
        Serial.println(".pad_struck");*/

        flag_hit = true;
        flag_pressed = false;
        velocity = 0;
        time_hit = millis();
      }
    }
  }
};

Pad pad1;
Pad pad2;
Pad pad3;
Pad pad4;
Pad pad5;
Pad pad6;
Pad pad7;
Pad pad8;
Pad pad9;
Pad pad10;
Pad pad11;
Pad pad12;
Pad pad13;
Pad pad14;
Pad pad15;
Pad pad16;

//PAD FUNCTIONS
void update_pad_settings() {
  pad1.update_settings();
  pad2.update_settings();
  pad3.update_settings();
  pad4.update_settings();
  pad5.update_settings();
  pad6.update_settings();
  pad7.update_settings();
  pad8.update_settings();
  pad9.update_settings();
  pad10.update_settings();
  pad11.update_settings();
  pad12.update_settings();
  pad13.update_settings();
  pad14.update_settings();
  pad15.update_settings();
  pad16.update_settings();
}


//DISPLAY MENU FUNCTIONS
/*
level 0 - Home
level 1 - Main Settings
level 2 - Select Pad
level 3 - Pad Settings
level 4 - Edit Pad Settings
level 5 - Play
level 6 - General Settings
level 7 - Edit General Settings
*/

void load_screen(int level, int sel_page, int sel_patch, int sel_pad) {

  switch (level) {
  case 0: { //HOMEPAGE
    //load homeselected
    display_home(sel_page);
    break;
  }
  case 1: { //Main Settings Page
    //load select patch 1 (settings)
    display_settings(sel_page);
    break;
  }
  case 2: { //Select Pad
    //load select selected
    display_pad(sel_page, sel_patch);
    break;
  }
  case 3: { //Display Pad Setting
    display_setting(sel_page, sel_patch, sel_pad, false); //no edit
    //load select selected 2
    break;
  }
  case 4: { //Edit Display Pad Setting
    display_setting(sel_page, sel_patch, sel_pad, true);
    break;
  }
  case 5: { //Play
    display_playscreen(sel_page);
    break;
  }
  case 6: { //General Settings
    display_general(sel_page, false);
    break;
  }
  case 7: { //Edit General up
    display_general(sel_page, true);
    break;
  }
  }
}

void display_loadingscreen() {
  display.clearDisplay();

  //header
  display.setTextSize(1); // Draw nX-scale text
  display.setTextColor(SSD1306_WHITE);
  //display.setCursor(5*20, 32-8);
  //display.println(F("V1.0")); //21 characters max

  display.setCursor((display.width() - (4 + 5 * 16)) / 2, 32 - 8);
  display.println(F("SACHEE MIDI PAD")); //21 characters max
  //display.drawLine(0, 8, display.width() - 1, 8, SSD1306_WHITE);

  //body
  display.setTextSize(5, 3); // Draw nX-scale nY-scale text

  int startpos = (display.width() - (4 + 5 * 3 * 5)) / 2; //get the middle of screen
  display.setCursor(startpos, 0);
  display.println(F("SMP"));

  display.display();
}
char* get_setting_title(int n) {
  if (n == 0) return (char*)"GENERAL";
  return (char*)"PATCH";
}
void display_settings(int page) {
  if (page < 0) page = 0;
  if (page > MAX_LIST_SETTINGS - 1) page = MAX_LIST_SETTINGS - 1;
  display.clearDisplay();

  //header
  display.setTextSize(1); // Draw nX-scale text
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(5, 0);
  display.println(F("SELECT PATCH")); //21 characters max

  display.drawLine(0, 7, display.width() - 1, 7, SSD1306_WHITE);

  //body

  //prints three elements starting from the page number
  for (int i = 0; i < 3; i++) { //3 is max no of list items
    display.setCursor(6, 9 + 8 * i);
    display.println((char*)get_setting_title(page));
    if (page != 0) {
      display.setCursor(6 + 5 * 7, 9 + 8 * i);
      display.println(page);
    }

    
    page++;
    if (page == MAX_LIST_SETTINGS) {
      break;
    }
  }

  //TODO: change this to background highlight and running instead of always at the top
  display_arrowhead(0);

  display.display();      // Show initial text
}

void display_arrowhead(int list_no) {
  display.setCursor(0, 9 + list_no * 8);
  display.println(F(">"));
}

void display_questionmark(int list_no) {
  display.setCursor(128 - 6, 9 + list_no * 8);
  display.println(F("?"));
}

void display_pad(int page, int patch) {
  if (page < 0) page = 0;
  if (page > MAX_LIST_PADS - 1) page = MAX_LIST_PADS - 1;

  display.clearDisplay();

  //header
  display.setTextSize(1); // Draw nX-scale text
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(5, 0);
  display.println(F("PATCH     SELECT PAD")); //21 characters max

  display.setCursor(5 + 5 * 7, 0); //7 is for the number of characters in " patch "
  display.println(patch + 1); //21 characters max

  display.drawLine(0, 7, display.width() - 1, 7, SSD1306_WHITE);

  //body

  //prints the three elements starting from the page number
  for (int i = 0; i < 3; i++) { //3 is max no of list items
    display.setCursor(6, 9 + 8 * i);
    //char* tmp = &list_of_settings[page++];
    display.println(F("PAD"));
    display.setCursor(6 + 5 * 5, 9 + 8 * i);
    display.println(++page);

    if (page == MAX_LIST_PADS) {
      break;
    }
  }

  //TODO: Change to background highlight
  display_arrowhead(0);
  display.display();
}

void display_setting(int page, int sel_patch, int sel_pad, bool editmode) {
  if (page < 0) page = 0;
  if (page > MAX_LIST_SETTINGS_SETTINGS - 1) page = MAX_LIST_SETTINGS_SETTINGS - 1;

  if (editmode) {
    //TODO: Background highlight instead of arrow
    //clear current values
    display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    display.setCursor(5 + 5 * 20, 9 + 8 * 0);
    display.println(F("   "));
    display.setCursor(5 + 5 * 20, 9 + 8 * 0);
    display.println(value.tmp);

    display_questionmark(0);
    display.display();
    return;
  }

  display.clearDisplay();

  //header
  display.setTextSize(1); // Draw nX-scale text
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(5, 0);
  display.println(F("PATCH         PAD")); //21 characters max
  display.setCursor(5 + 5 * 7, 0);
  display.println(sel_patch + 1); //21 characters max
  display.setCursor(5 + 5 * 22, 0);
  display.println(sel_pad + 1); //21 characters max

  display.drawLine(0, 7, display.width() - 1, 7, SSD1306_WHITE);

  //body
  display_arrowhead(0);


  //prints the settings (HAS TO BE 3 ELEMENTS!!!
  for (int i = 0; i < 3; i++) { //3 is max no of list items
    display.setCursor(6, 9 + 8 * i);
    //char* tmp = &list_of_settings[page++];
    display.println((char*)list_of_settings_settings[page]);

    //display value from master
    display.setCursor(5 * 21, 9 + 8 * i);
    display.println(mini[sel_pad][page]);
    page++;

    if (page == MAX_LIST_SETTINGS_SETTINGS) {
      break;
    }
  }

  display.display();
}

void display_general(int page, bool editmode) {
  if (page < 0) page = 0;
  if (page > MAX_LIST_SETTINGS_GENERAL - 1) page = MAX_LIST_SETTINGS_GENERAL - 1;

  if (editmode) {
    //print blank space
    display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    display.setCursor(5 * 18, 9 + 8 * 0);
    display.println(F("     "));                //clear previous text
    display.setCursor(5 * 18, 9 + 8 * 0);
    display.println(gvalue.tmp);

    display_questionmark(0);
    display.display();
    return;
  }

  display.clearDisplay();

  //header
  display.setTextSize(1); // Draw nX-scale text
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(5, 0);
  display.println(F("GENERAL SETTINGS")); //21 characters max

  display.drawLine(0, 7, display.width() - 1, 7, SSD1306_WHITE);

  //body
  display_arrowhead(0); //page cannot be more than 2

  for (int i = 0; i < 3; i++) { //3 is max no of list items
    //print value
    display.setCursor(6, 9 + 8 * i);
    display.println((char*)list_of_settings_general[page]);

    display.setCursor(5 * 18, 9 + 8 * i);
    display.println(general[page]);
    page++;


    if (page == MAX_LIST_SETTINGS_GENERAL) {
      break;
    }
  }

  display.display();
}

void display_home(int selected) {
  if (selected < 0) selected = 0;
  if (selected > 1) selected = 1;
  display.clearDisplay();

  //header
  display.setTextSize(1); // Draw nX-scale text
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(5, 0);
  display.println(F("OCTAPAD")); //21 characters max

  display.drawLine(0, 8, display.width() - 1, 8, SSD1306_WHITE);

  //body
  display.drawBitmap(
    (display.width() - 16) / 4,
    (display.height() - 16) / 1.2,
    bmp_note, BMP_NOTE_WIDTH, BMP_NOTE_HEIGHT, 1);
  display.drawBitmap(
    3 * ((display.width() - 16) / 4),
    (display.height() - 16) / 1.2,
    bmp_gear, BMP_GEAR_WIDTH, BMP_NOTE_HEIGHT, 1);


  //draw underlines

  //TODO: Change the selected view to a filled rounded rect than an underline
  display.drawLine(28 + selected * 56, 30, (28 + selected * 56) + 16, 30, SSD1306_WHITE);
  display.drawLine(28 + selected * 56, 31, (28 + selected * 56) + 16, 31, SSD1306_WHITE);
  display.display();
}

void display_playscreen(int patch) {
  if (patch < 0) patch = 0;
  if (patch > MAX_PATCHES - 1) patch = MAX_PATCHES - 1;
  display.clearDisplay();

  //header
  display.setTextSize(1); // Draw nX-scale text
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(5, 0);
  display.println(F("SELECTED PATCH")); //21 characters max

  display.drawLine(0, 8, display.width() - 1, 8, SSD1306_WHITE);

  //body

  int characters;
  patch > 8 ? characters = 2 : characters = 1; //8 cause the printed value is 9


  int startpos = ((display.width() - 1) / 2) - (3 + 5 * characters); //get the middle of screen

  display.setTextSize(4, 2); // Draw nX-scale nY-scale text
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(startpos, 15);
  display.println(patch + 1); //FIXED: randomchars showing on RHS, a random issue

  display.display();
}

//INPUT HANDLERS
void knob_tick()
{

  long newPosition = knob1.read();
  if (newPosition != oldPosition) {
    if (newPosition < oldPosition)
      current.up();
    else {
      current.down();
    }
    oldPosition = newPosition;
  }

}

void pressSelect()
{
  current.nextLevel();
}
void pressBack()
{
  current.previousLevel();
}

void pressMinus()
{
  current.down();
  //Serial.println("MINUS");
}
void longpressMinus()
{
  if (minus_tmp++ % SCROLL_SPEED == 0) current.down();
}

void pressPlus()
{
  current.up();
  //Serial.println("PLUS");
}
void longpressPlus()
{
  if (plus_tmp++ % SCROLL_SPEED == 0) current.up();
}

void forcemaster() {
  /*
  ////PATCH 2
  //PAD1
  master[1][0][SETTING_NOTE] = 95;
  master[1][0][SETTING_VELOCITY] = 127;   //max mapped max(127)
  master[1][0][SETTING_SENSITIVITY] = 50; //max raw
  master[1][0][SETTING_THRESHOLD] = 10;   //min raw

  //PAD2
  master[1][1][SETTING_NOTE] = 89;
  master[1][1][SETTING_VELOCITY] = 120;   //max mapped max(127)
  master[1][1][SETTING_SENSITIVITY] = 50; //max raw
  master[1][1][SETTING_THRESHOLD] = 10;   //min raw


  //PAD3
  master[1][2][SETTING_NOTE] = 92;
  master[1][2][SETTING_VELOCITY] = 120;   //max mapped max(127)
  master[1][2][SETTING_SENSITIVITY] = 50; //max raw
  master[1][2][SETTING_THRESHOLD] = 10;   //min raw

  //PAD4
  master[1][3][SETTING_NOTE] = 90;
  master[1][3][SETTING_VELOCITY] = 120;   //max mapped max(127)
  master[1][3][SETTING_SENSITIVITY] = 50; //max raw
  master[1][3][SETTING_THRESHOLD] = 10;   //min raw

  ////PATCH 1
  //PAD1
  master[0][0][SETTING_NOTE] = 75;
  master[0][0][SETTING_VELOCITY] = 127;   //max mapped max(127)
  master[0][0][SETTING_SENSITIVITY] = 50; //max raw
  master[0][0][SETTING_THRESHOLD] = 10;   //min raw

  //PAD2
  master[0][1][SETTING_NOTE] = 76;
  master[0][1][SETTING_VELOCITY] = 120;   //max mapped max(127)
  master[0][1][SETTING_SENSITIVITY] = 50; //max raw
  master[0][1][SETTING_THRESHOLD] = 10;   //min raw


  //PAD3
  master[0][2][SETTING_NOTE] = 77;
  master[0][2][SETTING_VELOCITY] = 120;   //max mapped max(127)
  master[0][2][SETTING_SENSITIVITY] = 50; //max raw
  master[0][2][SETTING_THRESHOLD] = 10;   //min raw

  //PAD4
  master[0][3][SETTING_NOTE] = 80;
  master[0][3][SETTING_VELOCITY] = 120;   //max mapped max(127)
  master[0][3][SETTING_SENSITIVITY] = 50; //max raw
  master[0][3][SETTING_THRESHOLD] = 10;   //min raw

  general[GENERAL_CHANNEL] = 1;
  general[GENERAL_PITCHBEND] = 15000;
  */
}

void setup() {


  Serial.begin(57600);

  MIDI.begin();

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;); // Don't proceed, loop forever
  }
  display.display();
  display.clearDisplay();
  display_loadingscreen();
  delay(2000);

  // INITIALIZING BUTTON FUNCTIONS
  btnSelect.attachClick(pressSelect); //ok
  btnSelect.attachDoubleClick(pressBack); //ok

  btnMinus.attachClick(pressMinus);
  btnMinus.attachDuringLongPress(longpressMinus);

  btnPlus.attachClick(pressPlus);
  btnPlus.attachDuringLongPress(longpressPlus);

  pinMode(PIN_MASTER_VOLUME, INPUT);

  //delay(500);
  current.level = 0;
  current.page = 0;

  //Load from EEPROM
  current.sel_patch = ROM.get_lastpatch();    //gets last patch
  ROM.load_to_general_from_EEPROM();

  current.initPageMax();
  load_screen(current.level, current.page, current.sel_patch, 0);

  //Testing EEPROM lastpatch get and update PASSED
  /*
  //Description: Test loads lastpatch from eeprom, updates with lastpatch + 1, loads new lastpatch from eeprom
  current.sel_patch = ROM.get_lastpatch();    //gets last patch
  Serial.print("sel_patch =");
  Serial.println(current.sel_patch);

  Serial.println("Updating...");
  ROM.update_lastpatch(ROM.get_lastpatch() + 1);

  current.sel_patch = ROM.get_lastpatch();
  Serial.print("sel_patch* =");
  Serial.println(current.sel_patch);
  // END of Testing EEPROM lastpatch get and update
  */

  //Testing General Load and update
  /*
  //Test: Loads general settings, increments each, saves it to eeprom, loads it again with new values

  ROM.load_to_general_from_EEPROM();

  //print general
  for (int i = 0; i < MAX_GENERAL_SETTINGS; i++) {
    Serial.print("general[");
    Serial.print(i);
    Serial.print("]:");
    Serial.println(general[i]);
  }
  Serial.println("-Updating EEPROM-");

  ROM.update_general_in_EEPROM(0, general[0] + 1);
  ROM.update_general_in_EEPROM(1, general[1] + 1);

  Serial.println("-Reading EEPROM-");

  ROM.load_to_general_from_EEPROM();
  //print general
  for (int i = 0; i < MAX_GENERAL_SETTINGS; i++) {
    Serial.print("general[");
    Serial.print(i);
    Serial.print("]:");
    Serial.println(general[i]);
  }
  Serial.print("--");

  //END Testing General Load and update
  */

  pad1.init_pad(1, A1);   //sets pads to notes from last patch
  pad2.init_pad(2, A2);
  pad3.init_pad(3, A3);
  pad4.init_pad(4, A4);
  pad5.init_pad(5, A5);
  pad6.init_pad(6, A6);
  pad7.init_pad(7, A7);
  pad8.init_pad(8, A8);
  pad1.init_pad(9, A9);   
  pad2.init_pad(10, A10);
  pad3.init_pad(11, A11);
  pad4.init_pad(12, A12);
  pad5.init_pad(13, A13);
  pad6.init_pad(14, A14);
  pad7.init_pad(15, A15);
  pad8.init_pad(16, A16);
}

void loop() {

  //polling Buttons
  btnSelect.tick();
  btnMinus.tick();
  btnPlus.tick();

  //polling pads
  pad1.tick();
  pad2.tick();
  pad3.tick();
  pad4.tick();
  pad5.tick();
  pad6.tick();
  pad7.tick();
  pad8.tick();
  pad9.tick();
  pad10.tick();
  pad11.tick();
  pad12.tick();
  pad13.tick();
  pad14.tick();
  pad15.tick();
  pad16.tick();
  //polling Knob
  knob_tick();

}
