#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <ft2build.h>
#include FT_FREETYPE_H

#include <array>
#include <iostream>
#include <map>
#include <string>

/** Shared pixel-space text and rectangle renderer, independent of sample shaders.
 * Create/load/draw only with the owning OpenGL context current. Callers select
 * blending, depth testing and other scene state (the help overlay preserves it).
 */
class TextRenderer
{
public:
	TextRenderer() = default;

	TextRenderer(GLuint width, GLuint height)
	{
		setWindowSize(width, height);
		initialize();
	}

	TextRenderer(const TextRenderer&) = delete;
	TextRenderer& operator=(const TextRenderer&) = delete;

	~TextRenderer()
	{
		// Static overlays can outlive glfwTerminate; their context already freed
		// the GPU objects. Never issue GL calls without the owning context.
		if (context_ && glfwGetCurrentContext() == context_)
			release();
	}

	bool initialize()
	{
		if (program_) return true;
		context_ = glfwGetCurrentContext();
		if (!context_) return false;
		program_ = createProgram();
		if (!program_) return false;
		glGenVertexArrays(1, &vao_);
		glGenBuffers(1, &vbo_);
		glBindVertexArray(vao_);
		glBindBuffer(GL_ARRAY_BUFFER, vbo_);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, nullptr, GL_DYNAMIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
		glBindVertexArray(0);
		return true;
	}

	void setWindowSize(GLuint width, GLuint height)
	{
		width_ = width;
		height_ = height;
	}

	/** Releases resources while the owning context is current. */
	void release()
	{
		clearGlyphs();
		if (vbo_) glDeleteBuffers(1, &vbo_);
		if (vao_) glDeleteVertexArrays(1, &vao_);
		if (program_) glDeleteProgram(program_);
		vbo_ = vao_ = program_ = 0;
		context_ = nullptr;
	}

	/** Rasterizes the existing 0..254 character range and uploads each glyph as an R8 texture. */
	bool Load(const std::string& path, unsigned int pixelSize)
	{
		if (!initialize()) return false;
		FT_Library library = nullptr;
		if (FT_Init_FreeType(&library))
		{
			std::cerr << "Text renderer could not initialize FreeType." << std::endl;
			return false;
		}
		FT_Face face = nullptr;
		if (FT_New_Face(library, path.c_str(), 0, &face))
		{
			std::cerr << "Text renderer could not load font: " << path << std::endl;
			FT_Done_FreeType(library);
			return false;
		}
		if (FT_Set_Pixel_Sizes(face, 0, pixelSize))
		{
			FT_Done_Face(face);
			FT_Done_FreeType(library);
			return false;
		}
		clearGlyphs();
		glActiveTexture(GL_TEXTURE0);
		GLint unpackAlignment = 0;
		glGetIntegerv(GL_UNPACK_ALIGNMENT, &unpackAlignment);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		for (unsigned int code = 0; code < 255; ++code)
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

	/** Streams and draws a solid six-vertex screen-space rectangle. */
	void RenderBox(float x, float y, float width, float height, const std::array<float, 4>& color)
	{
		bind();
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

	/** Draws text one glyph quad at a time using FreeType metrics for spacing. */
	void RenderText(const std::string& text, float x, float y, float scale, glm::vec3 rgb, float alpha = 1.0f)
	{
		bind();
		const std::array<float, 4> color = {rgb.r, rgb.g, rgb.b, alpha};
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

private:
	void bind() const
	{
		glUseProgram(program_);
		glUniform2f(glGetUniformLocation(program_, "viewport"), static_cast<float>(width_), static_cast<float>(height_));
		glUniform1i(glGetUniformLocation(program_, "glyph"), 0);
		glActiveTexture(GL_TEXTURE0);
		glBindVertexArray(vao_);
	}

	void clearGlyphs()
	{
		for (const auto& entry : glyphs_)
			glDeleteTextures(1, &entry.second.texture);
		glyphs_.clear();
	}

	struct Glyph
	{
		GLuint texture = 0;
		int width = 0;
		int height = 0;
		int bearingX = 0;
		int bearingY = 0;
		unsigned int advance = 0;
	};

	/** Compiles one text shader and reports diagnostics to stderr. */
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
			std::cerr << "Text renderer shader compilation failed: " << log.data() << std::endl;
			glDeleteShader(shader);
			return 0;
		}
		return shader;
	}

	/** Creates the minimal pixel-space shader shared by boxes and glyph quads. */
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
		{
			if (vertex) glDeleteShader(vertex);
			if (fragment) glDeleteShader(fragment);
			return 0;
		}
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
			std::cerr << "Text renderer shader linking failed: " << log.data() << std::endl;
			glDeleteProgram(program);
			return 0;
		}
		return program;
	}

	GLFWwindow* context_ = nullptr;
	GLuint width_ = 1, height_ = 1;
	GLuint program_ = 0, vao_ = 0, vbo_ = 0;
	std::map<char, Glyph> glyphs_;
};
