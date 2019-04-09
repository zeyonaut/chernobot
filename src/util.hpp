#pragma once

#include <tuple>
#include <string_view>
#include "expected.hpp"

#include <opencv2/core/mat.hpp>

#include <glad/glad.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_keycode.h>

class Window
{
	SDL_Window *m_sdl_window;
	SDL_GLContext m_gl_context;

	static struct initiative {initiative () {std::cout << "let's see if this works\n";}} initiator;

public:

	Window ()
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

	~Window ()
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

	SDL_GLContext *gl_context () {return &m_gl_context;}

	std::tuple<int, int> size ()
	{
		int w, h;
		SDL_GetWindowSize(m_sdl_window, &w, &h);

		return {w, h};
	}
};

struct TextureData
{
	std::tuple<int, int> size;
	unsigned char *gl_data;
	size_t linesize;
	unsigned char *cv_data;
	size_t cv_linesize;
};

class Texture
{
	cv::Mat m_mat;

	GLuint m_gl_id;
	int m_w;
	int m_h;

	enum {owned, none} m_state;
	
	Texture (GLuint gl_id, int w, int h, cv::Mat mat): m_gl_id(gl_id), m_w(w), m_h(h), m_state(owned), m_mat(mat){}

public:

	Texture (): m_gl_id(0), m_w(0), m_h(0), m_state(none) {}

	Texture (Texture const &texture) = delete;
	Texture (Texture &&texture): m_gl_id(texture.m_gl_id), m_w(texture.m_w), m_h(texture.m_h), m_state(owned), m_mat(texture.m_mat) {texture.m_state = none;}

	Texture &operator= (Texture const &texture) = delete;
	Texture &operator= (Texture &&texture) {if (m_state == owned) glDeleteTextures(1, &m_gl_id); texture.m_state = none; m_gl_id = texture.m_gl_id; m_w = texture.m_w; m_h = texture.m_h; m_state = owned; m_mat = texture.m_mat; return *this;}

	static tl::expected<Texture, std::string> from_path (std::string const &path)
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

		return {Texture{gl_id, w, h, cv::Mat{}}}; // TODO: Do we need to create a mat for this?
	}
	
	static Texture from_data (TextureData const &texture_data)
	{
		auto const channels = 3; //FIXME: get channel

		GLuint gl_id;
		glGenTextures(1, &gl_id);
		GLint last_id;
		glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_id);

		glBindTexture(GL_TEXTURE_2D, gl_id);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		auto const [w, h] = texture_data.size;

		glTexImage2D(GL_TEXTURE_2D, 0, channels == 3? GL_RGB : GL_RGBA , w, h, 0, channels == 1? GL_RED : channels == 2? GL_RG : channels == 3? GL_RGB : GL_RGBA, GL_UNSIGNED_BYTE, (void*) texture_data.gl_data);
		
		glBindTexture(GL_TEXTURE_2D, last_id);

		return {Texture{gl_id, w, h,
			cv::Mat{h, w, channels == 1? CV_8UC1 : channels == 2? CV_8UC2 : channels == 3? CV_8UC3 : CV_8UC4, (void *) texture_data.cv_data, texture_data.cv_linesize}}};
	}

	~Texture ()
	{
		if (m_state == owned) glDeleteTextures(1, &m_gl_id);
	}

	GLuint gl_id ()
	{
		assert(m_state != none);
		return m_gl_id;
	}

	void reload (TextureData const &texture_data)
	{
		auto const channels = 3; //FIXME: get channel

		if (m_state == owned) glDeleteTextures(1, &m_gl_id);

		GLint last_id;
		glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_id);

		glBindTexture(GL_TEXTURE_2D, m_gl_id);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		auto const [w, h] = texture_data.size;
		glTexImage2D(GL_TEXTURE_2D, 0, channels == 3? GL_RGB : GL_RGBA , w, h, 0, channels == 1? GL_RED : channels == 2? GL_RG : channels == 3? GL_RGB : GL_RGBA, GL_UNSIGNED_BYTE, (void*) texture_data.gl_data);
		
		glBindTexture(GL_TEXTURE_2D, last_id);
		
		m_mat = cv::Mat(h, w, channels == 1? CV_8UC1 : channels == 2? CV_8UC2 : channels == 3? CV_8UC3 : CV_8UC4, (void *) texture_data.cv_data, texture_data.cv_linesize);

		m_state = owned;
	}

	std::tuple<int, int> size ()
	{
		assert(m_state != none);
		return {m_w, m_h};
	}

	const cv::Mat *mat ()
	{
		return &m_mat;
	}
};