#pragma once

#include <string>
#include "overseer.hpp"

struct ControlMapping
{
	int axis;
	bool rev;
};

class JoystickSetupInterface
{
public:
	std::vector<ControlMapping> joy_mappings;
	SDL_Joystick *joy;
	std::string const default_mappings[5] = {"right", "forward", "clockwise", "gripper", "pitch"};

	struct Configurator
	{
		static constexpr char const *title = "Binding Interface";
		std::function<void()> render;
		std::function<bool()> confirm;
	};

	void setup_mappings (int size)
	{
		if (!joy_mappings.empty()) return;

		for (int i = 0; i < size && i < sizeof(default_mappings); ++i)
		{
			joy_mappings.push_back({i, false});
		}
	}

	void open_joystick (int joystick_index)
	{
		joy = SDL_JoystickOpen(joystick_index);
	}

	SDL_Joystick* sdl_joystick ()
	{
		return joy;
	}

	std::string directreturn (std::string in)
	{
		return in;
	}

	ControlMapping get_mapping (int id)
	{
		assert(joy_mappings.size() > 0);

		if (id >= joy_mappings.size()) return {-1, false};

		return joy_mappings[id];
	}

	int get_axis (int axis)
	{
		ControlMapping td = get_mapping(axis);

		if (td.axis == -1 || td.axis > SDL_JoystickNumAxes(joy)) return 0;

		int tr = SDL_JoystickGetAxis(joy, td.axis);

		if(td.rev) tr = -1 * tr;

		return tr;
	}
	
	bool axis_enabled (int axis)
	{
		ControlMapping td = get_mapping(axis);
		if (td.axis == -1 || td.axis > SDL_JoystickNumAxes(joy)) return false;
		else return true;
	}

	Configurator make_configurator ()
	{
		using namespace std::literals;
		
		return Configurator
		{
			[&]
			{
				if (!joy) {
					ImGui::Text("No joystick selected");
					return;
				}
				for (int i = 0; i < sizeof(default_mappings) / sizeof(*default_mappings); ++i)
				{
					std::string to_print = "Axis " + std::to_string(i) + " is "+ default_mappings[i];
					ImGui::Text("%s", to_print.c_str());
				}

				ImGui::NewLine();
				
				for (int i = 0; i < joy_mappings.size(); i++)
				{
					ImGui::Text("%s", "Axis "s.append(std::to_string(i)).c_str());

					ImGui::InputInt("Mapping##"s.append(std::to_string(i)).c_str(), &joy_mappings[i].axis, 1);
					ImGui::SameLine();
					ImGui::Checkbox("Reverse##"s.append(std::to_string(i)).c_str(), &joy_mappings[i].rev);
				}	
			},
			[&]
			{
				// FIXME: This can fail. Return false if it does.

				return true;
			}
		};
	}
};