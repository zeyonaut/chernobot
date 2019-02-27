#pragma once

#include <tuple>
#include <string_view>
#include "expected.hpp"

#include <glad/glad.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_keycode.h>

class window_t
{
	SDL_Window *m_sdl_window;
	SDL_GLContext m_gl_context;

	static struct initiative {initiative () {std::cout << "let's see if this works\n";}} initiator;

public:

	window_t ()
	{
		if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_JOYSTICK) != 0)
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
		m_sdl_window = SDL_CreateWindow("Chernobot - Master", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, /*current.w, current.h,*/ 720.f, 720.f, SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE|SDL_WINDOW_ALLOW_HIGHDPI);
		m_gl_context = SDL_GL_CreateContext(m_sdl_window);
		SDL_GL_SetSwapInterval(1);

		gladLoadGL();
		glClearColor(0, 0, 0, 1);
	}

	~window_t ()
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

struct TextureData
{
	int w;
	int h;
	unsigned char *data;
};

class texture_t
{
	GLuint m_gl_id;
	int m_w;
	int m_h;

	enum {owned, unowned, none} m_state;
	
	texture_t (GLuint gl_id, int w, int h, bool is_owned): m_gl_id(gl_id), m_w(w), m_h(h), m_state(is_owned? owned : unowned) {}

public:

	texture_t (): m_gl_id(0), m_w(0), m_h(0), m_state(none) {}

	texture_t (texture_t const &texture): m_gl_id(texture.m_gl_id), m_w(texture.m_w), m_h(texture.m_h), m_state(unowned) {}
	texture_t (texture_t &&texture): m_gl_id(texture.m_gl_id), m_w(texture.m_w), m_h(texture.m_h), m_state(owned) {texture.m_state = unowned;}

	texture_t &operator= (texture_t const &texture) {if (m_state == owned) glDeleteTextures(1, &m_gl_id);m_gl_id = texture.m_gl_id; m_w = texture.m_w; m_h = texture.m_h; m_state = unowned; return *this;}
	texture_t &operator= (texture_t &&texture) {if (m_state == owned) glDeleteTextures(1, &m_gl_id); texture.m_state = unowned; m_gl_id = texture.m_gl_id; m_w = texture.m_w; m_h = texture.m_h; m_state = owned; return *this;}

	static tl::expected<texture_t, std::string> from_path (std::string const &path)
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

		return {texture_t{gl_id, w, h, owned}};
	}
	
	static texture_t from_data (unsigned char const *const data, std::tuple<int, int> const size, int const channels)
	{
		GLuint gl_id;
		glGenTextures(1, &gl_id);
		GLint last_id;
		glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_id);

		glBindTexture(GL_TEXTURE_2D, gl_id);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		auto const [w, h] = size;

		glTexImage2D(GL_TEXTURE_2D, 0, channels == 3? GL_RGB : GL_RGBA , w, h, 0, channels == 1? GL_RED : channels == 2? GL_RG : channels == 3? GL_RGB : GL_RGBA, GL_UNSIGNED_BYTE, (void*) data);
		
		glBindTexture(GL_TEXTURE_2D, last_id);
		return {texture_t{gl_id, w, h, owned}};
	}

	~texture_t ()
	{
		if (m_state == owned) glDeleteTextures(1, &m_gl_id);
	}

	GLuint gl_id ()
	{
		assert(m_state != none);
		return m_gl_id;
	}

	void reload (unsigned char const *const data, std::tuple<int, int> const size, int const channels)
	{
		if (m_state == owned) glDeleteTextures(1, &m_gl_id);

		GLint last_id;
		glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_id);

		glBindTexture(GL_TEXTURE_2D, m_gl_id);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		auto const [w, h] = size;
		glTexImage2D(GL_TEXTURE_2D, 0, channels == 3? GL_RGB : GL_RGBA , w, h, 0, channels == 1? GL_RED : channels == 2? GL_RG : channels == 3? GL_RGB : GL_RGBA, GL_UNSIGNED_BYTE, (void*) data);
		
		glBindTexture(GL_TEXTURE_2D, last_id);
		
		m_state = owned;
	}

	std::tuple<int, int> size ()
	{
		assert(m_state != none);
		return {m_w, m_h};
	}
};
