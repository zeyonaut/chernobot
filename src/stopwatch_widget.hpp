#pragma once

#include <chrono>
#include <variant>
#include <vector>

#include <imgui/imgui.h>

class StopwatchWidget
{
	std::chrono::time_point<std::chrono::high_resolution_clock> initial_time;
	std::vector<std::chrono::seconds> lap_durations;
	
	struct Unpaused {};
	struct Paused {std::chrono::time_point<std::chrono::high_resolution_clock> time_when_paused;};
	std::variant<Unpaused, Paused> state;

	char const *imgui_id;

public:

	StopwatchWidget (char const *imgui_id): imgui_id(imgui_id)
	{
		reset();
	}

	void reset ()
	{
		lap_durations.clear();
		auto const now = std::chrono::high_resolution_clock::now();
		initial_time = now;
		state = Paused{now};
	}

	void toggle ()
	{
		if (auto paused = std::get_if<Paused>(&state); paused)
		{
			state = Unpaused{};
			initial_time += std::chrono::high_resolution_clock::now() - paused->time_when_paused;
		}
		else
		{
			state = Paused{std::chrono::high_resolution_clock::now()};
		}
	}

	void render ()
	{
		ImGui::PushID(imgui_id);

		bool is_paused = std::holds_alternative<Paused>(state);

		std::chrono::seconds const elapsed_duration = std::chrono::duration_cast<std::chrono::seconds>((is_paused? std::get_if<Paused>(&state)->time_when_paused : std::chrono::high_resolution_clock::now()) - initial_time);

		if (ImGui::Button("Toggle")) toggle();

		ImGui::SameLine();

		if (ImGui::Button("Lap")) lap_durations.push_back(elapsed_duration);
		
		ImGui::SameLine();

		if (ImGui::Button("Reset")) reset();
		
		ImGui::SameLine();

		ImGui::Text(is_paused? "Paused" : "Unpaused");

		auto const elapsed_s = elapsed_duration.count();

		auto const lapped_s = lap_durations.size() > 0? elapsed_s - lap_durations[lap_durations.size() - 1].count() : elapsed_s;

		ImGui::Text("Stopwatch Time: %02lld:%02lld; Lap: %02lld:%02lld", elapsed_s / 60, elapsed_s % 60, lapped_s / 60, lapped_s % 60);

		ImGui::NewLine();

		for (int i = 0; i < 5 && i < lap_durations.size(); ++i)
		{
			auto const lapped_s = lap_durations[lap_durations.size() - i - 1].count();
			ImGui::Text("Lap Time %02lu: %02lld:%02lld", lap_durations.size() - i, lapped_s / 60, lapped_s % 60);
		}

		if (lap_durations.size() <= 5)
		{
			for (int i = 0; i < 5 - lap_durations.size(); i++) ImGui::NewLine();
		}

		ImGui::PopID();
	}
};