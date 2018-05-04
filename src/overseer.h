#pragma once

#include <array>
#include <string>
#include <cstdint>

struct vec2
{
	float x, y;
};

int magnitude (const int tx, const int ty, const vec2 motor);

int curve (float input);

struct Controls
{
	float forward, right, up, clockwise, moclaw;
};

//void serialize_controls(std::array<std::uint8_t, 12>& pin_data, const Controls& c, int botflag);
void serialize_controls(std::array<std::uint16_t, 12>& pin_data, const Controls& c, int botflag);

//std::string serialize_data (std::array<std::uint8_t, 12> pin_data);
std::string serialize_data (std::array<std::uint16_t, 12> pin_data);