#pragma once

struct GLFWwindow;
struct ImVec4;

namespace nimir
{
namespace gl
{
    bool init(GLFWwindow*& window, int w, int h, const char* title);
    void quit();

    bool nextFrame(GLFWwindow*& window);
    void drawFrame(GLFWwindow*& window);

    void setClearColor(ImVec4 c);
}
}
