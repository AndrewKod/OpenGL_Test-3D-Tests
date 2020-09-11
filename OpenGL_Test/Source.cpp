#include <glad.c>
#include <glad.h>
#include <glfw3.h>
#include <stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Shader.h"
#include "Camera.h"
#include "Model.h"

#include <iostream>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
unsigned int loadTexture(const char *path);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = (float)SCR_WIDTH / 2.0;
float lastY = (float)SCR_HEIGHT / 2.0;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

glm::mat4 postProcModel(1.0f);
GLboolean bPPModelChanged = false;
glm::vec3 postProcPosition(-0.000f, 0.0f, -0.0f);
GLfloat postProcInitSize = 1.0f;
GLfloat postProcInitScale = 1.0f;
GLfloat postProcScale = postProcInitScale;
GLfloat postProcScaleDelta = 0.0011f;

GLfloat kernelAngle = 45.f;
GLfloat kernelTreshold = kernelAngle/2.f;

const GLint kernelSize = 9;
const GLint sectorsSize = 8;

//bottom sobel effect
GLfloat kernel[kernelSize] = {
	-1, -2, -1,
	 0,  0,  0,
	 1,  2,  1
};

GLfloat a = kernelTreshold;
GLfloat b = kernelAngle;


GLfloat kernelAngles[sectorsSize] = {
	 0,
	 45,
	 90,
	 135,
	 180,
	 225,
	 270,
	 315
};
GLfloat kernelSectors[sectorsSize] = {
	 a,
	 a + b,
	 a + b * 2,
	 a + b * 3,
	 a + b * 4,
	 a + b * 5,
	 a + b * 6,
	 a + b * 7
};

GLint sectorsToCells[sectorsSize] = {
	 5,
	 2,
	 1,
	 0,
	 3,
	 6,
	 7,
	 8
};

void GenCubeVAO(GLuint& cubeVAO, GLuint& cubeVBO);
void GenPlaneVAO(GLuint& planeVAO, GLuint& planeVBO);
void GenPostProcVAO(GLuint& postProcVAO, GLuint& postProcVBO);
void GenSkyboxVAO(GLuint& skyboxVAO, GLuint& skyboxVBO);
void GenReflectVAO(GLuint& reflectVAO, GLuint& reflectVBO);


void DrawCubes(Shader & shader, GLuint VAO, GLuint texture, glm::vec3 scale = glm::vec3(1.0f),
	bool bStencil = false, glm::vec4 borderColor = glm::vec4(0.f));
void DrawFloor(Shader & shader, GLuint VAO, GLuint texture);

void DrawTransparent(Shader & shader, GLuint VAO, GLuint texture, const std::vector<glm::vec3>& positions, bool bSortPositions);

void DrawScene(Shader & shader, Shader & skyboxShader,
	GLuint cubeVAO, GLuint planeVAO, GLuint skyboxVAO, GLuint uboBlock,
	GLuint cubeTexture, GLuint floorTexture, GLuint cubemapTexture);

void DrawPostProc(Shader & shader, GLuint VAO,
	GLuint textureColorbuffer, GLuint textureColorbufferMS, bool bUseKernel = false, bool bAntiAliasing = false);
void SetKernelValue(Shader & postProcShader, GLint cellID);
void SetCellValue(Shader & postProcShader, GLint cellID, GLfloat cellValue, GLint sectorID);

void ScalePostProc();

GLuint loadCubemap(const vector<std::string>& faces);
void DrawSkybox(Shader & shader, GLuint VAO, GLuint texture, const glm::mat4& view, const glm::mat4& projection);


void DrawReflectCube(Shader & shader, GLuint VAO, GLuint texture, glm::vec3 cameraPos);
void DrawRefractCube(Shader & shader, GLuint VAO, GLuint texture, GLuint testTex, glm::vec3 cameraPos);


void FillModelMatrices(GLuint amount, glm::mat4 *modelMatrices);
void UpdateVAO(Model & model, GLuint amount, glm::mat4 *modelMatrices, GLuint& modelMatricesVBO);

