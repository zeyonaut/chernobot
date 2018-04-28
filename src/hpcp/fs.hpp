#pragma once

namespace hpcp
{
	inline std::string get_directory(const std::string& path)
	{
#if !defined(_WIN32) && (defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__)))
		std::string::size_type delimiter_position = path.rfind("/");
		if (delimiter_position != std::string::npos) return path.substr(0, ++delimiter_position);
		else return "";
#endif
	}

	inline std::string get_base(const std::string& path)
	{
#if !defined(_WIN32) && (defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__)))
		std::string::size_type delimiter_position = path.rfind("/");
		if (delimiter_position != std::string::npos) return path.substr(++delimiter_position);
		else return path;
#endif
	}
}