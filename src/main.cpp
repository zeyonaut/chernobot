#include <imgui.h>
#include <serial/serial.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <glad/glad.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_keycode.h>

#include "util/imgui_impl_sdl_gl3.h"

#include <vector>
#include <exception>
#include <iostream>
#include <string> 
#include <array>

#include <qfs-cpp/qfs.hpp> // qfs doesn't work in linux - use std namespaced strcmp. Also, exe_path doesn't work.

#include "overseer.h"

#include <chrono>
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

#include "util/fin.h"

#include "util.hpp"

#include "macros.hpp"

#include "advd/streamref.hpp"
#include "advd/videostream.hpp"

#include <future>
#include <thread>
#include <queue>

#include <optional>
#include <string_view>

#include "chernobot.hpp"

//#include <opencv2/core/mat.hpp>

struct ConsoleWidget
{
	std::vector<std::string> history;
	std::string title;
	bool should_autoscroll;

	ConsoleWidget (const std::string_view title): title(title), should_autoscroll(true) {}

	void render (bool *p_open)
	{
		if (history.size() > 512 + 128) history.erase(history.begin(), history.begin() + 128);

		ImGui::SetNextWindowSize(ImVec2(600,240), ImGuiCond_FirstUseEver);
        if (ImGui::Begin(this->title.c_str(), p_open))
		{
			ImGui::Checkbox("Should Autoscroll", &should_autoscroll);
			ImGui::BeginChild("Log", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4,1));
			{
				for (auto &i : history)
				{
					ImGui::Text("%s", i.c_str());
				}
				if (should_autoscroll)
				{
					ImGui::SetScrollHere();
				}
			}
			ImGui::PopStyleVar();
			ImGui::EndChild(); 

		}
		ImGui::End();
	}
};

int run();

int main (int, char**) try {return run();} catch (...) {throw;} // Force the stack to unwind on an uncaught exception.

void show_framerate_meter()
{
	const float distance = 6.f;

	ImGuiIO& io = ImGui::GetIO();

	ImVec2 window_pos = ImVec2(io.DisplaySize.x - distance, distance);
		
	ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, ImVec2(1.f, 0.f));
	ImGui::SetNextWindowBgAlpha(240.f/255.f); // Transparent background
	
	if (ImGui::Begin("framerate_meter", nullptr, ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav))
	{
		ImGui::Text("%1.f f/s", ImGui::GetIO().Framerate);
	}
	ImGui::End();
}

