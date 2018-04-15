/*


[window + ui] --------------------------------------+
	^												|
	|												|
[ui logic (rendering, drawing, debug displays)] <---+
	^				^								|
	|				|								V
[driving outputs] <-O-----------------------[driving inputs] <-- [keyboard, mouse]			
	|				|
	V				|
[serial port] -------


window, ui, ui logic: folder panel

driving inputs, driving outputs: folder pilot

serial input, serial output: folder slave

/\ The naiive way

V the better way


[input-assigner]: 

provides a list of input types. something else can request an input (say, axis($device_id, $index), input-assigner does bounds checks and stuff)

*/

// x is positive on the left, y is positive in front

#include "overseer.h"
#include <imgui.h>
#include <cmath>

vec2 fleft_motor = {1, 1};
vec2 fright_motor = {-1, 1};
vec2 bleft_motor = {-1, 1};
vec2 bright_motor = {1, 1};

// calculate power of motor - tx = x (100 to -100), ty = y(100 to -100), motor is motor's vector magnitudes
int magnitude (const int tx, const int ty, const vec2 motor)
{
	const double PI = 3.1415926;
	// where {tx, ty} is the target vector, {motor.x, motor.y} is the motor's (fixed) vector,

	//if the target vector has no magnitude, the magnitude of the motor is none.
	if (((tx < 10 && tx >= 0)||(tx > -10 && tx <= 0)) && ((ty < 10 && ty >= 0 )||(ty > -10 && ty <= 0))) return 0;

	//dot product of the target and motor vectors.
	float dot_product = motor.x * tx + motor.y * ty;
	//magnitudes
	float target_mag = sqrt(tx * tx + ty * ty);
	float motor_mag = sqrt(motor.x * motor.x + motor.y * motor.y);

	//the cosine of the angle between the vectors is the dot product over the product of their magnitudes.
	float costheta = dot_product / (target_mag * motor_mag);

	float result = (acos(costheta) * 2) / PI; // acos -> [0 to PI] -> [0 to 2PI] -> [0 to 2]
	result -= 1; //[-1 to 1]
	result *= target_mag;

	float angle = atan2(ty, tx);
	result *= cos(4 * (angle))/2 + 1.5;

	return -(int)result;
}

int curve (int input)
{
	double curve_magnitude = 2;
	int max_value = 100;

	if (input > 0)
	{
		return (int)(round(pow((input/(max_value/pow((double)(max_value),1/(curve_magnitude)))), curve_magnitude)));
	}
	else
	{
		return -(int)(round(pow((input/(max_value/pow((double)(max_value),1/(curve_magnitude)))), curve_magnitude)));
	}
}

void serialize_controls(std::array<unsigned char, 12>& pin_data, const Controls& c, int botflag) //FIXME no null checks
{
	pin_data.fill(0 + 100);
	if (botflag == 0) //Saw
	{
		int fleft = magnitude(curve(c.right), curve(c.forward), fleft_motor) + curve(c.clockwise);
		int fright = magnitude(curve(c.right), curve(c.forward), fright_motor) - curve(c.clockwise);
		int bleft = magnitude(curve(c.right), curve(c.forward), bleft_motor) + curve(c.clockwise);
		int bright = magnitude(curve(c.right), curve(c.forward), bright_motor) - curve(c.clockwise);
		int fvert = curve(c.up);
		int bvert = curve(c.up);

		ImGui::Text("Forward-Left Motor: %d", fleft);
		ImGui::Text("Forward-Right Motor: %d", fright);
		ImGui::Text("Backward-Left Motor: %d", bleft);
		ImGui::Text("Backward-Right Motor: %d", bright);
		ImGui::Text("Forward-Up Motor: %d", fvert);
		ImGui::Text("Backward-Up Motor: %d", bvert);

		pin_data[0] = -1 * bleft + 100;//6 1
		pin_data[1] = bvert + 100;//7 2 TODO: FIGURE OUT WHETHER THE 'BACK ELEVATION' IS ON THE LEFT OR THE RIGHT. I believe the back elevation is actually right side.

		pin_data[3] = -1 *(fright) + 100;//2 4
		pin_data[4] = (bright) + 100;//4 5

		pin_data[5] = (fvert) + 100;//5 6
		pin_data[6] = (fleft) + 100;//3 7

		pin_data[2] = c.moclaw + 100;
	}
	else if (botflag == 1) //Lem
	{
		pin_data[0] = (-c.forward - c.clockwise) + 100;//6 1
		pin_data[1] = (-c.forward + c.clockwise) + 100;//7 2

		pin_data[2] = (c.up) + 100;//2 4
		pin_data[3] = (c.up) + 100;//4 5
	}
	else // ROMuLuS
	{
		pin_data[0] = (c.forward + c.clockwise) + 100;//6 1
		pin_data[1] = (c.forward - c.clockwise) + 100;//7 2

		pin_data[2] = (c.up) + 100;//2 4
		pin_data[3] = (c.up) + 100;//4 5
	}
}

std::string serialize_data (std::array<unsigned char, 12> pin_data)
{
	std::string serialized_data = "ÿ";
	for (unsigned char i : pin_data) serialized_data.push_back(i);
	return serialized_data;
}