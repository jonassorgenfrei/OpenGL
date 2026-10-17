#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <algorithm>
#include <array>
#include <string>
#include <utility>
#include <vector>

#include "filesystem.h"
#include "font.h"

#ifndef OPENGL_SAMPLE_NAME
#define OPENGL_SAMPLE_NAME "OpenGL Sample"
#endif

/**
 * Shared keyboard-help UI for all samples.
 *
 * The overlay is header-only because every sample is a separate executable.
 * Samples pass their shortcut lists explicitly after creating the GL context.
 * It initializes after a sample creates its OpenGL context, renders immediately
 * before buffer presentation, and restores the sample's OpenGL state afterward.
 */
namespace sample_help
{
/** One display-ready keyboard binding. */
struct Shortcut
{
	std::string key;
	std::string action;
};

/**
 * Uses the shared TextRenderer for glyphs and panel rectangles.
 * Resources intentionally live for the process lifetime and disappear with the
 * sample's OpenGL context; initialization failures disable the overlay safely.
 */
class HelpOverlay
{
public:
	/** Initializes resources once, after the caller has made its context current. */
	void initialize(GLFWwindow* window, std::string sampleName, std::vector<Shortcut> shortcuts)
	{
		if (initialized_ || failed_)
			return;

		// Resource creation changes bindings, so preserve the sample's state even
		// during initialization and error exits.
		State state;
		state.capture();
		window_ = window;
		sampleName_ = std::move(sampleName);
		shortcuts_ = {{"H", "Show / hide help"}};
		shortcuts_.insert(shortcuts_.end(), shortcuts.begin(), shortcuts.end());
		if (!text_.Load(FileSystem::getPath("content/fonts/GO-Regular.ttf"), 20))
		{
			text_.release();
			failed_ = true;
			state.restore();
			return;
		}

		initialized_ = true;
		state.restore();
	}

	/** Releases UI resources before the sample destroys its OpenGL context. */
	void shutdown()
	{
		if (initialized_)
			text_.release();
		initialized_ = false;
		failed_ = false;
		visible_ = false;
		hDown_ = false;
		window_ = nullptr;
	}

	/** Handles the H-key edge and draws either the full panel or compact hint. */
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

		// The overlay must be transparent to the scene renderer. Capture every GL
		// state value changed below and restore it before buffer presentation.
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
		text_.setWindowSize(width, height);

		if (visible_)
			renderPanel(width, height);
		else
		{
			const float hintWidth = 86.0f;
			text_.RenderBox(12.0f, 12.0f, hintWidth, 32.0f, {0.035f, 0.045f, 0.065f, 0.78f});
			renderText("H  Help", 23.0f, 21.0f, 0.72f, {0.86f, 0.90f, 0.98f, 1.0f});
		}

		state.restore();
	}

private:
	/** Snapshot of the mutable OpenGL state touched by overlay rendering. */
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
			glPolygonMode(GL_FRONT_AND_BACK, polygonMode[0]);
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

	/** Lays out a responsive panel and clips shortcut rows to the framebuffer. */
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
		text_.RenderBox(x, y, panelWidth, panelHeight, {0.025f, 0.032f, 0.052f, 0.92f});
		text_.RenderBox(x, y, 5.0f, panelHeight, {0.20f, 0.63f, 1.0f, 1.0f});
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

	void renderText(const std::string& text, float x, float y, float scale, const std::array<float, 4>& color)
	{
		text_.RenderText(text, x, y, scale, {color[0], color[1], color[2]}, color[3]);
	}

	GLFWwindow* window_ = nullptr;
	std::string sampleName_;
	std::vector<Shortcut> shortcuts_;
	TextRenderer text_;
	bool initialized_ = false;
	bool failed_ = false;
	bool visible_ = false;
	bool hDown_ = false;
};

/** Returns the one overlay owned by each sample executable. */
inline HelpOverlay& overlay()
{
	static HelpOverlay instance;
	return instance;
}

/** Initializes help after GLAD, using bindings declared by the sample. */
inline void initialize(GLFWwindow* window, std::vector<Shortcut> shortcuts)
{
	overlay().initialize(window, OPENGL_SAMPLE_NAME, std::move(shortcuts));
}

/** Draws help explicitly before the sample presents its framebuffer. */
inline void render()
{
	overlay().render();
}

/** Call while the sample's context is current, before glfwTerminate. */
inline void shutdown()
{
	overlay().shutdown();
}
} // namespace sample_help
