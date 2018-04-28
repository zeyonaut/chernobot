#include <Servo.h>
#include <stdint.h>

Servo s[12];
unsigned char buffer[13];

/*unsigned char left;
unsigned char right;
bool has_left_just_been_filled;*/

void setup () 
{
	for (int i = 0; i < 12; ++i)
	{
		s[i].attach(i+2); // pins 0 and 1 reserved for serial communication.
	}
	Serial.begin(115200);
	//has_left_just_been_filled = false;
}

unsigned int pin_index = 0;
unsigned char current_byte = 0;

void loop() 
{ 
	while (Serial.available() > 0)
	{
		current_byte = Serial.read();

	switch (current_byte)
	{
	  case 255:
		pin_index = 0;
		continue;
		
	  default:
		if (pin_index >= 12)
		{
		  pin_index = 0;
		  continue;
		}
		s[pin_index].writeMicroseconds(current_byte * 4 + 1100);
		++pin_index;
		continue;
	}
	}
}
