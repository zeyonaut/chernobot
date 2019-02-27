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

#include <qfs-cpp/qfs.hpp> //qfs doesn't work in linux - use std namespaced strcmp. Also, exe_path doesn't work.

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

#include <opencv2/core/mat.hpp>

int run();

int main (int, char**) try {return run();} catch (...) {throw;} // Force the stack to unwind on an uncaught exception.

int run()
{
	Fin fin;
	
	avdevice_register_all();

	window_t window;

	{
		ImGui::CreateContext();
		ImGui_ImplSdlGL3_Init(window.sdl_window());

		ImGui::StyleColorsDark();
		ImGuiStyle& style = ImGui::GetStyle();
		style.WindowRounding = 0.f;
		style.Alpha = 1.f;
		style.WindowBorderSize= 0.f;
		style.FrameBorderSize= 0.f;

		fin += []()
		{
			ImGui_ImplSdlGL3_Shutdown();
			ImGui::DestroyContext();
		};
	}

	std::array<uint16_t, 12> pin_data;

	Controls c;

	int joystick_index = -1;

	int port_index = -1;
	bool is_port_open = false;

	int is_lemming = 2;
	int mode = 0;

	serial::Serial prt("");
	prt.setBaudrate(115200);

	int acceleration = 33; 
	int eleveration = 400;//percent per second

	GLuint m_texture = 0;

	bool is_takeoff_sand_point = true;

	bool is_error = false;

	int elevate_direction = 0;
	int elevate_magnitude = 0;
	float elevate_amt = 0;

	bool is_using_bool_elev = true;

	texture_t lake_image = texture_t::from_path(qfs::real_path(qfs::dir(qfs::exe_path()) + "../res/lake.png")).value();
	auto const [w, h] = lake_image.size();

	const float PI = 3.1415926;

	float heading = 184;
	float ascent_airspeed = 93;
	float ascent_rate = 10;
	float time_until_failure = 43;
	float descent_airspeed = 64;
	float descent_rate = 6;
	float direction = 270;
	float wind_speed = 9.4;

	int no_of_turbines = 6;
	float rotor_diameter = 14;
	float current_kn = 4.5;
	float efficiency = 35;

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
		if(ImGui::Button("Reset"))
		{
			inital_time = std::chrono::high_resolution_clock::now();
			maintain_time_proper = std::chrono::high_resolution_clock::now() - std::chrono::high_resolution_clock::now();
			lapped_seconds.clear();
		}
		ImGui::SameLine();
		int current_stopwatch;
		if(!clock_state)
		{
			if(stopwatch_state && !clock_state_previous)
			{
				clock_state_previous = true;
			}
			else if(!stopwatch_state && clock_state_previous)
			{
				inital_time = std::chrono::high_resolution_clock::now() - maintain_time_proper;
				clock_state = true;
				clock_state_previous = false;
			}
			ImGui::Text("Stopped");
		}
		else
		{
			current_stopwatch = ((std::chrono::high_resolution_clock::now() - inital_time).count())/1000000000;
			if(lap_state && !lap_state_previous)
			{
				lap_state_previous = true;
			}
			else if(!lap_state && lap_state_previous)
			{
				lap_state_previous = false;
				lapped_seconds.push_back(current_stopwatch);
			}

			if(stopwatch_state && !clock_state_previous)
			{
				clock_state_previous = true;
			}
			else if(!stopwatch_state && clock_state_previous)
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
			if(lapped_seconds.size() + 1 > i)
			{
				ImGui::Text("Lap Time %02d: %02d:%02d", (int)(lapped_seconds.size() - i + 1),(int)((int)(lapped_seconds[lapped_seconds.size() - i])/60), (int)((int)(lapped_seconds[lapped_seconds.size() - i])%60));
			}
		}

		if(lapped_seconds.size() <= 5)
		{
			for(int i = 0; i < 5 - lapped_seconds.size(); i++)
			{
				ImGui::NewLine();
			}
		}
	};

	advd::Videostream vid {};

	std::cout << "DBG: ID = " << lake_image.gl_id() << '\n';
	
	std::future<TextureData> future_video_frame;

	bool running = true;
	while (running)
	{	
		
		if (vid.is_open() && !future_video_frame.valid())
		{
			future_video_frame = std::async(std::launch::async, [&] {return vid.current_frame();});
		}

		if (future_video_frame.valid() && future_video_frame.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)
		{
			auto a = future_video_frame.get();
			lake_image = texture_t::from_data(a.data, {a.w, a.h}, 3);

			if (vid.is_open())
			{
				future_video_frame = std::async(std::launch::async, [&] {return vid.current_frame();});
			}
		}

		/*
			INPUT CALCULATION AND STUFF
		*/

		auto const [window_w, window_h] = window.size();

		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			ImGui_ImplSdlGL3_ProcessEvent(&event);
			if (event.type == SDL_QUIT) running = false;
		}
		ImGui_ImplSdlGL3_NewFrame(window.sdl_window());

		key_now = std::chrono::high_resolution_clock::now();
		double time = std::chrono::duration_cast<std::chrono::duration<double>>(key_now - key_last).count();
		const Uint8* currentKeyStates = SDL_GetKeyboardState( NULL );

		int mdz = 4; //motor deadzone magnitude -- currently, the dead band appears to be around 4 times the intended magnitude. Please fix.
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
					//TODO: null cancellation
					if(SDL_JoystickGetButton(joy, 0) == 1 && SDL_JoystickGetButton(joy, 1) == 0) elevate_direction = -1;
					else if(SDL_JoystickGetButton(joy, 0) == 0 && SDL_JoystickGetButton(joy, 1) == 1) elevate_direction = 1;
					else elevate_direction = 0;
				}

				if (SDL_JoystickNumButtons(joy) >= 12)
				{
					//TODO: null cancellation
					if(SDL_JoystickGetButton(joy, 10) == 1) c.clockwise = 3 * c.clockwise/2;
					if(SDL_JoystickGetButton(joy, 11) == 1) c.clockwise =  2 * c.clockwise;
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

		std::vector<serial::PortInfo> portinfo = serial::list_ports();

		ImGui::SetNextWindowSize(ImVec2(window_w, window_h), ImGuiSetCond_Always);
		ImGui::SetNextWindowPos(ImVec2(0.f, 0.f), ImGuiSetCond_Always);
		ImGui::Begin("Oculus", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
		{
			ImGui::Image((ImTextureID)(uintptr_t)lake_image.gl_id(), ImVec2(window_w, window_h), ImVec2(0,0), ImVec2(1,1), ImVec4(255,255,255,255), ImVec4(255,255,255,0));
		}
		ImGui::End();

		ImGui::SetNextWindowSize(ImVec2(window_w/2, window_h), ImGuiSetCond_Always);
		ImGui::SetNextWindowPos(ImVec2(0.f, 0.f), ImGuiSetCond_Always);
		
		ImGui::Begin("Pilot Console", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
		{
			{
				ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

				static std::vector<advd::StreamRef> videostream_refs;
				static std::string videostream_ref_id = "";
				static int videostream_ref_index = -1;
				static auto last_id = videostream_ref_id;
				static auto last_index = videostream_ref_index;

				if (ImGui::Button("Configure Videostream..")) 
				{
					ImGui::OpenPopup("Videostream Options");

					videostream_refs = advd::StreamRef::enumerate();
					videostream_ref_index = -1;
					for (int i = 0; i < videostream_refs.size(); ++i)
					{
						if (videostream_refs[i].id() == videostream_ref_id) videostream_ref_index = i; 
					}
				}

				if (ImGui::BeginPopupModal("Videostream Options", NULL, ImGuiWindowFlags_AlwaysAutoResize))
				{
					ImGui::ListBoxHeader("Videostream", videostream_refs.size() + 1);
						if (ImGui::Selectable("<none>", videostream_ref_index == -1)) {videostream_ref_index = -1; videostream_ref_id = "";};
						for (int i = 0; i < videostream_refs.size(); ++i) if (ImGui::Selectable(videostream_refs[i].label().c_str(), videostream_ref_index == i)) {videostream_ref_index = i; videostream_ref_id = videostream_refs[i].id();};
					ImGui::ListBoxFooter();

					if (ImGui::Button("Finish"))
					{
						ImGui::CloseCurrentPopup();

						if (last_id != videostream_ref_id)
						{
							if (future_video_frame.valid())
							{
								// Let's get the frame before any invalidation occurs.
								future_video_frame.wait(); future_video_frame.get();
							}

							if (videostream_ref_index != -1)
							{
								vid.open(videostream_refs[videostream_ref_index]);
							}
							else
							{
								vid.close();
							}

							last_id = videostream_ref_id;
							last_index = videostream_ref_index; //TODO - last_index likely unnecessary
						}
					}
					ImGui::SameLine();
					if (ImGui::Button("Cancel"))
					{
						ImGui::CloseCurrentPopup();

						videostream_ref_id = last_id;
						videostream_ref_index = last_index;
					}

					ImGui::EndPopup();
				}
			}

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

			ImGui::NewLine();
			ImGui::Separator();
			ImGui::NewLine();

			ImGui::PushItemWidth(0.3f * window_w);
			ImGui::InputFloat("Heading (deg)", &heading);
			ImGui::InputFloat("Ascent Speed (m/s)", &ascent_airspeed);
			ImGui::InputFloat("Climb Rate (m/s)", &ascent_rate);
			ImGui::InputFloat("Time to Failure (s)", &time_until_failure);
			ImGui::InputFloat("Descent Speed (m/s)", &descent_airspeed);
			ImGui::InputFloat("Sink Rate (m/s)", &descent_rate);
			ImGui::InputFloat("Wind Direction (deg)", &direction);
			ImGui::InputFloat("Wind Speed (m/s)", &wind_speed);
			ImGui::PopItemWidth();

			ImGui::NewLine();
			ImGui::Separator();
			ImGui::NewLine();

			ImGui::PushItemWidth(0.3f * window_w);
			ImGui::InputInt("# of Turbines", &no_of_turbines);
			ImGui::InputFloat("Rotor Diameter (m)", &rotor_diameter);
			ImGui::InputFloat("Velocity (kn)", &current_kn);
			ImGui::InputFloat("Turbine Efficiency (%)", &efficiency);
			ImGui::PopItemWidth();
		}
		ImGui::End();
		
		glViewport(0, 0, (int)ImGui::GetIO().DisplaySize.x, (int)ImGui::GetIO().DisplaySize.y);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui::Render();
		ImGui_ImplSdlGL3_RenderDrawData(ImGui::GetDrawData());

		window.update();
	}

	return 0;
}