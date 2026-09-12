#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <algorithm>
#include <array>
#include <iostream>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "filesystem.h"

#ifndef OPENGL_SAMPLE_NAME
#define OPENGL_SAMPLE_NAME "OpenGL Sample"
#endif

namespace sample_help
{
struct Shortcut
{
	std::string key;
	std::string action;
};

inline bool isOneOf(const std::string& name, std::initializer_list<const char*> names)
{
	for (const char* candidate : names)
		if (name == candidate)
			return true;
	return false;
}

inline std::vector<Shortcut> shortcutsFor(const std::string& name)
{
	std::vector<Shortcut> result = {
		{"H", "Show / hide help"},
		{"Esc", "Exit sample"}
	};

	if (!isOneOf(name, {
		"Basics/CoordinateSystem", "Basics/Shader", "Basics/Texture",
		"Basics/Transformation", "Advanced_OpenGL/Instancing_Space", "Lighting/SSAO"}))
	{
		result.push_back({"F2", "Switch to fullscreen"});
	}

	if (isOneOf(name, {
		"Basics/Camera", "Lighting/Colors", "Lighting/Gooch_Model",
		"Advanced_OpenGL/Instancing_Space"}))
	{
		result.push_back({"W A S D", "Move camera"});
	}

	if (isOneOf(name, {
		"Advanced_OpenGL/AdvancedDataGLSL", "Advanced_OpenGL/AdvancedOGL",
		"Advanced_OpenGL/Anti_Aliasing_Off_Screen", "Advanced_OpenGL/Anti_Aliasing_On_Screen",
		"Advanced_OpenGL/Billboards", "Advanced_OpenGL/Compute_Shader",
		"Advanced_OpenGL/Compute_Shader_Particles", "Advanced_OpenGL/Geometry_Shader",
		"Advanced_OpenGL/Geometry_Shader_Exploding", "Advanced_OpenGL/Instancing",
		"Advanced_OpenGL/Mesh_Shader", "Advanced_OpenGL/no_Instancing_Space",
		"Advanced_OpenGL/Order_Independent_Transparency_Depth_Peeling",
		"Advanced_OpenGL/Order_Independent_Transparency_Weighted_Blended",
		"Advanced_OpenGL/Stencil_Silhouette", "Advanced_OpenGL/Tessellation",
		"Advanced_OpenGL/Tessellation_Terrain", "Advanced_OpenGL/Transform_Feedback",
		"Advanced_OpenGL/Transform_Feedback_Particles", "Lighting/Advanced_Lighting",
		"Lighting/Bloom", "Lighting/Deferred_Shading", "Lighting/Forward_Plus",
		"Lighting/Gamma_Correction", "Lighting/HDR", "Lighting/Lights",
		"Lighting/Normal_Mapping", "Lighting/Parallax_Mapping", "Lighting/Physically_Based_Bloom",
		"Lighting/SSAO", "Model/Advanced_Model", "Model/Basic", "Model/Skeletal_Animation",
		"PBR/Diffuse_irradiance", "PBR/Lighting", "PBR/Specular_IBL",
		"Shadows/Cascaded_Shadow_Mapping", "Shadows/Point_Shadows",
		"Shadows/Shadow_Mapping", "Shadows/Shadow_Volumes"}))
	{
		result.push_back({"Alt + LMB", "Orbit camera"});
		result.push_back({"Alt + MMB", "Pan camera"});
		result.push_back({"Alt + RMB", "Dolly camera"});
		result.push_back({"Mouse wheel", "Zoom camera"});
	}

	if (isOneOf(name, {"Basics/CoordinateSystem", "Basics/Texture", "Basics/Transformation"}))
		result.push_back({"Up / Down", "Adjust texture mix"});
	else if (name == "Advanced_OpenGL/AdvancedOGL")
	{
		result.push_back({"L", "Show / hide lights"});
		result.push_back({"W", "Toggle wireframe"});
	}
	else if (name == "Advanced_OpenGL/Billboards")
	{
		result.push_back({"W", "Toggle wireframe"});
		result.push_back({"F", "Toggle FPS logging"});
		result.push_back({"U", "Show / hide texture coordinates"});
		result.push_back({"A", "Toggle animation"});
		result.push_back({"Up / Down", "Adjust alpha threshold"});
	}
	else if (name == "Advanced_OpenGL/Compute_Shader")
	{
		result.push_back({"F1", "Toggle wireframe"});
		result.push_back({"F3", "Show / hide render buffers"});
		result.push_back({"A", "Pause / resume simulation"});
	}
	else if (name == "Advanced_OpenGL/Geometry_Shader_Exploding")
		result.push_back({"E", "Toggle explosion"});
	else if (name == "Advanced_OpenGL/Order_Independent_Transparency_Depth_Peeling")
	{
		result.push_back({"F", "Toggle FPS logging"});
		result.push_back({"Up / Down", "Adjust peel passes"});
	}
	else if (isOneOf(name, {"Advanced_OpenGL/Order_Independent_Transparency_Weighted_Blended",
		"Advanced_OpenGL/Tessellation_Terrain", "Model/Skeletal_Animation"}))
	{
		result.push_back({"F", "Toggle FPS logging"});
		if (name != "Advanced_OpenGL/Order_Independent_Transparency_Weighted_Blended")
			result.push_back({"W", "Toggle wireframe"});
	}
	else if (isOneOf(name, {"Advanced_OpenGL/Mesh_Shader", "Advanced_OpenGL/Tessellation",
		"Advanced_OpenGL/Transform_Feedback_Particles", "Lighting/Parallax_Mapping"}))
	{
		result.push_back({"W", "Toggle wireframe"});
		result.push_back({"Q / E", name == "Lighting/Parallax_Mapping" ? "Adjust height scale" : "Adjust scale"});
	}
	else if (name == "Lighting/Advanced_Lighting")
	{
		result.push_back({"B", "Toggle Blinn-Phong"});
		result.push_back({"O", "Toggle Oren-Nayar"});
	}
	else if (isOneOf(name, {"Lighting/Bloom", "Lighting/Physically_Based_Bloom"}))
	{
		result.push_back({"Space", "Toggle bloom"});
		result.push_back({"Q / E", "Adjust exposure"});
	}
	else if (name == "Lighting/Deferred_Shading")
		result.push_back({"D", "Toggle debug view"});
	else if (isOneOf(name, {"Lighting/Gamma_Correction", "Lighting/Normal_Mapping"}))
	{
		result.push_back({"B", "Toggle Blinn-Phong"});
		result.push_back({"Space", "Toggle gamma correction"});
	}
	else if (name == "Lighting/HDR")
	{
		result.push_back({"Space", "Toggle HDR"});
		result.push_back({"Q / E", "Adjust exposure"});
	}
	else if (isOneOf(name, {"Lighting/Lights", "Model/Advanced_Model", "Model/Basic"}))
		result.push_back({"L", "Show / hide lights"});
	else if (name == "Lighting/SSAO")
	{
		result.push_back({"A", "Cycle SSAO shader"});
		result.push_back({"B", "Toggle blur"});
		result.push_back({"D", "Toggle debug view"});
		result.push_back({"N", "Toggle Hammersley sampling"});
		result.push_back({"Q / E", "Adjust sample radius"});
	}
	else if (name == "PBR/Diffuse_irradiance")
	{
		result.push_back({"I", "Toggle irradiance map"});
		result.push_back({"1 / 2 / 3", "Select environment"});
	}
	else if (name == "Shadows/Cascaded_Shadow_Mapping")
	{
		result.push_back({"F", "Show / hide depth map"});
		result.push_back({"N", "Select cascade layer"});
		result.push_back({"C", "Freeze / refresh light matrices"});
	}
	else if (name == "Shadows/Point_Shadows")
	{
		result.push_back({"Space", "Toggle shadows"});
		result.push_back({"L", "Show / hide light"});
		result.push_back({"T", "Cycle shadow technique"});
	}
	else if (name == "Shadows/Shadow_Mapping")
	{
		result.push_back({"D", "Show / hide shadow map"});
		result.push_back({"P", "Toggle peter-panning fix"});
		result.push_back({"W", "Toggle wireframe"});
		result.push_back({"T", "Cycle shadow technique"});
	}
	else if (name == "Shadows/Shadow_Volumes")
	{
		result.push_back({"Space", "Toggle shadows"});
		result.push_back({"L", "Show / hide light"});
		result.push_back({"V", "Show / hide shadow volumes"});
		result.push_back({"W", "Toggle wireframe"});
	}
	else if (name == "Advanced_OpenGL/Stencil_Silhouette")
		result.push_back({"W", "Toggle wireframe"});
	else if (name == "Lighting/Forward_Plus")
	{
		result.push_back({"W", "Toggle wireframe"});
		result.push_back({"M", "Toggle heat map"});
		result.push_back({"T", "Show / hide tiles"});
		result.push_back({"L", "Show / hide lights"});
	}

	return result;
}

class HelpOverlay
{
public:
	void initialize(GLFWwindow* window, std::string sampleName)
	{
		if (initialized_ || failed_)
			return;

		State state;
		state.capture();
		window_ = window;
		sampleName_ = std::move(sampleName);
		shortcuts_ = shortcutsFor(sampleName_);
		program_ = createProgram();
		if (!program_)
		{
			failed_ = true;
			state.restore();
			return;
		}

		glGenVertexArrays(1, &vao_);
		glGenBuffers(1, &vbo_);
		glBindVertexArray(vao_);
		glBindBuffer(GL_ARRAY_BUFFER, vbo_);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, nullptr, GL_DYNAMIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
		glBindVertexArray(0);

		if (!loadFont(FileSystem::getPath("content/fonts/GO-Regular.ttf"), 20))
		{
			failed_ = true;
			state.restore();
			return;
		}

		initialized_ = true;
		state.restore();
	}

