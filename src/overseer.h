#pragma once

#include <array>
#include <string>

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

void serialize_controls(std::array<unsigned char, 12>& pin_data, const Controls& c, int botflag);

std::string serialize_data (std::array<unsigned char, 12> pin_data);