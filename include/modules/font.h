#pragma once
#include <iostream>
#include <map>
#include <array>
#include <memory>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "shader_m.h"
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
		shader->setBool("textured", true);
		glUniform3f(glGetUniformLocation(shader->ID, "textColor"), color.x, color.y, color.z);
		shader->setFloat("alpha", alpha);
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
		shader->setBool("textured", false);
		glUniform3fv(glGetUniformLocation(shader->ID, "textColor"), 1, color.data());
		shader->setFloat("alpha", color[3]);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
		glDrawArrays(GL_TRIANGLES, 0, 6);
	}

	~TextRenderer()
	{
		// Destroy with the owning context current, or release resources beforehand.
		release();
	}

	/** Release resources before the owning OpenGL context is destroyed. */
	void release()
	{
		clearCharacters();
		if (VBO) glDeleteBuffers(1, &VBO);
		if (VAO) glDeleteVertexArrays(1, &VAO);
		if (shader) glDeleteProgram(shader->ID);
		shader.reset();
		VBO = VAO = 0;
	}




private:
	bool initialize()
	{
		if (shader) return true;
		if (!glfwGetCurrentContext()) return false;
		// All samples share the Text sample's shader files.
		shader = std::make_unique<Shader>(
			FileSystem::getPath("samples/Basics/Text/shader/text.vert").c_str(),
			FileSystem::getPath("samples/Basics/Text/shader/text.frag").c_str());
		GLint success = GL_FALSE;
		glGetProgramiv(shader->ID, GL_LINK_STATUS, &success);
		if (!success)
		{
			glDeleteProgram(shader->ID);
			shader.reset();
			return false;
		}
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
		shader->use();
		const glm::mat4 projection = glm::ortho(0.0f, static_cast<GLfloat>(this->width), static_cast<GLfloat>(this->height), 0.0f);
		glUniformMatrix4fv(glGetUniformLocation(shader->ID, "projection"), 1, GL_FALSE, &projection[0][0]);
		shader->setInt("text", 0);
	}

	void clearCharacters()
	{
		for (const auto& character : Characters)
			glDeleteTextures(1, &character.second.TextureID);
		Characters.clear();
	}

	GLuint width = 1, height = 1;
	std::map<GLchar, Character> Characters; // Holds a list of pre-compiled Characters

	GLuint VAO = 0, VBO = 0;
	// Shader used for text rendering
	std::unique_ptr<Shader> shader;
};