int main()
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	////////////////////ANTI-ALIASING////////////////////
	//glfwWindowHint(GLFW_SAMPLES, 4);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
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

    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);
	
	glEnable(GL_STENCIL_TEST);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);
	glFrontFace(GL_CCW);

	////////////////////ANTI-ALIASING////////////////////
	//glEnable(GL_MULTISAMPLE);


    // build and compile shaders
    // -------------------------
    Shader shader("Shaders/Vertex Shader.glsl", "Shaders/Fragment Shader.glsl");
    Shader postProcShader("Shaders/Vertex Shader PP.glsl", "Shaders/Fragment Shader PP.glsl");
	Shader skyboxShader("Shaders/Cubemap Vertex Shader.glsl", "Shaders/Cubemap Fragment Shader.glsl");
	 
	Shader modelShader("Shaders/Vertex Shader Model.glsl", "Shaders/Fragment Shader Model.glsl",
		"Shaders/Geometry Shader Model.glsl");
	Shader modelShaderNormals("Shaders/Vertex Shader Model.glsl", "Shaders/Fragment Shader Model.glsl",
		"Shaders/Geometry Shader Model Normals.glsl");
	
	//////////////////////////////UNIFORM BUFFER//////////////////////////////
	//2x matrices 4x4
	unsigned int uboBlock;
	glGenBuffers(1, &uboBlock);
	glBindBuffer(GL_UNIFORM_BUFFER, uboBlock);
	glBufferData(GL_UNIFORM_BUFFER, 128, NULL, GL_STATIC_DRAW); // �������� 128 ���� ������
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	glBindBufferBase(
		GL_UNIFORM_BUFFER,
		0,				//binding point
		uboBlock);		//uniform buffer ID

	//postProc and skybox shaders are not need uniform buffer
	shader.BindUniformBuffer(
		"Matrices",		//uniform block name
		0);				//binding point
	
	modelShader.BindUniformBuffer("Matrices", 0);
	modelShaderNormals.BindUniformBuffer("Matrices", 0);

    // cube VAO
    GLuint cubeVAO, cubeVBO;
	GenCubeVAO(cubeVAO, cubeVBO);
    // plane VAO
	GLuint planeVAO, planeVBO;
	GenPlaneVAO(planeVAO, planeVBO);

    // screen quad VAO
	GLuint postProcVAO, postProcVBO;
	GenPostProcVAO(postProcVAO, postProcVBO);

    // load textures
    // -------------
    unsigned int cubeTexture = loadTexture("Textures/container.jpg");
    unsigned int floorTexture = loadTexture("Textures/matrix.jpg");

    // shader configuration
    // --------------------
    shader.UseProgram();
    shader.SetInt("texture1", 0);

	postProcShader.UseProgram();
	postProcShader.SetInt("screenTexture", 0);
	postProcShader.SetInt("screenTextureMS", 1);

    //////////////////////////////////////// framebuffer configuration /////////////////////////////////////
    // -------------------------
	GLuint samples = 4;

    unsigned int framebuffer;
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    // create a color attachment texture
    unsigned int textureColorbuffer;
    glGenTextures(1, &textureColorbuffer);
    glBindTexture(GL_TEXTURE_2D, textureColorbuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	////////////////////TEXTURE ANTI-ALIASING////////////////////
	GLuint textureColorbufferMS;
	glGenTextures(1, &textureColorbufferMS);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, textureColorbufferMS);
	glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, GL_RGB, SCR_WIDTH, SCR_HEIGHT, GL_TRUE);
	
	/////////////////////////////////////////////////////////////

    
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureColorbuffer, 0);
	////////////////////TEXTURE ANTI-ALIASING////////////////////
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D_MULTISAMPLE, textureColorbufferMS, 0);
	/////////////////////////////////////////////////////////////

    // create a renderbuffer object for depth and stencil attachment (we won't be sampling these)
    unsigned int rbo;
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, SCR_WIDTH, SCR_HEIGHT); // use a single renderbuffer object for both a depth AND stencil buffer.
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo); // now actually attach it
    // now that we actually created the framebuffer and added all attachments we want to check if it is actually complete now
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

	GLboolean bUseKernel = false;
	GLboolean bAntiAliasing = true;
	postProcShader.SetBool("bUseKernel", bUseKernel);
	postProcShader.SetBool("bAntiAliasing", bAntiAliasing);
		 
	if (bUseKernel)
	{
		glm::vec2 uvOffset(1.0f / SCR_WIDTH, 1.0f / SCR_HEIGHT);
		postProcShader.SetVec2("uvOffset", uvOffset);
	}

	if (bAntiAliasing)
	{		
		postProcShader.SetInt("samples", samples);
	}

	//set model for first time 
	//and update it then if necessary 
	postProcShader.SetMat4("model", postProcModel);

    // draw as wireframe
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);


	/////////////////////////////////////////////CUBEMAP/////////////////////////////////////////////

	

	vector<std::string> faces = 
	{
		"Cube Textures/skybox/right.jpg",
		"Cube Textures/skybox/left.jpg",
		"Cube Textures/skybox/top.jpg",
		"Cube Textures/skybox/bottom.jpg",
		"Cube Textures/skybox/front.jpg",
		"Cube Textures/skybox/back.jpg"

	};
	GLuint cubemapTexture = loadCubemap(faces);

	// skybox VAO
	GLuint skyboxVAO, skyboxVBO;
	GenSkyboxVAO(skyboxVAO, skyboxVBO);
	
	/////////////////////////////////////////////////REFLECT CUBEMAP//////////////////////////////////////////
	GLuint reflectVAO, reflectVBO;
	GenReflectVAO(reflectVAO, reflectVBO);


	/////////////////////////////////////MODEL////////////////////////////////////
	Model model("Models/backpack/backpack.obj", cubemapTexture, 1.f, false);
	modelShader.UseProgram();
	glm::mat4 mod = glm::mat4(1.0f);	
	mod = glm::translate(mod, glm::vec3(2.0f, 2.0f, -3.0f));
	mod = glm::rotate(mod, glm::radians( (GLfloat)glfwGetTime()), glm::vec3(1.0, 1.0, 1.0));
	modelShader.SetMat4("model", mod);

	modelShaderNormals.UseProgram();
	modelShaderNormals.SetMat4("model", mod);


	///////////////////////////////////INSTANCING//////////////////////////////////

	/*Shader skullShader = modelShader;
	
	Model skull("Models/Skull/Skull.obj", 0, 1.f, false);

	GLuint modelMatricesVBO = 0;

	GLuint amount = 51;
	glm::mat4 *modelMatrices = new glm::mat4[amount];
	FillModelMatrices(amount, modelMatrices);

	UpdateVAO(skull, amount, modelMatrices, modelMatricesVBO);*/



    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        // per-frame time logic
        // --------------------
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window);

		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		DrawScene(shader, skyboxShader, cubeVAO, planeVAO, skyboxVAO, uboBlock,
			cubeTexture, floorTexture, cubemapTexture);


		/////////////////////////////////////////////////FRAME BUFFER///////////////////////////////////////////////
        // render
        // ------
        // bind to framebuffer and draw scene as we normally would to color texture 
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);				

		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		DrawScene(shader, skyboxShader, cubeVAO, planeVAO, skyboxVAO, uboBlock,
			cubeTexture, floorTexture, cubemapTexture);

        // now bind back to default framebuffer and draw a quad plane with the attached framebuffer color texture
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        //glDisable(GL_DEPTH_TEST); // disable depth test so screen-space quad isn't discarded due to depth test.
        // clear all relevant buffers
        //glClearColor(1.0f, 1.0f, 1.0f, 1.0f); // set clear color to white (not really necessery actually, since we won't be able to see behind the quad anyways)
        //glClear(GL_COLOR_BUFFER_BIT);
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////
		
		DrawReflectCube(shader, reflectVAO, cubemapTexture, camera.Position);

		DrawRefractCube(shader, reflectVAO, cubemapTexture, cubeTexture, camera.Position);

		
		/*glm::mat4 mod = glm::mat4(1.0f);		
		mod = glm::translate(mod, glm::vec3(2.0f, 2.0f, -3.0f));
		mod = glm::rotate(mod, glm::radians((GLfloat)glfwGetTime()*10), glm::vec3(1.0, 1.0, 1.0));
		
		modelShader.UseProgram();
		modelShader.SetMat4("model", mod);

		modelShaderNormals.UseProgram();
		modelShaderNormals.SetMat4("model", mod);*/

		//Draw model
		modelShader.UseProgram();		
		modelShader.SetVec3("cameraPos", camera.Position);
		modelShader.SetFloat("time", glfwGetTime());
		modelShader.SetMat4("model", mod);
		modelShader.SetBool("bReflect", true);
		modelShader.SetBool("binvertUVs", true);
		model.Draw(modelShader);
		modelShader.SetBool("bReflect", false);
		modelShader.SetBool("binvertUVs", false);

		//Draw model normals
		modelShaderNormals.UseProgram();
		modelShaderNormals.SetBool("bShowNormals", true);
		modelShaderNormals.SetBool("binvertUVs", true);
		model.Draw(modelShaderNormals);
		modelShaderNormals.SetBool("bShowNormals", false);
		modelShaderNormals.SetBool("binvertUVs", false);


		//render of planet and asteroids
		/*skullShader.UseProgram();
		skullShader.SetBool("binvertUVs", false);
		skullShader.SetBool("bInstanced", true);
		skull.Draw(skullShader, true, amount);
		skullShader.SetBool("binvertUVs", true);
		skullShader.SetBool("bInstanced", false);*/


		DrawPostProc(postProcShader, postProcVAO, textureColorbuffer, textureColorbufferMS, bUseKernel, bAntiAliasing);


        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteVertexArrays(1, &planeVAO);
    glDeleteVertexArrays(1, &postProcVAO);
    glDeleteBuffers(1, &cubeVBO);
    glDeleteBuffers(1, &planeVBO);
    glDeleteBuffers(1, &postProcVBO);

	glDeleteVertexArrays(1, &skyboxVAO);
	glDeleteBuffers(1, &skyboxVBO);

	glDeleteVertexArrays(1, &reflectVAO);
	glDeleteBuffers(1, &reflectVBO);



	glDeleteFramebuffers(1, &framebuffer);

	//glDeleteBuffers(1, &modelMatricesVBO);
	//delete[] modelMatrices;

    glfwTerminate();
    return 0;
}

