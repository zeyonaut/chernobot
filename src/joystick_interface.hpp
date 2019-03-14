#pragma once

class JoystickInterface
{
	int tentative_joystick_index;

public: // TODO: temporarily made public.
	int joystick_index = -1;

public:
	struct Configurator
	{
		static constexpr char const *title = "Controller Interface";
		std::function<void()> render;
		std::function<bool()> confirm;
	};

	Configurator make_configurator()
	{
		tentative_joystick_index = joystick_index;
		// TODO: currently not defaulting like it should.. Switch to ID, then default.
		if (tentative_joystick_index >= SDL_NumJoysticks()) tentative_joystick_index = -1;

		return Configurator
		{
			[&]
			{
				ImGui::ListBoxHeader("Joystick", SDL_NumJoysticks() + 1);
					if (ImGui::Selectable("<none>", tentative_joystick_index == -1)) tentative_joystick_index = -1;
					for (int i = 0; i < SDL_NumJoysticks(); ++i) if (ImGui::Selectable(SDL_JoystickNameForIndex(i), tentative_joystick_index == i)) tentative_joystick_index = i;
				ImGui::ListBoxFooter();
			},
			[&]
			{
				joystick_index = tentative_joystick_index;

				// FIXME: This can fail. Return false if it does.

				return true;
			}
		};
	}
};