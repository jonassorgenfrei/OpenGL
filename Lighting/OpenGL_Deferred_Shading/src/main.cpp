/* 
 *	Deferred Shading
 */

#define LEARNOPENGL 0 // 0 ogldev | 1 learnogl
#define DEBUG_DEFFERED 1 // only works with ogldev

#define GLM_FORCE_SILENT_WARNINGS 1

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "shader_m.h"
#include "camera.h"
#include "model.h"
#include "stb_image.h"
#include "gBuffer.h"
#include "filesystem.h"

#include <iostream>

// callbacks
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
// current Mouse position 
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
/* load texture */
unsigned int loadTexture(const char *path, bool gammaCorrection);
void renderQuad();
void renderCube();
void renderSphere();

void icon(GLFWwindow* window);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 5.0f));
double lastX = SCR_WIDTH / 2.0;
double lastY = SCR_HEIGHT / 2.0;
bool firstMouse = true;

// timing 
float deltaTime = 0.0f; // time between current frame and last frame
float lastFrame = 0.0f; // time of last frame

int main()
{
	// glfw: initialize and configure
	// ------------------------------
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	// Resizable Window 
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // uncomment this statement to fix compilation on OS X
#endif

	// glfw window creation
	// --------------------
	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Advanced GL", NULL, NULL);
	/* creates and configures default framebuffer */
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);

	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);

	//tell GLFW to capture our mouse
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

	// glad: load all OpenGL function pointers
	// ---------------------------------------
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	icon(window);

	// configure global opengl state
	// -----------------------------
	/* DEPTH BUFFER */
	glEnable(GL_DEPTH_TEST);


	// build and compile shader program(s)
	// ------------------------------------
	Shader gBufferShader(FileSystem::getPath("shader/gBufferShader.vert").c_str(), FileSystem::getPath("shader/gBufferShader.frag").c_str());
	Shader lightingPassShader(FileSystem::getPath("shader/lightingPassShader.vert").c_str(), FileSystem::getPath("shader/lightingPassShader.frag").c_str());
	Shader shaderLightBox(FileSystem::getPath("shader/lightBox.vs").c_str(), FileSystem::getPath("shader/lightBox.fs").c_str());
	Shader pointLightShader(FileSystem::getPath("shader/pointLightShader.vs").c_str(), FileSystem::getPath("shader/pointLightShader.fs").c_str());
	Shader dirLightShader(FileSystem::getPath("shader/dirLightShader.vs").c_str(), FileSystem::getPath("shader/dirLightShader.fs").c_str());
	Shader stencilTestShader(FileSystem::getPath("shader/stencilTestShader.vs").c_str(), FileSystem::getPath("shader/stencilTestShader.fs").c_str());

	// load models
	// -----------
	Model object(FileSystem::getPath("../../content/models/nanosuit/nanosuit.obj").c_str());
	std::vector<glm::vec3> objectPositions;
	objectPositions.push_back(glm::vec3(-3.0, -3.0, -3.0));
	objectPositions.push_back(glm::vec3(0.0, -3.0, -3.0));
	objectPositions.push_back(glm::vec3(3.0, -3.0, -3.0));
	objectPositions.push_back(glm::vec3(-3.0, -3.0, 0.0));
	objectPositions.push_back(glm::vec3(0.0, -3.0, 0.0));
	objectPositions.push_back(glm::vec3(3.0, -3.0, 0.0));
	objectPositions.push_back(glm::vec3(-3.0, -3.0, 3.0));
	objectPositions.push_back(glm::vec3(0.0, -3.0, 3.0));
	objectPositions.push_back(glm::vec3(3.0, -3.0, 3.0));
	

	/*Model object(FileSystem::getPath("../../content/models/backpack/backpack.obj").c_str());
	std::vector<glm::vec3> objectPositions;
	objectPositions.push_back(glm::vec3(-3.0, -0.5, -3.0));
	objectPositions.push_back(glm::vec3(0.0, -0.5, -3.0));
	objectPositions.push_back(glm::vec3(3.0, -0.5, -3.0));
	objectPositions.push_back(glm::vec3(-3.0, -0.5, 0.0));
	objectPositions.push_back(glm::vec3(0.0, -0.5, 0.0));
	objectPositions.push_back(glm::vec3(3.0, -0.5, 0.0));
	objectPositions.push_back(glm::vec3(-3.0, -0.5, 3.0));
	objectPositions.push_back(glm::vec3(0.0, -0.5, 3.0));
	objectPositions.push_back(glm::vec3(3.0, -0.5, 3.0));
	*/

	
	// load textures
	// -------------
	// G-Buffer Setup 
	// --------------