void DrawScene(Shader & shader, Shader & skyboxShader,
	GLuint cubeVAO, GLuint planeVAO, GLuint skyboxVAO, GLuint uboBlock,
	GLuint cubeTexture, GLuint floorTexture, GLuint cubemapTexture)
{
	


	shader.UseProgram();

	glm::mat4 model = glm::mat4(1.0f);
	glm::mat4 view = camera.GetViewMatrix();
	glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 1000.0f);
	
	////////////////////////////////////////////UNIFORM BUFFER VALUES///////////////////////////////////////////////
	glBindBuffer(GL_UNIFORM_BUFFER, uboBlock);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), &view[0][0]);
	glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat4), &projection[0][0]);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	

	shader.SetBool("bStencil", false);

	glEnable(GL_DEPTH_TEST); // enable depth testing (is disabled for rendering screen-space quad)

	//glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
	
	//glStencilMask(0x00);
	//// floor
	//DrawFloor(shader, planeVAO, floorTexture);
	////grass
	//

	//// 1st. render pass, draw objects as normal, writing to the stencil buffer
	//// --------------------------------------------------------------------
	//glStencilFunc(GL_ALWAYS, 1, 0xFF);
	//glStencilMask(0xFF);
	//// cubes
	//DrawCubes(shader, cubeVAO, cubeTexture);

	//// 2nd. render pass: now draw slightly scaled versions of the objects, this time disabling stencil writing.
	//// Because the stencil buffer is now filled with several 1s. The parts of the buffer that are 1 are not drawn,
	//// thus only drawing the objects' size differences, making it look like borders.
	//// -----------------------------------------------------------------------------------------------------------------------------
	//glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
	//glStencilMask(0x00);
	////glDisable(GL_DEPTH_TEST);

	//float scale = 1.03f;
	//
	//// cubes
	//DrawCubes(shader, cubeVAO, cubeTexture, glm::vec3(scale), true);

	//glStencilMask(0xFF);
	//glStencilFunc(GL_ALWAYS, 0, 0xFF);
	//glEnable(GL_DEPTH_TEST);

	////////////////////////Alternative way to draw cubes with borders//////////////////////////
	glStencilOp(
		GL_KEEP,		//stenc fail
		GL_KEEP,		//depth fail, stenc pass
		GL_REPLACE);	//both pass

	// 1st. render pass, draw objects as normal, writing to the stencil buffer
	// --------------------------------------------------------------------
	glStencilFunc(GL_ALWAYS, 1, 0xFF);//replace stencil val by 2
	glStencilMask(0xFF);
	// cubes
	DrawCubes(shader, cubeVAO, cubeTexture);

	// 2nd. render pass: now draw slightly scaled versions of the objects, this time disabling stencil writing.
	// Because the stencil buffer is now filled with several 1s. The parts of the buffer that are 1 are not drawn,
	// thus only drawing the objects' size differences, making it look like borders.
	// -----------------------------------------------------------------------------------------------------------------------------
	//Draw cubes borders but keep writing to stencil buffer 1s
	glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
	glStencilMask(0xFF);
	//glDisable(GL_DEPTH_TEST);

	float scale = 1.03f;
	glm::vec4 borderColor(1.0, 1.0, 0.0, 1.0);
	// cubes for borders
	DrawCubes(shader, cubeVAO, cubeTexture, glm::vec3(scale), true, borderColor);

	
	//Test drawing borders over borders drawn already
	/*scale = 1.06f;	
	borderColor = glm::vec4(1.0, 0.0, 0.0, 1.0);
	DrawCubes(shader, cubeVAO, cubeTexture, glm::vec3(scale), true, borderColor);*/
	 
	//actually, not necessary, because it's not influencing on result
	//and using just to stop writing to stencil buffer
	//But the stencil buffer is now filled with several 1s on cubes and borders
	//and GPU drawing only parts of the floor outside 1s
	glStencilMask(0x00);
	// floor
	DrawFloor(shader, planeVAO, floorTexture);

	glStencilMask(0xFF);
	glStencilFunc(GL_ALWAYS, 0, 0xFF);
	

	//render Skybox
	glDepthFunc(GL_LEQUAL);
	glDepthMask(GL_FALSE);
	glFrontFace(GL_CW);
	glm::mat4 skyboxView = glm::mat4(glm::mat3(camera.GetViewMatrix()));
	DrawSkybox(skyboxShader, skyboxVAO, cubemapTexture, skyboxView, projection);
	glDepthMask(GL_TRUE);
	glFrontFace(GL_CCW);
	glDepthFunc(GL_LESS);

}

