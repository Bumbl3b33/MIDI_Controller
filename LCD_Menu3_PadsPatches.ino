/*
 Name:		LCD_Menu3_PadsPatches.ino
 Created:	13-Feb-21 10:32:13 PM
 Author:	BumbleBee

 Description: Access Menus for Patches and Pads (in SRAM)
*/


//LCD and Keypad Libraries
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <string.h>
#include <Keypad.h>
#include <Key.h>
#include <stdlib.h>


//KEYPAD
const byte ROWS = 4; //four rows
const byte COLS = 4; //three columns
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};

byte rowPins[ROWS] = { 7,6,5,4 }; //connect to the row pinouts of the keypad
byte colPins[COLS] = { 11, 10, 9,8 }; //connect to the column pinouts of the keypad

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);


//LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);  // set the LCD address to 0x27 for a 16 chars and 2 line display

int current_patch = 0;
int current_pad = 0;
int current_setting = 0;

#define MAX_PADS 8
#define MAX_PATCHES 2  
#define MAX_SETTINGS 4



char* title[MAX_SETTINGS] = { "Note",	"Velocity",	"Sensitivity%",	"PitchBend", };

//holds the max and min values for settings
int variables[2][MAX_SETTINGS] = {  
	//page1		page2	page3	page4
	{0,		0,		0,		0		},	//min (17000 for do not print)
	{127,	127,	100,	16383	}	//max (17000 for do not print)
};

//master MATRIX - holds 'patches'
int master[MAX_PATCHES][MAX_PADS][MAX_SETTINGS];

//running variables - while code is running
int tmp = 17000;
bool question_mark;

int current_level;
int current_page;
int current_page_max;
char* current_page_title = "BLANK";

int lcd_print(char* to_print, int col, int row, bool clear) {
	//prints to_print to LCD. 
	//col: 1-16
	//row: 1-2
	//to_print: must be <=16 characters
	if (strlen(to_print) > 16) {
		lcd.clear();
		lcd.setCursor(0, 0);
		lcd.print("LEN > 16 ERROR");
		return 1;
	}
	if (col > 16 || col < 1) {
		lcd.clear();
		lcd.setCursor(0, 0);
		lcd.print("COL ERROR");
		lcd.setCursor(0, 1);
		lcd.print(to_print);
		return 1;
	}

	if (row > 2 || row < 1) {
		lcd.clear();
		lcd.setCursor(0, 0);
		lcd.print("ROW ERROR");
		lcd.setCursor(0, 1);
		lcd.print(to_print);
		return 1;
	}

	if (clear == true)  lcd.clear();

	lcd.setCursor(col - 1, row - 1);
	lcd.print(to_print);
	return 0;
}

int lcd_print_int(int to_print_int, int col, int row, bool clear, bool questionmark) {
	//prints to_print to LCD. 
	//col: 1-16
	//row: 1-2
	//to_print: must be <=16 characters
	char buffer[16]; //to store converted int to string
	itoa(to_print_int, buffer, 10); //10 - base (DECIMAL)

	if (strlen(buffer) > 8) {
		lcd.clear();
		lcd.setCursor(0, 0);
		lcd.print("LEN > 8 ERROR");
		return 1;
	}

	//DO NOT PRINT CONDITION
	if (strcmp(buffer, "17000") == 0) return 0;

	if (row > 2 || row < 1) {
		lcd.clear();
		lcd.setCursor(0, 0);
		lcd.print("ROW ERROR");
		lcd.setCursor(0, 1);
		lcd.print(buffer);
		return 1;
	}

	if (col > 17 || col < 0) {
		lcd.clear();
		lcd.setCursor(0, 0);
		lcd.print("COL ERROR");
		lcd.setCursor(0, 1);
		lcd.print(buffer);
		return 1;
	}
	else if (col == 0) { //left align
		col = 1;
	}

	else if (col == 17) {//right align
		col = 16 - strlen(buffer);
	}
	//if (questionmark == true) col--; //move to left 1 space for question mark

	if (clear == true)  lcd.clear();

	lcd.setCursor(col - 1, row - 1);
	lcd.print(buffer);

	if (questionmark == true) {
		lcd.setCursor(16 - 1, row - 1);
		lcd.print("?");
	}

	return 0;
}

