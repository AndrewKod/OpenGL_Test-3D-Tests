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
GLfloat postProcScaleDelta = 0.0001f;

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

GLboolean freeCells[kernelSize];

void DrawCubes(Shader & shader, GLuint VAO, GLuint texture, glm::vec3 scale = glm::vec3(1.0f), bool bStencil = false);
void DrawFloor(Shader & shader, GLuint VAO, GLuint texture);

void DrawTransparent(Shader & shader, GLuint VAO, GLuint texture, const std::vector<glm::vec3>& positions, bool bSortPositions);

void DrawScene(Shader & shader, GLuint cubeVAO, GLuint planeVAO, GLuint cubeTexture, GLuint floorTexture);

void DrawPostProc(Shader & shader, GLuint VAO, GLuint texture, bool bUseKernel = false);
void ResetFreeCells();
void SetKernelValue(Shader & postProcShader, GLint cellID);

void ScalePostProc();

int main()
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

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


    // build and compile shaders
    // -------------------------
    Shader shader("Shaders/Vertex Shader.glsl", "Shaders/Fragment Shader.glsl");
    Shader postProcShader("Shaders/Vertex Shader PP.glsl", "Shaders/Fragment Shader PP.glsl");

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
    float planeVertices[] = {
        // positions          // texture Coords 
         5.0f, -0.5f,  5.0f,  2.0f, 0.0f,
        -5.0f, -0.5f,  5.0f,  0.0f, 0.0f,
        -5.0f, -0.5f, -5.0f,  0.0f, 2.0f,

         5.0f, -0.5f,  5.0f,  2.0f, 0.0f,
        -5.0f, -0.5f, -5.0f,  0.0f, 2.0f,
         5.0f, -0.5f, -5.0f,  2.0f, 2.0f
    };

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
    // cube VAO
    unsigned int cubeVAO, cubeVBO;
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);
    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), &cubeVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    // plane VAO
    unsigned int planeVAO, planeVBO;
    glGenVertexArrays(1, &planeVAO);
    glGenBuffers(1, &planeVBO);
    glBindVertexArray(planeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), &planeVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    // screen quad VAO
    unsigned int postProcVAO, postProcVBO;
    glGenVertexArrays(1, &postProcVAO);
    glGenBuffers(1, &postProcVBO);
    glBindVertexArray(postProcVAO);
    glBindBuffer(GL_ARRAY_BUFFER, postProcVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(postProcVertices), &postProcVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

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

    // framebuffer configuration
    // -------------------------
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
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureColorbuffer, 0);
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

	GLboolean bUseKernel = true;
	postProcShader.SetBool("bUseKernel", bUseKernel);
		
	glm::vec2 uvOffset(1.0f / SCR_WIDTH, 1.0f / SCR_HEIGHT);
	postProcShader.SetVec2("uvOffset", uvOffset);

	//set model for first time 
	//and update it then if necessary 
	postProcShader.SetMat4("model", postProcModel);

    // draw as wireframe
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

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
		DrawScene(shader, cubeVAO, planeVAO, cubeTexture, floorTexture);

        // render
        // ------
        // bind to framebuffer and draw scene as we normally would to color texture 
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);				

		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		DrawScene(shader, cubeVAO, planeVAO, cubeTexture, floorTexture);

        // now bind back to default framebuffer and draw a quad plane with the attached framebuffer color texture
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDisable(GL_DEPTH_TEST); // disable depth test so screen-space quad isn't discarded due to depth test.
        // clear all relevant buffers
        //glClearColor(1.0f, 1.0f, 1.0f, 1.0f); // set clear color to white (not really necessery actually, since we won't be able to see behind the quad anyways)
        //glClear(GL_COLOR_BUFFER_BIT);

		

		DrawPostProc(postProcShader, postProcVAO, textureColorbuffer, bUseKernel);


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

	glDeleteFramebuffers(1, &framebuffer);

    glfwTerminate();
    return 0;
}

void DrawScene(Shader & shader, GLuint cubeVAO, GLuint planeVAO, GLuint cubeTexture, GLuint floorTexture)
{
	glEnable(GL_DEPTH_TEST); // enable depth testing (is disabled for rendering screen-space quad)

	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);


	shader.UseProgram();

	glm::mat4 model = glm::mat4(1.0f);
	glm::mat4 view = camera.GetViewMatrix();
	glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
	shader.SetMat4("view", view);
	shader.SetMat4("projection", projection);

	shader.SetBool("bStencil", false);

	glStencilMask(0x00);
	// floor
	DrawFloor(shader, planeVAO, floorTexture);
	//grass


	// 1st. render pass, draw objects as normal, writing to the stencil buffer
	// --------------------------------------------------------------------
	glStencilFunc(GL_ALWAYS, 1, 0xFF);
	glStencilMask(0xFF);
	// cubes
	DrawCubes(shader, cubeVAO, cubeTexture);

	// 2nd. render pass: now draw slightly scaled versions of the objects, this time disabling stencil writing.
	// Because the stencil buffer is now filled with several 1s. The parts of the buffer that are 1 are not drawn,
	// thus only drawing the objects' size differences, making it look like borders.
	// -----------------------------------------------------------------------------------------------------------------------------
	glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
	glStencilMask(0x00);
	glDisable(GL_DEPTH_TEST);

	float scale = 1.03f;

	// cubes
	DrawCubes(shader, cubeVAO, cubeTexture, glm::vec3(scale), true);

	glStencilMask(0xFF);
	glStencilFunc(GL_ALWAYS, 0, 0xFF);
	glEnable(GL_DEPTH_TEST);

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