void ScalePostProc()
{
	postProcModel = glm::mat4(1.0f);

	GLfloat newSize = postProcInitSize * postProcScale;
	GLfloat offset = postProcInitSize - newSize;

	postProcModel = glm::translate(postProcModel, glm::vec3(offset, offset, -0.0f));
	postProcModel = glm::scale(postProcModel, glm::vec3(postProcScale));
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);

	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)//increase
	{
		postProcScale += postProcScaleDelta;
		postProcScale = glm::clamp(postProcScale, 0.3f, 2.0f);
		ScalePostProc();
		bPPModelChanged = true;
	}
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)//decrease
	{
		postProcScale -= postProcScaleDelta;
		postProcScale = glm::clamp(postProcScale, 0.3f, 2.0f);
		ScalePostProc();
		bPPModelChanged = true;
	}
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(yoffset);


}

// utility function for loading a 2D texture from file
// ---------------------------------------------------
unsigned int loadTexture(char const * path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

void DrawCubes(Shader & shader, GLuint VAO, GLuint texture, glm::vec3 scale, bool bStencil, glm::vec4 borderColor)
{
	shader.UseProgram();

	shader.SetBool("bStencil", bStencil);
	shader.SetVec4("borderColor", borderColor);
	
	glBindVertexArray(VAO);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);
	shader.SetInt("texture1", 0);

	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(-1.0f, 0.0f, -1.0f));
	model = glm::scale(model, scale);
	shader.SetMat4("model", model);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(2.0f, 0.0f, 0.0f));
	model = glm::scale(model, scale);
	shader.SetMat4("model", model);
	glDrawArrays(GL_TRIANGLES, 0, 36);

	glBindVertexArray(0);

	shader.SetBool("bStencil", false);
}

