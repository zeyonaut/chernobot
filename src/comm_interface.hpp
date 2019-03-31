#pragma once

struct ConsoleWidget
{
	std::vector<std::string> history;
	std::string title;
	bool should_autoscroll;

	ConsoleWidget (const std::string_view title): title(title), should_autoscroll(true) {}

	void render (bool *p_open)
	{
		if (history.size() > 512 + 128) history.erase(history.begin(), history.begin() + 128);

		ImGui::SetNextWindowSize(ImVec2(600,240), ImGuiCond_FirstUseEver);
        if (ImGui::Begin(this->title.c_str(), p_open))
		{
			ImGui::Checkbox("Should Autoscroll", &should_autoscroll);
			ImGui::BeginChild("Log", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4,1));
			{
				for (auto &i : history)
				{
					ImGui::Text("%s", i.c_str());
				}
				if (should_autoscroll)
				{
					ImGui::SetScrollHere();
				}
			}
			ImGui::PopStyleVar();
			ImGui::EndChild(); 

		}
		ImGui::End();
	}
};

class CommInterface
{
	std::vector<serial::PortInfo> port_infos;
	int port_index = -1;

	// TODO: better control flow, less lambda magic.
	int tentative_port_index;

	std::string nextline;

public: // TODO: temporarily made public.
	serial::Serial port;
	bool is_good;

public:
	CommInterface ()
	{
		port.setBaudrate(38400);
		is_good = false;
	}

	struct Configurator
	{
		static constexpr char const *title = "Communications Interface";
		std::function<void()> render;
		std::function<bool()> confirm;
	};

	Configurator make_configurator()
	{
		port_infos = serial::list_ports();

		tentative_port_index = port_index;
		// TODO: currently not defaulting like it should.. Switch to ID, then default.
		if (tentative_port_index >= port_infos.size()) tentative_port_index = -1;

		return Configurator
		{
			[&]
			{
				ImGui::ListBoxHeader("Slave", port_infos.size() + 1);
					if (ImGui::Selectable("<none>", tentative_port_index == -1)) tentative_port_index = -1;
					for (int i = 0; i < port_infos.size(); ++i) if (ImGui::Selectable(port_infos[i].port.c_str(), tentative_port_index == i)) tentative_port_index = i;
				ImGui::ListBoxFooter();
			},
			[&]
			{
				port_index = tentative_port_index;

				if (tentative_port_index != -1)
				{
					try
					{
						port.setPort(port_infos[port_index].port);
						port.open();
						is_good = true;
					}
					catch (...)
					{
						port.close();
						port.setPort("");
						is_good = false;
						
						return false;
					}
				}
				else
				{
					port.close();
					port.setPort("");
					is_good = false;
				}

				return true;
			}
		};
	}

	void update (ConsoleWidget *console, std::array<uint16_t, 12> const &pin_data)
	{
		try
		{
			if (port.isOpen())
			{
				if (is_good)
				{
					port.write(serialize_data(pin_data) + "\x17");
					is_good = false;
				}
			
				std::string next;
				for (int i = 0; i < port.available(); ++i)
				{
					next = port.read();
					if (next == "\n")
					{
						console->history.push_back(nextline);
						nextline = "";
					}
					else if (next == "\x17")
					{
						is_good = true;
					}
					else
					{
						nextline += next;
					}
				}
			}
		}
		catch (std::exception e)
		{
			nextline = "";
			port.close();
			port_index = -1;
			is_good = false;
		}
	}
};