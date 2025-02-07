#include <iostream>

#include <stdio.h>
#include <math.h>

#include <ew/external/glad.h>

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <ew/shader.h>
#include <ew/model.h>
#include <ew/camera.h>
#include <ew/cameraController.h>
#include <ew/transform.h>
#include <ew/texture.h>

void framebufferSizeCallback(GLFWwindow* window, int width, int height);
GLFWwindow* initWindow(const char* title, int width, int height);
void drawUI();
void resetCamera(ew::Camera* camera, ew::CameraController* controller);
void constructFramebuffer(unsigned int& fbo, unsigned int& colorbufferTexture, unsigned int& rbo);
void constructQuad(unsigned int& vao, unsigned int& vbo);
void recalculateOffsets(ew::Shader& screenShader);

struct Material {
	float Ka = 1.0;
	float Kd = 0.5;
	float Ks = 0.5;
	float Shininess = 128;
};

// Global state
int screenWidth = 1080;
int screenHeight = 720;
float prevFrameTime;
float deltaTime;

ew::Camera camera;
ew::CameraController cameraController;
Material material;

// too lazy to figure out a proper solution for bools in ImGui
int doInvert = 0;
int doEdgeDetection = 0;

float invOffset = 300.0f;

int main() {
	GLFWwindow* window = initWindow("Assignment 1", screenWidth, screenHeight);
	glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

	ew::Shader shader = ew::Shader("assets/shaders/lit.vert", "assets/shaders/lit.frag");
	ew::Shader screenShader = ew::Shader("assets/shaders/screen.vert", "assets/shaders/screen.frag");

	ew::Model monkeyModel = ew::Model("assets/suzanne.obj");
	ew::Transform monkeyTransform;
	// Handles to OpenGL object are unsigned integers
	GLuint brickTexture = ew::loadTexture("assets/PavingStones143_1K-JPG_Color.jpg");

	// default camera values
	camera.position = glm::vec3(0.0f, 0.0f, 5.0f);
	camera.target = glm::vec3(0.0f, 0.0f, 0.0f); // Look at the center of the scene
	camera.aspectRatio = (float)screenWidth / screenHeight;
	camera.fov = 60.0f; // Vertical field of view, in degrees

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK); // Back face culling
	//glEnable(GL_DEPTH_TEST); // Depth testing

	// construct and complete framebuffer
	unsigned int fbo;
	unsigned int colorbufferTexture;
	unsigned int rbo;
	constructFramebuffer(fbo, colorbufferTexture, rbo);

	// set up quad vao and vbo
	unsigned int quadVAO, quadVBO;
	constructQuad(quadVAO, quadVBO);

	// kernel for edge detection effect
	const float edgeKernel[9] = {1, 1, 1, 1, -8, 1, 1, 1, 1};

	screenShader.use();
	screenShader.setFloat("edgeKernel0", edgeKernel[0]);
	screenShader.setFloat("edgeKernel1", edgeKernel[1]);
	screenShader.setFloat("edgeKernel2", edgeKernel[2]);
	screenShader.setFloat("edgeKernel3", edgeKernel[3]);
	screenShader.setFloat("edgeKernel4", edgeKernel[4]);
	screenShader.setFloat("edgeKernel5", edgeKernel[5]);
	screenShader.setFloat("edgeKernel6", edgeKernel[6]);
	screenShader.setFloat("edgeKernel7", edgeKernel[7]);
	screenShader.setFloat("edgeKernel8", edgeKernel[8]);

	// main render loop
	while(!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		float time = (float)glfwGetTime();
		deltaTime = time - prevFrameTime;
		prevFrameTime = time;

		// update camera (aspect ratio & position)
		camera.aspectRatio = (float)screenWidth / screenHeight; // it's not inside framebufferSizeCallback, but it'll do
		cameraController.move(window, &camera, deltaTime); // cam control before actually using camera for anything

		// Bind brick texture to texture unit 0
		glBindTextureUnit(0, brickTexture);

		// Rotate model around Y axis
		monkeyTransform.rotation = glm::rotate(monkeyTransform.rotation, deltaTime, glm::vec3(0.0, 1.0, 0.0));

		// ---------- RENDER (with framebuffer) ---------- //
		/* draws to a framebuffer, then applies post-processing effects
		* https://learnopengl.com/Advanced-OpenGL/Framebuffers */

		// bind to framebuffer and draw scene as we normally would to color texture
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glEnable(GL_DEPTH_TEST); // enable depth testing (is disabled for rendering screen-space quad)

		// make sure we clear the framebuffer's content
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// render as if you were rendering to default buffer
		shader.use();

		// material properties
		shader.setInt("_MainTex", 0);
		shader.setFloat("_Material.Ka", material.Ka);
		shader.setFloat("_Material.Kd", material.Kd);
		shader.setFloat("_Material.Ks", material.Ks);
		shader.setFloat("_Material.Shininess", material.Shininess);

		shader.setVec3("_EyePos", camera.position); // send camera position to shader

		// transform.modelMatrix() combines translation, rotation, and scale into a 4x4 model matrix
		shader.setMat4("_Model", monkeyTransform.modelMatrix());
		shader.setMat4("_ViewProjection", camera.projectionMatrix() * camera.viewMatrix());

		monkeyModel.draw(); // Draws monkey model using current shader

		// now bind back to default framebuffer and draw a quad plane with the attached framebuffer color texture
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glDisable(GL_DEPTH_TEST); // disable depth test so screen-space quad isn't discarded due to depth test.
		// clear all relevant buffers
		glClearColor(1.0f, 1.0f, 1.0f, 1.0f); // clear quad color (technically not necessary)
		glClear(GL_COLOR_BUFFER_BIT);

		// draw to screen quad
		screenShader.use();
		glBindVertexArray(quadVAO);
		glBindTexture(GL_TEXTURE_2D, colorbufferTexture);	// use the color attachment texture as the texture of the quad plane
		glDrawArrays(GL_TRIANGLES, 0, 6);

		screenShader.setInt("invert", doInvert);
		screenShader.setInt("edgeDetection", doEdgeDetection);
		recalculateOffsets(screenShader);

		drawUI(); // draws ImGui elements
		glfwSwapBuffers(window);
	}

	glDeleteFramebuffers(1, &fbo); // program ended, delete framebuffers

	printf("Shutting down...");
}

