#include <imgui.h>
#include <serial/serial.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <glad/glad.h>
//#include <GLFW/glfw3.h>
#include <SDL.h>

#include "imgui_impl_sdl_gl3.h"

#include <vector>
using std::vector;
#include <exception>
#include <iostream>
#include <string>
#include <array>
using std::string;

#include "overseer.h"

int eventFilter( const SDL_Event *e )
{
    /*if( e->type == SDL_VIDEORESIZE )
    {
        SDL_SetVideoMode(e->resize.w,e->resize.h,0,SDL_ANYFORMAT | SDL_RESIZABLE);
        draw();
    }*/
    return 1; // return 1 so all events are added to queue
}

int main (int, char**)
{
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_JOYSTICK) != 0)
    {
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }

    //SDL_SetEventFilter( &eventFilter );

    // Setup window
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_DisplayMode current;
    SDL_GetCurrentDisplayMode(0, &current);
    SDL_Window* window = SDL_CreateWindow("console", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_SetSwapInterval(1); // Enable vsync

	gladLoadGL();

	ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Enable Gamepad Controls

    ImGui_ImplSdlGL3_Init(window);

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

	bool running = true;

	while (running)
	{	
		int window_w = 640; int window_h = 480;
		SDL_GetWindowSize(window, &window_w, &window_h);
		SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSdlGL3_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                running = false;
        }
        ImGui_ImplSdlGL3_NewFrame(window);

		vector<serial::PortInfo> portinfo = serial::list_ports();

		if (ImGui::BeginMainMenuBar())
		{
			ImGui::PushItemWidth(0.3f * window_w);

			if (portIndex >= portinfo.size()) portIndex = -1;
			if (ImGui::BeginCombo("Slave", std::string(portIndex >= 0? portinfo[portIndex].port : "<none>").c_str(), ImGuiComboFlags_NoArrowButton))
			{
				{
					bool is_none_selected = portIndex == -1;
					if (ImGui::Selectable("<none>", is_none_selected)) portIndex = -1;
					if (is_none_selected) ImGui::SetItemDefaultFocus();
				}
				for (int i = 0; i < portinfo.size(); ++i)
				{
					bool is_selected = portIndex == i;
					if (ImGui::Selectable(portinfo[i].port.c_str(), is_selected)) portIndex = i;
					if (is_selected) ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}

	    	if (variableIndex >= SDL_NumJoysticks()) variableIndex = -1;
			if (ImGui::BeginCombo("Joystick", std::string(variableIndex >= 0? SDL_JoystickNameForIndex(variableIndex) : "<none>").c_str(), ImGuiComboFlags_NoArrowButton))
			{
				{
					bool is_none_selected = variableIndex == -1;
					if (ImGui::Selectable("<none>", is_none_selected)) variableIndex = -1;
					if (is_none_selected) ImGui::SetItemDefaultFocus();
				}
				for (int i = 0; i < SDL_NumJoysticks(); ++i)
				{
					bool is_selected = variableIndex == i;
					if (ImGui::Selectable(SDL_JoystickNameForIndex(i), is_selected)) variableIndex = i;
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

		if (variableIndex >= -1)
		{
			SDL_Joystick* joy = SDL_JoystickOpen(variableIndex);

			if (joy)
			{
				if (SDL_JoystickNumAxes(joy) > 3)
				{
					c.forward = -1 * (int) (100.f * SDL_JoystickGetAxis(joy, 1)/32767.f);
					c.right = (int) (100.f * SDL_JoystickGetAxis(joy, 0)/32767.f);
					c.up = -1 * (int) (100.f * SDL_JoystickGetAxis(joy, 3)/32767.f);
					c.clockwise = (int) (100.f * SDL_JoystickGetAxis(joy, 2)/32767.f);
				}
				SDL_JoystickClose(joy);
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

        glViewport(0, 0, (int)ImGui::GetIO().DisplaySize.x, (int)ImGui::GetIO().DisplaySize.y);
        glClear(GL_COLOR_BUFFER_BIT);
		ImGui::Render();
        ImGui_ImplSdlGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
	}

	ImGui_ImplSdlGL3_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

	return 0;
}