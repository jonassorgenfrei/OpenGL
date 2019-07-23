#ifndef G_BUFFER_H
#define G_BUFFER_H

#define ZERO_MEM(a) memset(a, 0, sizeof(a))
#define GBUFFER_TEXTURE_TYPE_FINAL 3

#include <glad/glad.h>

#include <iostream>

class GBuffer {
	public:
		enum GBUFFER_TEXTURE_TYPE {
			GBUFFER_TEXTURE_TYPE_COLOR,	// Buffer für die Farbe
			GBUFFER_TEXTURE_TYPE_REFLECTION, // Buffer für die Reflexionseigenschaften
			GBUFFER_TEXTURE_TYPE_NORMAL, //  Buffer für die Normalen
			GBUFFER_NUM_TEXTURES
		};

		GBuffer() {
			m_fbo = 0;
			m_depthTexture = 0;
			ZERO_MEM(m_textures);
		};

		~GBuffer() {
			if (m_fbo != 0) {
				glDeleteFramebuffers(1, &m_fbo);
			}

			if (m_textures[0] != 0) {
				glDeleteTextures(sizeof(m_textures)/sizeof(m_textures[0]), m_textures);
			}

			if (m_depthTexture != 0) {
				glDeleteTextures(1, &m_depthTexture);
			}

			if (m_finalTexture != 0) {
				glDeleteTextures(1, &m_finalTexture);
			}
		};

		bool init(unsigned int WindowWidth, unsigned int WindowHeight) {
			// Create the FBO
			glGenFramebuffers(1, &m_fbo);
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_fbo);
			
			// create gBuffer Textures
			glGenTextures(sizeof(m_textures) / sizeof(m_textures[0]), m_textures);
			glGenTextures(1, &m_finalTexture);
			glGenTextures(1, &m_depthTexture);

			// Color Buffer

