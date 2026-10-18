#pragma once
#include <iostream>
#include <map>
#include <array>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "filesystem.h"

// include Freetype
#include <ft2build.h>
#include <freetype/freetype.h>

/// Holds all state information relevant to a character as loaded using FreeType
struct Character {
	GLuint TextureID;   // ID handle of the glyph texture
	glm::ivec2 Size;    // Size of glyph
	glm::ivec2 Bearing;  // Offset from baseline to left/top of glyph
	GLuint Advance;    // Horizontal offset to advance to next glyph
};

class TextRenderer {
public:
	TextRenderer() = default;
	TextRenderer(const TextRenderer&) = delete;
	TextRenderer& operator=(const TextRenderer&) = delete;

	// Constructor
	/// <summary>
	/// Initializes a new instance of the <see cref="TextRenderer"/> class.
	/// </summary>
	/// <param name="w">The width of the color Framebuffer.</param>
	/// <param name="h">The height of the color Framebuffer.</param>
	TextRenderer(GLuint w, GLuint h) {
		this->width = w;
		this->height = h;

		initialize();
	};

	void setWindowSize(GLuint width, GLuint height) {
		this->width = width;
		this->height = height;

	}

	// Pre-compiles a list of characters from the given font
	bool Load(std::string font, GLuint fontSize) {
		if (!initialize()) return false;
		// Then initialize and load the FreeType library
		FT_Library ft;
		if (FT_Init_FreeType(&ft)) // All functions return a value different than 0 whenever an error occurred
		{
			std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
			return false;
		}
		// Load font as face
		FT_Face face;
		if (FT_New_Face(ft, font.c_str(), 0, &face))
		{
			std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;
			FT_Done_FreeType(ft);
			return false;
		}
		// Set size to load glyphs as
		if (FT_Set_Pixel_Sizes(face, 0, fontSize))
		{
			FT_Done_Face(face);
			FT_Done_FreeType(ft);
			return false;
		}
		clearCharacters();
		glActiveTexture(GL_TEXTURE0);
		GLint unpackAlignment = 0;
		glGetIntegerv(GL_UNPACK_ALIGNMENT, &unpackAlignment);

			// Disable byte-alignment restriction (assure there are no alignment issues)
	// OGL requires that textures all have a 4-byte alignment e.g. their size is
	// always a multiple of 4 bytes
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		// Then for the first 128 ASCII characters + 128 other characters, pre-load/compile their characters and store them
		for (GLubyte c = 0; c < 255; c++) // lol see what I did there
		{
			// Load character glyph
			if (FT_Load_Char(face, c, FT_LOAD_RENDER))
			{
				std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
				continue;
			}
			// Generate texture
			GLuint texture;
			glGenTextures(1, &texture);
			glBindTexture(GL_TEXTURE_2D, texture);
			glTexImage2D(
				GL_TEXTURE_2D,
				0,
				GL_RED,
				face->glyph->bitmap.width,
				face->glyph->bitmap.rows,
				0,
				GL_RED,
				GL_UNSIGNED_BYTE,
				face->glyph->bitmap.buffer
			);
			// Set texture options
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			/*
			 * width: the width (in pixels) of the bitmap accessed via face->glyph->bitmap.width.
			 * height: the height (in pixels) of the bitmap accessed via face->glyph->bitmap.rows.
			 * bearingX: the horizontal bearing e.g. the horizontal position (in pixels) of the bitmap relative to the origin accessed via face->glyph->bitmap_left.
			 * bearingY: the vertical bearing e.g. the vertical position (in pixels) of the bitmap relative to the baseline accessed via face->glyph->bitmap_top.
			 * advance: the horizontal advance e.g. the horizontal distance (in 1/64th pixels) from the origin to the origin of the next glyph. Accessed via face->glyph->advance.x.
			 */
			// Now store character for later use
			Character character = {
				texture,
				glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
				glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
				static_cast<GLuint>(face->glyph->advance.x)
			};
			Characters.insert(std::pair<GLchar, Character>(c, character));
		}
		glBindTexture(GL_TEXTURE_2D, 0);
		glPixelStorei(GL_UNPACK_ALIGNMENT, unpackAlignment);
		// Destroy FreeType once we're finished
		FT_Done_Face(face);
		FT_Done_FreeType(ft);
		return !Characters.empty();
	};

