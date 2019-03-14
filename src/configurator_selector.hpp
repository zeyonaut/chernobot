#pragma once

#include <vector>
#include <string>
#include <variant>
#include <functional>

#include <imgui.h>

template <typename T> using Configurator = decltype(std::declval<T>().make_configurator());

template <typename T> constexpr bool is_configurable =
	std::is_same_v<bool, decltype(std::declval<Configurator<T>>().confirm())> &&
	std::is_same_v<void, decltype(std::declval<Configurator<T>>().render())> &&
	std::is_same_v<char const *, std::remove_const_t<decltype(Configurator<T>::title)>>;

template <typename ...Ts> class ConfiguratorSelector
{
	static_assert((is_configurable<Ts> && ...));

	struct Closed {};
	struct Root {};
	
	std::variant<Closed, Root, Configurator<Ts> ...> configurator;

	template <typename T> void render_options (T *t)
	{
		if (ImGui::Button(Configurator<T>::title, ImVec2(192, 0))) init<T>(t);
	}
	template <typename T, typename ...Args> void render_options (T *t, Args *...args)
	{
		render_options<T>(t);
		if constexpr (sizeof...(Args) > 0)
		{
			render_options<Args ...>(args ...);
		}
	}

public:
	void open ()
	{
		if (!is_open())
		{
			ImGui::OpenPopup("###configurator");

			configurator = Root{};
		}
	}

	void close ()
	{
		if (is_open())
		{
			cancel();

			configurator = Closed{};
		}
	}

	bool is_open ()
	{
		return !std::holds_alternative<Closed>(configurator);
	}
	
	template <typename T> void init (T *configurable)
	{
		static_assert(is_configurable<T> && (std::is_same_v<T, Ts> || ...));

		cancel();

		configurator = configurable->make_configurator();
	}

	void render (Ts *...ts)
	{
		ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
		// TODO: allow customizing the ID? Maybe find another way of generating IDs?
		if (ImGui::BeginPopupModal("###configurator", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar))
		{
			std::visit
			(
				[&] (auto &&arg)
				{
					using T = std::decay_t<decltype(arg)>;
					
					if constexpr (std::is_same_v<T, Root>)
					{
						render_options<Ts ...>(ts ...);
					}
					else if constexpr (!std::is_same_v<T, Closed>)
					{
						arg.render();

						if (ImGui::Button("Finish", ImVec2(128, 0))) confirm();
						ImGui::SameLine();
						if (ImGui::Button("Cancel", ImVec2(128, 0))) cancel();
					}
					else
					{
						ImGui::CloseCurrentPopup();
					}
				},
				configurator
			);
			ImGui::EndPopup();
		}
	}

	void cancel ()
	{
		std::visit
		(
			[&] (auto &&arg)
			{
				using T = std::decay_t<decltype(arg)>;

				if constexpr (std::is_same_v<T, Root>)
				{
					configurator = Closed{};
				}
				else if constexpr (!std::is_same_v<T, Closed>)
				{
					configurator = Root{};
				}
			},
			configurator
		);
	}
	
	void confirm ()
	{
		std::visit
		(
			[&] (auto &&arg)
			{
				using T = std::decay_t<decltype(arg)>;

				if constexpr (std::is_same_v<T, Root>)
				{
					configurator = Closed{};
				}
				else if constexpr (!std::is_same_v<T, Closed>)
				{
					if (arg.confirm()) configurator = Root{};
				}
			},
			configurator
		);
	}
};