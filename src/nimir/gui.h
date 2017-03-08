#pragma once

struct GLFWwindow;

namespace nimir
{
namespace gui
{
    bool init(GLFWwindow* window, bool install_callbacks);
    void quit();
    
    void nextFrame();
    void drawFrame();

    void invalidateDeviceObjects();
    bool createDeviceObjects();

    void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    void charCallback(GLFWwindow* window, unsigned int c);
}
}
