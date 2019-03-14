#pragma once

#include <vector>
#include <string>
#include <functional>
#include <future>
#include <optional>

#include <imgui.h>

#include "advd/streamref.hpp"
#include "advd/videostream.hpp"

class VideoInterface
{
	// Cached values from the previous frame.
	std::vector<advd::StreamRef> videostream_refs;
	std::string videostream_ref_id = "";
	int videostream_ref_index = -1;

	// TODO: better control flow, less lambda magic.
	std::string tentative_videostream_ref_id;
	int tentative_videostream_ref_index;

	struct Configurator
	{
		static constexpr char const *title = "Video Interface";
		std::function<void()> render;
		std::function<bool()> confirm;
	};

	advd::Videostream videostream;
	std::future<TextureData> future_frame;
public: // TODO: temporarily made public.
	std::optional<Texture> current_frame;

public:
	Configurator make_configurator()
	{
		videostream_refs = advd::StreamRef::enumerate();

		tentative_videostream_ref_index = -1;

		for (int i = 0; i < videostream_refs.size(); ++i)
		{
			// Try to default to the previous id.
			if (videostream_refs[i].id() == videostream_ref_id)
			{
				tentative_videostream_ref_index = i;
				tentative_videostream_ref_id = videostream_ref_id;
			}
		}
		return Configurator
		{
			[&]
			{
				ImGui::ListBoxHeader("Videostream", videostream_refs.size() + 1);
					if (ImGui::Selectable("<none>", tentative_videostream_ref_index == -1)) {tentative_videostream_ref_index = -1; tentative_videostream_ref_id = "";};
					for (int i = 0; i < videostream_refs.size(); ++i) if (ImGui::Selectable(videostream_refs[i].label().c_str(), tentative_videostream_ref_index == i)) {tentative_videostream_ref_index = i; tentative_videostream_ref_id = videostream_refs[i].id();};
				ImGui::ListBoxFooter();
			},
			[&]
			{
				if (tentative_videostream_ref_id != videostream_ref_id)
				{
					videostream_ref_id = tentative_videostream_ref_id;
					videostream_ref_index = tentative_videostream_ref_index;

					if (future_frame.valid())
					{
						// FIXME: This is a bad and buggy workaround to clearing and the blocking issue.
						future_frame.wait(); future_frame.get();
					}

					if (videostream_ref_index != -1)
					{
						// FIXME: This can fail. Return false if it does.
						videostream.open(videostream_refs[videostream_ref_index]);
					}
					else
					{
						videostream.close();
						current_frame.reset();
					}
				}
				
				return true;
			}
		};
	}

	void update ()
	{
		if (videostream.is_open() && !future_frame.valid())
		{
			future_frame = std::async(std::launch::async, [&] {return videostream.current_frame();});
		}

		if (future_frame.valid() && future_frame.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)
		{
			auto a = future_frame.get();
			current_frame = Texture::from_data(a.data, {a.w, a.h}, 3);

			if (videostream.is_open())
			{
				future_frame = std::async(std::launch::async, [&] {return videostream.current_frame();});
			}
		}
	}
};