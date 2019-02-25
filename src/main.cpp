#include <imgui.h>
#include <serial/serial.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <glad/glad.h>
#include <SDL.h>
#include <SDL_keycode.h>

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




int run();

int main (int, char**) try {return run();} catch (...) {throw;} // Force the stack to unwind on an uncaught exception.

int run()
{
	Fin fin;
	
	avdevice_register_all();

	/*AVFormatContext *format_context = avformat_alloc_context();
	fin += [&]{avformat_free_context(format_context);};*/

//avformat_open_input(&format_context, "Face", fmt, )
	  /*for (int i = 0; i < device_list->nb_devices; i++) {
			av_log(avctx, AV_LOG_INFO, "\t'%s'\n", device_list->devices[i]->device_description);
	}*/

/*
	avdevice_register_all();
		//av_log_set_level(AV_LOG_ERROR);

		AVInputFormat*   cameraFormat		= av_find_input_format("avfoundation");
		AVFormatContext* cameraFormatContext = avformat_alloc_context();
		AVCodec*		 cameraCodec		 = NULL;
		AVCodecContext*  cameraCodecContext  = NULL;
		int			  cameraVideo		 = 0;
		std::string		   cameraSource	= "FaceTime HD Camera (Built-in)" //"USB 2.0 PC Cam";;
		AVFrame	*		 rawFrame = NULL, *convertedFrame = NULL;
		SwsContext*	  imageConversionContext ;
		unsigned char*		   frameBuffer;
		int			  frameBytes;

		AVPacket		 cameraPacket;
		int			  isFrameFinished = 0;*/
/*
		if (avformat_open_input(&cameraFormatContext, cameraSource.toStringz, cameraFormat, null) != 0) return;
		//if (avformat_find_stream_info(inFormatContext) < 0) return;
		av_dump_format(cameraFormatContext, 0, cameraSource.toStringz, 0);

		for (int i = 0; i < cameraFormatContext.nb_streams; i++)
		{	   
				if (cameraFormatContext.streams[i].codec.codec_type == AVMediaType.AVMEDIA_TYPE_VIDEO)
				{	   
						cameraVideo = i;
						break;
				}
		}

		if(cameraVideo == -1) return;

		cameraCodec = avcodec_find_decoder(cameraFormatContext.streams[cameraVideo].codec.codec_id);
		cameraCodecContext = cameraFormatContext.streams[cameraVideo].codec;

		//warn: camera source size is added manually. find a way to fix it.
		cameraCodecContext.width = 640;

		cameraCodecContext.height = 480;
		cameraCodecContext.pix_fmt = AVPixelFormat.AV_PIX_FMT_UYVY422;

		//warn: find a way to input uyvy422 as pixel format for camera.
		if(cameraCodec == null) return;
		if(cameraCodecContext == null) return;
		if(avcodec_open2(cameraCodecContext, cameraCodec, null) < 0) return;

		cameraCodecContext.pix_fmt = AVPixelFormat.AV_PIX_FMT_UYVY422;

		rawFrame = av_frame_alloc(); convertedFrame = av_frame_alloc();

		frameBytes = avpicture_get_size(AVPixelFormat.AV_PIX_FMT_UYVY422, cameraCodecContext.width, cameraCodecContext.height);
		frameBuffer = cast(ubyte*) av_malloc(frameBytes * ubyte.sizeof);
		avpicture_fill(cast(AVPicture*) convertedFrame, frameBuffer, AVPixelFormat.AV_PIX_FMT_UYVY422, cameraCodecContext.width, cameraCodecContext.height);

		// writefln("%i%i", cameraCodecContext.pix_fmt, AVPixelFormat.AV_PIX_FMT_RGB24);
		imageConversionContext = sws_getCachedContext(null, cameraCodecContext.width, cameraCodecContext.height, cameraCodecContext.pix_fmt, cameraCodecContext.width, cameraCodecContext.height, AVPixelFormat.AV_PIX_FMT_BGR24, SWS_BICUBIC, null, null, null);
		if (imageConversionContext == null) return;
*/
	// Set up SDL window and OpenGL
	window_t window;

	// Set up ImGui
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

	serial::Serial port("");
	port.setBaudrate(115200);

	std::array<uint16_t, 12> pin_data;

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

//STOPWATCH

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
	
	while (running)
	{	

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
			{
				ImGui::Image((ImTextureID)(uintptr_t)lake_image.gl_id(), ImVec2(100, 50), ImVec2(0,0), ImVec2(1,1), ImVec4(255,255,255,255), ImVec4(255,255,255,0));
				
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
							/*AVInputFormat *format = av_find_input_format("avfoundation"); // macOS only
							AVFormatContext *format_ctx = avformat_alloc_context();

							if (avformat_open_input(&format_ctx, videostream_ref_id.c_str(), format, NULL) != 0) return false;
							if (avformat_find_stream_info(format_ctx, NULL) < 0) return false;
							av_dump_format(format_ctx, 0, videostream_ref_id.c_str(), 0);

							avformat_free_context(format_ctx);*/

							if (videostream_ref_index != -1)
							{
								vid.open(videostream_refs[videostream_ref_index]);
								lake_image = vid.current_frame();
							}
							else
							{
								vid.close();
							}

							/*		
							AVInputFormat*   cameraFormat		= av_find_input_format("avfoundation");
							AVFormatContext* cameraFormatContext = avformat_alloc_context();
							AVCodec*		 cameraCodec		 = NULL;
							AVCodecContext*  cameraCodecContext  = NULL;
							int			  cameraVideo		 = 0;
							std::string		   cameraSource	= "FaceTime HD Camera (Built-in)" //"USB 2.0 PC Cam";;
							AVFrame	*		 rawFrame = NULL, *convertedFrame = NULL;
							SwsContext*	  imageConversionContext ;
							unsigned char*		   frameBuffer;
							int			  frameBytes;

							AVPacket		 cameraPacket;
							int			  isFrameFinished = 0;

							if (avformat_open_input(&cameraFormatContext, cameraSource.toStringz, cameraFormat, null) != 0) return;
							//if (avformat_find_stream_info(inFormatContext) < 0) return;
							av_dump_format(cameraFormatContext, 0, cameraSource.toStringz, 0);

							for (int i = 0; i < cameraFormatContext.nb_streams; i++)
							{	   
									if (cameraFormatContext.streams[i].codec.codec_type == AVMediaType.AVMEDIA_TYPE_VIDEO)
									{	   
											cameraVideo = i;
											break;
									}
							}

							if(cameraVideo == -1) return;

							cameraCodec = avcodec_find_decoder(cameraFormatContext.streams[cameraVideo].codec.codec_id);
							cameraCodecContext = cameraFormatContext.streams[cameraVideo].codec;

							//warn: camera source size is added manually. find a way to fix it.
							cameraCodecContext.width = 640;

							cameraCodecContext.height = 480;
							cameraCodecContext.pix_fmt = AVPixelFormat.AV_PIX_FMT_UYVY422;

							//warn: find a way to input uyvy422 as pixel format for camera.
							if(cameraCodec == null) return;
							if(cameraCodecContext == null) return;
							if(avcodec_open2(cameraCodecContext, cameraCodec, null) < 0) return;

							cameraCodecContext.pix_fmt = AVPixelFormat.AV_PIX_FMT_UYVY422;

							rawFrame = av_frame_alloc(); convertedFrame = av_frame_alloc();

							frameBytes = avpicture_get_size(AVPixelFormat.AV_PIX_FMT_UYVY422, cameraCodecContext.width, cameraCodecContext.height);
							frameBuffer = cast(ubyte*) av_malloc(frameBytes * ubyte.sizeof);
							avpicture_fill(cast(AVPicture*) convertedFrame, frameBuffer, AVPixelFormat.AV_PIX_FMT_UYVY422, cameraCodecContext.width, cameraCodecContext.height);

							// writefln("%i%i", cameraCodecContext.pix_fmt, AVPixelFormat.AV_PIX_FMT_RGB24);
							imageConversionContext = sws_getCachedContext(null, cameraCodecContext.width, cameraCodecContext.height, cameraCodecContext.pix_fmt, cameraCodecContext.width, cameraCodecContext.height, AVPixelFormat.AV_PIX_FMT_BGR24, SWS_BICUBIC, null, null, null);
							if (imageConversionContext == null) return;
							*/

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
			
			// STOPWATCH

			ImGui::NewLine();

			//prv lapped_seconds, inital_time, maintain_time_proper, clock_state, clock_state_previous, lap_state_previous

			update_imgui_stopwatch();

			/*
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
		/*ImGui::SetNextWindowSize(ImVec2(window_w/2, window_h), ImGuiSetCond_Always);
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
			float power_generated = no_of_turbines * 0.5 * (1025 * (pow(rotor_diameter / 2, 2) * PI) * pow((current_kn * 0.5144444444444), 3) * efficiency / (100  * 1000000 )); //percent to ratio  | watts to megawatts
			
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
				
				float scalefactor = content_width /(1000.f * 15.f); //Given a distance in meters, times one km/1000 m, times one width/15 km then multiply by contentregion/width to get the distance in content regions

				if (ImGui::RadioButton("NAS Sand Point", is_takeoff_sand_point)) is_takeoff_sand_point = true; ImGui::SameLine();
				if (ImGui::RadioButton("Renton Airfield", !is_takeoff_sand_point)) is_takeoff_sand_point = false;
				
				ImVec2 orig = ImGui::GetCursorScreenPos(); //top left

				ImGui::Image((ImTextureID)(uintptr_t)lake_image.gl_id(), ImVec2(content_width, h * content_width/w), ImVec2(0,0), ImVec2(1,1), ImVec4(255,255,255,255), ImVec4(255,255,255,0));

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
		*/
		glViewport(0, 0, (int)ImGui::GetIO().DisplaySize.x, (int)ImGui::GetIO().DisplaySize.y);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui::Render();
		ImGui_ImplSdlGL3_RenderDrawData(ImGui::GetDrawData());

		window.update();
	}

	return 0;
}