	/// <summary>
	/// Renders the text.
	/// </summary>
	/// <param name="text">The text.</param>
	/// <param name="x">The x coord.</param>
	/// <param name="y">The y coord.</param>
	/// <param name="scale">The scale.</param>
	/// <param name="color">The text color.</param>
	/// <param name="width">The width of the window.</param>
	void RenderText(std::string text,
					GLfloat x, GLfloat y, GLfloat scale,
					glm::vec3 color, GLfloat alpha = 1.0f)	{
		// Activate corresponding render state
		bind();
		glUniform1i(glGetUniformLocation(shader, "textured"), GL_TRUE);
		glUniform4f(glGetUniformLocation(shader, "tint"), color.x, color.y, color.z, alpha);
		glActiveTexture(GL_TEXTURE0);
		glBindVertexArray(this->VAO);

		// Iterate through all characters
		std::string::const_iterator c;
		for (c = text.begin(); c != text.end(); c++)
		{

			auto found = Characters.find(*c);
			if (found == Characters.end()) continue;
			Character ch = found->second;	// extract the character

			// calculate the quad's dimensions using the character's metrics
			GLfloat xpos = x + ch.Bearing.x * scale;
			GLfloat ypos = y + (this->Characters['H'].Bearing.y - ch.Bearing.y) * scale;

			GLfloat w = ch.Size.x * scale;
			GLfloat h = ch.Size.y * scale;

			// Update VBO for each character
			GLfloat vertices[6][4] = {		// dynamically generate a set of 6 vertices
				{ xpos,		ypos + h,   0.0, 1.0 },
				{ xpos + w, ypos,       1.0, 0.0 },
				{ xpos,		ypos,		0.0, 0.0 },

				{ xpos,     ypos + h,	0.0, 1.0 },
				{ xpos + w, ypos + h,   1.0, 1.0 },
				{ xpos + w, ypos,		1.0, 0.0 }
			};
			// Render glyph texture over quad
			glBindTexture(GL_TEXTURE_2D, ch.TextureID);
			// Update content of VBO memory
			glBindBuffer(GL_ARRAY_BUFFER, VBO);
			glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); // Be sure to use glBufferSubData and not glBufferData

			glBindBuffer(GL_ARRAY_BUFFER, 0);
			// Render quad
			glDrawArrays(GL_TRIANGLES, 0, 6);
			// Now advance cursors for next glyph (note that advance is number of 1/64 pixels)
			x += (ch.Advance >> 6) * scale; // Bitshift by 6 to get value in pixels (2^6 = 64 (divide amount of 1/64th pixels by 64 to get amount of pixels))
		}
		glBindVertexArray(0);
		glBindTexture(GL_TEXTURE_2D, 0);
	}


	/** Streams and draws a solid six-vertex screen-space rectangle. */
	void RenderBox(float x, float y, float width, float height, const std::array<float, 4>& color)
	{
		bind();
		glBindVertexArray(this->VAO);
		const float vertices[6][4] = {
			{x, y, 0.0f, 0.0f}, {x + width, y, 0.0f, 0.0f}, {x + width, y + height, 0.0f, 0.0f},
			{x, y, 0.0f, 0.0f}, {x + width, y + height, 0.0f, 0.0f}, {x, y + height, 0.0f, 0.0f}
		};
		glUniform1i(glGetUniformLocation(shader, "textured"), GL_FALSE);
		glUniform4fv(glGetUniformLocation(shader, "tint"), 1, color.data());
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
		glDrawArrays(GL_TRIANGLES, 0, 6);
	}

	~TextRenderer()
	{
		// Static overlays may outlive glfwTerminate; never call GL without
		// the context that owns these resources.
		if (context && glfwGetCurrentContext() == context)
			release();
	}

	/** Release resources before the owning OpenGL context is destroyed. */
	void release()
	{
		clearCharacters();
		if (VBO) glDeleteBuffers(1, &VBO);
		if (VAO) glDeleteVertexArrays(1, &VAO);
		if (shader) glDeleteProgram(shader);
		VBO = VAO = shader = 0;
		context = nullptr;
	}




private:
	bool initialize()
	{
		if (shader) return true;
		context = glfwGetCurrentContext();
		if (!context) return false;
		shader = createProgram();
		if (!shader) return false;
		// Configure VAO/VBO for texture quads
		glGenVertexArrays(1, &this->VAO);
		glGenBuffers(1, &this->VBO);
		glBindVertexArray(this->VAO);
		glBindBuffer(GL_ARRAY_BUFFER, this->VBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
		return true;
	}

	void bind() const
	{
		glUseProgram(shader);
		glUniform2f(glGetUniformLocation(shader, "viewport"), static_cast<GLfloat>(width), static_cast<GLfloat>(height));
		glUniform1i(glGetUniformLocation(shader, "glyph"), 0);
	}

	void clearCharacters()
	{
		for (const auto& character : Characters)
			glDeleteTextures(1, &character.second.TextureID);
		Characters.clear();
	}

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

	GLuint width = 1, height = 1;
	std::map<GLchar, Character> Characters; // Holds a list of pre-compiled Characters

	GLuint VAO = 0, VBO = 0;
	// Shader used for text rendering
	GLuint shader = 0;
	GLFWwindow* context = nullptr;
};