void DrawFloor(Shader & shader, GLuint VAO, GLuint texture)
{
	shader.UseProgram();

	glBindVertexArray(VAO);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);
	shader.SetInt("texture1", 0);

	shader.SetMat4("model", glm::mat4(1.0f));
	glDrawArrays(GL_TRIANGLES, 0, 6);

	glBindVertexArray(0);
}

void SetKernelValue(Shader & postProcShader, GLint cellID)
{
	char num[2];
	_itoa_s(cellID, num, 10);

	postProcShader.SetFloat(std::string("kernel[") + num + string("]"), kernel[cellID]);
}

void SetCellValue(Shader & postProcShader, GLint cellID, GLfloat cellValue, GLint sectorID)
{
	kernel[cellID] = cellValue;
	//set opposite cell value
	GLint oppositeId = sectorsToCells[(sectorID + 4) % sectorsSize];
	kernel[oppositeId] = -cellValue;	

	SetKernelValue(postProcShader, cellID);
	SetKernelValue(postProcShader, oppositeId);
}

void DrawPostProc(Shader & postProcShader, GLuint postProcVAO,
	GLuint textureColorbuffer, GLuint textureColorbufferMS, bool bUseKernel, bool bAntiAliasing)
{
	postProcShader.UseProgram();

	glBindVertexArray(postProcVAO);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textureColorbuffer);	// use the color attachment texture as the texture of the quad plane
	
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, textureColorbufferMS);

	if (bPPModelChanged)
	{
		postProcShader.SetMat4("model", postProcModel);
		bPPModelChanged = false;
	}
	
	if (bUseKernel)
	{
		

		//Applying rotating sobel effect

		//Sobel kernel is 3x3 sized
		//and we are calculating values of it's cells (excluding central) 
		//depending on angle calculated further.
		//Each cell matches with defined sector

		

		GLfloat angle = (GLint)(glfwGetTime() * 100.f) % 360;
		
		//angle = 0.f;

		GLint sectorID = 0;
		//handle 0 sector because it's between 337.5 and 22.5 degrees in CCW order
		if (angle < kernelTreshold)
			sectorID = 0;
		else
			sectorID = ((GLint)((angle - kernelTreshold) / kernelAngle) + 1) % sectorsSize;
		GLint leftSectorID = (sectorsSize + sectorID + 1) % sectorsSize;
		GLint leftleftSectorID = (sectorsSize + sectorID + 2) % sectorsSize;
		GLint rightSectorID = (sectorsSize + sectorID - 1) % sectorsSize;

		GLfloat offset = 0.f;
		//handle 0 sector
		if(angle > 360.f - kernelTreshold)
			offset = angle - 360.f;
		else
			offset = angle - kernelAngles[sectorID];

		GLfloat delta = 0.5f * ((offset) / kernelTreshold);

		GLfloat midCellValue = 2.f - abs(delta);
		GLfloat leftCellValue = 1.f + delta;
		GLfloat leftleftCellValue = delta;
		GLfloat rightCellValue = 1.f - delta;
		
		for (int i = 0; i < kernelSize; i++)
		{
			if (i == 4)
				continue;

			if (i == sectorsToCells[sectorID])//mid cell value
			{
				SetCellValue(postProcShader, i, midCellValue, sectorID);
			}
			else if (i == sectorsToCells[leftSectorID])//left cell value
			{
				SetCellValue(postProcShader, i, leftCellValue, leftSectorID);
			}
			else if (i == sectorsToCells[leftleftSectorID])//left left cell value
			{
				SetCellValue(postProcShader, i, leftleftCellValue, leftleftSectorID);
			}
			else if (i == sectorsToCells[rightSectorID])//right cell value
			{
				SetCellValue(postProcShader, i, rightCellValue, rightSectorID);
			}	

			
		}

	}

	glDrawArrays(GL_TRIANGLES, 0, 6);

	glBindVertexArray(0);
}