#if LEARNOPENGL
	// learnopenGL
	unsigned int gBuffer;
	glGenFramebuffers(1, &gBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
	unsigned int gPosition, gNormal, gAlbedoSpec;

	// position color buffer
	glGenTextures(1, &gPosition);
	glBindTexture(GL_TEXTURE_2D, gPosition);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_FLOAT, NULL); // RGB only 3 Channels !
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPosition, 0);

	// normal color buffer
	glGenTextures(1, &gNormal);
	glBindTexture(GL_TEXTURE_2D, gNormal);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_FLOAT, NULL); // RGB only 3 Channels !
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal, 0);

	// color + spec color buffer
	glGenTextures(1, &gAlbedoSpec);
	glBindTexture(GL_TEXTURE_2D, gAlbedoSpec);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL); // needs RGBA only 4 Channels !
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gAlbedoSpec, 0);
	// Store Albedo & specular in a single texture!! 
	// it's possible to combine data in single tex. espac. in a complex pipeline

	// explicitly tell OpenGl which color attachments to be used (of. FB)
	unsigned int attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
	glDrawBuffers(3, attachments);

	// set up render buffer obj. as depth buffer & check for completeness
	unsigned int rboDepth;
	glGenRenderbuffers(1, &rboDepth);
	glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);
	// finally check if FB is complete
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "Framebuffer not complete!" << std::endl;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
#else
	// OpenGLDev
	GBuffer gBuffer = GBuffer();
	if (!gBuffer.init(SCR_WIDTH, SCR_HEIGHT)) {
		std::cout << "Failed to initialise GBuffer" << std::endl;
		exit(1);
	}
