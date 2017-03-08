#include "graphics.h"
#include "gui.h"

#include <stdio.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>

namespace nimir
{
namespace gl
{
    static void error(int error, const char* description)
    {
        fprintf(stderr, "Error %d: %s\n", error, description);
    }

    bool init(GLFWwindow*& window, int w, int h, const char* title)
    {
        glfwSetErrorCallback(error);
        if (!glfwInit()) return false;
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        #if __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
        #endif
        window = glfwCreateWindow(w, h, title, NULL, NULL);
        glfwMakeContextCurrent(window);
        gladLoadGL();
        glfwSwapInterval( 1 );

        return true;
    }

    void quit()
    {
        glfwTerminate();
    }

    bool nextFrame(GLFWwindow*& window)
    {
        glfwPollEvents();
        return !glfwWindowShouldClose(window);
    }

    void drawFrame(GLFWwindow*& window)
    {
        glfwSwapBuffers(window);
    }

    void setClearColor(ImVec4 c)
    {
        glClearColor(c.x, c.y, c.z, c.w);
    }
}
}