GLuint loadCubemap(const vector<std::string>& faces)
{
	GLuint textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

	int width, height, nrChannels;
	for (unsigned int i = 0; i < faces.size(); i++)
	{
		unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
		if (data)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
				0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
			);
			stbi_image_free(data);
		}
		else
		{
			std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
			stbi_image_free(data);
		}
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	return textureID;
}

void DrawSkybox(Shader & shader, GLuint VAO, GLuint texture, const glm::mat4& view, const glm::mat4& projection)
{	
	shader.UseProgram();
	// ... ������� ������� � ������������ ������

	shader.SetMat4("view", view);
	shader.SetMat4("projection", projection);

	glBindVertexArray(VAO);

	glBindTexture(GL_TEXTURE_CUBE_MAP, texture);
	glDrawArrays(GL_TRIANGLES, 0, 36);

	glBindVertexArray(0);
	
}

void DrawReflectCube(Shader & shader, GLuint VAO, GLuint texture, glm::vec3 cameraPos)
{
	shader.UseProgram();

	shader.SetBool("bReflect", true);
	/*shader.SetBool("bReflectF", true);*/

	shader.SetVec3("cameraPos", cameraPos);

	glBindVertexArray(VAO);

	glEnable(GL_TEXTURE_CUBE_MAP);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_CUBE_MAP, texture);
	shader.SetInt("skybox", 1);
	
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(2.0f, 2.0f, 0.0f));	
	shader.SetMat4("model", model);
	glDrawArrays(GL_TRIANGLES, 0, 36);

	glBindVertexArray(0);

	shader.SetBool("bReflect", false);
	/*shader.SetBool("bReflectF", false);*/
	glEnable(GL_TEXTURE_2D);
}

void DrawRefractCube(Shader & shader, GLuint VAO, GLuint texture, GLuint testTex, glm::vec3 cameraPos)
{
	shader.UseProgram();

	shader.SetBool("bRefract", true);
	/*shader.SetBool("bReflectF", true);*/

	shader.SetVec3("cameraPos", cameraPos);

	glBindVertexArray(VAO);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, testTex);
	shader.SetInt("texture1", 0);

	//glEnable(GL_TEXTURE_CUBE_MAP);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_CUBE_MAP, texture);
	shader.SetInt("skybox", 1);

	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, 2.0f, 0.0f));
	shader.SetMat4("model", model);
	glDrawArrays(GL_TRIANGLES, 0, 36);

	glBindVertexArray(0);

	shader.SetBool("bRefract", false);
	/*shader.SetBool("bReflectF", false);*/
	glEnable(GL_TEXTURE_2D);
}

void GenCubeVAO(GLuint& cubeVAO, GLuint& cubeVBO)
{
	// set up vertex data (and buffer(s)) and configure vertex attributes
	// ------------------------------------------------------------------
	float cubeVertices[] = {
		// positions          // texture Coords
		-0.5f, -0.5f, -0.5f,  0.0f, 0.0f,//back
		 0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
		 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
		 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
		-0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

		-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,//front         
		 0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
		 0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
		 0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
		-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
		-0.5f,  0.5f,  0.5f,  0.0f, 1.0f,

		-0.5f,  0.5f,  0.5f,  1.0f, 0.0f,//left        
		-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
		-0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
		-0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
		-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

		 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,//right
		 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
		 0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
		 0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
		 0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
		 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

		-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,//bot         
		 0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
		 0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
		 0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
		-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

		-0.5f,  0.5f, -0.5f,  0.0f, 1.0f,//top
		 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
		 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
		 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
		-0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
		-0.5f,  0.5f, -0.5f,  0.0f, 1.0f
	};
	glGenBuffers(1, &cubeVBO);
	glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), &cubeVertices, GL_STATIC_DRAW);

	glGenVertexArrays(1, &cubeVAO);
	
	glBindVertexArray(cubeVAO);
	
	
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	glBindVertexArray(0);
}

void GenPlaneVAO(GLuint& planeVAO, GLuint& planeVBO)
{
	float planeVertices[] = {
		// positions          // texture Coords 
		 5.0f, -0.5f,  5.0f,  2.0f, 0.0f,
		-5.0f, -0.5f,  5.0f,  0.0f, 0.0f,
		-5.0f, -0.5f, -5.0f,  0.0f, 2.0f,

		 5.0f, -0.5f,  5.0f,  2.0f, 0.0f,
		-5.0f, -0.5f, -5.0f,  0.0f, 2.0f,
		 5.0f, -0.5f, -5.0f,  2.0f, 2.0f
	};

	glGenVertexArrays(1, &planeVAO);
	glGenBuffers(1, &planeVBO);
	glBindVertexArray(planeVAO);
	glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), &planeVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	glBindVertexArray(0);
}

