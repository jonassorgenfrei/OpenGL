/*
 * Simplified Gaussian splatting
 *
 * Loads an unmodified, standard 3D Gaussian Splatting PLY. The vertex shader
 * evaluates its degree-3 spherical harmonics for the current view direction
 * and projects each Gaussian's full 3D covariance to the screen. Splats are
 * sorted back-to-front on the CPU to keep the teaching example approachable.
 */

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "modules/camera.h"
#include "modules/filesystem.h"
#include "modules/shader_m.h"
#include "modules/window.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
constexpr unsigned int SCR_WIDTH = 1280;
constexpr unsigned int SCR_HEIGHT = 720;

// Scaniverse/3DGS camera space has Y down and Z forward. Rotating 180 degrees
// about X converts it to this sample's OpenGL convention without editing the
// source PLY.
const glm::mat4 SPLAT_TO_OPENGL(
     1.0f,  0.0f,  0.0f, 0.0f,
     0.0f, -1.0f,  0.0f, 0.0f,
     0.0f,  0.0f, -1.0f, 0.0f,
     0.0f,  0.0f,  0.0f, 1.0f);

struct Splat
{
    glm::vec3 center;
    glm::vec3 logScale;
    glm::vec4 rotation; // PLY order: w, x, y, z
    float opacityLogit;
    std::uint32_t shIndex;
};

struct SplatScene
{
    std::vector<Splat> splats;
    // 48 floats per Gaussian: 3 DC values followed by 45 higher-order values.
    std::vector<float> sphericalHarmonics;
};

Camera camera(glm::vec3(0.0f, 0.1f, 5.0f));
float lastX = SCR_WIDTH * 0.5f;
float lastY = SCR_HEIGHT * 0.5f;
float deltaTime = 0.0f;
float lastFrame = 0.0f;
bool firstMouse = true;
bool pointMode = false;
bool pointKeyPressed = false;
int shDegree = 3;

void framebufferSizeCallback(GLFWwindow*, int width, int height)
{
    glViewport(0, 0, width, height);
}

void mouseCallback(GLFWwindow* window, double xpos, double ypos)
{
    // Match the Alt-modified camera controls used by Diffuse_irradiance.
    if (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS &&
        glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
    {
        if (firstMouse)
        {
            lastX = static_cast<float>(xpos);
            lastY = static_cast<float>(ypos);
            firstMouse = false;
        }

        const float xoffset = static_cast<float>(xpos) - lastX;
        const float yoffset = lastY - static_cast<float>(ypos);
        lastX = static_cast<float>(xpos);
        lastY = static_cast<float>(ypos);
        camera.rotate(xoffset, yoffset);
    }

    if (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS &&
        glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS)
    {
        if (firstMouse)
        {
            lastX = static_cast<float>(xpos);
            lastY = static_cast<float>(ypos);
            firstMouse = false;
        }

        const float xoffset = static_cast<float>(xpos) - lastX;
        const float yoffset = lastY - static_cast<float>(ypos);
        lastX = static_cast<float>(xpos);
        lastY = static_cast<float>(ypos);

        camera.translate(xoffset > 0.0f ? LEFT : RIGHT, std::abs(xoffset) * 0.05f);
        camera.translate(yoffset > 0.0f ? DOWN : UP, std::abs(yoffset) * 0.05f);
    }

    if (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS &&
        glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
    {
        if (firstMouse)
        {
            lastX = static_cast<float>(xpos);
            firstMouse = false;
        }

        const float xoffset = static_cast<float>(xpos) - lastX;
        lastX = static_cast<float>(xpos);
        camera.translate(xoffset > 0.0f ? FORWARD : BACKWARD, std::abs(xoffset) * 0.05f);
    }

    if ((glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_RELEASE ||
         glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_RELEASE) &&
        (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_RELEASE ||
         glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_RELEASE) &&
        (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_RELEASE ||
         glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE))
    {
        firstMouse = true;
    }
}

void scrollCallback(GLFWwindow*, double, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_F2) == GLFW_PRESS)
    {
        const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
        glfwSetWindowMonitor(window, glfwGetPrimaryMonitor(), 0, 0,
                             mode->width, mode->height, mode->refreshRate);
    }

    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS && !pointKeyPressed)
    {
        pointMode = !pointMode;
        pointKeyPressed = true;
        glfwSetWindowTitle(window, pointMode
            ? "Gaussian Splatting - Point Samples"
            : "Gaussian Splatting - Gaussian Splats");
        std::cout << "Render mode: " << (pointMode ? "points" : "Gaussian splats") << '\n';
    }
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_RELEASE)
        pointKeyPressed = false;

    for (int degree = 0; degree <= 3; ++degree)
    {
        if (glfwGetKey(window, GLFW_KEY_0 + degree) == GLFW_PRESS && shDegree != degree)
        {
            shDegree = degree;
            std::cout << "Spherical harmonics degree: " << shDegree << '\n';
        }
    }
}

