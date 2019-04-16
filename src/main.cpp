#include <imgui.h>
#include <serial/serial.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <glad/glad.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_keycode.h>

#include "examples/imgui_impl_sdl.h"
#include "examples/imgui_impl_opengl3.h"

#include <vector>
#include <exception>
#include <iostream>
#include <string> 
#include <array>

#include "overseer.hpp"

#include <cmath>
extern "C"
{
#include <libavdevice/avdevice.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
}

#include "util.hpp"

#include "advd/streamref.hpp"
#include "advd/videostream.hpp"

#include <future>
#include <thread>
#include <queue>

#include <optional>
#include <string_view>

#include "configurator_selector.hpp"

#include "video_interface.hpp"
#include "comm_interface.hpp"
#include "joystick_interface.hpp"
#include "stopwatch_widget.hpp"

#include "oculus.hpp"
#include <opencv2/highgui.hpp>

int run();

int main (int, char**) try {return run();} catch (...) {throw;} // Force the stack to unwind on an uncaught exception.

void show_framerate_meter()
{
	const float distance = 6.f;

	ImGuiIO& io = ImGui::GetIO();

	ImVec2 window_pos = ImVec2(io.DisplaySize.x - distance, distance);
		
	ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, ImVec2(1.f, 0.f));
	ImGui::SetNextWindowBgAlpha(128.f/255.f); // Transparent background
	
	if (ImGui::Begin("framerate_meter", nullptr, ImGuiWindowFlags_Tooltip | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav))
	{
		ImGui::Text("ESC/Configure | BACKTICK/Analyze | %1.f f/s", io.Framerate);
	}
	ImGui::End();
}

enum class UIEvent
{
	toggle_configurator_selector,
};

