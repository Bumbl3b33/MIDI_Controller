/*
 Name:		LCD_Menu2.ino
 Created:	13-Feb-21 3:51:59 PM
 Author:	BumbleBee

 Description: Menu on LCD with Edit as option
*/


//LCD and Keypad Libraries
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <string.h>
#include <Keypad.h>
#include <Key.h>

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

const int pages = 4;
char* title[pages] = { "Home","Page2","Page3","Page4" };
int variables[3][pages] = {
	{17000,	0	,0	,8000	},	//value [17000 is (Do not Print)]
	{17000,	0	,0	,0		},	//min
	{17000,	127 ,127,16383	}	//max
};

int current_page = 0;
int tmp = 17000;
bool question_mark = true;

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

	if (strlen(buffer) > 16) {
		lcd.clear();
		lcd.setCursor(0, 0);
		lcd.print("LEN > 16 ERROR");
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

	if (col > 17|| col < 0) {
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
		lcd.setCursor(16-1, row - 1);
		lcd.print("?");
	}

	return 0;
}

void setup() {
	lcd.init();
	lcd.backlight();

}


// the loop function runs over and over again until power down or reset
void loop() {
	//search for Keypad Input
	char key = keypad.getKey();
	if (key != NO_KEY) {
		if (key == '1') {
			current_page <= 0 ? current_page = 0 : current_page--;
			tmp = variables[0][current_page];

		}
		else if (key == '2') {
			current_page >= pages - 1 ? current_page = pages - 1 : current_page++;
			tmp = variables[0][current_page];
		}

		else if (key == '4') { //Subtract
			tmp > variables[1][current_page] ? tmp-- : tmp;			
		}
		else if (key == '5') { //Add
			tmp < variables[2][current_page] ? tmp++ : tmp;

		}
		else if (key == '6') { //Save Changes
			variables[0][current_page] = tmp;
		}

		tmp != variables[0][current_page] ? question_mark = true: question_mark = false;

		//current_page < 0 ? current_page = 0 : current_page;
		//current_page > pages-1? current_page = pages -1 : current_page;

		lcd_print(title[current_page], 1, 1, true);
		lcd_print_int(variables[0][current_page], 0, 2, false,false); //actual value on left
		lcd_print_int(tmp, 17, 2, false, question_mark); //tmp on right
	}

	//if key1 is pressed,
		//show_in_display(   

	//delay(10);
}

