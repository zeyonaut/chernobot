#pragma once

#include <tuple>
#include <vector>

#include <imgui/imgui.h>

#include "util.hpp"
#include "video_interface.hpp"

class Oculus
{
	char const *imgui_id;
	std::vector<std::shared_ptr<Texture>> snapped_frames;
	// TODO: find better way to handle the stream/image dichotomy. Maybe an optional<size_t> would work best.
	int frame_selected = -1; 

public:
	enum class State
	{
		present,
		past,
	};

	// TODO: See if encapsulation is necessary.
	State state;

	Oculus (char const *imgui_id): imgui_id(imgui_id), state(State::present) {}
	
	void snap_frame (VideoInterface *video_interface)
	{
		if (video_interface->current_frame) snapped_frames.push_back(video_interface->current_frame);
	}

	// TODO: Store the snapped_frames ourself: video_interface should know nothing about it.
	void render (std::tuple<int, int> size, VideoInterface *video_interface)
	{
		auto const [window_w, window_h] = size;
		ImGui::SetNextWindowSize(ImVec2(window_w, window_h), ImGuiSetCond_Always);
		ImGui::SetNextWindowPos(ImVec2(0.f, 0.f), ImGuiSetCond_Always);
		if (state == State::present) ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2 {0.f, 0.f});
		ImGui::Begin(imgui_id, nullptr, ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
		{
			if (state == State::present)
			{
				auto const content_size = ImGui::GetContentRegionAvail();

				if (video_interface->current_frame)
				{

					auto const [w, h] = video_interface->current_frame->size();
					auto const source_ratio = w * 1.0 / h;
					auto const target_ratio = content_size.x / content_size.y;
					auto const cursor_pos = ImGui::GetCursorPos();
					auto const target_size = source_ratio > target_ratio? ImVec2(content_size.x, h * content_size.x/w) : ImVec2(w * content_size.y/h, content_size.y);
					
					ImGui::SetCursorPos(ImVec2(source_ratio > target_ratio? cursor_pos.x : cursor_pos.x + (content_size.x - target_size.x) / 2, target_ratio > source_ratio? cursor_pos.y : cursor_pos.y + (content_size.y - target_size.y) / 2));
					ImGui::Image(reinterpret_cast<ImTextureID>(video_interface->current_frame->gl_id()), target_size, ImVec2(0,0), ImVec2(1,1), ImVec4(255,255,255,255), ImVec4(255,255,255,0));
				}
				else
				{
					auto const size = ImGui::CalcTextSize("[No Videostream Connected]");
					
					ImGui::SetCursorPos(ImVec2 {(content_size.x - size.x) / 2, (content_size.y - size.y) / 2});
					ImGui::Text("[No Videostream Connected]");
				}
			}
			else if (state == State::past)
			{
				ImGui::BeginChild("selector", ImVec2(128, 0), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);
				
				auto const window_padding_x = 2 * ImGui::GetStyle().WindowPadding.x + ImGui::GetStyle().ScrollbarSize;
				auto const total_padding_x = window_padding_x + 2 * ImGui::GetStyle().FramePadding.x; // One for the child window, one for the image button.

				if (ImGui::Button("Stream", ImVec2(128 - window_padding_x, 0))) frame_selected = -1;

				for (int i = snapped_frames.size() - 1; i >= 0; --i)
				{
					auto const [w, h] = snapped_frames[i]->size();

					if (ImGui::ImageButton(reinterpret_cast<ImTextureID>(snapped_frames[i]->gl_id()), ImVec2(128 - total_padding_x, (128 - total_padding_x) * h / w), ImVec2(0,0), ImVec2(1,1), -1))
					{
						frame_selected = i;
					}
				}

				if (frame_selected >= snapped_frames.size()) frame_selected = -1;
				ImGui::EndChild();
				ImGui::SameLine();

				// right
				ImGui::BeginGroup();
					ImGui::Text("Image ID: %d", frame_selected);
					ImGui::BeginChild("selected", ImVec2(0, frame_selected != -1? -ImGui::GetFrameHeightWithSpacing() : 0)); // Leave room for 1 line below us
						auto const content_size = ImGui::GetContentRegionAvail();

						auto const frame = frame_selected == -1? video_interface->current_frame : snapped_frames[frame_selected];
						
						if (frame)
						{
							auto const [w, h] = frame->size();
							auto const source_ratio = w * 1.0 / h;
							auto const target_ratio = content_size.x / content_size.y;
							auto const cursor_pos = ImGui::GetCursorPos();
							auto const target_size = source_ratio > target_ratio? ImVec2(content_size.x, h * content_size.x/w) : ImVec2(w * content_size.y/h, content_size.y);
							
							ImGui::SetCursorPos(ImVec2(source_ratio > target_ratio? cursor_pos.x : cursor_pos.x + (content_size.x - target_size.x) / 2, target_ratio > source_ratio? cursor_pos.y : cursor_pos.y + (content_size.y - target_size.y) / 2));
							ImGui::Image(reinterpret_cast<ImTextureID>(frame->gl_id()), target_size, ImVec2(0,0), ImVec2(1,1), ImVec4(255,255,255,255), ImVec4(255,255,255,0));
							
							//TODO: Analyze image here and draw stuff on the screen. - only if 
						}
						else
						{
							auto const size = ImGui::CalcTextSize("[No Frame Available]");

							ImGui::SetCursorPos(ImVec2 {(content_size.x - size.x) / 2, (content_size.y - size.y) / 2});
							ImGui::Text("[No Frame Available]");
						}
						
					ImGui::EndChild();
					if (frame_selected != -1 && ImGui::Button("Delete", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
					{
						snapped_frames.erase(snapped_frames.begin() + frame_selected);
						if (frame_selected > 0) frame_selected -= 1; // If it equals zero, we can rely on the bounds check in render () to CYA
					}
				ImGui::EndGroup();
			}
		}
		ImGui::End();
		if (state == State::present) ImGui::PopStyleVar();
	}
};