#endif // LEARNOPENGL

	// lighting info
	// -------------
	const unsigned int NR_LIGHTS = 32;
	std::vector<glm::vec3> lightPositions;
	std::vector<glm::vec3> lightColors;
	srand(13);

	for (unsigned int i = 0; i < NR_LIGHTS; i++)
	{
		// calculate slightly random offsets
        float xPos = static_cast<float>(((rand() % 100) / 100.0) * 6.0 - 3.0);
        float yPos = static_cast<float>(((rand() % 100) / 100.0) * 6.0 - 4.0);
        float zPos = static_cast<float>(((rand() % 100) / 100.0) * 6.0 - 3.0);
        lightPositions.push_back(glm::vec3(xPos, yPos, zPos));
        // also calculate random color
        float rColor = static_cast<float>(((rand() % 100) / 200.0f) + 0.5); // between 0.5 and 1.)
        float gColor = static_cast<float>(((rand() % 100) / 200.0f) + 0.5); // between 0.5 and 1.)
        float bColor = static_cast<float>(((rand() % 100) / 200.0f) + 0.5); // between 0.5 and 1.)
        lightColors.push_back(glm::vec3(rColor, gColor, bColor));
	}

	// shader configuration
	// --------------------
	lightingPassShader.use();
	lightingPassShader.setInt("gPosition", 0);
	lightingPassShader.setInt("gNormal", 1);
	lightingPassShader.setInt("gAlbedoSpec", 2);

	// render loop
	// -----------
	while (!glfwWindowShouldClose(window))
	{
		// Back-Face Culling
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
		glFrontFace(GL_CCW);
		// 
		// Set frame time
		auto currentFrame = static_cast<float>(glfwGetTime());
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		// Check and call events
		processInput(window);

#if !LEARNOPENGL and !DEBUG_DEFFERED
		gBuffer.startFrame(); // inform GBuffer about the start of new Frames
#endif

		// render
		// ------
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// 1. geometry pass: render all geometric/color data to g-buffer
#if LEARNOPENGL
		//learnopenGl
		glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // clear the current FBO (G buffer)
#else 
		//opengldev
		gBuffer.bindForGeomPass(); //setting GBuffer object for writing
		

	#if !DEBUG_DEFFERED
		// only geometry pass updates the depth buffer
		glDepthMask(GL_TRUE); // enable writing into depth buffer befoe clearing it!
		// prevent anything but this pass from writing into the depth buffer
		// needs the depth buffer in order to populate the G-Buffer with closest pixels
		// in lightpass we have a single texel per screen pixel so we don't have anything to write into the deoth buffer
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // clear the current FBO (G buffer)
		//limit depth test to geometry pass
		glEnable(GL_DEPTH_TEST); 
				 
	//	glDisable(GL_BLEND); // manipulated elsewhere
	#else
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // clear the current FBO (G buffer)
	#endif //!DEBUG_DEFFERED

#endif //LEARNOPENGL		
			glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
			glm::mat4 view = camera.GetViewMatrix();
			glm::mat4 model(1.0);
			gBufferShader.use();
			gBufferShader.setMat4("projection", projection);
			gBufferShader.setMat4("view", view);
			//For each object
			for (unsigned int i = 0; i < objectPositions.size(); i++) {
				model = glm::mat4(1.0);
				model = glm::translate(model, objectPositions[i]);
				model = glm::scale(model, glm::vec3(0.25f));
				gBufferShader.setMat4("model", model);
				object.Draw(gBufferShader);
			}
#if !LEARNOPENGL and !DEBUG_DEFFERED
			// When we get here the depth buffer is already populated and the stencil pass
			// depends on it, but it does not write to it.
			glDepthMask(GL_FALSE);

			glDisable(GL_DEPTH_TEST); // manipulated elsewhere

#endif
			glBindFramebuffer(GL_FRAMEBUFFER, 0); // DrawFramebuffer

			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			// 2. lighting pass: use g-buffer to calculate the scene's lighting
#if LEARNOPENGL
			
			//learnopengl.com
			/*
				Problem unnessary Function calls
			 */
			// render 2D screen-filled quad & execute an exp. lighting fs on each pixel
			lightingPassShader.use();
			// bind all gBuffer Textures
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, gPosition);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, gNormal);
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, gAlbedoSpec);

			// send light relevant uniforms
			for (unsigned int i = 0; i < lightPositions.size(); i++)
			{
				lightingPassShader.setVec3("lights[" + std::to_string(i) + "].Position", lightPositions[i]);
				lightingPassShader.setVec3("lights[" + std::to_string(i) + "].Color", lightColors[i]);
				// update attenuation parameters and calculate radius
				const float constant = 1.0; // note that we don't send this to the shader, we assume it is always 1.0 (in our case)
				const float linear = 0.7;
				const float quadratic = 1.8;
				lightingPassShader.setFloat("lights[" + std::to_string(i) + "].Linear", linear);
				lightingPassShader.setFloat("lights[" + std::to_string(i) + "].Quadratic", quadratic);
				// then calculate radius of light volume/sphere
				float lightMax = std::fmaxf(std::fmaxf(lightColors[i].r, lightColors[i].g), lightColors[i].b);
				float radius =
					(-linear + std::sqrtf(linear * linear - 4 * quadratic * (constant - (256.0 / 5.0) * lightMax)))
					/ (2 * quadratic); // return value roughly between 1.0 and 5.0 (based on light's max intensity)
				lightingPassShader.setFloat("lights[" + std::to_string(i) + "].Radius", radius);
	}
			lightingPassShader.setVec3("viewPos", camera.Position);
			renderQuad(); // Render Quad to draw on */

			// copy depth information stored in the geometry pass into the default 
			// framebuffer's depth buffer
			glBindFramebuffer(GL_READ_FRAMEBUFFER, gBuffer); // specify read buffer
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0); // write to default framebuffer
			// blit to default framebuffer. Note that this may or may not work as the internal formats of both the FBO and default framebuffer have to match.
			// the internal formats are implementation defined. This works on all of my systems, but if it doesn't on yours you'll likely have to write to the
			// depth buffer in another shader stage (or somehow see to match the default framebuffer's internal format with the FBO's internal format).
			glBlitFramebuffer(
				0, 0, SCR_WIDTH, SCR_HEIGHT, 0, 0, SCR_WIDTH, SCR_HEIGHT, GL_DEPTH_BUFFER_BIT, GL_NEAREST
			); // allows to copy user-defined region of a framebuffer to a user-defined region of another framebuffer
			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			// render all light cubes with forward rendering as we'd normally do
			shaderLightBox.use();
			shaderLightBox.setMat4("projection", projection);
			shaderLightBox.setMat4("view", view);
			for (unsigned int i = 0; i < lightPositions.size(); i++) {
				model = glm::mat4(1.0);
				model = glm::translate(model, lightPositions[i]);
				model = glm::scale(model, glm::vec3(0.125f));
				shaderLightBox.setMat4("model", model);
				shaderLightBox.setVec3("lightColor", lightColors[i]);
				renderCube();
			}
			
