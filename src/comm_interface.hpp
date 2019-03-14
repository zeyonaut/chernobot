#pragma once

class CommInterface
{
	std::vector<serial::PortInfo> port_infos;
	int port_index = -1;

	// TODO: better control flow, less lambda magic.
	int tentative_port_index;

public: // TODO: temporarily made public.
	serial::Serial port;

public:
	CommInterface ()
	{
		port.setBaudrate(115200);
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
					}
					catch (...)
					{
						port.close();
						port.setPort("");
						
						return false;
					}
				}
				else
				{
					port.close();
					port.setPort("");
				}

				return true;
			}
		};
	}
};