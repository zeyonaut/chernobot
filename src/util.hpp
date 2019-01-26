#pragma once

#include <tuple>
#include <string_view>
#include "expected.hpp"

class windowly // Did you ever hear of the tragedy of Darth Plagueis the Wise?
{
	SDL_Window *m_sdl_window;
	SDL_GLContext m_gl_context;

	static struct initiative {initiative () {std::cout << "let's see if this works\n";}} initiator;

public:

	windowly ()
	{
		if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_JOYSTICK) != 0) // TODO expose a single-use initializer
		{
			printf("Error: %s\n", SDL_GetError());
			throw "a fit"; // TODO proper error handling for SDL & GL. I wonder if a separate function is a good idea?
		}

		SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
		SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
		SDL_DisplayMode current;
		SDL_GetCurrentDisplayMode(0, &current);
		m_sdl_window = SDL_CreateWindow("Chernobot - Master", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, current.w, current.h, SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE|SDL_WINDOW_ALLOW_HIGHDPI);
		m_gl_context = SDL_GL_CreateContext(m_sdl_window);
		SDL_GL_SetSwapInterval(1);

		gladLoadGL();
		glClearColor(0, 0, 0, 1);
	}

	~windowly ()
	{
		SDL_GL_DeleteContext(m_gl_context);
		SDL_DestroyWindow(m_sdl_window);
		SDL_Quit();
	}

	void update ()
	{
		SDL_GL_SwapWindow(m_sdl_window);
	}

	SDL_Window *sdl_window () {return m_sdl_window;}

	std::tuple<int, int> size ()
	{
		int w, h;
		SDL_GetWindowSize(m_sdl_window, &w, &h);

		return {w, h};
	}
};
/*
template <typename valid> class resultant
{
	union
	{
		valid m_value;
		std::exception_ptr m_error;
	};

	bool m_is_valid;
	bool m_is_owned;

public:
	resultant (valid const &value): m_value(value), m_is_valid(true), m_is_owned(false) {}
	resultant (valid &&value): m_value(std::move(value)), m_is_valid(true), m_is_owned(true) {value.m_is_owned = false;}

	resultant (resultant<valid> const &result): m_is_valid(result.m_is_valid), m_is_owned(false)
	{
		if (m_is_valid) m_value = result.m_value;
		else m_error = result.m_error;
	}
	resultant (resultant<valid> &&result): m_is_valid(true), m_is_owned(true)
	{
		if (m_is_valid) m_value = std::move(result.m_value);
		else m_error = std::move(result.m_error);
		result.m_is_owned = false;
	}

	resultant<valid> &operator= (resultant<valid> const &result)
	{
		resultant<valid> duplicate {result};

		return duplicate;
	}
	resultant<valid> &operator= (resultant<valid> &&result)
	{
		resultant<valid> owner {result};

		return owner;
	}

	static resultant<valid> err (std::exception_ptr &&error)
	{
		resultant<valid> result;
		result.m_is_valid = false;
		result.m_is_owned = true;
		new (&result.m_error) std::exception_ptr(std::move(error));

		return result;
	}

	static resultant<valid> err ()
	{
		return err(std::current_exception());
	}

	static resultant<valid> err (std::string_view error)
	{
		err()
	}

	~resultant ()
	{
		if (!m_is_owned)
		{
			using std::exception_ptr; 
			if (m_is_valid) m_value.~valid();
			else m_error.~exception_ptr();
		}
	}

	operator bool () {return m_is_valid;}
	
	valid &operator* () {return m_value;}
};
*/
class texturely
{
	GLuint m_gl_id;
	int m_w;
	int m_h;

	bool m_is_owned;
	
	texturely (GLuint gl_id, int w, int h, bool is_owned): m_gl_id(gl_id), m_w(w), m_h(h), m_is_owned(is_owned) {}

public:

	texturely (texturely const &texture): m_gl_id(texture.m_gl_id), m_w(texture.m_w), m_h(texture.m_h), m_is_owned(false) {}
	texturely (texturely &&texture): m_gl_id(texture.m_gl_id), m_w(texture.m_w), m_h(texture.m_h), m_is_owned(true) {texture.m_is_owned = false;}

	texturely &operator= (texturely const &texture) {if (m_is_owned) glDeleteTextures(1, &m_gl_id);m_gl_id = texture.m_gl_id; m_w = texture.m_w; m_h = texture.m_h; m_is_owned = false; return *this;}
	texturely &operator= (texturely &&texture) {if (m_is_owned) glDeleteTextures(1, &m_gl_id); texture.m_is_owned = false; m_gl_id = texture.m_gl_id; m_w = texture.m_w; m_h = texture.m_h; m_is_owned = true; return *this;}

	static tl::expected<texturely, std::string> load (std::string const &path)
	{
		int comp, w, h; // TODO use comp
		unsigned char* image = stbi_load(path.c_str(), &w, &h, &comp, STBI_rgb_alpha);

		if (image == nullptr) return tl::unexpected("failed to load image at \"" + path + "\"");

		GLuint gl_id;
		glGenTextures(1, &gl_id);
		glBindTexture(GL_TEXTURE_2D, gl_id);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		glBindTexture(GL_TEXTURE_2D, 0);

		stbi_image_free(image);

		return {texturely{gl_id, w, h, true}};
	}

	~texturely ()
	{
		if (m_is_owned) glDeleteTextures(1, &m_gl_id);
	}

	GLuint gl_id ()
	{
		return m_gl_id;
	}

	std::tuple<int, int> size ()
	{
		return {m_w, m_h};
	}
};