#else
	//OpenGLDev
	#if DEBUG_DEFFERED
			// draw 4 GBuffer Textures as quads on full screen
			gBuffer.bindForReading(); // SOURCE FBO to be bound to GL_READ_FRAMEBUFFER

			GLsizei HalfWidth = (GLsizei)(SCR_WIDTH / 2.0f);
			GLsizei HalfHeight = (GLsizei)(SCR_HEIGHT / 2.0f);
			/* copy from the G buffer textures into the screen */
			gBuffer.setReadBuffer(GBuffer::GBUFFER_TEXTURE_TYPE_POSITION); // Bind specific texture to GL_READ_BUFFER (only can copy from a single Texture at a time)
			glBlitFramebuffer(0, 0, SCR_WIDTH, SCR_HEIGHT, // SRC_RECTANGLE
				0, 0, HalfWidth, HalfHeight, GL_COLOR_BUFFER_BIT, GL_LINEAR);// Destination Rectangle; SRC {Color, Depth or Stenticle Buffer}; Handle possible scaling {GL_NEAREST, GL_LINEAR (only for GL_COLOR_BUFFER_BIT)}

			gBuffer.setReadBuffer(GBuffer::GBUFFER_TEXTURE_TYPE_ALBEDOSPEC);
			glBlitFramebuffer(0, 0, SCR_WIDTH, SCR_HEIGHT,
				0, HalfHeight, HalfWidth, SCR_HEIGHT, GL_COLOR_BUFFER_BIT, GL_LINEAR);

			gBuffer.setReadBuffer(GBuffer::GBUFFER_TEXTURE_TYPE_NORMAL);
			glBlitFramebuffer(0, 0, SCR_WIDTH, SCR_HEIGHT,
				HalfWidth, HalfHeight, SCR_WIDTH, SCR_HEIGHT, GL_COLOR_BUFFER_BIT, GL_LINEAR);

			gBuffer.setReadBuffer(GBuffer::GBUFFER_TEXTURE_TYPE_TEXCOORD);
			glBlitFramebuffer(0, 0, SCR_WIDTH, SCR_HEIGHT,
				HalfWidth, 0, SCR_WIDTH, HalfHeight, GL_COLOR_BUFFER_BIT, GL_LINEAR);
	#else 
			/*
			 * We need stencil to be enabled in the stencil pass to get the stencil
			 * buffer updated and we also need it in the light pass because we render light
			 * only if the stencil passes.
			 */
			glEnable(GL_STENCIL_TEST); // enable stencil test
			
			//gBuffer.bindForReading(); // set Buffer for reading
			//glClear(GL_COLOR_BUFFER_BIT); // clear color buffer

			/*
			 * Its better to use separate shaders than adding banch inside the shader 
			 */

			/*
			 * Problems with current implementation:
			 *	-> when camera enters the light volume the light disappears
			 *			reas: only render front face of the bounding sphere 
			 *				if disable Backface cull. due to blending we will get an increased light (render twice)
			 *				and only half of it when inse 
			 *	-> second problem is that the bounding sphere doesn't really bound the light and sometimes obj. that are outside of it are also lit beause the sphere covers them in screen space so we calc. lighting on them
			 *
			 */

			// For POINT LIGHTS
			pointLightShader.use();
			pointLightShader.setMat4("projection", projection);
			pointLightShader.setMat4("view", view);
			pointLightShader.setVec3("viewPos", camera.Position);
			pointLightShader.setInt("gPosition", 0);
			pointLightShader.setInt("gNormal", 1);
			pointLightShader.setInt("gAlbedoSpec", 2);
			pointLightShader.setVec2("gScreenSize", glm::vec2(SCR_WIDTH, SCR_HEIGHT));

			stencilTestShader.use();
			stencilTestShader.setMat4("projection", projection);
			stencilTestShader.setMat4("view", view);

			// Render bounding sphere for each point light
			for (unsigned int i = 0; i < lightPositions.size(); i++) {

				/* Setup Point Light Volume */
				model = glm::mat4(1.0);
				model = glm::translate(model, lightPositions[i]);
				const float constant = 1.0; // note that we don't send this to the shader, we assume it is always 1.0 (in our case)
				const float linear = 0.7;
				const float quadratic = 1.8;
				float lightMax = std::fmaxf(std::fmaxf(lightColors[i].r, lightColors[i].g), lightColors[i].b);
				float radius =
					(-linear + std::sqrtf(linear * linear - 4 * quadratic * (constant - (256.0 / 5.0) * lightMax)))
					/ (2 * quadratic); // return value roughly between 1.0 and 5.0 (based on light's max intensity)
				model = glm::scale(model, glm::vec3(radius));


				// 2.5 BEGINN Stencil Pass
				stencilTestShader.use();
				// Disable color/depth write and enable stencil
				gBuffer.bindForStencilPass(); // only bind stencil buffer
				glEnable(GL_DEPTH_TEST);

				glDisable(GL_CULL_FACE); // disable culling because we want to process both, the front and the 
										// back faces on each polygon

				glClear(GL_STENCIL_BUFFER_BIT); // clear stencil buffer

				// enable Stencil test, but succeed always (only dpeth test matters)
				glStencilFunc(GL_ALWAYS, 0, 0);

				glStencilOpSeparate(GL_BACK, GL_KEEP, GL_INCR_WRAP, GL_KEEP);
				glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_DECR_WRAP, GL_KEEP);
				stencilTestShader.setMat4("model", model);
				renderSphere(); // render bounding sphere based on the light params 

				//gBuffer.bindForReadingTex();
				
				// END Stencil Pass

				// for each light we do a stencil pass (marks the relevant pixel)
				// for each, becuase if stencil vlaue gets greater than zero due to one of the lights, we cannt tell whether another light src
				// which also overlaps the sampe pixel is relevant or not
				// point light pass, depends on the stencil value

				gBuffer.bindForLightPass(); // setup GBuffer
				pointLightShader.use();

				glStencilFunc(GL_NOTEQUAL, 0, 0xFF); // set up stencil 
				// test to pass, when the stencil value is not equal to zero

				glDisable(GL_DEPTH_TEST); // disable depth test dont need; on some GPUs performance +
				/* Blending for both light types, each light Source handled by its own
				   draw call.
				   Blending: takes a SRCcolor (out of FS) abd a DESTcolor (FB)
							performs calc. on them
				 */
				glEnable(GL_BLEND); // Enable Blending
				glBlendEquation(GL_FUNC_ADD); // GPU will simply add the source and the destination
				glBlendFunc(GL_ONE, GL_ONE); // true addition
				/*
					Res: 1 * src + 1 * dst
				 */

				glEnable(GL_CULL_FACE); // enable culling of the front face polygons
				//glCullFace(GL_FRONT); // because the camera may be inside the light volume
				// and if we do back face culling as normally we will not see the light until we exit its
				// colume 
		
				// Render bounding sphere as usual
				pointLightShader.setMat4("model", model);
				pointLightShader.setVec3("gPointLight.Base.Color", lightColors[i]);
				pointLightShader.setFloat("gPointLight.Base.AmbientIntensity", 0.1);
				pointLightShader.setFloat("gPointLight.Base.DiffuseIntensity", 0.1);
				pointLightShader.setVec3("gPointLight.Position", lightPositions[i]);
				pointLightShader.setFloat("gPointLight.Atten.Constant", constant);
				pointLightShader.setFloat("gPointLight.Atten.Linear", linear);
				pointLightShader.setFloat("gPointLight.Atten.Exp", quadratic);
				renderSphere(); // Render bounding sphere for each point light
				//glCullFace(GL_BACK);
		
				glDisable(GL_BLEND);
			}

			/*
			 * The directional light does not need a stencil test because
			 * its volume is unlimited and the final pass simply copies
			 * the texture.
			 */
			glDisable(GL_STENCIL_TEST); // disable stencil test

			gBuffer.bindForLightPass();
			// For DIRECTIONAL LIGHTS
			/* Render Quad from (-1,-1) to (1,1) after Multilication with identity Matrix: after persp. devide and screen space transform 
				result as Quad from (0,0) to (SCREEN_WIDTH, SCREEN_HEIGHT) */
			dirLightShader.use();
			//gBuffer.bindForReadingTex();
			dirLightShader.setVec3("viewPos", camera.Position);
			dirLightShader.setInt("gPosition", 0);
			dirLightShader.setInt("gNormal", 1);
			dirLightShader.setInt("gAlbedoSpec", 2);
			dirLightShader.setVec2("gScreenSize", glm::vec2(SCR_WIDTH, SCR_HEIGHT));
			dirLightShader.setVec3("gDirectionalLight.Base.Color", glm::vec3(1.0,1.0,1.0));
			dirLightShader.setFloat("gDirectionalLight.Base.AmbientIntensity", 0.05);
			dirLightShader.setFloat("gDirectionalLight.Base.DiffuseIntensity", 0.05);
			dirLightShader.setVec3("gDirectionalLight.Direction", glm::vec3(0, 0, -1.0));
			glDisable(GL_DEPTH_TEST);
			glEnable(GL_BLEND);
			glBlendEquation(GL_FUNC_ADD);
			glBlendFunc(GL_ONE, GL_ONE);
			renderQuad();
			glDisable(GL_BLEND);
			/* DS FINAL PASS */
			gBuffer.bindForFinalPass();
			glBlitFramebuffer(0,0, SCR_WIDTH, SCR_HEIGHT,
								0,0, SCR_WIDTH, SCR_HEIGHT, GL_COLOR_BUFFER_BIT, GL_LINEAR);
			// blit from the color buffer inside the G Buffer into the screen
			// added an intermediate color buffer because:
			//		the G Buffer combines as a target the values from the depth buffer
			//		when run the point light pass, we setup the stencil stuff and we need to use
			//		the values from the depth buffer
			//		PROBLEM: if we render into the default FBO we wont have access to the 
			//		depth buffer from the G Buffer
			//		But the G Buffer must have its own depth buffer, because when we render into 
			//		its FBO we dont have access to the depth buffer from the default FBO
			//		SOLUTION: Add a color buffer to render into the GBuffer FBO and in the final pass, blit it to the
			//		default FBO color buffer


			// due to complexity of the GBuffer class
	#endif //DEBUG_DEFFERED
	
