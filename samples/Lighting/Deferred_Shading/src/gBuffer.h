#ifndef G_BUFFER_H
#define G_BUFFER_H

#include <glad/glad.h>

#include <iostream>

class GBuffer {
	public:
		enum GBUFFER_TEXTURE_TYPE {
			GBUFFER_TEXTURE_TYPE_POSITION,	//position color buffer 
			GBUFFER_TEXTURE_TYPE_NORMAL, // normals color buffer
			GBUFFER_TEXTURE_TYPE_ALBEDOSPEC, // color(diffuse) + spec color buffer (Store Albedo & specular in a single texture) 
			GBUFFER_TEXTURE_TYPE_TEXCOORD,
			GBUFFER_NUM_TEXTURES
		};

		GBuffer() = default;
		GBuffer(const GBuffer&) = delete;
		GBuffer& operator=(const GBuffer&) = delete;

		~GBuffer() {
			destroy();
		}

		void destroy() {
			if (m_fbo != 0) {
				glDeleteFramebuffers(1, &m_fbo);
				m_fbo = 0;
			}

			if (m_textures[0] != 0) {
				glDeleteTextures(GBUFFER_NUM_TEXTURES, m_textures);
				for (GLuint& texture : m_textures)
					texture = 0;
			}

			if (m_depthTexture != 0) {
				glDeleteTextures(1, &m_depthTexture);
				m_depthTexture = 0;
			}

			if (m_finalTexture != 0) {
				glDeleteTextures(1, &m_finalTexture);
				m_finalTexture = 0;
			}
		}

		bool init(unsigned int windowWidth, unsigned int windowHeight) {
			// Create the FBO
			glGenFramebuffers(1, &m_fbo);
			glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
			
			// create gBuffer Textures
			glGenTextures(GBUFFER_NUM_TEXTURES, m_textures);
			glGenTextures(1, &m_depthTexture);

			glGenTextures(1, &m_finalTexture); // allocate output (final) texture

			for (unsigned int i = 0; i < GBUFFER_NUM_TEXTURES; i++) {
				// creates the storage area of the texture (without initializing it)
				glBindTexture(GL_TEXTURE_2D, m_textures[i]);
					//if texture id is ALBEDOSPEC using RGBA
				glTexImage2D(GL_TEXTURE_2D, 0, (i == GBUFFER_TEXTURE_TYPE_ALBEDOSPEC) ? GL_RGBA32F : GL_RGB32F, windowWidth, windowHeight, 0, (i == GBUFFER_TEXTURE_TYPE_ALBEDOSPEC) ? GL_RGBA : GL_RGB, GL_FLOAT, nullptr);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); // prevents unnecessary interpolation between the texels that might create some fine distortions
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				// attaches the texture to the FBO as a target
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, m_textures[i], 0);
			}

			// Depth/stencil requires a different format and attachment point.
			glBindTexture(GL_TEXTURE_2D, m_depthTexture);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH32F_STENCIL8, windowWidth, windowHeight, 0, GL_DEPTH_STENCIL, GL_FLOAT_32_UNSIGNED_INT_24_8_REV, nullptr);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, m_depthTexture, 0);// attached to Depth & Stencil attachment
		
			//final 
			glBindTexture(GL_TEXTURE_2D, m_finalTexture);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, windowWidth, windowHeight, 0, GL_RGBA, GL_FLOAT, nullptr);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT4, GL_TEXTURE_2D, m_finalTexture, 0); // attach to color attachment 4

			// explicitly tell OpenGL which color attachments to be used (of. FB)
			// enable writing to all four textures
			GLenum drawBuffer[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3};
			glDrawBuffers(GBUFFER_NUM_TEXTURES, drawBuffer); // supply all geometry attachments

			// finally check if FB is complete
			if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
				std::cout << "Framebuffer not complete!" << std::endl;
				return false;
			}

			// restore default FBO
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

			return true;
		}	

		void startFrame() {
			// clear final texture which is attached to attachment point number 4
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_fbo);
			glDrawBuffer(GL_COLOR_ATTACHMENT4);
			glClear(GL_COLOR_BUFFER_BIT);
		
		}
		void bindForGeomPass() {
			// Configure all geometry attachments as draw buffers.
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_fbo);
			GLenum drawBuffer[] = { GL_COLOR_ATTACHMENT0,
				GL_COLOR_ATTACHMENT1,
				GL_COLOR_ATTACHMENT2,
				GL_COLOR_ATTACHMENT3 };
			glDrawBuffers(GBUFFER_NUM_TEXTURES, drawBuffer);
		};
		void bindForStencilPass() {
			// must disable the draw buffers
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_fbo);
			glDrawBuffer(GL_NONE);
			// Avoid overwriting the final color texture with the stencil sphere.
		};
		void bindForLightPass() {
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_fbo);
			glDrawBuffer(GL_COLOR_ATTACHMENT4);
			for (unsigned int i = 0; i < GBUFFER_NUM_TEXTURES; i++) {
				glActiveTexture(GL_TEXTURE0 + i); // Binding default 3 textures
				glBindTexture(GL_TEXTURE_2D, m_textures[GBUFFER_TEXTURE_TYPE_POSITION + i]);
			}
		};
		void bindForFinalPass() {
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
			glBindFramebuffer(GL_READ_FRAMEBUFFER, m_fbo);
			glReadBuffer(GL_COLOR_ATTACHMENT4);
			// set things up for the blitting 
			// default FBO is target | G Buffer FBO is sources
		};

		/**
		 * Binds the textures as a target during the geometry pass
		 */
		void bindForWriting() {
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_fbo);
		};

		/**
		 * Binds the FBOs as input so its contents can be dumped to the screen
		 */
		void bindForReading() {
			glBindFramebuffer(GL_READ_FRAMEBUFFER, m_fbo);
		};

		void setReadBuffer(GBUFFER_TEXTURE_TYPE TextureType)
		{
			glReadBuffer(GL_COLOR_ATTACHMENT0 + TextureType);
		}

		void bindForReadingTex() {
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0); // disconnecting it from the GL_DRAW_FRAMEBUFFER (binding default FB)
			for (unsigned int i = 0; i < sizeof(m_textures) / sizeof(m_textures[0]); i++) {
				glActiveTexture(GL_TEXTURE0 + i); // Binding default 3 textures
				glBindTexture(GL_TEXTURE_2D, m_textures[GBUFFER_TEXTURE_TYPE_POSITION]+i);
			}
		}

	private:
		GLuint m_fbo = 0;
		GLuint m_textures[GBUFFER_NUM_TEXTURES]{}; // Textures for the vertex attributes
		GLuint m_depthTexture = 0; // Texture to serve as the depth buffer
		GLuint m_finalTexture = 0; // final texture for the color
};

#endif // !G_BUFFER_H