void GenPostProcVAO(GLuint& postProcVAO, GLuint& postProcVBO)
{
	//have to be written in CW order
	float postProcVertices[] = { // vertex attributes for a quad that fills the entire screen in Normalized Device Coordinates.
		// positions   // texCoords
		-0.0f, -0.0f,  0.0f, 0.0f,
		-0.0f,  1.0f,  0.0f, 1.0f,
		 1.0f, -0.0f,  1.0f, 0.0f,

		 1.0f, -0.0f,  1.0f, 0.0f,
		-0.0f,  1.0f,  0.0f, 1.0f,
		 1.0f,  1.0f,  1.0f, 1.0f
	};

	glGenVertexArrays(1, &postProcVAO);
	glGenBuffers(1, &postProcVBO);
	glBindVertexArray(postProcVAO);
	glBindBuffer(GL_ARRAY_BUFFER, postProcVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(postProcVertices), &postProcVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
	glBindVertexArray(0);
}

void GenSkyboxVAO(GLuint& skyboxVAO, GLuint& skyboxVBO)
{
	float skyboxVertices[] = {
		// positions          
		-1.0f,  1.0f, -1.0f,
		-1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,
		 1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,

		-1.0f, -1.0f,  1.0f,
		-1.0f, -1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f,  1.0f,
		-1.0f, -1.0f,  1.0f,

		 1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,

		-1.0f, -1.0f,  1.0f,
		-1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f, -1.0f,  1.0f,
		-1.0f, -1.0f,  1.0f,

		-1.0f,  1.0f, -1.0f,
		 1.0f,  1.0f, -1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		-1.0f,  1.0f,  1.0f,
		-1.0f,  1.0f, -1.0f,

		-1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f,  1.0f,
		 1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f,  1.0f,
		 1.0f, -1.0f,  1.0f
	};

	glGenVertexArrays(1, &skyboxVAO);
	glGenBuffers(1, &skyboxVBO);
	glBindVertexArray(skyboxVAO);
	glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glBindVertexArray(0);
}

void GenReflectVAO(GLuint& reflectVAO, GLuint& reflectVBO)
{
	float reflectVertices[] = {
		// positions			// texture Coords	 //normals
		-0.5f, -0.5f, -0.5f,	0.0f, 0.0f,			 0.0f,  0.0f, -1.0f,//back
		 0.5f, -0.5f, -0.5f,	1.0f, 0.0f,			 0.0f,  0.0f, -1.0f,
		 0.5f,  0.5f, -0.5f,	1.0f, 1.0f,			 0.0f,  0.0f, -1.0f,
		 0.5f,  0.5f, -0.5f,	1.0f, 1.0f,			 0.0f,  0.0f, -1.0f,
		-0.5f,  0.5f, -0.5f,	0.0f, 1.0f,			 0.0f,  0.0f, -1.0f,
		-0.5f, -0.5f, -0.5f,	0.0f, 0.0f,			 0.0f,  0.0f, -1.0f,

		-0.5f, -0.5f,  0.5f,	0.0f, 0.0f,			 0.0f,  0.0f,  1.0f,//front         
		 0.5f,  0.5f,  0.5f,	1.0f, 1.0f,			 0.0f,  0.0f,  1.0f,
		 0.5f, -0.5f,  0.5f,	1.0f, 0.0f,			 0.0f,  0.0f,  1.0f,
		 0.5f,  0.5f,  0.5f,	1.0f, 1.0f,			 0.0f,  0.0f,  1.0f,
		-0.5f, -0.5f,  0.5f,	0.0f, 0.0f,			 0.0f,  0.0f,  1.0f,
		-0.5f,  0.5f,  0.5f,	0.0f, 1.0f,			 0.0f,  0.0f,  1.0f,

		-0.5f,  0.5f,  0.5f,	1.0f, 0.0f,			-1.0f,  0.0f,  0.0f,//left        
		-0.5f, -0.5f, -0.5f,	0.0f, 1.0f,			-1.0f,  0.0f,  0.0f,
		-0.5f,  0.5f, -0.5f,	1.0f, 1.0f,			-1.0f,  0.0f,  0.0f,
		-0.5f, -0.5f, -0.5f,	0.0f, 1.0f,			-1.0f,  0.0f,  0.0f,
		-0.5f,  0.5f,  0.5f,	1.0f, 0.0f,			-1.0f,  0.0f,  0.0f,
		-0.5f, -0.5f,  0.5f,	0.0f, 0.0f,			-1.0f,  0.0f,  0.0f,

		 0.5f,  0.5f,  0.5f,	1.0f, 0.0f,			 1.0f,  0.0f,  0.0f,//right
		 0.5f,  0.5f, -0.5f,	1.0f, 1.0f,			 1.0f,  0.0f,  0.0f,
		 0.5f, -0.5f, -0.5f,	0.0f, 1.0f,			 1.0f,  0.0f,  0.0f,
		 0.5f, -0.5f, -0.5f,	0.0f, 1.0f,			 1.0f,  0.0f,  0.0f,
		 0.5f, -0.5f,  0.5f,	0.0f, 0.0f,			 1.0f,  0.0f,  0.0f,
		 0.5f,  0.5f,  0.5f,	1.0f, 0.0f,			 1.0f,  0.0f,  0.0f,

		-0.5f, -0.5f, -0.5f,	0.0f, 1.0f,			 0.0f, -1.0f,  0.0f,//bot         
		 0.5f, -0.5f,  0.5f,	1.0f, 0.0f,			 0.0f, -1.0f,  0.0f,
		 0.5f, -0.5f, -0.5f,	1.0f, 1.0f,			 0.0f, -1.0f,  0.0f,
		 0.5f, -0.5f,  0.5f,	1.0f, 0.0f,			 0.0f, -1.0f,  0.0f,
		-0.5f, -0.5f, -0.5f,	0.0f, 1.0f,			 0.0f, -1.0f,  0.0f,
		-0.5f, -0.5f,  0.5f,	0.0f, 0.0f,			 0.0f, -1.0f,  0.0f,

		-0.5f,  0.5f, -0.5f,	0.0f, 1.0f,			 0.0f,  1.0f,  0.0f,//top
		 0.5f,  0.5f, -0.5f,	1.0f, 1.0f,			 0.0f,  1.0f,  0.0f,
		 0.5f,  0.5f,  0.5f,	1.0f, 0.0f,			 0.0f,  1.0f,  0.0f,
		 0.5f,  0.5f,  0.5f,	1.0f, 0.0f,			 0.0f,  1.0f,  0.0f,
		-0.5f,  0.5f,  0.5f,	0.0f, 0.0f,			 0.0f,  1.0f,  0.0f,
		-0.5f,  0.5f, -0.5f,	0.0f, 1.0f,			 0.0f,  1.0f,  0.0f
	};

	glGenVertexArrays(1, &reflectVAO);
	glGenBuffers(1, &reflectVBO);
	glBindVertexArray(reflectVAO);
	glBindBuffer(GL_ARRAY_BUFFER, reflectVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(reflectVertices), &reflectVertices, GL_STATIC_DRAW);
	//pos
	glEnableVertexAttribArray(0);	
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	//UV
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	//normal
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));
	glBindVertexArray(0);
}