#endif
			
		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		// -------------------------------------------------------------------------------
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// optional: de-allocate all resources once they've outlived their purpose:
	// ------------------------------------------------------------------------
	//glDeleteVertexArrays(1, &planeVAO);
	//glDeleteBuffers(1, &planeVBO);;
	
	// glfw: terminate, clearing all previously allocated GLFW resources.
	// ------------------------------------------------------------------
	glfwTerminate();
	return 0;
}

// renderSphere() renders a 1x1 3D sphere in NDC.
// -------------------------------------------------
unsigned int sphereVAO = 0;
unsigned int indexCount;

void renderSphere()
{
	// initialize (if necessary)
	if (sphereVAO == 0)
	{
		glGenVertexArrays(1, &sphereVAO);

		unsigned int vbo, ebo;
		glGenBuffers(1, &vbo);
		glGenBuffers(1, &ebo);

		std::vector<glm::vec3> positions;
		std::vector<glm::vec2> uv;
		std::vector<glm::vec3> normals;
		std::vector<unsigned int> indices;

		const unsigned int X_SEGMENTS = 64;
		const unsigned int Y_SEGMENTS = 64;
		const float PI = 3.14159265359f;
		for (unsigned int y = 0; y <= Y_SEGMENTS; ++y)
		{
			for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
			{
				float xSegment = (float)x / (float)X_SEGMENTS;
				float ySegment = (float)y / (float)Y_SEGMENTS;
				float xPos = std::cos(xSegment * 2.0f * PI) * std::sin(ySegment * PI);
				float yPos = std::cos(ySegment * PI);
				float zPos = std::sin(xSegment * 2.0f * PI) * std::sin(ySegment * PI);

				positions.push_back(glm::vec3(xPos, yPos, zPos));
				uv.push_back(glm::vec2(xSegment, ySegment));
				normals.push_back(glm::vec3(xPos, yPos, zPos));
			}
		}

		bool oddRow = false;
		for (int y = 0; y < Y_SEGMENTS; ++y)
		{
			if (!oddRow) // even rows: y == 0, y == 2; and so on
			{
				for (int x = 0; x <= X_SEGMENTS; ++x)
				{
					indices.push_back(y       * (X_SEGMENTS + 1) + x);
					indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
				}
			}
			else
			{
				for (int x = X_SEGMENTS; x >= 0; --x)
				{
					indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
					indices.push_back(y       * (X_SEGMENTS + 1) + x);
				}
			}
			oddRow = !oddRow;
		}
		indexCount = (unsigned int)indices.size();

		std::vector<float> data;
		for (int i = 0; i < positions.size(); ++i)
		{
			data.push_back(positions[i].x);
			data.push_back(positions[i].y);
			data.push_back(positions[i].z);
			if (uv.size() > 0)
			{
				data.push_back(uv[i].x);
				data.push_back(uv[i].y);
			}
			if (normals.size() > 0)
			{
				data.push_back(normals[i].x);
				data.push_back(normals[i].y);
				data.push_back(normals[i].z);
			}
		}
		glBindVertexArray(sphereVAO);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), &data[0], GL_STATIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
		GLsizei stride = (3 + 2 + 3) * sizeof(float);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)(5 * sizeof(float)));
	}
	// render Cube
	glBindVertexArray(sphereVAO);
	glDrawElements(GL_TRIANGLE_STRIP, indexCount, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
}

