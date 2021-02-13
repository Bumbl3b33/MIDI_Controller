/*
 Name:		PlayNoteOnPress.ino
 Created:	12-Feb-21 9:05:43 PM
 Author:	BumbleBee
 
 Description: MIDI Events (on Loop)
*/


static byte last_status = 0;

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

void setup()
{
	//pinMode(LED_BUILTIN, OUTPUT);
	Serial.begin(57600);

	//MIDI.begin();                      // Launch MIDI on channel 1 (listening?)
}

void loop()
{
	
	byte noteon[2] = { 0x3C,0x40 }; //note, velocity
	send_channel_message(0x80, 2, 2, noteon);
	delay(1000);

	byte noteoff[2] = { 0x3C,0x40 }; //note, velocity
	send_channel_message(0x90, 1, 2, noteoff);
	delay(1000);

	byte patchchange[1] = { 0x3C }; //patch number
	send_channel_message(0xC0, 1, 1, patchchange);
	delay(1000);

	byte pitchbend[2] = { 0x10,0x4E }; //see pitchbendhelp.png
	send_channel_message(0xE0, 1, 2, pitchbend);
	delay(1000);

	//I think the DAW has to be configured to understand these messages as volume (for channel)
	byte channelvolumecoarse[2] = { 0x07 ,0x10 }; //0x07 for volume, 0x10 -> coarse volume 0-127
	send_channel_message(0xB0, 1, 2, channelvolumecoarse);
	delay(1000);

	byte mastervolume[8] = { 0xF0, 0x7F, 0x7F,0x04,0x01,0x10,0x4E,0xF7 }; //see pitchbendhelp.png as well
	send_system_message(8, mastervolume);
	delay(1000);

}