void drawUI() {
	ImGui_ImplGlfw_NewFrame();
	ImGui_ImplOpenGL3_NewFrame();
	ImGui::NewFrame();

	ImGui::Begin("Settings");

	if(ImGui::Button("Reset Camera")) { resetCamera(&camera, &cameraController); }
	if(ImGui::CollapsingHeader("Material")) {
		ImGui::SliderFloat("AmbientK", &material.Ka, 0.0f, 1.0f);
		ImGui::SliderFloat("DiffuseK", &material.Kd, 0.0f, 1.0f);
		ImGui::SliderFloat("SpecularK", &material.Ks, 0.0f, 1.0f);
		ImGui::SliderFloat("Shininess", &material.Shininess, 2.0f, 1024.0f);
	}
	if(ImGui::CollapsingHeader("Post-Process Effects")) {
		ImGui::SliderInt("Invert", &doInvert, 0, 1);
		ImGui::SliderInt("Edge Detection", &doEdgeDetection, 0, 1);
		ImGui::SliderFloat("Inverse Offset", &invOffset, 100.0f, 1000.0f);
	}

	ImGui::End();

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
	screenWidth = width;
	screenHeight = height;
}

void resetCamera(ew::Camera* camera, ew::CameraController* controller) {
	camera->position = glm::vec3(0, 0, 5.0f);
	camera->target = glm::vec3(0);
	controller->yaw = controller->pitch = 0;
}

/* creates and completes a framebuffer, including a depth and stencil buffer
	* https://learnopengl.com/Advanced-OpenGL/Framebuffers */