// renderCube() renders a 1x1 3D cube in NDC.
// -------------------------------------------------
unsigned int cubeVAO = 0;
unsigned int cubeVBO = 0;
void renderCube()
{
	// initialize (if necessary)
	if (cubeVAO == 0)
	{
		float vertices[] = {
			// back face
			-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
			 1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
			 1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
			 1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
			-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
			-1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
			// front face
			-1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
			 1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
			 1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
			 1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
			-1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
			-1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
			// left face
			-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
			-1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
			-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
			-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
			-1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
			-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
			// right face
			 1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
			 1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
			 1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right         
			 1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
			 1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
			 1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left     
			// bottom face
			-1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
			 1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
			 1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
			 1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
			-1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
			-1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
			// top face
			-1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
			 1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
			 1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right     
			 1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
			-1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
			-1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left        
		};
		glGenVertexArrays(1, &cubeVAO);
		glGenBuffers(1, &cubeVBO);
		// fill buffer
		glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
		// link vertex attributes
		glBindVertexArray(cubeVAO);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}
	// render Cube
	glBindVertexArray(cubeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
}

// renderQuad() renders a 1x1 XY quad in NDC
// -----------------------------------------
unsigned int quadVAO = 0;
unsigned int quadVBO;
void renderQuad()
{
	if (quadVAO == 0)
	{
		float quadVertices[] = {
			// positions        // texture Coords
			-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
			-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
			 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
			 1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
		};
		// setup plane VAO
		glGenVertexArrays(1, &quadVAO);
		glGenBuffers(1, &quadVBO);
		glBindVertexArray(quadVAO);
		glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	}
	glBindVertexArray(quadVAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
}


// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{	

	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	float cameraSpeed = 2.5f * deltaTime; // adjust accordingly
	(void)cameraSpeed;
	if (glfwGetKey(window, GLFW_KEY_F2) == GLFW_PRESS)
	{
		const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
		glfwSetWindowMonitor(window, glfwGetPrimaryMonitor(), 0, 0, mode->width, mode->height, mode->refreshRate);
	}

}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	//Rotation 
	if (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS && 
		glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
		if (firstMouse)
		{
			lastX = xpos;
			lastY = ypos;
			firstMouse = false;
		}

		double xoffset = xpos - lastX;
		double yoffset = lastY - ypos;
		// reversed since y-coordinates range from bottom to top
		lastX = xpos;
		lastY = ypos;

		camera.rotate((float)xoffset, (float)yoffset);
	} 

	//Moving 
	if (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS &&
		glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS) {
		if (firstMouse)
		{
			lastX = xpos;
			lastY = ypos;
			firstMouse = false;
		}

		double xoffset = xpos - lastX;
		double yoffset = lastY - ypos;
		// reversed since y-coordinates range from bottom to top
		lastX = xpos;
		lastY = ypos;

		//camera.ProcessMouseMovement(xoffset, yoffset);
		if (xoffset > 0) {
			camera.translate(LEFT, (float)xoffset*0.05f);
		}
		else {
			camera.translate(RIGHT, (float)xoffset*-0.05f);
		}

		if (yoffset > 0) {
			camera.translate(DOWN, (float)yoffset*0.05f);
		}
		else {
			camera.translate(UP, (float)yoffset*-0.05f);
		}
	}
	
	//Zoom 
	if (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS &&
		glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
		if (firstMouse)
		{
			lastX = xpos;
			//lastY = ypos;
			firstMouse = false;
		}

		double xoffset = xpos - lastX;
		//float yoffset = lastY - ypos;
		// reversed since y-coordinates range from bottom to top
		lastX = xpos;
		//lastY = ypos;

		//camera.ProcessMouseMovement(xoffset, yoffset);
		if (xoffset > 0.0) {
			camera.translate(FORWARD, (float)xoffset*0.05f);
		}
		else {
			camera.translate(BACKWARD, (float)xoffset*-0.05f);
		}
	}

	if ((	glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_RELEASE ||
			glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_RELEASE) && 
		(	glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_RELEASE ||
			glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_RELEASE) && 
		(	glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_RELEASE ||
				glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE)) {
		firstMouse = true;
	}
	
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	(void)window;
	(void)xoffset;
	camera.ProcessMouseScroll((float)yoffset);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	// make sure the viewport matches the new window dimensions; note that width and 
	// height will be significantly larger than specified on retina displays.
	(void)window;
	glViewport(0, 0, width, height);
}

