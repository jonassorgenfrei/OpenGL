#version 330 core
out vec4 fragmentColor;

in vec2 texCoord;

//notice the sampler
uniform sampler2DMS screencapture;	// to retrieve the color per subsample
uniform int viewport_width;
uniform int viewport_height;
				
void main(){
	//texelFetch requires a vec of ints for indexing (since we're indexing pixel locations)
	//texture coords is range [0, 1], we need range [0, viewport_dim].
	//texture coords are essentially a percentage, so we can multiply text coords by total size 
	ivec2 vpCoords = ivec2(viewport_width, viewport_height);
	vpCoords.x = int(vpCoords.x * texCoord.x); 
	vpCoords.y = int(vpCoords.y * texCoord.y);

	//do a simple average since this is just a demo
	vec4 sample1 = texelFetch(screencapture, vpCoords, 0);	// retrieve the color per sample
	vec4 sample2 = texelFetch(screencapture, vpCoords, 1);
	vec4 sample3 = texelFetch(screencapture, vpCoords, 2);
	vec4 sample4 = texelFetch(screencapture, vpCoords, 3);
	fragmentColor = (sample1 + sample2 + sample3 + sample4) / 4.0f;
}





/*
	// EXMPALE : CPP CODE 
	// -------------------------------------- AA FRAME BUFFER SETUP-------------------------------------------------------------
	GLuint msaaFB;
	glGenFramebuffers(1, &msaaFB);
	glBindFramebuffer(GL_FRAMEBUFFER, msaaFB); //bind both read/write to the target framebuffer

	GLuint texMutiSampleColor;
	glGenTextures(1, &texMutiSampleColor);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, texMutiSampleColor);
	glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, GL_RGB, width, height, GL_TRUE);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, texMutiSampleColor, 0);

	GLuint rendbufferMSAADepthStencil;
	glGenRenderbuffers(1, &rendbufferMSAADepthStencil);
	glBindRenderbuffer(GL_RENDERBUFFER, rendbufferMSAADepthStencil);
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_DEPTH24_STENCIL8, width, height);
	glBindRenderbuffer(GL_RENDERBUFFER, 0); //unbind the render buffer
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rendbufferMSAADepthStencil);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		std::cerr << "error on setup of framebuffer" << std::endl;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0); //bind to default frame buffer

		// -------------------------------------- END FRAME BUFFER -------------------------------------------------------------

                ...
                ...
                ...
                ...
                ...
            //renderloop
                    ...
		...
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		glDisable(GL_DEPTH_TEST); //don't let depth buffer disrupt drawing (although this doesn't seem to affect setup)

		postprocessShader.use();
		glBindVertexArray(postVAO);
		glActiveTexture(GL_TEXTURE0);
		//------------------ notice that this is NOT GL_TEXTURE_2D -------------------
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, texMutiSampleColor);
		//------------------ notice that this is NOT GL_TEXTURE_2D -------------------
		glDrawArrays(GL_TRIANGLES, 0, 6);

		glEnable(GL_DEPTH_TEST); //don't let depth buffer disrupt drawing
                    ...
                    ...
*/