int lcd_print_title(int level, int page, int page_max) {
	lcd.clear();
	char* buffer;
	//print arrows
	if (page != page_max - 1) {
		lcd.setCursor(15, 0);
		lcd.print(">");
	}
	if (page != 0) {
		lcd.setCursor(0, 0);
		lcd.print("<");
	}
	if (level != 2) { //print page number
		lcd.setCursor(14, 0);
		lcd.print(page+1);
	}
	//print title
	if (level == 0) {
		lcd.setCursor(1, 0); //PRINTS TITLE
		lcd.print("PATCH");// buffer
	}
	else if (level == 1) {
		lcd.setCursor(1, 0); //PRINTS TITLE
		lcd.print("PAD");// buffer

	}
	else if (level == 2){
		lcd.setCursor(1, 0); //PRINTS TITLE
		lcd.print(title[page]);// buffer
	}

}

int navigator() {
	//search for Keypad Input
	char key = keypad.getKey();
	if (key != NO_KEY) {
		if (key == '1') { //LEFT
			if (current_page <= 0) return;
			current_page--;

			//current_page <= 0 ? current_page = 0 : current_page--;
			if (current_level == 2) tmp = master[current_patch][current_pad][current_page];
		}
		else if (key == '2') { //RIGHT
			if (current_page >= current_page_max - 1) return;
			current_page++;
			//current_page >= current_page_max - 1 ? current_page = current_page_max - 1 : current_page++;
			if (current_level == 2) tmp = master[current_patch][current_pad][current_page];
		}

		else if (key == '4') { //Subtract
			if (current_level == 2) {
				tmp > variables[0][current_page] ? tmp-- : tmp;
			}
		}
		else if (key == '5') { //Add
			if (current_level == 2) {
				tmp < variables[1][current_page] ? tmp++ : tmp;
			}
		}
		else if (key == '6') { //Save
			if (current_level == 2){
			master[current_patch][current_pad][current_page] = tmp;
			}
		}
		else if (key == 'B') { //select
			if (current_level >= 2) return;
			current_page_max = get_current_page_max(++current_level);
			
			if (current_level == 1) current_patch = current_page;
			if (current_level == 2) current_pad = current_page;
			current_page = 0;
		}
		else if (key == 'A') { //back
			if (current_level <= 0) return;
			current_page_max = get_current_page_max(--current_level);
			if (current_level == 0) current_page = current_patch;
			if (current_level == 1) current_page = current_pad;
		}

		lcd_print_title(current_level, current_page, current_page_max);

		if (current_level == 2) {
			tmp != master[current_patch][current_pad][current_page] ? question_mark = true : question_mark = false;

			//lcd_print(title[current_page], 1, 1, true);
			lcd_print_int(master[current_patch][current_pad][current_page], 0, 2, false, false); //actual value on left
			lcd_print_int(tmp, 17, 2, false, question_mark); //tmp on right
		}

		
	}
}

int get_current_page_max(int current_level) {
	if (current_level == 0) return MAX_PATCHES;
	if (current_level == 1) return MAX_PADS;
	if (current_level == 2) return MAX_SETTINGS;
	return 0;
}

char* get_title(int current_level) {
	if (current_level == 0) return "PATCH";
	if (current_level == 1) return "PAD";
	if (current_level == 2) return title[current_setting];
}

void setup() {
	lcd.init();
	lcd.noBacklight();

	//hard coding test settings
	master[0][0][0] = 46;
	master[0][0][1] = 27;
	master[0][0][2] = 50;
	master[0][0][3] = 8000;
	master[0][1][0] = 69;
	master[0][1][1] = 70;
	master[0][1][2] = 75;
	master[0][1][3] = 6000;
	master[1][1][0] = 85;
	master[1][1][1] = 86;
	master[1][1][2] = 87;
	master[1][1][3] = 5000;
	
	current_level = 0;
	current_page = 0;
	current_page_max = get_current_page_max(current_level);

	//lcd_print_title(current_level, current_page, current_page_max);

	//lcd_print(get_title(current_level), 1, 1, true);
	//lcd_print_int(master[0][0][current_setting], 0, 2, false, false); //actual value on left
	//lcd_print_int(tmp, 17, 2, false, question_mark); //tmp on right
}


// the loop function runs over and over again until power down or reset
void loop() {
	navigator();
	delay(10); 
	/*
	lcd_print_title(current_level, current_page, current_page_max);
	delay(1000);
	current_level++;
	current_page++;*/

}