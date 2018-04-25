#include <imgui.h>
#include <serial/serial.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <glad/glad.h>
#include <SDL.h>
#include <SDL_keycode.h>

#include "imgui_impl_sdl_gl3.h"

#include <vector>
#include <exception>
#include <iostream>
#include <string> 
#include <array>

#include "overseer.h"

#include <chrono>

int main (int, char**)
{
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_JOYSTICK) != 0)
    {
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }

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
    SDL_Window* window = SDL_CreateWindow("console", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 360, 720, SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_SetSwapInterval(1); // Enable vsync

	gladLoadGL();

	ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    ImGui_ImplSdlGL3_Init(window);

    ImGui::StyleColorsDark();
	ImGuiStyle& style = ImGui::GetStyle();
	style.WindowRounding = 0.f;
	style.Alpha = 1.f;
	style.WindowBorderSize= 0.f;
	style.FrameBorderSize= 0.f;

	glClearColor(0, 0, 0, 1);

	serial::Serial port("");
	port.setBaudrate(115200);

	std::array<uint8_t, 12> pin_data;

	Controls c;

	int joystick_index = -1;

	int port_index = -1;
	bool is_port_open = false;

	int is_lemming = 0;

	serial::Serial prt("");
	prt.setBaudrate(115200);

	bool running = true;

	auto key_last = std::chrono::high_resolution_clock::now();
	auto key_now = std::chrono::high_resolution_clock::now();

	int acceleration = 33; //percent per second

	while (running)
	{	
		int window_w = 640; int window_h = 480;
		SDL_GetWindowSize(window, &window_w, &window_h);
		SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSdlGL3_ProcessEvent(&event);
            if (event.type == SDL_QUIT) running = false;
    	}
        ImGui_ImplSdlGL3_NewFrame(window);

        key_now = std::chrono::high_resolution_clock::now();
    	double time = std::chrono::duration_cast<std::chrono::duration<double>>(key_now - key_last).count();
        const Uint8* currentKeyStates = SDL_GetKeyboardState( NULL );

        int mdz = 25; //motor deadzone magnitude -- currently, the dead band appears to be around 4 times the intended magnitude. Please fix.
		if( currentKeyStates[ SDL_SCANCODE_W ] )
		{
			if (c.forward < mdz) c.forward = mdz; 
			c.forward += (acceleration * time);
		}
		else if( currentKeyStates[ SDL_SCANCODE_S ] )
		{
			if (c.forward > -mdz) c.forward = -mdz; 
			c.forward -= (acceleration * time);
		}
		else c.forward = 0;

		if( currentKeyStates[ SDL_SCANCODE_A ] )
		{
			if (c.right > -mdz) c.right = -mdz; 
			c.right -= (acceleration * time);
		}
		else if( currentKeyStates[ SDL_SCANCODE_D ] )
		{
			if (c.right < mdz) c.right = mdz; 
			c.right += (acceleration * time);
		}
		else c.right = 0;

		if( currentKeyStates[ SDL_SCANCODE_I ] )
		{
			if (c.up < mdz) c.up = mdz; 
			c.up += (acceleration * time);
		}
		else if( currentKeyStates[ SDL_SCANCODE_K ] )
		{
			if (c.up > -mdz) c.up = -mdz; 
			c.up -= (acceleration * time);
		}
		else c.up = 0;

		if( currentKeyStates[ SDL_SCANCODE_J ] )
		{
			if (c.clockwise > -mdz) c.clockwise = -mdz; 
			c.clockwise -= (acceleration * time);
		}
		else if( currentKeyStates[ SDL_SCANCODE_L ] )
		{
			if (c.clockwise < mdz) c.clockwise = mdz; 
			c.clockwise += (acceleration * time);
		}
		else c.clockwise = 0;

		if( currentKeyStates[ SDL_SCANCODE_U ] )
		{
			if (c.moclaw > 0) c.moclaw = 0; 
			c.moclaw -= (acceleration * time * 0.5);
		}
		else if( currentKeyStates[ SDL_SCANCODE_O ] )
		{
			if (c.moclaw < 0) c.moclaw = 0; 
			c.moclaw += (acceleration * time * 0.5);
		}
		else c.moclaw = 0;

		key_last = std::chrono::high_resolution_clock::now();

		std::vector<serial::PortInfo> portinfo = serial::list_ports();

		//TODO: No need for main menu now. Be aware that the next window stars a few pixels below the top to account for the main menu.
		if (ImGui::BeginMainMenuBar())
		{
			ImGui::PushItemWidth(0.3f * window_w);

			ImGui::PopItemWidth();

			ImGui::EndMainMenuBar();
		}

		ImGui::SetNextWindowSize(ImVec2(window_w, window_h), ImGuiSetCond_Always);
		ImGui::SetNextWindowPos(ImVec2(0.f, ImGui::GetFrameHeight()), ImGuiSetCond_Always);
		ImGui::Begin("Console", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

		//ImGui::ListBox("listbox\n(single select)", &listbox_item_current, listbox_items, IM_ARRAYSIZE(listbox_items), 4);

		//TODO? i thought we needed to track the names but it looks like that won't be necessary - some testing appears that it somehow preserves integrity.
		if (port_index >= portinfo.size()) port_index = -1;
		ImGui::ListBoxHeader("Slave", portinfo.size() + 1);
		if (ImGui::Selectable("<none>", port_index == -1)) port_index = -1;
		for (int i = 0; i < portinfo.size(); ++i) if (ImGui::Selectable(portinfo[i].port.c_str(), port_index == i)) port_index = i;
		ImGui::ListBoxFooter();

		if (joystick_index >= SDL_NumJoysticks()) joystick_index = -1;
		ImGui::ListBoxHeader("Joystick", SDL_NumJoysticks() + 1);
		if (ImGui::Selectable("<none>", joystick_index == -1)) joystick_index = -1;
		for (int i = 0; i < SDL_NumJoysticks(); ++i) if (ImGui::Selectable(SDL_JoystickNameForIndex(i), joystick_index == i)) joystick_index = i;
		ImGui::ListBoxFooter();

		ImGui::RadioButton("Saw", &is_lemming, 0); ImGui::SameLine();
		ImGui::RadioButton("Lem", &is_lemming, 1); ImGui::SameLine();
		ImGui::RadioButton("Romulus", &is_lemming, 2);

		if (joystick_index >= -1)
		{
			SDL_Joystick* joy = SDL_JoystickOpen(joystick_index);

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

		ImGui::PushItemWidth(-1);

		int max = 100;


		c.forward = c.forward > max? max : c.forward < -max? -max : c.forward;
		c.right = c.right > max? max : c.right < -max? -max : c.right;
		c.up = c.up > max? max : c.up < -max? -max : c.up;
		c.clockwise = c.clockwise > max? max : c.clockwise < -max? -max : c.clockwise;
		c.moclaw = c.moclaw > 15? 15 : c.moclaw < -15? -15 : c.moclaw;

		ImGui::SliderFloat("Forward", &c.forward, -100.f, 100.f);
		ImGui::SliderFloat("Right", &c.right, -100.f, 100.f);
		ImGui::SliderFloat("Up", &c.up, -100.f, 100.f);
		ImGui::SliderFloat("Clockwise", &c.clockwise, -100.f, 100.f);
		ImGui::SliderFloat("ClawOpening", &c.moclaw, -100.f, 100.f);

		serialize_controls(pin_data, c, is_lemming);

		ImGui::PopItemWidth();

		try
		{
			if (port_index != -1)
			{
				if (!is_port_open)
				{
					is_port_open = true;
					prt.setPort(portinfo[port_index].port);
					prt.open();
				}

				if (prt.isOpen()) prt.write(serialize_data(pin_data));
			}
			else
			{
				is_port_open = false;
				prt.close();
				prt.setPort("");
			}
		}
		catch (std::exception e)
		{
			is_port_open = false;
			prt.close();
			prt.setPort("");
			port_index = -1;
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