float readLittleEndianFloat(std::istream& file)
{
    unsigned char bytes[4]{};
    if (!file.read(reinterpret_cast<char*>(bytes), sizeof(bytes)))
        throw std::runtime_error("PLY file ended before all vertices were read");

    const std::uint32_t bits = static_cast<std::uint32_t>(bytes[0]) |
        (static_cast<std::uint32_t>(bytes[1]) << 8U) |
        (static_cast<std::uint32_t>(bytes[2]) << 16U) |
        (static_cast<std::uint32_t>(bytes[3]) << 24U);
    float value = 0.0f;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

SplatScene loadSplats(const std::string& path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file)
        throw std::runtime_error("Could not open splat file: " + path);

    std::string line;
    if (!std::getline(file, line) || line != "ply")
        throw std::runtime_error("Not a PLY file: " + path);

    bool binaryLittleEndian = false;
    bool readingVertexElement = false;
    bool headerComplete = false;
    std::size_t vertexCount = 0;
    std::vector<std::string> properties;

    while (std::getline(file, line))
    {
        std::istringstream headerLine(line);
        std::string keyword;
        headerLine >> keyword;

        if (keyword == "format")
        {
            std::string format;
            headerLine >> format;
            binaryLittleEndian = (format == "binary_little_endian");
        }
        else if (keyword == "element")
        {
            std::string element;
            std::size_t count = 0;
            headerLine >> element >> count;
            readingVertexElement = (element == "vertex");
            if (readingVertexElement)
                vertexCount = count;
        }
        else if (keyword == "property" && readingVertexElement)
        {
            std::string type;
            std::string name;
            headerLine >> type >> name;
            if (type != "float" && type != "float32")
                throw std::runtime_error("This example expects float properties in the PLY vertex element");
            properties.push_back(name);
        }
        else if (keyword == "end_header")
        {
            headerComplete = true;
            break;
        }
    }

    if (!binaryLittleEndian || !headerComplete || vertexCount == 0)
        throw std::runtime_error("The sample expects a binary little-endian 3DGS PLY with vertices");

    const auto propertyIndex = [&properties](const std::string& name) {
        const auto found = std::find(properties.begin(), properties.end(), name);
        if (found == properties.end())
            throw std::runtime_error("Missing PLY vertex property: " + name);
        return static_cast<std::size_t>(std::distance(properties.begin(), found));
    };

    const std::size_t x = propertyIndex("x");
    const std::size_t y = propertyIndex("y");
    const std::size_t z = propertyIndex("z");
    const std::array<std::size_t, 3> scale = {
        propertyIndex("scale_0"), propertyIndex("scale_1"), propertyIndex("scale_2")
    };
    const std::array<std::size_t, 4> rotation = {
        propertyIndex("rot_0"), propertyIndex("rot_1"),
        propertyIndex("rot_2"), propertyIndex("rot_3")
    };
    const std::size_t opacity = propertyIndex("opacity");
    const std::array<std::size_t, 3> dc = {
        propertyIndex("f_dc_0"), propertyIndex("f_dc_1"), propertyIndex("f_dc_2")
    };
    std::array<std::size_t, 45> rest{};
    for (std::size_t coefficient = 0; coefficient < rest.size(); ++coefficient)
        rest[coefficient] = propertyIndex("f_rest_" + std::to_string(coefficient));

    SplatScene scene;
    scene.splats.reserve(vertexCount);
    scene.sphericalHarmonics.reserve(vertexCount * 48);
    std::vector<float> vertexValues(properties.size());
    for (std::size_t vertex = 0; vertex < vertexCount; ++vertex)
    {
        for (float& value : vertexValues)
            value = readLittleEndianFloat(file);

        Splat splat{};
        splat.center = glm::vec3(vertexValues[x], vertexValues[y], vertexValues[z]);
        splat.logScale = glm::vec3(vertexValues[scale[0]], vertexValues[scale[1]],
                                   vertexValues[scale[2]]);
        splat.rotation = glm::vec4(vertexValues[rotation[0]], vertexValues[rotation[1]],
                                   vertexValues[rotation[2]], vertexValues[rotation[3]]);
        splat.opacityLogit = vertexValues[opacity];
        splat.shIndex = static_cast<std::uint32_t>(vertex);
        scene.splats.push_back(splat);

        for (const std::size_t index : dc)
            scene.sphericalHarmonics.push_back(vertexValues[index]);
        for (const std::size_t index : rest)
            scene.sphericalHarmonics.push_back(vertexValues[index]);
    }
    return scene;
}

} // namespace

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Gaussian Splatting", nullptr, nullptr);
    if (!window)
    {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetCursorPosCallback(window, mouseCallback);
    glfwSetScrollCallback(window, scrollCallback);
    // Keep the system cursor available, as in the other advanced samples.
    // Mouse movement over the window rotates the camera without trapping it.
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
    {
        std::cerr << "Failed to initialize GLAD\n";
        glfwTerminate();
        return -1;
    }
    icon(window);

    SplatScene scene;
    try
    {
        scene = loadSplats(FileSystem::getPath("content/gaussian_splats/fox_statue.ply"));
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        glfwTerminate();
        return -1;
    }
    if (scene.splats.empty())
    {
        std::cerr << "The splat scene contains no valid records\n";
        glfwTerminate();
        return -1;
    }

    Shader shader(FileSystem::getSamplePath("shader/gaussian_splat.vert").c_str(),
                  FileSystem::getSamplePath("shader/gaussian_splat.frag").c_str());
    std::vector<Splat>& splats = scene.splats;

    const std::array<float, 12> quad = {
        -1.0f, -1.0f,  1.0f, -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f
    };

    unsigned int vao = 0;
    unsigned int quadVbo = 0;
    unsigned int instanceVbo = 0;
    unsigned int shBuffer = 0;
    unsigned int shTexture = 0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &quadVbo);
    glGenBuffers(1, &instanceVbo);
    glGenBuffers(1, &shBuffer);
    glGenTextures(1, &shTexture);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, quadVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);

    glBindBuffer(GL_ARRAY_BUFFER, instanceVbo);
    glBufferData(GL_ARRAY_BUFFER, splats.size() * sizeof(Splat), nullptr, GL_STREAM_DRAW);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Splat),
                          reinterpret_cast<void*>(offsetof(Splat, center)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Splat),
                          reinterpret_cast<void*>(offsetof(Splat, logScale)));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(Splat),
                          reinterpret_cast<void*>(offsetof(Splat, rotation)));
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, sizeof(Splat),
                          reinterpret_cast<void*>(offsetof(Splat, opacityLogit)));
    glEnableVertexAttribArray(5);
    glVertexAttribIPointer(5, 1, GL_UNSIGNED_INT, sizeof(Splat),
                           reinterpret_cast<void*>(offsetof(Splat, shIndex)));

    for (unsigned int attribute = 1; attribute <= 5; ++attribute)
        glVertexAttribDivisor(attribute, 1);

    glBindVertexArray(0);

    // Buffer textures make all 48 coefficients available without consuming
    // dozens of vertex attributes. Twelve RGBA texels belong to each splat.
    glBindBuffer(GL_TEXTURE_BUFFER, shBuffer);
    glBufferData(GL_TEXTURE_BUFFER,
                 scene.sphericalHarmonics.size() * sizeof(float),
                 scene.sphericalHarmonics.data(), GL_STATIC_DRAW);
    glBindTexture(GL_TEXTURE_BUFFER, shTexture);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, shBuffer);

    shader.use();
    shader.setInt("shData", 0);

    glm::mat4 lastSortedView(0.0f);

    // All geometry is transparent and sorted back-to-front. A depth test would
    // incorrectly hide overlapping splats, so blending alone determines their
    // visibility in this minimal scene.
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_PROGRAM_POINT_SIZE);

    while (!glfwWindowShouldClose(window))
    {
        const float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        processInput(window);

        const glm::mat4 view = camera.GetViewMatrix();
        if (view != lastSortedView)
        {
            // Alpha blending requires far-to-near camera-plane depth, not
            // Euclidean distance from the camera.
            const glm::mat4 viewModel = view * SPLAT_TO_OPENGL;
            std::sort(splats.begin(), splats.end(), [&viewModel](const Splat& a, const Splat& b) {
                const float aDepth = (viewModel * glm::vec4(a.center, 1.0f)).z;
                const float bDepth = (viewModel * glm::vec4(b.center, 1.0f)).z;
                return aDepth < bDepth;
            });
            glBindBuffer(GL_ARRAY_BUFFER, instanceVbo);
            glBufferSubData(GL_ARRAY_BUFFER, 0, splats.size() * sizeof(Splat), splats.data());
            lastSortedView = view;
        }

        int width = 1;
        int height = 1;
        glfwGetFramebufferSize(window, &width, &height);
        glClearColor(0.025f, 0.035f, 0.055f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        const glm::mat4 projection = glm::perspective(
            glm::radians(camera.Zoom), static_cast<float>(width) / static_cast<float>(height), 0.1f, 100.0f);
        shader.use();
        shader.setMat4("view", view);
        shader.setMat4("projection", projection);
        shader.setMat4("model", SPLAT_TO_OPENGL);
        shader.setBool("pointMode", pointMode);
        shader.setInt("shDegree", shDegree);
        shader.setVec3("cameraPosition", camera.Position);
        shader.setVec2("viewportSize", static_cast<float>(width), static_cast<float>(height));

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_BUFFER, shTexture);

        glBindVertexArray(vao);
        glDrawArraysInstanced(pointMode ? GL_POINTS : GL_TRIANGLES,
                              0, pointMode ? 1 : 6,
                              static_cast<GLsizei>(splats.size()));

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteBuffers(1, &instanceVbo);
    glDeleteBuffers(1, &shBuffer);
    glDeleteTextures(1, &shTexture);
    glDeleteBuffers(1, &quadVbo);
    glDeleteVertexArrays(1, &vao);
    glfwTerminate();
    return 0;
}