void UpdateVAO(Model & model, GLuint amount, glm::mat4 *modelMatrices, GLuint& modelMatricesVBO)
{
	// ������� VBO	
	glGenBuffers(1, &modelMatricesVBO);
	glBindBuffer(GL_ARRAY_BUFFER, modelMatricesVBO);
	glBufferData(GL_ARRAY_BUFFER, amount * sizeof(glm::mat4), &modelMatrices[0], GL_STATIC_DRAW);

	for (unsigned int i = 0; i < model.meshes.size(); i++)
	{
		unsigned int VAO = model.meshes[i].VAO;
		glBindVertexArray(VAO);
		// ��������� ���������
		GLsizei vec4Size = sizeof(glm::vec4);
		glEnableVertexAttribArray(6);
		glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)0);
		glEnableVertexAttribArray(7);
		glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(vec4Size));
		glEnableVertexAttribArray(8);
		glVertexAttribPointer(8, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(2 * vec4Size));
		glEnableVertexAttribArray(9);
		glVertexAttribPointer(9, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(3 * vec4Size));

		glVertexAttribDivisor(6, 1);
		glVertexAttribDivisor(7, 1);
		glVertexAttribDivisor(8, 1);
		glVertexAttribDivisor(9, 1);

		glBindVertexArray(0);
	}
}

//Planet and asteroids have same model
//modelMatrices[0] is planet model
void FillModelMatrices(GLuint amount, glm::mat4 *modelMatrices)
{
	glm::mat4 modelP(1.0f);
	modelP = glm::translate(modelP, glm::vec3(0.0f, -3.0f, 0.0f));
	modelP = glm::rotate(modelP, glm::radians(-90.f), glm::vec3(1.0f, 0.0f, 0.0f));
	modelP = glm::scale(modelP, glm::vec3(1.0f, 1.0f, 1.0f));

	modelMatrices[0] = modelP;

	srand(glfwGetTime()); // ������ seed ��� ���������� ����. �����
	float radius = 50.0;
	float offset = 2.5f;
	for (unsigned int i = 1; i < amount; i++)
	{
		glm::mat4 model(1.0f);
		// 1. �������: ����������� ����� ���������� �������� 'radius' 
		// � �������� �������� � �������� [-offset, offset]
		float angle = (float)i / (float)amount * 360.0f;
		float displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
		float x = sin(angle) * radius + displacement;
		displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
		// ������ ���� ������ ������� �������, ��� ������� � ��������� XZ
		float y = displacement * 0.4f;
		displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
		float z = cos(angle) * radius + displacement;
		model = glm::translate(model, glm::vec3(x, y, z));

		// 2. ���������������: ��������� ��������������� � �������� (0.05, 0.25f)
		float scale = (rand() % 20) / 100.0f + 0.05;
		model = glm::scale(model, glm::vec3(scale));

		// 3. �������: ������� �� ��������� ���� ����� 
		float rotAngle = (rand() % 360);
		model = glm::rotate(model, rotAngle, glm::vec3(0.4f, 0.6f, 0.8f));

		// 4. ��������� � ������ ������
		modelMatrices[i] = model;
	}
}