// utility function for loading a 2D texture from file
// ---------------------------------------------------
unsigned int loadTexture(char const *path, bool gammaCorrection)
{
	/*
	 * Careful: specular-maps anf normal-maps are almost always in lin. space!!! Using SRGB will break down the lightning
	 */
	unsigned int textureID;
	glGenTextures(1, &textureID);

	int width, height, nrComponents;
	unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
	if (data)
	{
		GLenum internalFormat = NULL;
		GLenum dataFormat = NULL;
		if (nrComponents == 1) {
			internalFormat = dataFormat = GL_RED;
		}
		else if (nrComponents == 3) {
			internalFormat = (gammaCorrection) ? GL_SRGB : GL_RGB;	// GL_SRGB => OpenGL will correct image to linear color space
			dataFormat = GL_RGB;
		}
		else if (nrComponents == 4) {
			internalFormat = (gammaCorrection) ? GL_SRGB_ALPHA : GL_RGBA; // GL_SRGB_ALPHA => OpenGL will correct image to linear color space
			dataFormat = GL_RGBA;
		}

		if (internalFormat && dataFormat){
			glBindTexture(GL_TEXTURE_2D, textureID);
			glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, dataFormat, GL_UNSIGNED_BYTE, data);
			glGenerateMipmap(GL_TEXTURE_2D);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, (internalFormat == GL_RGBA || internalFormat == GL_SRGB_ALPHA) ? GL_CLAMP_TO_EDGE : GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, (internalFormat == GL_RGBA || internalFormat == GL_SRGB_ALPHA) ? GL_CLAMP_TO_EDGE : GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		}
		stbi_image_free(data);
	}
	else
	{
		std::cout << "Texture failed to load at path: " << path << std::endl;
		stbi_image_free(data);
	}

	return textureID;
}

