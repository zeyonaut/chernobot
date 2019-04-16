#include "overseer.hpp"
#include <cmath>
#include <cstdint>
#include <algorithm> //clamp;
#include <iostream>

// x is positive on the left, y is positive in front
vec2 fleft_motor = {1, 1};
vec2 fright_motor = {-1, 1};
vec2 bleft_motor = {-1, 1};
vec2 bright_motor = {1, 1};

// calculate power of motor - tx = x (100 to -100), ty = y(100 to -100), motor is motor's vector magnitudes
int magnitude (const int tx, const int ty, const vec2 motor)
{
	const double PI = 3.1415926;
	// where {tx, ty} is the target vector, {motor.x, motor.y} is the motor's (fixed) vector,

	// if the target vector has no magnitude, the magnitude of the motor is none.
	if (((tx < 10 && tx >= 0)||(tx > -10 && tx <= 0)) && ((ty < 10 && ty >= 0 )||(ty > -10 && ty <= 0))) return 0;

	// dot product of the target and motor vectors.
	float dot_product = motor.x * tx + motor.y * ty;
	// magnitudes
	float target_mag = sqrt(tx * tx + ty * ty);
	float motor_mag = sqrt(motor.x * motor.x + motor.y * motor.y);

	// the cosine of the angle between the vectors is the dot product over the product of their magnitudes.
	float costheta = dot_product / (target_mag * motor_mag);

	float result = (acos(costheta) * 2) / PI; // acos -> [0 to PI] -> [0 to 2PI] -> [0 to 2]
	result -= 1; // [-1 to 1]
	result *= target_mag;

	float angle = atan2(ty, tx);
	result *= cos(4 * (angle))/2 + 1.5;

	return -(int)result;
}

int curve (float input)
{
	double curve_magnitude = 2;
	int max_value = 100;

	if (input > 0)
	{
		return (int)(round(pow(((int)input/(max_value/pow((double)(max_value),1/(curve_magnitude)))), curve_magnitude)));
	}
	else
	{
		return -(int)(round(pow((int)(input/(max_value/pow((double)(max_value),1/(curve_magnitude)))), curve_magnitude)));
	}
}

void serialize_controls(std::array<std::uint16_t, 12>& pin_data, const Controls& c, int botflag)
{
	using std::clamp;

	pin_data.fill(400);
	if (botflag == 0) // Saw
	{
		int fleft = magnitude(c.right, c.forward, fleft_motor) + c.clockwise;
		int fright = magnitude(c.right, c.forward, fright_motor) - c.clockwise;
		int bleft = magnitude(c.right, c.forward, bleft_motor) + c.clockwise;
		int bright = magnitude(c.right, c.forward, bright_motor) - c.clockwise;
		int lvert = c.up;
		int rvert = c.up;

		fleft = clamp(-400, fleft, 400);
		fright = clamp(-400, fright, 400);
		bleft = clamp(-400, bleft, 400);
		bright = clamp(-400, bright, 400);
		lvert = clamp(-400, lvert, 400);
		rvert = clamp(-400, rvert, 400);

		pin_data[0] = 1500 - bleft; // 6 1 //not reversed because backwards config and counterclockwise config cancels each other out //reversed due to wiring minutia
		pin_data[1] = 1500 + rvert;
		
		pin_data[3] = 1500 - fright; // 2 4 //reversed due to wiring minutia
		pin_data[4] = 1500 - bright; // 4 5 //reversed due to backwards configuration

		pin_data[5] = 1500 - lvert; // 5 6 //reversed due to ccwise config
		pin_data[6] = 1500 - fleft; // 3 7 //reversed due to ccwise config

		pin_data[2] = 1500 + c.moclaw;
	}
	else // ROMuLuS
	{
		int rmotor = -(c.forward - c.right);
		int lmotor = -(c.forward + c.right);
		int umotor = (c.up);
		int gripper_close = -(c.clockwise);

		rmotor = clamp(-400, rmotor, 400);
		lmotor = clamp(-400, lmotor, 400);
		umotor = clamp(-400, umotor, 400);
		gripper_close = clamp(-400, gripper_close, 400);

		pin_data[0] = 400 + rmotor; // 6 1 //right motor
		pin_data[1] = 400 + lmotor; // 7 2 //left motor

		pin_data[2] = 400 - umotor; // 2 4 // 4 is forward elevation
		pin_data[3] = 400 + umotor; // 4 5 // 5 is rear elevation

		pin_data[4] = 400 + gripper_close;
	}
}

std::string serialize_data (std::array<std::uint16_t, 12> pin_data)
{
	std::string serialized_data = "";
	for (size_t i = 0; i < pin_data.size(); ++i)
	{
		//serialized_data += std::to_string(i);
		serialized_data += "(";
		serialized_data.push_back((char)(i < 10? i + '0' : (i - 10) + 'A'));
		serialized_data += std::to_string(pin_data[i]);
		serialized_data += ")";
	}

	return serialized_data;
}

#if 0
/*
	Space efficient protocol results in significant losses of data.
	[April 1, 2019] Actually, maybe it doesn't. But it might not be as useful as we think.
*/

std::string serialize_data (std::array<std::uint16_t, 12> pin_data)
{
	std::string serialized_data = "";
	serialized_data.push_back((std::uint8_t) 255);

	for (auto i : pin_data) 
	{
		serialized_data.push_back((std::uint8_t) (i/128));
		serialized_data.push_back((std::uint8_t) (i%128));
	}
	
	return serialized_data;
}
#endif