int run()
{	
	avdevice_register_all();

	cv::namedWindow( "cvdebug", cv::WINDOW_AUTOSIZE );

	Window window;

	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		//ImGui_ImplSdlGL3_Init(window.sdl_window());
		ImGui_ImplSDL2_InitForOpenGL(window.sdl_window(), window.gl_context());
    	ImGui_ImplOpenGL3_Init("#version 150");

		ImGui::StyleColorsDark();
		ImGuiStyle& style = ImGui::GetStyle();
		style.WindowRounding = 0.f;
		style.Alpha = 1.f;
		style.WindowBorderSize= 0.f;
		style.FrameBorderSize= 0.f;

		ImVec4* colors = style.Colors;
		colors[ImGuiCol_WindowBg]			   = ImVec4(0.10f, 0.10f, 0.12f, 0.90f);
		colors[ImGuiCol_ChildBg]				= ImVec4(0.08f, 0.08f, 0.09f, 0.90f);
		colors[ImGuiCol_Border]				 = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
		colors[ImGuiCol_FrameBg]				= ImVec4(0.14f, 0.14f, 0.16f, 0.90f);
		colors[ImGuiCol_FrameBgHovered]		 = ImVec4(0.16f, 0.16f, 0.19f, 0.90f);
		colors[ImGuiCol_FrameBgActive]		  = ImVec4(0.19f, 0.19f, 0.25f, 0.90f);
		colors[ImGuiCol_TitleBg]				= ImVec4(0.08f, 0.08f, 0.09f, 0.90f);
		colors[ImGuiCol_TitleBgActive]		  = ImVec4(0.08f, 0.08f, 0.09f, 0.90f);
		colors[ImGuiCol_TitleBgCollapsed]	   = ImVec4(0.08f, 0.08f, 0.09f, 0.90f);
		colors[ImGuiCol_MenuBarBg]			  = ImVec4(0.08f, 0.08f, 0.09f, 0.90f);
		colors[ImGuiCol_ScrollbarBg]			= ImVec4(0.00f, 0.00f, 0.00f, 0.50f);
		colors[ImGuiCol_CheckMark]			  = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
		colors[ImGuiCol_ModalWindowDarkening]	   = ImVec4(0.00f, 0.00f, 0.00f, 0.38f);
	}

	std::array<uint16_t, 12> pin_data;

	Controls c;

	// TODO: get this legacy stuff under control.

	int is_lemming = 2;

	int acceleration = 33; 
	int eleveration = 400;

	int elevate_direction = 0;
	int elevate_magnitude = 0;
	float elevate_amt = 0;

	bool is_using_bool_elev = true;

	// legacy end!

	auto key_last = std::chrono::high_resolution_clock::now();
	auto key_now = std::chrono::high_resolution_clock::now();

	bool show_demo = false;
	bool show_debug = false;
	
	ConfiguratorSelector<VideoInterface, CommInterface, JoystickInterface> selector{"###configurator"};
	VideoInterface video_interface;
	CommInterface comm_interface;
	JoystickInterface joystick_interface;
	ConsoleWidget console{"Chatter"};
	Oculus oculus{"###oculus"};
	StopwatchWidget stopwatch_widget{"###stopwatch"};

	std::vector<UIEvent> events;

	for (bool running = true; running;)
	{
		events.clear();

		for (SDL_Event event; SDL_PollEvent(&event);)
		{
			ImGui_ImplSDL2_ProcessEvent(&event);
			if (event.type == SDL_QUIT) running = false;
			else if (event.type == SDL_KEYDOWN && event.key.repeat == 0)
			{
				switch (event.key.keysym.sym)
				{
					case SDLK_ESCAPE:
						events.push_back(UIEvent::toggle_configurator_selector);
					break;

					case SDLK_KP_ENTER:
					case SDLK_RETURN:
					case SDLK_RETURN2:
						// TODO: trigger configurator 'confirm' event.
					break;

					case SDLK_SPACE:
						oculus.snap_frame(&video_interface);
						// TODO: get configurator to open subsections on demand.
					break;
					case SDLK_BACKQUOTE:
						if (oculus.state == Oculus::State::present) oculus.state = Oculus::State::past;
						else if (oculus.state == Oculus::State::past) oculus.state = Oculus::State::present;
					break;
					case SDLK_d:
						show_debug = !show_debug;
					break;
				}
			}
		}

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplSDL2_NewFrame(window.sdl_window());
		ImGui::NewFrame();

		for (auto const event: events)
		{
			switch (event)
			{
				case UIEvent::toggle_configurator_selector:
				{
					if (selector.is_open()) selector.cancel();
					else selector.open();
				}
				break;
			}
		}
		
		video_interface.update();

		auto const [window_w, window_h] = window.size();

		key_now = std::chrono::high_resolution_clock::now();
		double time = std::chrono::duration_cast<std::chrono::duration<double>>(key_now - key_last).count();
		const Uint8* currentKeyStates = SDL_GetKeyboardState(NULL);

		int mdz = 4;
		if (currentKeyStates[SDL_SCANCODE_I])
		{
			if (c.forward < mdz) c.forward = mdz; 
			c.forward += (acceleration * time);
		}
		else if (currentKeyStates[SDL_SCANCODE_K])
		{
			if (c.forward > -mdz) c.forward = -mdz; 
			c.forward -= (acceleration * time);
		}
		else c.forward = 0;

		if (currentKeyStates[SDL_SCANCODE_J])
		{
			if (c.right > -mdz) c.right = -mdz; 
			c.right -= (acceleration * time);
		}
		else if (currentKeyStates[SDL_SCANCODE_L])
		{
			if (c.right < mdz) c.right = mdz; 
			c.right += (acceleration * time);
		}
		else c.right = 0;

		if (currentKeyStates[SDL_SCANCODE_W])
		{
			if (c.up < mdz) c.up = mdz; 
			c.up += (acceleration * time);
		}
		else if (currentKeyStates[SDL_SCANCODE_S])
		{
			if (c.up > -mdz) c.up = -mdz; 
			c.up -= (acceleration * time);
		}
		else c.up = 0;

		if (currentKeyStates[SDL_SCANCODE_U])
		{
			c.clockwise = 100;
		}
		else if (currentKeyStates[SDL_SCANCODE_O])
		{
			c.clockwise = -100;
		}
		else c.clockwise = 0;

		if (currentKeyStates[SDL_SCANCODE_U])
		{
			if (c.moclaw > 0) c.moclaw = 0; 
			c.moclaw -= (acceleration * time * 0.5);
		}
		else if (currentKeyStates[SDL_SCANCODE_O])
		{
			if (c.moclaw < 0) c.moclaw = 0; 
			c.moclaw += (acceleration * time * 0.5);
		}
		else c.moclaw = 0;

		if (joystick_interface.joystick_index >= -1)
		{
			// TODO: streamline control flow for joystick interface
			SDL_Joystick* joy = SDL_JoystickOpen(joystick_interface.joystick_index);

			if (joy)
			{
				if (SDL_JoystickNumAxes(joy) > 3)
				{
					
					c.forward = -1 * (int) (400.f * SDL_JoystickGetAxis(joy, 1)/32767.f);
					c.right = (int) (400.f * SDL_JoystickGetAxis(joy, 0)/32767.f);
					c.up = -1 * (int) (400.f * SDL_JoystickGetAxis(joy, 3)/32767.f);
					c.clockwise = (int) (200.f * SDL_JoystickGetAxis(joy, 2)/32767.f);

					elevate_magnitude = 400 - (int)(400 * ((32768 + (int)SDL_JoystickGetAxis(joy, 3))/65535.f));
				}

				if (SDL_JoystickNumButtons(joy) >= 2)
				{
					// TODO: null cancellation
					if (SDL_JoystickGetButton(joy, 0) == 1 && SDL_JoystickGetButton(joy, 1) == 0) elevate_direction = -1;
					else if (SDL_JoystickGetButton(joy, 0) == 0 && SDL_JoystickGetButton(joy, 1) == 1) elevate_direction = 1;
					else elevate_direction = 0;
				}

				if (SDL_JoystickNumButtons(joy) >= 12)
				{
					// TODO: null cancellation
					if (SDL_JoystickGetButton(joy, 10) == 1) c.clockwise = 3 * c.clockwise/2;
					if (SDL_JoystickGetButton(joy, 11) == 1) c.clockwise =  2 * c.clockwise;
				}

				SDL_JoystickClose(joy);
			}
		}

		if (elevate_direction != 0)
		{
			if (elevate_direction > 0 && elevate_amt > elevate_magnitude) elevate_amt += eleveration * -time;
			else if (elevate_direction < 0 && elevate_amt < -elevate_magnitude) elevate_amt +=  eleveration * time;
			else elevate_amt += eleveration * time * elevate_direction;
		}
		else
		{
			if ((int) elevate_amt > 0) elevate_amt -= eleveration * time;
			else if ((int) elevate_amt < 0) elevate_amt += eleveration * time;
			else elevate_amt = 0;
		}

		//c.up = (int)elevate_amt;

		key_last = std::chrono::high_resolution_clock::now();

		oculus.render(window.size(), &video_interface);

		if (show_demo) ImGui::ShowDemoWindow();

		if (show_debug)
		{
			ImGui::SetNextWindowSize(ImVec2(window_w/2, window_h), ImGuiSetCond_Always);
			ImGui::SetNextWindowPos(ImVec2(0.f, 0.f), ImGuiSetCond_Always);
			ImGui::SetNextWindowBgAlpha(240.f/255.f);
			ImGui::Begin("Pilot Console", nullptr, (!show_debug? ImGuiWindowFlags_AlwaysAutoResize : 0) | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
			{			
				if (ImGui::Button("DEBUG ONLY SHOW DEMO")) show_demo = !show_demo;

				ImGui::SameLine();

				ImGui::RadioButton("Saw", &is_lemming, 0); ImGui::SameLine();
				ImGui::RadioButton("Romulus", &is_lemming, 2);

				ImGui::PushItemWidth(0);

				ImGui::NewLine();

				stopwatch_widget.render();

				int max = 400;
			
				if (ImGui::RadioButton("Use Trigger Elevation", is_using_bool_elev)) is_using_bool_elev = true; ImGui::SameLine();
				if (ImGui::RadioButton("Be a Scrub", !is_using_bool_elev)) is_using_bool_elev = false;

				c.forward = c.forward > max? max : c.forward < -max? -max : c.forward;
				c.right = c.right > max? max : c.right < -max? -max : c.right;
				c.up = c.up > max? max : c.up < -max? -max : c.up;
				c.clockwise = c.clockwise > max? max : c.clockwise < -max? -max : c.clockwise;
				c.moclaw = c.moclaw > 15? 15 : c.moclaw < -15? -15 : c.moclaw;

				ImGui::SliderFloat("Forward", &c.forward, -max, max);
				ImGui::SliderFloat("Right", &c.right, -max, max);
				ImGui::SliderFloat("Up", &c.up, -max, max);
				ImGui::SliderFloat("Clockwise", &c.clockwise, -max, max);
				ImGui::SliderFloat("ClawOpening", &c.moclaw, -max, max);

				ImGui::PopItemWidth();
			}
			ImGui::End();
		}

		serialize_controls(pin_data, c, is_lemming);
		comm_interface.update(&console, pin_data);

		console.render(nullptr);

		show_framerate_meter();
		
		selector.render(&video_interface, &comm_interface, &joystick_interface);

		glViewport(0, 0, static_cast<int>(ImGui::GetIO().DisplaySize.x), static_cast<int>(ImGui::GetIO().DisplaySize.y));
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		window.update();
	}

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();

	return 0;
}