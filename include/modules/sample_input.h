#pragma once
#include <GLFW/glfw3.h>
#include <array>

// One window per executable. Poll at the start of each frame; continuous motion
// uses down[], while toggles use pressed[] (a transition, not keyboard repeat).
namespace sample_input
{
inline std::array<bool, GLFW_KEY_LAST + 1> down{};
inline std::array<bool, GLFW_KEY_LAST + 1> pressed{};

inline void poll(GLFWwindow *window)
{
    glfwPollEvents();
    for (int key = GLFW_KEY_SPACE; key <= GLFW_KEY_LAST; ++key)
    {
        const bool current = glfwGetKey(window, key) == GLFW_PRESS;
        pressed[key] = current && !down[key];
        down[key] = current;
    }
    if (down[GLFW_KEY_ESCAPE])
        glfwSetWindowShouldClose(window, true);
}
} // namespace sample_input