			// creates the storage area of the texture (without initializing it)
			glBindTexture(GL_TEXTURE_2D, m_textures[GBUFFER_TEXTURE_TYPE_COLOR]);
			//if texture id is ALBEDOSPEC using RGBA
			glTexImage2D(GL_TEXTURE_2D, 0,  GL_RGBA8, WindowWidth, WindowHeight, 0,  GL_RGBA, GL_UNSIGNED_BYTE, NULL);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); // prevents unnecessary interpolation between the texels that might create some fine distortions
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			// attaches the texture to the FBO as a target
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + GBUFFER_TEXTURE_TYPE_COLOR, GL_TEXTURE_2D, m_textures[GBUFFER_TEXTURE_TYPE_COLOR], 0);

			// Reflection Buffer

			// creates the storage area of the texture (without initializing it)
			glBindTexture(GL_TEXTURE_2D, m_textures[GBUFFER_TEXTURE_TYPE_REFLECTION]);
			//if texture id is ALBEDOSPEC using RGBA
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, WindowWidth, WindowHeight, 0, GL_RGBA, GL_FLOAT, NULL);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); // prevents unnecessary interpolation between the texels that might create some fine distortions
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			// attaches the texture to the FBO as a target
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + GBUFFER_TEXTURE_TYPE_REFLECTION, GL_TEXTURE_2D, m_textures[GBUFFER_TEXTURE_TYPE_REFLECTION], 0);

			// Normal Buffer

			// creates the storage area of the texture (without initializing it)
			glBindTexture(GL_TEXTURE_2D, m_textures[GBUFFER_TEXTURE_TYPE_NORMAL]);
			//if texture id is ALBEDOSPEC using RGBA
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, WindowWidth, WindowHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); // prevents unnecessary interpolation between the texels that might create some fine distortions
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			// attaches the texture to the FBO as a target
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + GBUFFER_TEXTURE_TYPE_NORMAL, GL_TEXTURE_2D, m_textures[GBUFFER_TEXTURE_TYPE_NORMAL], 0);

			//final 
			glBindTexture(GL_TEXTURE_2D, m_finalTexture);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, WindowWidth, WindowHeight, 0, GL_RGBA, GL_FLOAT, NULL);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, m_finalTexture, 0); // attacht to number 4

			// depth (explicitly, because it requires diff. format and is attached to the FBO at a diff. spot)
			glBindTexture(GL_TEXTURE_2D, m_depthTexture);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, WindowWidth, WindowHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL); // leaves a full byte for the stencile value in each pixel
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_depthTexture, 0);// attached to Depth & Stencil attachment


			// finally check if FB is complete
			if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
				std::cout << "Framebuffer not complete!";
				if (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT) {
					std::cout << " Incomplete Attachment";
				}
				else if (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT) {
					std::cout << " Incomplete Missing Attachment";
				}
				else if (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_UNSUPPORTED) {
					std::cout << " FB Unsupported";
				}
				else if (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER) {
					std::cout << " target is not GL_FRAMEBUFFER";
				}
				else {
					std::cout << "Unknown";
				}
				std::cout << std::endl;

				return false;
			}

			// restore default FBO
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

			return true;
		}	

		void clearFinalTexture() {
			// clear final texture
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_fbo);
			glDrawBuffer(GL_COLOR_ATTACHMENT3);
			glClearColor(0, 0, 0, 1);
			glClear(GL_COLOR_BUFFER_BIT);
		}

		void bindForGeomPass() {
			// now we keep changing the FBO, config the draw buffers for the attributes !
			glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

			// initialise color- & reflection-buffer to 0
			GLenum drawBuffer[] = { GL_COLOR_ATTACHMENT0 + GBUFFER_TEXTURE_TYPE_COLOR,
									GL_COLOR_ATTACHMENT0 + GBUFFER_TEXTURE_TYPE_REFLECTION
								  };

			glDrawBuffers(sizeof(drawBuffer) / sizeof(drawBuffer[0]), drawBuffer); // supplying array if attchment locations 
			glClearColor(0, 0, 0, 1);
			glClear(GL_COLOR_BUFFER_BIT);

			// initialise Normal-Buffer to (0.5, 0.5, 0.5)
			glDrawBuffer(GL_COLOR_ATTACHMENT0 + GBUFFER_TEXTURE_TYPE_NORMAL);
			glClearColor(0.5f, 0.5f, 0.5f, 1);
			glClear(GL_COLOR_BUFFER_BIT);

			GLuint attachments[] = {
				 GL_COLOR_ATTACHMENT0 + GBUFFER_TEXTURE_TYPE_COLOR,
				 GL_COLOR_ATTACHMENT0 + GBUFFER_TEXTURE_TYPE_REFLECTION,
				 GL_COLOR_ATTACHMENT0 + GBUFFER_TEXTURE_TYPE_NORMAL,
			};
			glDrawBuffers(sizeof(attachments) / sizeof(attachments[0]), attachments);
		};

		void bindForLightPass() {
			glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
			glDrawBuffer(GL_COLOR_ATTACHMENT3);

			// Color Buffer
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, m_textures[GBUFFER_TEXTURE_TYPE_COLOR]);
			// Reflection Buffer
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, m_textures[GBUFFER_TEXTURE_TYPE_REFLECTION]);
			// Normal Buffer
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, m_textures[GBUFFER_TEXTURE_TYPE_NORMAL]);
			// Depth Buffer
			glActiveTexture(GL_TEXTURE3);
			glBindTexture(GL_TEXTURE_2D, m_depthTexture);
		};

		void bindForFinalPass() {
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
			glBindFramebuffer(GL_READ_FRAMEBUFFER, m_fbo);
			glReadBuffer(GL_COLOR_ATTACHMENT3);
			// set things up for the blitting 
			// default FBO is target | G Buffer FBO is sources
		};

		void setReadBuffer(GBUFFER_TEXTURE_TYPE TextureType)
		{
			glReadBuffer(GL_COLOR_ATTACHMENT0 + TextureType);
		}

	private:
		GLuint m_fbo;
		GLuint m_textures[GBUFFER_NUM_TEXTURES]; // Textures for the vertex attributes
		GLuint m_depthTexture; // Texture to serve as the depth buffer
		GLuint m_finalTexture; // Final Texture
};

#endif // !G_BUFFER_H