void icon(GLFWwindow* window) {
	//GLFW ICON
	// a simple glfw logo
	const char* const logo[] =
	{
		"................",
		"................",
		"...0000..0......",
		"...0.....0......",
		"...0.00..0......",
		"...0..0..0......",
		"...0000..0000...",
		"................",
		"................",
		"...1111..1111...",
		"......1..1......",
		"......1..1111...",
		"...1..1.....1...",
		"...1111..1111...",
		"................",
		"................"
	};

	const unsigned char icon_colors[5][4] =
	{
		{ 0,   0,   0, 255 }, // black
		{ 255,   0,   0, 255 }, // red
		{ 0, 255,   0, 255 }, // green
		{ 0,   0, 255, 255 }, // blue
		{ 255, 255, 255, 255 }  // white
	};
	int x, y;
	unsigned char pixels[16 * 16 * 4];
	unsigned char* target = pixels;
	GLFWimage img = { 16, 16, pixels };

	for (y = 0; y < img.width; y++)
	{
		for (x = 0; x < img.height; x++)
		{
			if (logo[y][x] == '0')
				memcpy(target, icon_colors[0], 4);
			else if (logo[y][x] == '1')
				memcpy(target, icon_colors[1], 4);
			else
				memset(target, 0, 4);
			target += 4;
		}
	}

	glfwSetWindowIcon(window, 1, &img);
}