#pragma once

#include "overseer.hpp"

// TODO: Not This
using namespace std;

struct control_mapping {
    int axis;
    bool rev;
};

class JoystickSetupInterface
{
    
public:
    std::vector<control_mapping> joy_mappings;
    SDL_Joystick* joy;
    std::string default_mappings[5] = {std::string("right"), std::string("forward"), std::string("clockwise"), std::string("gripper"), std::string("pitch")};

	struct Configurator
	{
		static constexpr char const *title = "Controller Setup Interface";
		std::function<void()> render;
		std::function<bool()> confirm;
	};

    void setup_mappings(int size) {
        if(!joy_mappings.empty())
            return;
        for(int i = 0; i < size && i < sizeof(default_mappings); i++) {
            joy_mappings.push_back((control_mapping){i, false});
        }
    }

    void open_joystick(int joystick_index) {
        joy = SDL_JoystickOpen(joystick_index);
    }

    SDL_Joystick* sdl_joystick() {
        return joy;
    }

    string directreturn(string in) {
        return in;
    }

    control_mapping get_mapping(int id) {
        assert(joy_mappings.size() > 0);
        if(id >= joy_mappings.size()) {
            return (control_mapping){-1, false};
        }
        return joy_mappings[id];
    }

    int get_axis(int axis) {
        control_mapping td = get_mapping(axis);
        if(td.axis == -1 || td.axis > SDL_JoystickNumAxes(joy)) {
            return 0;
        }
        int tr = SDL_JoystickGetAxis(joy, td.axis);
        if(td.rev)
            tr = -1 * tr;
        return tr;
    }
    
    bool axis_enabled(int axis) {
        control_mapping td = get_mapping(axis);
        if(td.axis == -1 || td.axis > SDL_JoystickNumAxes(joy)) {
            return false;
        }
        return true;
    }

	Configurator make_configurator()
	{
		return Configurator
		{
			[&]
			{
                for(int i = 0; i < sizeof(default_mappings)/sizeof(*default_mappings); i++) {
                    std::string to_print = "Axis " + std::to_string(i) + " is "+ default_mappings[i];
                    ImGui::Text(to_print.c_str());
                }
                ImGui::NewLine();
				for(int i = 0; i < joy_mappings.size(); i++) {
                    ImGui::Text(("Axis " + std::to_string(i)).c_str());

                    ImGui::InputInt(std::string("Mapping##").append(std::to_string(i)).c_str(), &(joy_mappings.at(i).axis), 1);
                    ImGui::SameLine();
                    ImGui::Checkbox(std::string("Reverse##").append(std::to_string(i)).c_str(), &(joy_mappings.at(i).rev));
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