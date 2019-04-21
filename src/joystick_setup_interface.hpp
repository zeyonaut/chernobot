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
    std::map <string, control_mapping> joystick_mappings;
    SDL_Joystick* joy;

	struct Configurator
	{
		static constexpr char const *title = "Controller Setup Interface";
		std::function<void()> render;
		std::function<bool()> confirm;
	};

    void setup_mappings(int size) {
        if(!joystick_mappings.empty())
            return;
        string default_mappings[5] = {"right", "forward", "clockwise", "up", "moclaw"};
        for(int i = 0; i < size && i < sizeof(default_mappings); i++) {
            joystick_mappings[default_mappings[i]] = (control_mapping){i, false};
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

    control_mapping get_mapping(string input) {
        assert(joystick_mappings.size() > 0);
        std::map<string, control_mapping>::iterator tr = joystick_mappings.find(input);
        if(tr == joystick_mappings.end())
            // TODO: return something sensible
            return (control_mapping){0, false};
        return tr->second;
    }

    int get_axis(string axis) {
        control_mapping td = get_mapping(axis);
        int tr = SDL_JoystickGetAxis(joy, td.axis);
        if(td.rev)
            tr = -1 * tr;
        return tr;
    }

	Configurator make_configurator()
	{
		return Configurator
		{
			[&]
			{
                ImGui::BeginChild("Setup");

				for(std::map<string, control_mapping>::iterator it = joystick_mappings.begin(); it != joystick_mappings.end(); it++) {
                    ImGui::Text((it->first).c_str());
                    control_mapping curr = it->second;
                    ImGui::InputInt("Mapping", &(curr.axis));
                    ImGui::SameLine();
                    ImGui::Checkbox("Reverse", &(curr.rev));
                }	
                ImGui::EndChild();
			},
			[&]
			{

				// FIXME: This can fail. Return false if it does.

				return true;
			}
		};
	}
};