	void render()
	{
		if (!initialized_ || !window_)
			return;

		const bool hDown = glfwGetKey(window_, GLFW_KEY_H) == GLFW_PRESS;
		if (hDown && !hDown_)
			visible_ = !visible_;
		hDown_ = hDown;

		int width = 0;
		int height = 0;
		glfwGetFramebufferSize(window_, &width, &height);
		if (width <= 0 || height <= 0)
			return;

		State state;
		state.capture();

		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		glViewport(0, 0, width, height);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);
		glDisable(GL_SCISSOR_TEST);
		glDisable(GL_STENCIL_TEST);
		glDisable(GL_RASTERIZER_DISCARD);
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		glEnable(GL_BLEND);
		glBlendEquation(GL_FUNC_ADD);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glUseProgram(program_);
		glUniform2f(glGetUniformLocation(program_, "viewport"), static_cast<float>(width), static_cast<float>(height));
		glActiveTexture(GL_TEXTURE0);
		glUniform1i(glGetUniformLocation(program_, "glyph"), 0);
		glBindVertexArray(vao_);

		if (visible_)
			renderPanel(width, height);
		else
		{
			const float hintWidth = 86.0f;
			renderBox(12.0f, 12.0f, hintWidth, 32.0f, {0.035f, 0.045f, 0.065f, 0.78f});
			renderText("H  Help", 23.0f, 21.0f, 0.72f, {0.86f, 0.90f, 0.98f, 1.0f});
		}