void DrawCubes(Shader & shader, GLuint VAO, GLuint texture, glm::vec3 scale, bool bStencil)
{
	
	shader.SetBool("bStencil", bStencil ? true : false);
	
	glBindVertexArray(VAO);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);
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
}

void DrawFloor(Shader & shader, GLuint VAO, GLuint texture)
{
	glBindVertexArray(VAO);

	glBindTexture(GL_TEXTURE_2D, texture);
	shader.SetMat4("model", glm::mat4(1.0f));
	glDrawArrays(GL_TRIANGLES, 0, 6);

	glBindVertexArray(0);
}

void ResetFreeCells()
{
	for (int i = 0; i < kernelSize; i++)
	{
		freeCells[i] = true;
	}
}

void SetKernelValue(Shader & postProcShader, GLint cellID)
{
	char num[2];
	_itoa_s(cellID, num, 10);

	postProcShader.SetFloat(std::string("kernel[") + num + string("]"), kernel[cellID]);
}

void DrawPostProc(Shader & postProcShader, GLuint postProcVAO, GLuint textureColorbuffer, bool bUseKernel)
{
	postProcShader.UseProgram();

	glBindVertexArray(postProcVAO);

	glBindTexture(GL_TEXTURE_2D, textureColorbuffer);	// use the color attachment texture as the texture of the quad plane
	
	if (bPPModelChanged)
	{
		postProcShader.SetMat4("model", postProcModel);
		bPPModelChanged = false;
	}
	
	if (bUseKernel)
	{
		ResetFreeCells();

		//Applying rotating sobel effect

		//Sobel kernel is 3x3 sized
		//and we are calculating values of it's cells (excluding central) 
		//depending on angle calculated further.
		//Each cell matches with defined sector

		

		GLfloat angle = (GLint)(glfwGetTime() * 10.f) % 360;
		
		angle = 0.f;

		GLint sectorID = 0;
		//handle 0 sector because it's between 337.5 and 22.5 degrees in CCW order
		if (angle < kernelTreshold)
			sectorID = 0;
		else
			sectorID = ((GLint)((angle - kernelTreshold) / kernelAngle) + 1) % sectorsSize;

		GLfloat offset = 0.f;
		//handle 0 sector
		if(angle > 360.f - kernelTreshold)
			offset = angle - 360.f;
		else
			offset = angle - kernelAngles[sectorID];

		GLfloat delta = 0.5f *(abs(offset) / kernelTreshold);

		GLfloat midCellValue = 2.f - delta;
		GLfloat leftCellValue = 1.f + delta;
		GLfloat leftleftCellValue = delta;
		GLfloat rightCellValue = 1.f - delta;
		
		for (int i = 0; i < kernelSize; i++)
		{
			if (i == 4)
				continue;

			if (i == sectorsToCells[sectorID])//mid cell value
			{
				kernel[i] = midCellValue;
				//set opposite cell value
				GLint oppositeId = sectorsToCells[(sectorID + 4) % sectorsSize];
				kernel[oppositeId] = -midCellValue;
				
				freeCells[i] = false;
				freeCells[oppositeId] = false;

				SetKernelValue(postProcShader, i);
				SetKernelValue(postProcShader, oppositeId);
			}
			else if (i == sectorsToCells[(sectorID + 1) % sectorsSize])//left cell value
			{

			}
			else if (i == sectorsToCells[(sectorID + 2) % sectorsSize])//left left cell value
			{

			}
			else if (i == sectorsToCells[(sectorID - 1) % sectorsSize])//right cell value
			{

			}
			
			/*else if (freeCells[i])
			{
				kernel[i] = 0.f;
				SetKernelValue(postProcShader, i);
			}*/

			char num[2];
			_itoa_s(i, num, 10);

			postProcShader.SetFloat(std::string("kernel[") + num + string("]"), kernel[i]);
		}

	}

	glDrawArrays(GL_TRIANGLES, 0, 6);

	glBindVertexArray(0);
}