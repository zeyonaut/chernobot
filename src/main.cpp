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

#include "hpcp/fs.hpp"
#include "overseer.h"

#include <chrono>

#include <cmath>

#if defined (__APPLE__)
#include <mach-o/dyld.h>
//#include <sys/param.h>
#include <libgen.h>
#endif

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
	SDL_Window* window = SDL_CreateWindow("Chernobot - Pilot Console", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 720, 800, SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);
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

	int is_lemming = 2;
	int mode = 0;

	serial::Serial prt("");
	prt.setBaudrate(115200);

	bool running = true;

	int acceleration = 33; 
	int eleveration = 100;//percent per second

	GLuint m_texture = 0;

	bool is_takeoff_sand_point = true;

	bool is_error = false;

	std::string img_path = "../res/lake.png";

	int elevate_direction = 0;
	int elevate_magnitude = 0;
	float elevate_amt = 0;

	bool is_using_bool_elev = true;

	/*
	- elevate_magnitude is the unsigned intp of the (sensitivity) axis. 
	- elevate direction is either hat_switch
	*/

#if defined (__APPLE__)
	{
		char* real_path;
		char* path;
		uint32_t size = 1024;

		do {free(path); path = (char*) malloc(size);} //TODO: Check for null.
		while (_NSGetExecutablePath(path, &size) != 0);

		real_path = (char*) malloc(size); //TODO: Check for null.
		realpath(hpcp::get_directory(path).append("../res/lake.png").c_str(), real_path); //TODO: use safer function for path canonicalization.
		img_path = std::string(real_path);

		free(path);
		free(real_path);
	}
