#include <imgui.h>
#include <serial/serial.h>

#include "nimir/graphics.h"
#include "nimir/gui.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <vector>
using std::vector;
#include <exception>
#include <iostream>
#include <string>
#include <array>
using std::string;

struct vec2
{
	float x, y;
};
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

char to_hex (int num)
{
	//assert (num > 0 && num < 16)

	if (num < 10) return (char) (48 + num);

	else return (char) ((num - 10) + 65);
}

//convert 0d[0d0, 0d200] -> 0x[0d0, 0d200]
std::string conv (int num)
{
	if (num > 200) num = 200;
	if (num < 0) num = 0;
	int first = num/16;
	int last = num%16;
	std::string x(1, to_hex(first));
	x.push_back(to_hex(last));
	return x;
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

std::string serialize_data (std::array<int, 12> pin_data)
{
	std::string serialized_data = "T";
	for (int i : pin_data) serialized_data.append(conv(i));
	return serialized_data;
}

int main (int, char**)
{
	GLFWwindow* window = NULL;
	nimir::gl::init(window, 480, 720, "ARE WE BLIND?! DEPLOY THE GARRISON!");
	nimir::gui::init(window, false);
	serial::Serial port("");
	port.setBaudrate(115200);

	glfwSetMouseButtonCallback(window, nimir::gui::mouseButtonCallback);
	glfwSetScrollCallback(window, nimir::gui::scrollCallback);
	glfwSetKeyCallback(window, nimir::gui::keyCallback);
	glfwSetCharCallback(window, nimir::gui::charCallback);

	std::array<int, 12> pin_data;

	float* axes = NULL;

	// Control values, from -100 to 100

	int forward = 0;
	int right = 0;
	int up = 0;
	int clockwise = 0;

	// Motor values, from -100 to 100;
	int fleft = 0;
	int fright = 0;
	int bleft = 0;
	int bright = 0;
	int fvert = 0;
	int bvert = 0;

	int moclaw = 0;

	bool opening = false;
	bool closing = false;
	bool pressing_camera = false;

	// Selected Joystick in Combo Box
	int variableIndex = -1;

	int portIndex = -1;
	bool portOpen = false;

	int is_lemming = 0;

	serial::Serial prt("");
	prt.setBaudrate(115200);

	while (nimir::gl::nextFrame(window))
	{
		nimir::gui::nextFrame();
		{
			int window_w, window_h;
			glfwGetWindowSize(window, &window_w, &window_h);
			ImGui::SetNextWindowSize(ImVec2(window_w, window_h), ImGuiSetCond_Always);
			ImGui::SetNextWindowPos(ImVec2(0.f, 0.f), ImGuiSetCond_Always);
			ImGui::Begin("Console", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

			ImGui::Text("Application running at %.1f FPS", ImGui::GetIO().Framerate);

			{
				const char* joysticks [GLFW_JOYSTICK_LAST];
				int maxJoystick = GLFW_JOYSTICK_LAST;
				for (int i = 0; i < GLFW_JOYSTICK_LAST; i++)
				{
					if(glfwJoystickPresent(i)) joysticks[i] = glfwGetJoystickName(i);
					else {joysticks[i] = "";}
				}

				vector<serial::PortInfo> portinfo = serial::list_ports();
				const char** ports = new const char* [portinfo.size()];
				for (int i = 0; i < portinfo.size(); i++)
				{
					ports[i] = portinfo[i].port.c_str();
				}

				string output = "";

				if (1==1)
				{
					ImGui::Combo("Serial Port", &portIndex, ports, portinfo.size());

					ImGui::Combo("Joystick", &variableIndex, joysticks, GLFW_JOYSTICK_LAST);
					//inputJoystick[variableIndex] = joystickIndex;
					//writefln("%d%d", inputJoystick[variableIndex], inputJoystick[variableIndex] != -1? glfwJoystickPresent(inputJoystick[variableIndex]) : 0);

					ImGui::RadioButton("Saw", &is_lemming, 0); ImGui::SameLine();
					ImGui::RadioButton("Lem", &is_lemming, 1); ImGui::SameLine();
					ImGui::RadioButton("Romulus", &is_lemming, 2);

					if (variableIndex != -1? glfwJoystickPresent(variableIndex) == 1 : false)
					{
						int axcount;
						const float* rawAxes = glfwGetJoystickAxes(variableIndex, &axcount);
						int bucount;
						const unsigned char* rawButtons = glfwGetJoystickButtons(variableIndex, &bucount);

						//TODO: Do things with values of axes and buttons

						if (3 < axcount)
						{
							forward = -1 * (int) (100 * rawAxes[1]);
							right = (int) (100 * rawAxes[0]);
							up = -1 * (int) (100 *rawAxes[3]);
							clockwise = (int) (100 *rawAxes[2]);
						}

						if (3 < bucount)
						{
							opening = rawButtons[1] == GLFW_PRESS;
							closing = rawButtons[2] == GLFW_PRESS;
							pressing_camera = rawButtons[3] == GLFW_PRESS;
							if (opening == true && closing == true) opening = closing = false;
						}

						ImGui::Text("Forward: %d", forward);
						ImGui::Text("Right: %d", right);
						ImGui::Text("Up: %d", up);
						ImGui::Text("Clockwise: %d", clockwise);
						ImGui::Text("Claw State: %s", opening? "Opening" : closing? "Closing" : "Neutral");
					}

					ImGui::SliderInt("Forward", &forward, -100.f, 100.f);
					ImGui::SliderInt("Right", &right, -100.f, 100.f);
					ImGui::SliderInt("Up", &up, -100.f, 100.f);
					ImGui::SliderInt("Clockwise", &clockwise, -100.f, 100.f);

					ImGui::SliderInt("Chlaw", &moclaw, -100.f, 100.f);

					fleft = magnitude(curve(right), curve(forward), fleft_motor) + curve(clockwise);
					fright = magnitude(curve(right), curve(forward), fright_motor) - curve(clockwise);
					bleft = magnitude(curve(right), curve(forward), bleft_motor) + curve(clockwise);
					bright = magnitude(curve(right), curve(forward), bright_motor) - curve(clockwise);
					fvert = curve(up);
					bvert = curve(up);

					/*left = magnitude(right, forward, fleft_motor) + clockwise;
					fright = magnitude(right, forward, fright_motor) - cclockwise;
					bleft = magnitude(right, forward, bleft_motor) + clockwise;
					bright = magnitude(right, forward, bright_motor) - clockwise;
					fvert = up;
					bvert = up;*/

					ImGui::Text("Forward-Left Motor: %d", fleft);
					ImGui::Text("Forward-Right Motor: %d", fright);
					ImGui::Text("Backward-Left Motor: %d", bleft);
					ImGui::Text("Backward-Right Motor: %d", bright);
					ImGui::Text("Forward-Up Motor: %d", fvert);
					ImGui::Text("Backward-Up Motor: %d", bvert);
					ImGui::Text(pressing_camera? "The Chimera is being pressed" : "The Chimera is being dragged into hyperspace\n by giant floating WHALES");

					pin_data.fill(0 + 100);
					if (is_lemming == 0) //Saw
					{
						pin_data[0] = -1 * bleft + 100;//6 1
						pin_data[1] = bvert + 100;//7 2 TODO: FIGURE OUT WHETHER THE 'BACK ELEVATION' IS ON THE LEFT OR THE RIGHT. I believe the back elevation is actually right side.

						pin_data[3] = -1 *(fright) + 100;//2 4
						pin_data[4] = (bright) + 100;//4 5

						pin_data[5] = (fvert) + 100;//5 6
						pin_data[6] = (fleft) + 100;//3 7

						pin_data[2] = moclaw + 100;
					}
					else if (is_lemming == 1) //Lem
					{
						pin_data[0] = (-forward - clockwise) + 100;//6 1
						pin_data[1] = (-forward + clockwise) + 100;//7 2

						pin_data[2] = (up) + 100;//2 4
						pin_data[3] = (up) + 100;//4 5
					}
					else // ROMuLuS
					{
						pin_data[0] = (forward + clockwise) + 100;//6 1
						pin_data[1] = (forward - clockwise) + 100;//7 2

						pin_data[2] = (up) + 100;//2 4
						pin_data[3] = (up) + 100;//4 5
					}

					ImGui::Text("Output: %s", serialize_data(pin_data).c_str());

					try
					{
						if (portIndex != -1)
						{
							if (!portOpen)
							{
								portOpen = true;
								prt.setPort(portinfo[portIndex].port);
								prt.open();
							}

							if (prt.isOpen()) prt.write(serialize_data(pin_data));
						}
						else
						{
							portOpen = false;
							prt.close();
							prt.setPort("");
						}
					}
					catch (std::exception e)
					{
						portOpen = false;
						prt.close();
						prt.setPort("");
						portIndex = -1;
					}

				}
			}

			ImGui::End();
		}

		nimir::gl::setClearColor(ImColor(0, 0, 0));

		nimir::gui::drawFrame();
		nimir::gl::drawFrame(window);
	}

	nimir::gui::quit();
	nimir::gl::quit();

	return 0;
}
