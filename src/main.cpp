#include <imgui.h>
#include <serial/serial.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "imgui_impl_glfw_gl3.h"

#include <vector>
using std::vector;
#include <exception>
#include <iostream>
#include <string>
#include <array>
using std::string;

#include "overseer.h"

int main (int, char**)
{
	glfwSetErrorCallback
	(
		[](int error, const char* description)
	    {
	        fprintf(stderr, "Error %d: %s\n", error, description);
	    }
    );
	if (!glfwInit()) return 1;
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#if __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
	GLFWwindow* window = glfwCreateWindow(1280, 720, "chernobot - console", NULL, NULL);
	glfwMakeContextCurrent(window);
	glfwSwapInterval( 1 );
	gladLoadGL();

	ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Enable Gamepad Controls
    ImGui_ImplGlfwGL3_Init(window, false);

    glfwSetMouseButtonCallback(window, [](GLFWwindow* window, int button, int action, int mods) { ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods); });
	glfwSetScrollCallback(window, [](GLFWwindow* window, double xoffset, double yoffset) { ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset); });
	glfwSetKeyCallback(window, [](GLFWwindow* window, int key, int scancode, int action, int mods) { ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods); });
	glfwSetCharCallback(window, [](GLFWwindow* window, unsigned int c) { ImGui_ImplGlfw_CharCallback(window, c); });

    // Setup style
    ImGui::StyleColorsDark();

	ImGuiStyle& style = ImGui::GetStyle();
	style.WindowRounding = 0.f;
	style.Alpha = 1.f;
	style.WindowBorderSize= 0.f;
	style.FrameBorderSize= 0.f;

	glClearColor(0, 0, 0, 1);

	serial::Serial port("");
	port.setBaudrate(115200);

	std::array<unsigned char, 12> pin_data;

	// Selected Joystick in Combo Box
	Controls c;

	int variableIndex = -1;

	int portIndex = -1;
	bool portOpen = false;

	int is_lemming = 0;

	serial::Serial prt("");
	prt.setBaudrate(115200);

	while (!glfwWindowShouldClose(window))
	{	
		glfwPollEvents();

		int window_w, window_h;
		glfwGetWindowSize(window, &window_w, &window_h);

		ImGui_ImplGlfwGL3_NewFrame();

		const char* joysticks [GLFW_JOYSTICK_LAST];
		int maxJoystick = GLFW_JOYSTICK_LAST;
		for (int i = 0; i < GLFW_JOYSTICK_LAST; i++)
		{
			if(glfwJoystickPresent(i)) joysticks[i] = glfwGetJoystickName(i);
			else {joysticks[i] = "";}
		}

		vector<serial::PortInfo> portinfo = serial::list_ports();

		if (ImGui::BeginMainMenuBar())
	    {
	    	ImGui::PushItemWidth(0.3f * window_w);

			if (ImGui::BeginCombo("##slave_selector", std::string("slave: ").append(portIndex >= 0 && portIndex < portinfo.size()? portinfo[portIndex].port : "none").c_str(), ImGuiComboFlags_NoArrowButton))
			{
				for (int i = 0; i < portinfo.size(); ++i)
				{
					bool is_selected = portIndex == i;
					if (ImGui::Selectable(portinfo[i].port.c_str(), is_selected)) portIndex = i;
					if (is_selected) ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}

			if (ImGui::BeginCombo("##joy_selector", std::string("joy: ").append(variableIndex >= 0 && variableIndex < GLFW_JOYSTICK_LAST? joysticks[variableIndex] : "none").c_str(), ImGuiComboFlags_NoArrowButton))
			{
				for (int i = 0; i < GLFW_JOYSTICK_LAST; ++i)
				{
					bool is_selected = variableIndex == i;
					if (ImGui::Selectable(joysticks[i], is_selected)) variableIndex = i;
					if (is_selected) ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}

			ImGui::PopItemWidth();

			ImGui::EndMainMenuBar();
		}

		ImGui::SetNextWindowSize(ImVec2(window_w, window_h), ImGuiSetCond_Always);
		ImGui::SetNextWindowPos(ImVec2(0.f, ImGui::GetFrameHeight()), ImGuiSetCond_Always);
		ImGui::Begin("Console", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

		ImGui::Text("Application running at %.1f FPS", ImGui::GetIO().Framerate);

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
				c.forward = -1 * (int) (100 * rawAxes[1]);
				c.right = (int) (100 * rawAxes[0]);
				c.up = -1 * (int) (100 *rawAxes[3]);
				c.clockwise = (int) (100 *rawAxes[2]);
			}

			if (3 < bucount)
			{
				//opening = rawButtons[1] == GLFW_PRESS;
			}

		}

		ImGui::SliderInt("Forward", &c.forward, -100.f, 100.f);
		ImGui::SliderInt("Right", &c.right, -100.f, 100.f);
		ImGui::SliderInt("Up", &c.up, -100.f, 100.f);
		ImGui::SliderInt("Clockwise", &c.clockwise, -100.f, 100.f);

		ImGui::SliderInt("Chlaw", &c.moclaw, -100.f, 100.f);

		serialize_controls(pin_data, c, is_lemming);

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

		ImGui::End();

		int w, h;
        glfwGetFramebufferSize(window, &w, &h);
		glViewport(0, 0, w, h);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui::Render();
		ImGui::Render();
        ImGui_ImplGlfwGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
	}

	ImGui_ImplGlfwGL3_Shutdown();
    ImGui::DestroyContext();

	glfwDestroyWindow(window);
    glfwTerminate();

	return 0;
}