int run()
{
	Fin fin;
	
	avdevice_register_all();

	Window window;

	{
		ImGui::CreateContext();
		ImGui_ImplSdlGL3_Init(window.sdl_window());

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

		fin += []()
		{
			ImGui_ImplSdlGL3_Shutdown();
			ImGui::DestroyContext();
		};
	}

	std::array<uint16_t, 12> pin_data;

	Controls c;

	// TODO - get this legacy stuff under control.

	int is_lemming = 2;
	serial::Serial prt("");
	prt.setBaudrate(115200);

	int acceleration = 33; 
	int eleveration = 400;

	int elevate_direction = 0;
	int elevate_magnitude = 0;
	float elevate_amt = 0;

	bool is_using_bool_elev = true;

	// legacy end!

	std::optional<Texture> current_frame;

	// TODO - get this stopwatch stuff in its own class.

	auto key_last = std::chrono::high_resolution_clock::now();
	auto key_now = std::chrono::high_resolution_clock::now();

	bool clock_state = false;
	bool clock_state_previous = false;
	auto inital_time = std::chrono::high_resolution_clock::now();
	auto maintain_time_proper = (std::chrono::high_resolution_clock::now() - std::chrono::high_resolution_clock::now());
	std::vector<int> lapped_seconds;
	bool lap_state_previous = false;

	auto update_imgui_stopwatch = [&]
	{
		bool stopwatch_state = ImGui::Button("Stopwatch");
		ImGui::SameLine();
		bool lap_state = ImGui::Button("Lap");
		ImGui::SameLine();
		if (ImGui::Button("Reset"))
		{
			inital_time = std::chrono::high_resolution_clock::now();
			maintain_time_proper = std::chrono::high_resolution_clock::now() - std::chrono::high_resolution_clock::now();
			lapped_seconds.clear();
		}
		ImGui::SameLine();
		int current_stopwatch;
		if (!clock_state)
		{
			if (stopwatch_state && !clock_state_previous)
			{
				clock_state_previous = true;
			}
			else if (!stopwatch_state && clock_state_previous)
			{
				inital_time = std::chrono::high_resolution_clock::now() - maintain_time_proper;
				clock_state = true;
				clock_state_previous = false;
			}
			ImGui::Text("Stopped");
		}
		else
		{
			current_stopwatch = ((std::chrono::high_resolution_clock::now() - inital_time).count()) / 1000000000;
			if (lap_state && !lap_state_previous)
			{
				lap_state_previous = true;
			}
			else if (!lap_state && lap_state_previous)
			{
				lap_state_previous = false;
				lapped_seconds.push_back(current_stopwatch);
			}

			if (stopwatch_state && !clock_state_previous)
			{
				clock_state_previous = true;
			}
			else if (!stopwatch_state && clock_state_previous)
			{
				maintain_time_proper = std::chrono::high_resolution_clock::now() - inital_time;
				clock_state = false;
				clock_state_previous = false;
			}
			ImGui::Text("Running");
		}

		ImGui::Text("Stopwatch Time: %02d:%02d; Lap: %02d:%02d", ((int)(current_stopwatch)/60),((int)(current_stopwatch)%60), lapped_seconds.size()>0? (current_stopwatch - lapped_seconds[lapped_seconds.size()-1])/60 : current_stopwatch/60, lapped_seconds.size()>0? (current_stopwatch - lapped_seconds[lapped_seconds.size()-1]) % 60 : current_stopwatch % 60);

		// LAP DISPLAY

		ImGui::NewLine();

		for(int i = 1; i <= 5 ; i++)
		{
			if (lapped_seconds.size() + 1 > i)
			{
				ImGui::Text("Lap Time %02d: %02d:%02d", (int)(lapped_seconds.size() - i + 1),(int)((int)(lapped_seconds[lapped_seconds.size() - i])/60), (int)((int)(lapped_seconds[lapped_seconds.size() - i])%60));
			}
		}

		if (lapped_seconds.size() <= 5)
		{
			for(int i = 0; i < 5 - lapped_seconds.size(); i++)
			{
				ImGui::NewLine();
			}
		}
	};

	// Stopwatch end!

	advd::Videostream vid {};

	std::future<TextureData> future_video_frame;

	bool show_demo = false;

	bool show_debug = true;

	ConsoleWidget console {"Chatter"};

	bool running = true;

	ConfiguratorWidget configurator;
	bool config_closed = true;

	while (running)
	{	
		ImGui_ImplSdlGL3_NewFrame(window.sdl_window());
		
		if (vid.is_open() && !future_video_frame.valid())
		{
			future_video_frame = std::async(std::launch::async, [&] {return vid.current_frame();});
		}

		if (future_video_frame.valid() && future_video_frame.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)
		{
			auto a = future_video_frame.get();
			current_frame = Texture::from_data(a.data, {a.w, a.h}, 3);

			if (vid.is_open())
			{
				future_video_frame = std::async(std::launch::async, [&] {return vid.current_frame();});
			}
		}

		auto const [window_w, window_h] = window.size();

		// TODO - this is insanity. Transfer to its own class, and make this the update() function.

		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			ImGui_ImplSdlGL3_ProcessEvent(&event);
			if (event.type == SDL_QUIT) running = false;
			else if (event.type == SDL_KEYDOWN && event.key.repeat == 0)
			{
				switch (event.key.keysym.sym)
				{
					case SDLK_ESCAPE:
						switch (configurator.state)
						{
							case ConfiguratorWidget::State::root:
								config_closed = !config_closed;
								if (!config_closed) ImGui::OpenPopup("###configurator");
							break;
							default:
								configurator.go_back();
							break;
						}
					break;

					case SDLK_KP_ENTER:
					case SDLK_RETURN:
					case SDLK_RETURN2:
						// TODO: trigger configurator 'confirm' event.
					break;

					case SDLK_v:
						// TODO: get configurator to open subsections on demand.
					break;
					case SDLK_c:
						// TODO: get configurator to open subsections on demand.
					break;
					case SDLK_b:
						// TODO: get configurator to open subsections on demand.
					break;
				}
			}
		}

		// TODO - this is insanity - relocate to own class as member variables
		// TODO - find better solution for caching previous values.
		static std::vector<advd::StreamRef> videostream_refs;
		static std::string videostream_ref_id = "";
		static int videostream_ref_index = -1;
		static auto last_id = videostream_ref_id;
		static auto last_index = videostream_ref_index;

		static std::vector<serial::PortInfo> portinfo;
		static int port_index = -1;
		static auto last_port_index = port_index;

		static int joystick_index = -1;
		static auto tentative_joystick_index = joystick_index;

		// TODO - this is insanity - transfer to own class as 

		key_now = std::chrono::high_resolution_clock::now();
		double time = std::chrono::duration_cast<std::chrono::duration<double>>(key_now - key_last).count();
		const Uint8* currentKeyStates = SDL_GetKeyboardState(NULL);

		int mdz = 4; // Motor deadzone magnitude -- currently, the dead band appears to be around 4 times the intended magnitude. Please fix.
		if (currentKeyStates[SDL_SCANCODE_W])
		{
			if (c.forward < mdz) c.forward = mdz; 
			c.forward += (acceleration * time);
		}
		else if (currentKeyStates[SDL_SCANCODE_S])
		{
			if (c.forward > -mdz) c.forward = -mdz; 
			c.forward -= (acceleration * time);
		}
		else c.forward = 0;

		if (currentKeyStates[SDL_SCANCODE_A])
		{
			if (c.right > -mdz) c.right = -mdz; 
			c.right -= (acceleration * time);
		}
		else if (currentKeyStates[SDL_SCANCODE_D])
		{
			if (c.right < mdz) c.right = mdz; 
			c.right += (acceleration * time);
		}
		else c.right = 0;

		if (currentKeyStates[SDL_SCANCODE_I])
		{
			if (c.up < mdz) c.up = mdz; 
			c.up += (acceleration * time);
		}
		else if (currentKeyStates[SDL_SCANCODE_K])
		{
			if (c.up > -mdz) c.up = -mdz; 
			c.up -= (acceleration * time);
		}
		else c.up = 0;

		if (currentKeyStates[SDL_SCANCODE_J])
		{
			if (c.clockwise > -mdz) c.clockwise = -mdz; 
			c.clockwise -= (acceleration * time);
		}
		else if (currentKeyStates[SDL_SCANCODE_L])
		{
			if (c.clockwise < mdz) c.clockwise = mdz; 
			c.clockwise += (acceleration * time);
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

		if (joystick_index >= -1)
		{
			SDL_Joystick* joy = SDL_JoystickOpen(joystick_index);

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

		ImGui::SetNextWindowSize(ImVec2(window_w, window_h), ImGuiSetCond_Always);
		ImGui::SetNextWindowPos(ImVec2(0.f, 0.f), ImGuiSetCond_Always);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2 {0.f, 0.f});
		ImGui::Begin("Oculus", nullptr, ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
		{
			if (current_frame) ImGui::Image((ImTextureID)(uintptr_t)current_frame->gl_id(), ImVec2(window_w, window_h), ImVec2(0,0), ImVec2(1,1), ImVec4(255,255,255,255), ImVec4(255,255,255,0));
			else
			{
				auto size = ImGui::CalcTextSize("[No Videostream Connected]");
				
				ImGui::SetCursorPos(ImVec2 {(window_w - size.x) / 2, (window_h - size.y) / 2});

				ImGui::Text("[No Videostream Connected]");
			}
		}
		ImGui::End();
		ImGui::PopStyleVar();

		if (show_demo) ImGui::ShowDemoWindow();

		if (show_debug) ImGui::SetNextWindowSize(ImVec2(window_w/2, window_h), ImGuiSetCond_Always);
		ImGui::SetNextWindowPos(ImVec2(0.f, 0.f), ImGuiSetCond_Always);
		ImGui::SetNextWindowBgAlpha(240.f/255.f); // Transparent background
		ImGui::Begin("Pilot Console", nullptr, (!show_debug? ImGuiWindowFlags_AlwaysAutoResize : 0) | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
		{
			if (ImGui::Button("Show Debug Menu")) show_debug = !show_debug; 

			if (!show_debug)
			{
				ImGui::End();
				goto pilot_console_end;
			}
			
			if (ImGui::Button("DEBUG ONLY SHOW DEMO")) show_demo = !show_demo;

			ImGui::RadioButton("Saw", &is_lemming, 0); ImGui::SameLine();
			ImGui::RadioButton("Romulus", &is_lemming, 2);

			ImGui::PushItemWidth(0);

			ImGui::NewLine();

			update_imgui_stopwatch();

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

			serialize_controls(pin_data, c, is_lemming);

			ImGui::PopItemWidth();

			/*
			SERIAL COMMUNICATION
			*/

			static std::string nextline;

			try
			{
				if (prt.isOpen())
				{
					prt.write(serialize_data(pin_data));
				
					std::string next;
					for (int i = 0; i < prt.available(); ++i)
					{
						next = prt.read();
						if (next == "\n")
						{
							console.history.push_back(nextline);
							nextline = "";
						}
						else
						{
							nextline += next;
						}
					}
				}
			}
			catch (std::exception e)
			{
				//is_port_open = false;
				prt.close();
				prt.setPort("");
				port_index = -1;
			}
		}
		ImGui::End();

		pilot_console_end:
		
		console.render(nullptr);

		show_framerate_meter();
		
		//TODO this is insanity

		ImGui::PushStyleVar(ImGuiStyleVar_WindowTitleAlign, ImVec2(0.5, 0.5));

		// TODO: don't be wasteful constructing strings every frame. Why would you do this?
        ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
		if (ImGui::BeginPopupModal("###configurator", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar))
		{
			if (config_closed) ImGui::CloseCurrentPopup();
			
			configurator.run
			(
				[&] (std::optional<advd::StreamRef> maybe_stream_ref)
				{
					if (future_video_frame.valid())
					{
						// Let's get the frame before any invalidation occurs.
						// TODO - invalidation can occur anyways which makes a black screen (? I think) so we're going to need a little more safety on the videostream side of things. Better fix it before it becomes an issue.
						future_video_frame.wait(); future_video_frame.get();
					}

					if (maybe_stream_ref)
					{
						vid.open(*maybe_stream_ref);
					}
					else
					{
						vid.close();
						current_frame.reset();
					}
				},
				[&] (std::optional<std::string> port_address)
				{
					if (port_address)
					{
						try
						{
							prt.setPort(*port_address);
							prt.open();
						}
						catch (...)
						{
							prt.close();
							prt.setPort("");
						}
					}
					else
					{
						prt.close();
						prt.setPort("");
					}
				}
			);

			ImGui::EndPopup();
		}
		ImGui::PopStyleVar();

		glViewport(0, 0, (int)ImGui::GetIO().DisplaySize.x, (int)ImGui::GetIO().DisplaySize.y);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui::Render();
		ImGui_ImplSdlGL3_RenderDrawData(ImGui::GetDrawData());

		window.update();
	}

	return 0;
}