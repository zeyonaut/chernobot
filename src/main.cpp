#include <imgui.h>
#include <libserialport.h>
#include <vector>
#include <exception>

#include "nimir/graphics.h"
#include "nimir/gui.h"

/*extern "C" {
#include <libavdevice/avdevice.h>
#include <libswscale/swscale.h>
}*/

#include <stdio.h>

#include <string>
using std::string;
using std::vector;

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <serial/serial.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <stdlib.h>

const double PI =  3.1415926;

#define ABS(a) a > 0? a : (a*-1)
#define MAX(a, b) ABS(a) > ABS(b)? ABS(a) : ABS(b)
#define MIN(a, b) a > b? b : a

struct vec2
{
    float x, y;
};
// x is positive on the left, y is positive in front
vec2 fleft_motor = {1, 1};
vec2 fright_motor = {-1, 1};
vec2 bleft_motor = {-1, 1};
vec2 bright_motor = {1, 1};

int max = 0;

// calculate power of motor - tx = x (100 to -100), ty = y(100 to -100), motor is motor's vector magnitudes
int magnitude(const int tx, const int ty, const vec2 motor)
{
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
    //printf ("%0.0f\n", result);

    //TODO: divide the result by the 100/160;
    //
    ///if ((tx + ty) < 0) if (max < -(tx + ty)) max = -(tx + ty);
    //else if (max < (tx + ty)) max = (tx + ty);

    return -(int)result;
}

const char* to_hex(int num)
{
    if (num == 0) return "0";
    else if (num == 1) return "1";
    else if (num == 2) return "2";
    else if (num == 3) return "3";

    else if (num == 4) return "4";
    else if (num == 5) return "5";
    else if (num == 6) return "6";
    else if (num == 7) return "7";

    else if (num == 8) return "8";
    else if (num == 9) return "9";
    else if (num == 10) return "A";
    else if (num == 11) return "B";

    else if (num == 12) return "C";
    else if (num == 13) return "D";
    else if (num == 14) return "E";
    else if (num == 15) return "F";
}

const char* conv(int num)
{
    num += 100;
    if (num > 200) num = 200;
    if (num < 0) num = 0;
    int first = num/16;
    int last = num%16;
    std::string x(to_hex(first));
    x.append(to_hex(last));
    return x.c_str();
}

const char* conv_norm(int num)
{
    if (num > 200) num = 200;
    if (num < 0) num = 0;
    int first = num/16;
    int last = num%16;
    std::string x(to_hex(first));
    x.append(to_hex(last));
    return x.c_str();
}


int main(int, char**)
{
    GLFWwindow* window = NULL;
    nimir::gl::init(window, 1280, 720, "example");
    nimir::gui::init(window, false);
    serial::Serial port("");
    port.setBaudrate(115200);

    glfwSetMouseButtonCallback(window, nimir::gui::mouseButtonCallback);
    glfwSetScrollCallback(window, nimir::gui::scrollCallback);
    glfwSetKeyCallback(window, nimir::gui::keyCallback);
    glfwSetCharCallback(window, nimir::gui::charCallback);
    bool show_test_window = true;

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

    bool opening = false;
    bool closing = false;
    bool pressing_camera = false;
    // Selected Joystick in Combo Box

    int variableIndex = -1;

    int portIndex = -1;

    bool portOpen = false;

    // Main loop

    // Loading images

    serial::Serial prt("");
    prt.setBaudrate(115200);

    //Main loop
    while (nimir::gl::nextFrame(window))
    {

        nimir::gui::nextFrame();
        {
            ImGui::Begin("Input");

            //sample image: //ImGui::Image(&m_texture, ImVec2(128, 80), ImVec2(0,0), ImVec2(1,1), ImVec4(255,255,255,255), ImVec4(255,255,255,0));

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

				if (true)
				{
                    ImGui::Combo("Serial Port", &portIndex, ports, portinfo.size());

					ImGui::Combo("Joystick", &variableIndex, joysticks, GLFW_JOYSTICK_LAST);
					//inputJoystick[variableIndex] = joystickIndex;
					//writefln("%d%d", inputJoystick[variableIndex], inputJoystick[variableIndex] != -1? glfwJoystickPresent(inputJoystick[variableIndex]) : 0);
					if (variableIndex != -1? glfwJoystickPresent(variableIndex) == 1 : false)
					{
						int axcount;
						const float* rawAxes = glfwGetJoystickAxes(variableIndex, &axcount);
                        int bucount;
						const unsigned char* rawButtons = glfwGetJoystickButtons(variableIndex, &bucount);

                        //TODO: Do things with values of axes and buttons

                        if (3 < axcount)
                        {
                            forward = (int) (-100 * rawAxes[1]);
                            right = (int) (100 * rawAxes[0]);
                            up = (int) (-100 *rawAxes[3]);
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

                        fleft = magnitude(right, forward, fleft_motor);
                        //fleft = (fleft + clockwise)/2;
                        fright = magnitude(right, forward, fright_motor);
                        //fright = (fright - clockwise)/2;
                        bleft = magnitude(right, forward, bleft_motor);
                        //bleft = (bleft + clockwise)/2;
                        bright = magnitude(right, forward, bright_motor);
                        //bright = (bright - clockwise)/2;
                        fvert = up;
                        bvert = up;

			fleft += clockwise;
			fright -= clockwise;
			bleft  += clockwise;
			bright -= clockwise;


                        ImGui::Text("Forward-Left Motor: %d", fleft);
                        ImGui::Text("Forward-Right Motor: %d", fright);
                        ImGui::Text("Backward-Left Motor: %d", bleft);
                        ImGui::Text("Backward-Right Motor: %d", bright);
                        ImGui::Text("Forward-Up Motor: %d", fvert);
                        ImGui::Text("Backward-Up Motor: %d", bvert);

                        output = "T";

                        output.append(conv((int)(fright - clockwise)));//1
                        output.append(conv((int)(fleft + clockwise)));//2
                        output.append(conv((int)(bright - clockwise)));//3
                        output.append(conv((int)(bleft + clockwise)));//4

                        output.append(conv((int)fvert));//5
                        output.append(conv((int)bvert));//6
                        output.append(opening? conv_norm(200) : closing? conv_norm(0) : conv_norm(100));//7
                        output.append("00");//8
                        output.append("00");//9
                        output.append("00");//10
                        output.append("00");//11
                        output.append(pressing_camera? conv_norm(0) :  conv_norm(180));//12


                        //Left up: Axis 1
                        //Left Horizontal: Axis 0
                        //Right Horizontal: Axis 2
                        //Right up: Axis 3
                        //Green: Button 1
                        //Red: Button 2
					}
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

                            if (prt.isOpen()) prt.write(output);
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


        /*if (show_test_window)
        {
            ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiSetCond_FirstUseEver);
            ImGui::ShowTestWindow(&show_test_window);
        }*/
        nimir::gl::setClearColor(ImColor(114, 144, 154));

        nimir::gui::drawFrame();
        nimir::gl::drawFrame(window);
    }

    nimir::gui::quit();
    nimir::gl::quit();

    return 0;
}