		state.restore();
	}

private:
	struct Glyph
	{
		GLuint texture = 0;
		int width = 0;
		int height = 0;
		int bearingX = 0;
		int bearingY = 0;
		unsigned int advance = 0;
	};

	struct State
	{
		GLint program = 0, vao = 0, arrayBuffer = 0, activeTexture = 0, texture = 0;
		GLint drawFramebuffer = 0, blendSrcRgb = 0, blendDstRgb = 0, blendSrcAlpha = 0, blendDstAlpha = 0;
		GLint blendEquationRgb = 0, blendEquationAlpha = 0;
		GLint viewport[4] = {}, polygonMode[2] = {};
		GLboolean blend = GL_FALSE, depth = GL_FALSE, cull = GL_FALSE, scissor = GL_FALSE;
		GLboolean stencil = GL_FALSE, rasterizerDiscard = GL_FALSE;
		GLboolean colorMask[4] = {};

		void capture()
		{
			glGetIntegerv(GL_CURRENT_PROGRAM, &program);
			glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vao);
			glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &arrayBuffer);
			glGetIntegerv(GL_ACTIVE_TEXTURE, &activeTexture);
			glActiveTexture(GL_TEXTURE0);
			glGetIntegerv(GL_TEXTURE_BINDING_2D, &texture);
			glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &drawFramebuffer);
			glGetIntegerv(GL_VIEWPORT, viewport);
			glGetIntegerv(GL_POLYGON_MODE, polygonMode);
			glGetIntegerv(GL_BLEND_SRC_RGB, &blendSrcRgb);
			glGetIntegerv(GL_BLEND_DST_RGB, &blendDstRgb);
			glGetIntegerv(GL_BLEND_SRC_ALPHA, &blendSrcAlpha);
			glGetIntegerv(GL_BLEND_DST_ALPHA, &blendDstAlpha);
			glGetIntegerv(GL_BLEND_EQUATION_RGB, &blendEquationRgb);
			glGetIntegerv(GL_BLEND_EQUATION_ALPHA, &blendEquationAlpha);
			glGetBooleanv(GL_COLOR_WRITEMASK, colorMask);
			blend = glIsEnabled(GL_BLEND);
			depth = glIsEnabled(GL_DEPTH_TEST);
			cull = glIsEnabled(GL_CULL_FACE);
			scissor = glIsEnabled(GL_SCISSOR_TEST);
			stencil = glIsEnabled(GL_STENCIL_TEST);
			rasterizerDiscard = glIsEnabled(GL_RASTERIZER_DISCARD);
		}

		void restore() const
		{
			setEnabled(GL_BLEND, blend);
			setEnabled(GL_DEPTH_TEST, depth);
			setEnabled(GL_CULL_FACE, cull);
			setEnabled(GL_SCISSOR_TEST, scissor);
			setEnabled(GL_STENCIL_TEST, stencil);
			setEnabled(GL_RASTERIZER_DISCARD, rasterizerDiscard);
			glColorMask(colorMask[0], colorMask[1], colorMask[2], colorMask[3]);
			glBlendFuncSeparate(blendSrcRgb, blendDstRgb, blendSrcAlpha, blendDstAlpha);
			glBlendEquationSeparate(blendEquationRgb, blendEquationAlpha);
			glPolygonMode(GL_FRONT, polygonMode[0]);
			glPolygonMode(GL_BACK, polygonMode[1]);
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, drawFramebuffer);
			glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
			glUseProgram(program);
			glBindVertexArray(vao);
			glBindBuffer(GL_ARRAY_BUFFER, arrayBuffer);
			glBindTexture(GL_TEXTURE_2D, texture);
			glActiveTexture(activeTexture);
		}

		static void setEnabled(GLenum capability, GLboolean enabled)
		{
			if (enabled) glEnable(capability); else glDisable(capability);
		}
	};

	static GLuint compile(GLenum type, const char* source)
	{
		GLuint shader = glCreateShader(type);
		glShaderSource(shader, 1, &source, nullptr);
		glCompileShader(shader);
		GLint success = GL_FALSE;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
		if (!success)
		{
			std::array<char, 1024> log{};
			glGetShaderInfoLog(shader, static_cast<GLsizei>(log.size()), nullptr, log.data());
			std::cerr << "Help overlay shader compilation failed: " << log.data() << std::endl;
			glDeleteShader(shader);
			return 0;
		}
		return shader;
	}

	static GLuint createProgram()
	{
		static const char* vertexSource = R"GLSL(#version 330 core
layout (location = 0) in vec4 vertex;
out vec2 uv;
uniform vec2 viewport;
void main()
{
    vec2 ndc = vec2(vertex.x * 2.0 / viewport.x - 1.0,
                    1.0 - vertex.y * 2.0 / viewport.y);
    gl_Position = vec4(ndc, 0.0, 1.0);
    uv = vertex.zw;
}
)GLSL";
		static const char* fragmentSource = R"GLSL(#version 330 core
