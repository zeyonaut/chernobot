#pragma once

#include <memory>
#include <optional>
#include <string>
#include "advd/streamref.hpp"
#include <imgui.h>
#include <serial/serial.h>

#include <SDL2/SDL.h>

#include <variant>
#include <functional>

class ConfiguratorWidget
{
public:
	std::vector<advd::StreamRef> videostream_refs;
	std::string videostream_ref_id = "";
	int videostream_ref_index = -1;
	std::string tentative_videostream_ref_id = "";
	int tentative_videostream_ref_index = -1; //

	std::vector<serial::PortInfo> portinfo;
	int port_index = -1;
	int tentative_port_index = -1; //TODO need a better indicator.

	int joystick_index = -1;
	int tentative_joystick_index = -1;

	enum class State
	{
		root,
		video,
		comm,
		binds,
	}
		state;

	ConfiguratorWidget ()
	{
		state = State::root;
	}

	void go_back ()
	{
		switch (state)
		{
			case State::video:
			case State::comm:
			case State::binds:
				state = State::root;
			break;
			default: break;
		}
	}

	void run (std::function<void(std::optional<advd::StreamRef>)> video_changed, std::function<void(std::optional<std::string>)> port_changed)
	{
		switch (state)
		{
			case State::root:
				if (ImGui::Button("Videostream..", ImVec2(192, 0))) 
				{
					state = State::video;

					videostream_refs = advd::StreamRef::enumerate();
					tentative_videostream_ref_index = -1;
					for (int i = 0; i < videostream_refs.size(); ++i)
					{
						// Try to default to the previous id_tindex.
						if (videostream_refs[i].id() == videostream_ref_id)
						{
							tentative_videostream_ref_index = i;
							tentative_videostream_ref_id = videostream_ref_id;
						}
					}
				}
				if (ImGui::Button("Communications..", ImVec2(192, 0))) 
				{
					state = State::comm;

					portinfo = serial::list_ports();
					// TODO: currently not defaulting like it should.. Switch to ID, then default.
					if (port_index >= portinfo.size()) tentative_port_index = -1;
				}
				if (ImGui::Button("Bindings..", ImVec2(192, 0))) 
				{
					state = State::binds;

					tentative_joystick_index = joystick_index;
				}
			break;
			case State::video:
				ImGui::ListBoxHeader("Videostream", videostream_refs.size() + 1);
					if (ImGui::Selectable("<none>", tentative_videostream_ref_index == -1)) {tentative_videostream_ref_index = -1; tentative_videostream_ref_id = "";};
					for (int i = 0; i < videostream_refs.size(); ++i) if (ImGui::Selectable(videostream_refs[i].label().c_str(), tentative_videostream_ref_index == i)) {tentative_videostream_ref_index = i; tentative_videostream_ref_id = videostream_refs[i].id();};
				ImGui::ListBoxFooter();

				if (ImGui::Button("Finish", ImVec2(128, 0)))
				{
					state = State::root;

					if (tentative_videostream_ref_id != videostream_ref_id)
					{
						videostream_ref_id = tentative_videostream_ref_id;
						videostream_ref_index = tentative_videostream_ref_index; // TODO - last_index likely unnecessary
						video_changed(videostream_ref_index == -1? std::nullopt : std::make_optional(videostream_refs[videostream_ref_index]));
					}
				}
				ImGui::SameLine();
				if (ImGui::Button("Cancel", ImVec2(128, 0)))
				{
					state = State::root;
				}
			break;
			case State::comm:
				ImGui::ListBoxHeader("Slave", portinfo.size() + 1);
					if (ImGui::Selectable("<none>", tentative_port_index == -1)) tentative_port_index = -1;
					for (int i = 0; i < portinfo.size(); ++i) if (ImGui::Selectable(portinfo[i].port.c_str(), tentative_port_index == i)) tentative_port_index = i;
				ImGui::ListBoxFooter();
			
				if (ImGui::Button("Finish", ImVec2(128, 0)))
				{
					state = State::root;

					if (tentative_port_index != port_index)
					{
						//FIXME: if port becomes unavailable while the user is deciding, then it doesn't reset to -1.
						port_index = tentative_port_index;
						port_changed(port_index == -1? std::nullopt : std::make_optional(portinfo[port_index].port));
					}
				}
				ImGui::SameLine();
				if (ImGui::Button("Cancel", ImVec2(128, 0)))
				{
					state = State::root;
				}
			break;
			case State::binds:
				if (tentative_joystick_index >= SDL_NumJoysticks()) tentative_joystick_index = -1;
				ImGui::ListBoxHeader("Joystick", SDL_NumJoysticks() + 1);
				if (ImGui::Selectable("<none>", tentative_joystick_index == -1)) tentative_joystick_index = -1;
				for (int i = 0; i < SDL_NumJoysticks(); ++i) if (ImGui::Selectable(SDL_JoystickNameForIndex(i), tentative_joystick_index == i)) tentative_joystick_index = i;
				ImGui::ListBoxFooter();
				if (ImGui::Button("Finish", ImVec2(128, 0)))
				{
					state = State::root;

					tentative_joystick_index = joystick_index; // TODO: should use not index but id
				}
				ImGui::SameLine();
				if (ImGui::Button("Cancel", ImVec2(128, 0)))
				{
					state = State::root;
				}
			break;
		}
	}
};
//*/