#ifndef G_BUFFER_H
#define G_BUFFER_H

#define ZERO_MEM(a) memset(a, 0, sizeof(a))

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

		GBuffer() {
			m_fbo = 0;
			m_depthTexture = 0;
			m_finalTexture = 0;
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
			glGenTextures(1, &m_depthTexture);

			glGenTextures(1, &m_finalTexture); // allocate output (final) texture

			for (unsigned int i = 0; i < sizeof(m_textures) / sizeof(m_textures[0]); i++) {
				// creates the storage area of the texture (without initializing it)
				glBindTexture(GL_TEXTURE_2D, m_textures[i]);
					//if texture id is ALBEDOSPEC using RGBA
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, WindowWidth, WindowHeight, 0, (i == GBUFFER_TEXTURE_TYPE_ALBEDOSPEC) ? GL_RGBA : GL_RGB, GL_FLOAT, NULL);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); // prevents unnecessary interpolation between the texels that might create some fine distortions
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				// attaches the texture to the FBO as a target
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, m_textures[i], 0);
			}

			// depth (explicitly, because it requires diff. format and is attached to the FBO at a diff. spot)
			glBindTexture(GL_TEXTURE_2D, m_depthTexture);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH32F_STENCIL8, WindowWidth, WindowHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL); // leaves a full byte for the stencile value in each pixel
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, m_depthTexture, 0);// attached to Depth & Stencil attachment
		
			//final 
			glBindTexture(GL_TEXTURE_2D, m_finalTexture);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, WindowWidth, WindowHeight, 0, GL_RGB, GL_FLOAT, NULL);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT4, GL_TEXTURE_2D, m_finalTexture, 0); // attacht to number 4

			// explicitly tell OpenGl which color attachments to be used (of. FB)
			// enable writing to all four textures
			//GLenum drawBuffer[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3};
			//glDrawBuffers(sizeof(drawBuffer)/sizeof(drawBuffer[0]), drawBuffer); // supplying array if attchment locations 

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
			// now we keep changing the FBO, config the draw buffers for the attributes !
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_fbo);
			GLenum drawBuffer[] = { GL_COLOR_ATTACHMENT0,
				GL_COLOR_ATTACHMENT1,
				GL_COLOR_ATTACHMENT2,
				GL_COLOR_ATTACHMENT3 };
			glDrawBuffers(sizeof(drawBuffer) / sizeof(drawBuffer[0]), drawBuffer); // supplying array if attchment locations 
		};
		void bindForStencilPass() {
			// must disable the draw buffers
			glDrawBuffer(GL_NONE);
			// avoid garbaging the final buffer with a black image of the bounding sphere!
		};
		void bindForLightPass() {
			glDrawBuffer(GL_COLOR_ATTACHMENT4);
			for (unsigned int i = 0; i < sizeof(m_textures) / sizeof(m_textures[0]); i++) {
				glActiveTexture(GL_TEXTURE0 + i); // Binding default 3 textures
				glBindTexture(GL_TEXTURE_2D, m_textures[GBUFFER_TEXTURE_TYPE_POSITION] + i);
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
		/*	void bindForWriting() {
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_fbo);
		};*/

		/**
		 * Binds the FBOs as input so its contents can be dumped to the screen
		 */
		/*void bindForReading() {
			glBindFramebuffer(GL_READ_FRAMEBUFFER, m_fbo);
		};*/

		void setReadBuffer(GBUFFER_TEXTURE_TYPE TextureType)
		{
			glReadBuffer(GL_COLOR_ATTACHMENT0 + TextureType);
		}

		/*void bindForReadingTex() {
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0); // disconnecting it from the GL_DRAW_FRAMEBUFFER (binding default FB)
			for (unsigned int i = 0; i < sizeof(m_textures) / sizeof(m_textures[0]); i++) {
				glActiveTexture(GL_TEXTURE0 + i); // Binding default 3 textures
				glBindTexture(GL_TEXTURE_2D, m_textures[GBUFFER_TEXTURE_TYPE_POSITION]+i);
			}
		}*/

	private:
		GLuint m_fbo;
		GLuint m_textures[GBUFFER_NUM_TEXTURES]; // Textures for the vertex attributes
		GLuint m_depthTexture; // Texture to serve as the depth buffer
		GLuint m_finalTexture; // final texture for the color
};

#endif // !G_BUFFER_H