in vec2 uv;
out vec4 color;
uniform sampler2D glyph;
uniform vec4 tint;
uniform bool textured;
void main()
{
    color = textured ? vec4(tint.rgb, tint.a * texture(glyph, uv).r) : tint;
}
)GLSL";

		GLuint vertex = compile(GL_VERTEX_SHADER, vertexSource);
		GLuint fragment = compile(GL_FRAGMENT_SHADER, fragmentSource);
		if (!vertex || !fragment)
			return 0;
		GLuint program = glCreateProgram();
		glAttachShader(program, vertex);
		glAttachShader(program, fragment);
		glLinkProgram(program);
		glDeleteShader(vertex);
		glDeleteShader(fragment);
		GLint success = GL_FALSE;
		glGetProgramiv(program, GL_LINK_STATUS, &success);
		if (!success)
		{
			std::array<char, 1024> log{};
			glGetProgramInfoLog(program, static_cast<GLsizei>(log.size()), nullptr, log.data());
			std::cerr << "Help overlay shader linking failed: " << log.data() << std::endl;
			glDeleteProgram(program);
			return 0;
		}
		return program;
	}

	bool loadFont(const std::string& path, unsigned int pixelSize)
	{
		FT_Library library = nullptr;
		if (FT_Init_FreeType(&library))
		{
			std::cerr << "Help overlay could not initialize FreeType." << std::endl;
			return false;
		}
		FT_Face face = nullptr;
		if (FT_New_Face(library, path.c_str(), 0, &face))
		{
			std::cerr << "Help overlay could not load font: " << path << std::endl;
			FT_Done_FreeType(library);
			return false;
		}
		FT_Set_Pixel_Sizes(face, 0, pixelSize);
		GLint unpackAlignment = 0;
		glGetIntegerv(GL_UNPACK_ALIGNMENT, &unpackAlignment);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		for (unsigned int code = 32; code < 127; ++code)
		{
			if (FT_Load_Char(face, code, FT_LOAD_RENDER))
				continue;
			GLuint texture = 0;
			glGenTextures(1, &texture);
			glBindTexture(GL_TEXTURE_2D, texture);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RED,
				static_cast<GLsizei>(face->glyph->bitmap.width),
				static_cast<GLsizei>(face->glyph->bitmap.rows), 0,
				GL_RED, GL_UNSIGNED_BYTE, face->glyph->bitmap.buffer);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glyphs_[static_cast<char>(code)] = {
				texture, static_cast<int>(face->glyph->bitmap.width),
				static_cast<int>(face->glyph->bitmap.rows), face->glyph->bitmap_left,
				face->glyph->bitmap_top, static_cast<unsigned int>(face->glyph->advance.x)};
		}
		glBindTexture(GL_TEXTURE_2D, 0);
		glPixelStorei(GL_UNPACK_ALIGNMENT, unpackAlignment);
		FT_Done_Face(face);
		FT_Done_FreeType(library);
		return !glyphs_.empty();
	}

	void renderPanel(int width, int height)
	{
		const float scale = width < 900 ? 0.72f : 0.82f;
		const float lineHeight = 25.0f * scale;
		const float panelWidth = std::min(530.0f, static_cast<float>(width) - 24.0f);
		const float panelHeight = std::min(
			86.0f + lineHeight * static_cast<float>(shortcuts_.size()),
			static_cast<float>(height) - 24.0f);
		const float x = 18.0f;
		const float y = 18.0f;
		renderBox(x, y, panelWidth, panelHeight, {0.025f, 0.032f, 0.052f, 0.92f});
		renderBox(x, y, 5.0f, panelHeight, {0.20f, 0.63f, 1.0f, 1.0f});
		renderText("Keyboard shortcuts", x + 22.0f, y + 18.0f, 1.0f, {0.95f, 0.97f, 1.0f, 1.0f});
		renderText(sampleName_, x + 22.0f, y + 47.0f, 0.66f, {0.53f, 0.68f, 0.86f, 1.0f});

		float lineY = y + 77.0f;
		for (const Shortcut& shortcut : shortcuts_)
		{
			if (lineY + lineHeight > y + panelHeight)
				break;
			renderText(shortcut.key, x + 22.0f, lineY, scale, {0.42f, 0.76f, 1.0f, 1.0f});
			renderText(shortcut.action, x + 155.0f, lineY, scale, {0.88f, 0.91f, 0.96f, 1.0f});
			lineY += lineHeight;
		}
	}

	void renderBox(float x, float y, float width, float height, const std::array<float, 4>& color)
	{
		const float vertices[6][4] = {
			{x, y, 0.0f, 0.0f}, {x + width, y, 0.0f, 0.0f}, {x + width, y + height, 0.0f, 0.0f},
			{x, y, 0.0f, 0.0f}, {x + width, y + height, 0.0f, 0.0f}, {x, y + height, 0.0f, 0.0f}
		};
		glUniform1i(glGetUniformLocation(program_, "textured"), GL_FALSE);
		glUniform4fv(glGetUniformLocation(program_, "tint"), 1, color.data());
		glBindBuffer(GL_ARRAY_BUFFER, vbo_);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
		glDrawArrays(GL_TRIANGLES, 0, 6);
	}

	void renderText(const std::string& text, float x, float y, float scale, const std::array<float, 4>& color)
	{
		glUniform1i(glGetUniformLocation(program_, "textured"), GL_TRUE);
		glUniform4fv(glGetUniformLocation(program_, "tint"), 1, color.data());
		for (char character : text)
		{
			auto found = glyphs_.find(character);
			if (found == glyphs_.end())
				continue;
			const Glyph& glyph = found->second;
			const float xpos = x + glyph.bearingX * scale;
			const float ypos = y + (glyphs_['H'].bearingY - glyph.bearingY) * scale;
			const float width = glyph.width * scale;
			const float height = glyph.height * scale;
			const float vertices[6][4] = {
				{xpos, ypos + height, 0.0f, 1.0f}, {xpos + width, ypos, 1.0f, 0.0f}, {xpos, ypos, 0.0f, 0.0f},
				{xpos, ypos + height, 0.0f, 1.0f}, {xpos + width, ypos + height, 1.0f, 1.0f}, {xpos + width, ypos, 1.0f, 0.0f}
			};
			glBindTexture(GL_TEXTURE_2D, glyph.texture);
			glBindBuffer(GL_ARRAY_BUFFER, vbo_);
			glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
			glDrawArrays(GL_TRIANGLES, 0, 6);
			x += static_cast<float>(glyph.advance >> 6) * scale;
		}
	}

	GLFWwindow* window_ = nullptr;
	std::string sampleName_;
	std::vector<Shortcut> shortcuts_;
	std::map<char, Glyph> glyphs_;
	GLuint program_ = 0;
	GLuint vao_ = 0;
	GLuint vbo_ = 0;
	bool initialized_ = false;
	bool failed_ = false;
	bool visible_ = false;
	bool hDown_ = false;
};

inline HelpOverlay& overlay()
{
	static HelpOverlay instance;
	return instance;
}

inline void initialize(GLFWwindow* window)
{
	overlay().initialize(window, OPENGL_SAMPLE_NAME);
}

inline void renderAndSwap(GLFWwindow* window)
{
	overlay().render();
	glfwSwapBuffers(window);
}
} // namespace sample_help