#endif

	int w;
	int h;
	if (!is_error)
	{
		int comp;
		unsigned char* image = stbi_load(img_path.c_str(), &w, &h, &comp, STBI_rgb_alpha);

		if(image == nullptr) {std::cout << "ERR: the executable is not in a child directory of chernobot root. Please run from tmp or bin.\n"; is_error = true;}
		else
		{
			glGenTextures(1, &m_texture);
			glBindTexture(GL_TEXTURE_2D, m_texture);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
			glBindTexture(GL_TEXTURE_2D, 0);

			stbi_image_free(image);
		}
	}

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

	while (running)
	{	

		/*
			INPUT CALCULATION AND STUFF
		*/

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
					
					c.forward = -1 * (int) (100.f * SDL_JoystickGetAxis(joy, 1)/32767.f);
					c.right = (int) (100.f * SDL_JoystickGetAxis(joy, 0)/32767.f);
					c.up = -1 * (int) (100.f * SDL_JoystickGetAxis(joy, 3)/32767.f);
					c.clockwise = (int) (50.f * SDL_JoystickGetAxis(joy, 2)/32767.f);

					elevate_magnitude = 100 - (int)(100 * ((32768 + (int)SDL_JoystickGetAxis(joy, 3))/65535.f));
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

		c.up = (int)elevate_amt;

		key_last = std::chrono::high_resolution_clock::now();

		/*
			IMGUI DEBUGINFO
		*/

		std::vector<serial::PortInfo> portinfo = serial::list_ports();

			//TODO: No need for main menu now. Be aware that the next window stars a few pixels below the top to account for the main menu.
		/*ImGui::SetNextWindowSize(ImVec2(window_w, window_h - ImGui::GetFrameHeight()), ImGuiSetCond_Always);
		ImGui::SetNextWindowPos(ImVec2(0.f, ImGui::GetFrameHeight()), ImGuiSetCond_Always);
		ImGui::Begin("Pilot Console", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
		*/
		ImGui::SetNextWindowSize(ImVec2(window_w/2, window_h), ImGuiSetCond_Always);
		ImGui::SetNextWindowPos(ImVec2(0.f, 0.f), ImGuiSetCond_Always);
		ImGui::Begin("Pilot Console", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
		//if (mode == 0) //pilot
		{
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
			//ImGui::RadioButton("Lem", &is_lemming, 1); ImGui::SameLine();
			ImGui::RadioButton("Romulus", &is_lemming, 2);

			ImGui::PushItemWidth(0);

			//*
			int max = 100;
			/*/
			int max = 400;
			//*/

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
		//else if (mode == 1) //Crash Calculator
		ImGui::SetNextWindowSize(ImVec2(window_w/2, window_h), ImGuiSetCond_Always);
		ImGui::SetNextWindowPos(ImVec2(window_w/2, 0.f), ImGuiSetCond_Always);
		ImGui::Begin("Calculator", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
		{
			float airvec_r = ascent_airspeed * time_until_failure;

			float airvec_north = airvec_r * cos(heading * PI/ 180);
			float airvec_east = airvec_r * sin(heading * PI/ 180);

			float desvec_r = descent_airspeed * (ascent_rate * time_until_failure / descent_rate);

			float desvec_north = desvec_r * cos(heading * PI/ 180);
			float desvec_east = desvec_r * sin(heading * PI/ 180);

			float wind_r = wind_speed * (ascent_rate * time_until_failure / descent_rate);

			float wind_north = wind_r * cos((int)(direction - 180) % 360 * PI/ 180);
			float wind_east = wind_r * sin((int)(direction - 180) % 360 * PI/ 180);
			float power_generated = no_of_turbines * 0.5 * (1025 * (pow(rotor_diameter / 2, 2) * PI) * pow((current_kn * 0.5144444444444), 3) * efficiency / (100 /*percent to ratio*/ * 1000000 /*watts to megawatts*/));
			
			ImGui::Text("II.1. Ascent Vector: [%d] m, heading [%d]", (int)(airvec_r + 0.5), (int)(heading));
			ImGui::Text("II.2. Descent Vector: [%d] m, heading [%d]", (int)(desvec_r + 0.5), (int)(heading));
			ImGui::Text("II.3. Wind Vector: [%d] m, heading [%d]", (int)(wind_r + 0.5), (int)(direction - 180) % 360);
			ImGui::Text("III. Power Generated: [%.4f] MW", power_generated);

			if (!is_error)
			{

				ImGui::NewLine();
				ImGui::Text("Flight Data Preview:");
				ImGui::Separator();

				int content_width = ImGui::GetWindowWidth() - ImGui::GetStyle().ScrollbarSize - 20;
				
				float scalefactor = content_width /(1000.f * 15.f); /*Given a distance in meters, times one km/1000 m, times one width/15 km then multiply by contentregion/width to get the distance in content regions*/

				if (ImGui::RadioButton("NAS Sand Point", is_takeoff_sand_point)) is_takeoff_sand_point = true; ImGui::SameLine();
				if (ImGui::RadioButton("Renton Airfield", !is_takeoff_sand_point)) is_takeoff_sand_point = false;
				
				ImVec2 orig = ImGui::GetCursorScreenPos(); //top left

				ImGui::Image((ImTextureID)(uintptr_t)m_texture, ImVec2(content_width, h * content_width/w), ImVec2(0,0), ImVec2(1,1), ImVec4(255,255,255,255), ImVec4(255,255,255,0));

				ImDrawList* draw_list = ImGui::GetWindowDrawList();
				draw_list->PushClipRect(orig, ImVec2(orig.x + content_width, orig.y + h * content_width/w), false);
				if (is_takeoff_sand_point)
				{
					orig.x += scalefactor * 8507.692;
					orig.y += scalefactor * 8353.846;
				}
				else
				{
					orig.x += scalefactor * 10692.307;
					orig.y += scalefactor * 27615.384;
				}

				//draw_list->AddLine(ImGui::GetMousePos(), ImVec2(ImGui::GetMousePos().x + scalefactor * 3999, ImGui::GetMousePos().y - scalefactor * 3999), ImColor(0, 255, 0, 255), 6);
				draw_list->AddLine(orig, ImVec2(orig.x + scalefactor * (airvec_east), orig.y - scalefactor * (airvec_north)), ImColor(0, 255, 0, 255), 6);
				draw_list->AddLine(ImVec2(orig.x + scalefactor * (airvec_east), orig.y - scalefactor * (airvec_north)), ImVec2(orig.x + scalefactor * (airvec_east + desvec_east), orig.y - scalefactor * (airvec_north + desvec_north)), ImColor(255, 0, 0, 255), 6);
				draw_list->AddLine(ImVec2(orig.x + scalefactor * (airvec_east + desvec_east), orig.y - scalefactor * (airvec_north + desvec_north)), ImVec2(orig.x + scalefactor * (airvec_east + desvec_east + wind_east), orig.y - scalefactor * (airvec_north + desvec_north + wind_north)), ImColor(0, 0, 255, 255), 6);
				draw_list->PopClipRect();
			}
		}
		ImGui::End();

		glViewport(0, 0, (int)ImGui::GetIO().DisplaySize.x, (int)ImGui::GetIO().DisplaySize.y);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui::Render();
		ImGui_ImplSdlGL3_RenderDrawData(ImGui::GetDrawData());
		SDL_GL_SwapWindow(window);
	}

	glDeleteTextures(0, &m_texture);

	ImGui_ImplSdlGL3_Shutdown();
	ImGui::DestroyContext();

	SDL_GL_DeleteContext(gl_context);
	SDL_DestroyWindow(window);
	SDL_Quit();

#if defined (__APPLE__)
	//free(realp);
#endif

	return 0;
}