#include <Servo.h>
#include <stdint.h>

Servo s[12];

void setup () 
{
	for (int i = 0; i < 12; ++i)
	{
		s[i].attach(i+2); // pins 0 and 1 reserved for serial communication.
	}
	Serial.begin(115200);
}

unsigned int pin_index = 0;

unsigned char current_byte = 0;
uint16_t current_us = 0;
bool are_first_seven_bits_filled = false;

void loop()
{
	while (Serial.available() > 0)
	{
		current_byte = Serial.read();

		switch (current_byte)
		{
			case 255:
				pin_index = 0;
				are_first_seven_bits_filled = false;
				continue;

			default: //TODO check first for first bit filled;
				if (pin_index >= 12)
				{
					pin_index = 0;
					are_first_seven_bits_filled = false;
					continue;
				}

				if (!are_first_seven_bits_filled)
				{
					current_us = 128 * current_byte;
          are_first_seven_bits_filled = true;
				}
				else
				{
					current_us += current_byte;
					s[pin_index].writeMicroseconds(current_us);
         
          current_us = 0;
          are_first_seven_bits_filled = false;
          ++pin_index;
				}
				continue;
		}
	}
}