void constructFramebuffer(unsigned int& fbo, unsigned int& colorbufferTexture, unsigned int& rbo) {
	// create framebuffer
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	// generate texture for color buffer
	glGenTextures(1, &colorbufferTexture);
	glBindTexture(GL_TEXTURE_2D, colorbufferTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, screenWidth, screenHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);

	// attach it to currently bound framebuffer object
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorbufferTexture, 0);

	// create renderbuffer for depth & stencil buffers
	glGenRenderbuffers(1, &rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, rbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, screenWidth, screenHeight);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

	// ensure that framebuffer is complete
	if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
	glBindFramebuffer(GL_FRAMEBUFFER, 0); // rebind to default framebuffer
}

/* constructs a fullscreen quad for use with post-processing effects
	* https://learnopengl.com/Advanced-OpenGL/Framebuffers */
void constructQuad(unsigned int& quadVAO, unsigned int& quadVBO) {
	float quadVertices[] = { // vertex attributes for a quad that fills the entire screen in Normalized Device Coordinates.
		// positions   // texCoords
		-1.0f,  1.0f,  0.0f, 1.0f,
		-1.0f, -1.0f,  0.0f, 0.0f,
		 1.0f, -1.0f,  1.0f, 0.0f,

		-1.0f,  1.0f,  0.0f, 1.0f,
		 1.0f, -1.0f,  1.0f, 0.0f,
		 1.0f,  1.0f,  1.0f, 1.0f
	};

	glGenVertexArrays(1, &quadVAO);
	glGenBuffers(1, &quadVBO);
	glBindVertexArray(quadVAO);
	glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
}

void recalculateOffsets(ew::Shader& screenShader) {
	float offset = 1.0f / invOffset;

	// offsets for convolution matrix
	float offsets[9][2] = {
		{ -offset, offset  },  // top-left
		{  0.0f,   offset  },  // top-center
		{  offset, offset  },  // top-right
		{ -offset, 0.0f    },  // center-left
		{  0.0f,   0.0f    },  // center-center
		{  offset, 0.0f    },  // center-right
		{ -offset, -offset },  // bottom-left
		{  0.0f,   -offset },  // bottom-center
		{  offset, -offset }   // bottom-right
	};

	screenShader.use();
	screenShader.setVec2("offset0", offsets[0][0], offsets[0][1]);
	screenShader.setVec2("offset1", offsets[1][0], offsets[1][1]);
	screenShader.setVec2("offset2", offsets[2][0], offsets[2][1]);
	screenShader.setVec2("offset3", offsets[3][0], offsets[3][1]);
	screenShader.setVec2("offset4", offsets[4][0], offsets[4][1]);
	screenShader.setVec2("offset5", offsets[5][0], offsets[5][1]);
	screenShader.setVec2("offset6", offsets[6][0], offsets[6][1]);
	screenShader.setVec2("offset7", offsets[7][0], offsets[7][1]);
	screenShader.setVec2("offset8", offsets[8][0], offsets[8][1]);
}

/// <summary>
/// Initializes GLFW, GLAD, and IMGUI
/// </summary>
/// <param name="title">Window title</param>
/// <param name="width">Window width</param>
/// <param name="height">Window height</param>
/// <returns>Returns window handle on success or null on fail</returns>
GLFWwindow* initWindow(const char* title, int width, int height) {
	printf("Initializing...");
	if (!glfwInit()) {
		printf("GLFW failed to init!");
		return nullptr;
	}

	GLFWwindow* window = glfwCreateWindow(width, height, title, NULL, NULL);
	if (window == NULL) {
		printf("GLFW failed to create window");
		return nullptr;
	}
	glfwMakeContextCurrent(window);

	if (!gladLoadGL(glfwGetProcAddress)) {
		printf("GLAD Failed to load GL headers");
		return nullptr;
	}

	//Initialize ImGUI
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init();

	return window;
}
