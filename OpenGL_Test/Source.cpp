#include <glad.c>
#include <glad.h>
#include <glfw3.h>
#include <stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/rotate_vector.hpp>

#include "Shader.h"
#include "Camera.h"
#include "Model.h"

#include <iostream>
#include <random>








void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void processInput(GLFWwindow *window);
unsigned int loadTexture(const char *path, bool bSRGB = false);

// settings
unsigned int SCR_WIDTH = 800;
unsigned int SCR_HEIGHT = 600;

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

bool bLoadSRGB = false;

GLuint settingsUBO = 0;
	
/////////////////////////////SETTINGS////////////////////////////////
//Gamma-correction				G
//Instancing					I
//Show Normals					N
//Post Process Sobel Effect		P
//Shadows						O
//Anti-Aliasing					U
//
/////////////////////////////////////////////////////////////////////

struct Settings
{
	GLint bGammaCorrection =		false;//F
	
	GLint bExplode =				false;//M
	GLint bPostProcess =			false;//P	

	GLint bAntiAliasing =			false;//U
	GLint bBlit =					false;

	GLint bPointLights =			false;//1
	GLint bDirectionalLight =		false;//2
	GLint bSpotLight =				false;//3

	GLint bShadows =				false;//O
	GLint bUseNormalMap =			false;//M
	GLint bUseParallaxMapping =		false;//B


	GLint bHDR =					false;//H excluded from settingsUBO
	GLint bBloom =					false;//G excluded from settingsUBO

	GLint bShowDirLightDepthMap =	false;//L Draw depth map on post process rectangle

	GLint bInstancing =				false;//I excluded from settingsUBO
	GLint bShowNormals =			false;//N excluded from settingsUBO

	GLint bDeferredRendering =		false;//T excluded from settingsUBO
	

	void UpdateSettings()
	{
		glBindBuffer(GL_UNIFORM_BUFFER, settingsUBO);
		glBufferSubData(GL_UNIFORM_BUFFER, offsetof(Settings, bGammaCorrection),		sizeof(GLint), &bGammaCorrection);
		//glBufferSubData(GL_UNIFORM_BUFFER, offsetof(Settings, bInstancing),			sizeof(GLint), &bInstancing);
		//glBufferSubData(GL_UNIFORM_BUFFER, offsetof(Settings, bShowNormals),			sizeof(GLint), &bShowNormals);
		glBufferSubData(GL_UNIFORM_BUFFER, offsetof(Settings, bPostProcess),			sizeof(GLint), &bPostProcess);
		glBufferSubData(GL_UNIFORM_BUFFER, offsetof(Settings, bAntiAliasing),			sizeof(GLint), &bAntiAliasing);
		glBufferSubData(GL_UNIFORM_BUFFER, offsetof(Settings, bBlit),					sizeof(GLint), &bBlit);
		glBufferSubData(GL_UNIFORM_BUFFER, offsetof(Settings, bPointLights),			sizeof(GLint), &bPointLights);
		glBufferSubData(GL_UNIFORM_BUFFER, offsetof(Settings, bDirectionalLight),		sizeof(GLint), &bDirectionalLight);
		glBufferSubData(GL_UNIFORM_BUFFER, offsetof(Settings, bSpotLight),				sizeof(GLint), &bSpotLight);
		glBufferSubData(GL_UNIFORM_BUFFER, offsetof(Settings, bShadows),				sizeof(GLint), &bShadows);
		glBufferSubData(GL_UNIFORM_BUFFER, offsetof(Settings, bUseNormalMap),			sizeof(GLint), &bUseNormalMap);
		glBufferSubData(GL_UNIFORM_BUFFER, offsetof(Settings, bUseParallaxMapping),		sizeof(GLint), &bUseParallaxMapping);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
	}
}
settings;

//Here we are using vec4 for easier calculations for uniform buffer

struct DirectionalLight {
	glm::vec4 direction;

	glm::vec4 ambient;
	glm::vec4 diffuse;
	glm::vec4 specular;	
};

struct SpotLight {
	glm::vec4  position;
	glm::vec4  direction;	

	glm::vec4 ambient;
	glm::vec4 diffuse;
	glm::vec4 specular;
	
	float cutOff;
	float outerCutOff;
};

struct PointLight 
{
	glm::vec4 position;

	glm::vec4 ambient;
	glm::vec4 diffuse;
	glm::vec4 specular;

	//light fading
	GLfloat constant;
	GLfloat linear;
	GLfloat quadratic;
	
	GLint bEnabled;

	PointLight(){}

	PointLight(glm::vec4 position, glm::vec4 ambient, glm::vec4 diffuse, glm::vec4 specular,
		GLint bEnabled = true, float constant = 1.0f, float linear = 0.007f, float quadratic = 0.0002f)
	{
		this->position = position;
		
		this->ambient = ambient;
		this->diffuse = diffuse;
		this->specular = specular;

		//light fading
		this->constant = constant;
		this->linear = linear;
		this->quadratic = quadratic;

		this->bEnabled = bEnabled;
	}
};

GLsizeiptr CalcStructSizeUBO(GLsizeiptr structSize);

#define NUM_POINT_LIGHTS 16

GLuint lightsUBO = 0;
GLuint pointLightPositionsUBO = 0;
GLuint dirLightDirectionUBO = 0;
GLuint spotLightDirectionUBO = 0;
GLuint pointLightRadiusUBO = 0;
glm::vec4 pointLightRadiuses[NUM_POINT_LIGHTS];
struct Lights
{
	DirectionalLight directionalLight;
	SpotLight spotLight;
	PointLight pointLights[NUM_POINT_LIGHTS];

	//Calculating lights Size
	//everu struct is multiple of vec4
	GLsizeiptr dirLightSize = CalcStructSizeUBO(sizeof(DirectionalLight));
	GLsizeiptr spotLightSize = CalcStructSizeUBO(sizeof(SpotLight));
	GLsizeiptr pointLightSize = CalcStructSizeUBO(sizeof(PointLight));
	GLsizeiptr pointLightsSize = pointLightSize * NUM_POINT_LIGHTS;

	//every array member is multiple of vec4
	GLsizeiptr lightsSize = dirLightSize + spotLightSize + pointLightsSize;

	void UpdateLights()
	{
		GLint dirLightOffset = 0;
		GLint spotLightOffset = dirLightSize;
		GLint pointLightOffset = dirLightSize + spotLightSize;
				
		GLint maxSize = GL_MAX_UNIFORM_BLOCK_SIZE;

		//update lights UBOs
		glBindBuffer(GL_UNIFORM_BUFFER, lightsUBO);		
		
		glBufferSubData(GL_UNIFORM_BUFFER, dirLightOffset,		dirLightSize,		&directionalLight);
		glBufferSubData(GL_UNIFORM_BUFFER, spotLightOffset,		spotLightSize,		&spotLight);
		glBufferSubData(GL_UNIFORM_BUFFER, pointLightOffset,	pointLightsSize,	&pointLights[0]);


		glBindBuffer(GL_UNIFORM_BUFFER, pointLightPositionsUBO);

		for (GLint i = 0; i < NUM_POINT_LIGHTS; i++)
		{
			glBufferSubData(GL_UNIFORM_BUFFER, 
				i* sizeof(glm::vec4), sizeof(glm::vec4), &pointLights[i].position);			
		}
		


		glBindBuffer(GL_UNIFORM_BUFFER, dirLightDirectionUBO);

		glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::vec4), &directionalLight.direction);


		glBindBuffer(GL_UNIFORM_BUFFER, spotLightDirectionUBO);

		glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::vec4), &spotLight.direction);
		
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
	}
}
lights;

void UpdatePointLightRadiusUBO()
{
	glBindBuffer(GL_UNIFORM_BUFFER, pointLightRadiusUBO);

	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(pointLightRadiuses), &pointLightRadiuses[0]);

	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}


void UpdateMatrices(const GLuint& uboBlock);

void GenCubeVAO(GLuint& cubeVAO, GLuint& cubeVBO, GLuint& tanVBO, GLuint& bitanVBO);
void GenPlaneVAO(GLuint& planeVAO, GLuint& planeVBO, GLuint& tanVBO, GLuint& bitanVBO);
void GenTangentsAndBitangents(float vertices[],
	int verticesSize, int verticesStep, int posOffset, int uvOffset, int vertsPerFace,
	glm::vec3 tangents[], glm::vec3 biangents[]);
void AddTangentsAndBitangentsAttributes(GLuint& VAO, GLuint& tanVBO, GLuint& bitanVBO,
	glm::vec3 tangents[], glm::vec3 bitangents[], GLsizei arrSize);
void GenPostProcVAO(GLuint& postProcVAO, GLuint& postProcVBO);
void GenSkyboxVAO(GLuint& skyboxVAO, GLuint& skyboxVBO);

void FillModelMatrices(std::vector<glm::mat4>& cubeModelMatrices);


void DrawCubes(Shader & shader, GLuint VAO,
	const std::vector<glm::mat4>& cubeModelMatrices, const std::vector<GLuint>* pointLightDepthCubemaps = nullptr,
	GLuint diffTexture = 0, GLuint specTexture = 0,
	GLuint wallTexture = 0, GLuint wallNormalMap = 0, GLuint wallHeightMap = 0,
	GLuint dirLightDepthMapTex = 0,
	glm::vec3 scale = glm::vec3(1.0f), bool bStencil = false, glm::vec4 borderColor = glm::vec4(0.f)
	);
void DrawFloor(Shader & shader, GLuint VAO,
	GLuint diffTexture = 0, GLuint specTexture = 0,
	GLuint wallTexture = 0, GLuint wallNormalMap = 0, GLuint wallHeightMap = 0,
	GLuint dirLightDepthMapTex = 0,
	const std::vector<GLuint>* pointLightDepthCubemaps = nullptr);

void DrawTransparent(Shader & shader, GLuint VAO, GLuint texture, const std::vector<glm::vec3>& positions, bool bSortPositions);

void DrawScene(Shader & shader, Shader & skyboxShader,
	GLuint cubeVAO, const std::vector<glm::mat4>& cubeModelMatrices,
	GLuint planeVAO, GLuint skyboxVAO, GLuint uboBlock,
	GLuint cubeDiffTexture, GLuint cubeSpecTexture,
	GLuint floorTexture, GLuint cubemapTexture,
	GLuint wallTexture, GLuint wallNormalMap, GLuint wallHeightMap,
	GLuint dirLightDepthMapTex, const std::vector<GLuint>* pointLightDepthCubemaps);

void DrawPostProc(Shader & shader, GLuint VAO,
	GLuint textureColorbuffer, GLuint textureColorbufferMS, GLuint screenBlitTexture);
void SetKernelValue(Shader & postProcShader, GLint cellID);
void SetCellValue(Shader & postProcShader, GLint cellID, GLfloat cellValue, GLint sectorID);
void SetupFrameBuffer(GLuint& framebuffer, GLuint& colorBuffer, GLuint& renderBuffer);
void SetupFrameBufferMS(GLuint& framebuffer, GLuint& colorBuffer, GLuint& renderBuffer, GLint samples,
	GLuint& intermediateFBO, GLuint& screenTexture);

void ScalePostProc();

GLuint loadCubemap(const vector<std::string>& faces, bool bSRGB = false);
void DrawSkybox(Shader & shader, GLuint VAO, GLuint texture, const glm::mat4& view, const glm::mat4& projection);


void DrawReflectCube(Shader & shader, GLuint VAO,
	GLuint cubeMapTexture, GLuint cubeDiffTexture, GLuint cubeSpecTexture, glm::vec3 cameraPos);
void DrawRefractCube(Shader & shader, GLuint VAO, GLuint cubeMapTexture, glm::vec3 cameraPos);


void FillModelMatrices(GLuint amount, glm::mat4 *modelMatrices);
void UpdateVAO(Model & model, GLuint amount, glm::mat4 *modelMatrices, GLuint& modelMatricesVBO);

void AddDirectedLight();
void AddPointLights(std::vector<PointLight>& pointLights, std::vector<glm::mat4>& pointLightsModelMatrices);
void SetPointLights(std::vector<PointLight>& pointLights);
void AddSpotLight();

void UpdateSpotLight();
void UpdatePointLights(std::vector<PointLight>& pointLights,
	std::vector<glm::mat4>& pointLightsModelMatrices,
	Shader& lampShader, GLuint lampVAO);

void DrawPointLights(std::vector<PointLight>& pointLights,
	std::vector<glm::mat4>& pointLightsModelMatrices,
	Shader& lampShader, GLuint lampVAO);

//////////////////////////////////////////SHADOWS//////////////////////////////////////////

//////////////////////////////DIRECTED LIGHT SHADOWS///////////////////////////
const GLuint DIR_SHADOW_WIDTH = 1024, DIR_SHADOW_HEIGHT = 1024;

void SetupDirLightFBO(GLuint& dirLightFBO, GLuint& dirLightDepthMapTex);

void DrawSceneForDirShadows(Shader & dirLightDepthShader, 
	GLuint cubeVAO, GLuint planeVAO, const std::vector<glm::mat4>& cubeModelMatrices);

///////////////////////////////PIONT LIGHTS SHADOWS////////////////////////////
const GLuint POINT_SHADOW_WIDTH = 1024, POINT_SHADOW_HEIGHT = 1024;
const GLuint CUBE_FACES = 6;

void SetupPointLightsFBOs(std::vector<GLuint>& pointLightFBOs, std::vector<GLuint>& pointLightDepthCubemaps,
	GLuint buffersNum);

void DrawSceneForPointShadows(Shader & pointLightDepthShader, const std::vector<GLuint>& pointLightFBOs,
	const glm::mat4& pointShadowProj, GLuint pointLightsNum,
	GLuint cubeVAO, GLuint planeVAO, const std::vector<glm::mat4>& cubeModelMatrices);

void SetShaderValues(Shader & pointLightDepthShader, GLuint pointLightID);

////////////////////////////////////////HDR & BLOOM///////////////////////////////////////////
void SetupHDRFrameBuffer(GLuint& framebuffer, GLuint colorBuffers[], GLuint& renderBuffer);
void DrawHDR(Shader & HDR_Shader, GLuint postProcVAO,
	GLuint colorBuffers[], GLuint pingpongColorsBuffers[]);
void SetupPingPongFBOs(GLuint pingpongFBOs[], GLuint pingpongColors[]);
void DrawBlurQuad(Shader & blurShader, GLuint postProcVAO,
	GLuint colorBuffer);
void GenScreenQuadVAO(GLuint& screenQuadVAO, GLuint& screenQuadVBO);


struct LightSpaceMatrices
{
	glm::mat4 dirLightSpaceMatrix;
	std::vector<std::vector<glm::mat4>> pointLightSpaceMatrices;

}
lightSpaceMatrices;


////////////////////////////////////DEFERRED RENDERING/////////////////////////////////
void Setup_G_Buffer(GLuint& gBuffer,
	GLuint& gPosition, GLuint& gModelNormal, GLuint& gMaterialNormal,
	GLuint& gAlbedoSpec, GLuint& gTangent, GLuint& gBitangent,
	GLuint& gRenderBuffer);

void FillModelMatrices_DR(GLuint dimensions, GLfloat scale, glm::mat4 *modelMatrices, glm::mat4 *normalMatrices);

void Gen_DR_VAO(GLuint& DR_VAO, GLuint& DR_VBO);

void Draw_DR_Quad(Shader & lightPassShader, GLuint DR_VAO,
	GLuint& gPosition, GLuint& gModelNormal, GLuint& gMaterialNormal,
	GLuint& gAlbedoSpec, GLuint& gTangent, GLuint& gBitangent,
	GLuint& ssaoColorBuffer);

void UpdateNormalMatricesVAO(Model & model, GLuint amount, glm::mat4 *normalMatrices, GLuint& normalMatricesVBO);

void AddPointLights_DR(std::vector<PointLight>& pointLights, std::vector<glm::mat4>& pointLightsModelMatrices,
	GLfloat areaSize);

void UpdatePointLights_DR(std::vector<PointLight>& pointLights,
	std::vector<glm::mat4>& pointLightsModelMatrices,
	Shader& lampShader, GLuint lampVAO);

float lerp(float a, float b, float f)
{
	return a + f * (b - a);
}

void Gen_AO_Kernel(std::vector<glm::vec3>& ssaoKernel);
void Gen_AO_NoiseTexture(GLuint& noiseTexture);
void Gen_AO_FBO(GLuint& ssaoFBO, GLuint& ssaoColorBuffer);
void Gen_AO_BlurFBO(GLuint& ssaoBlurFBO, GLuint& ssaoColorBufferBlur);

void Draw_AO_Quad(Shader & AO_Shader, GLuint AO_VAO,
	GLuint& gPosition, GLuint& gNormal,
	GLuint& gTangent, GLuint& gBitangent,
	GLuint& noiseTexture, std::vector<glm::vec3>& ssaoKernel);
void Draw_AO_Blur_Quad(Shader & AO_Shader, GLuint AO_VAO,
	GLuint& ssaoColorBuffer);
///////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////PBR/////////////////////////////////////////
void Render_PBR_Spheres(Shader& PBR_Shader,
	GLuint& albedo, GLuint& normal, GLuint& metallic, GLuint& roughness, GLuint& ao,
	GLint nrRows, GLint nrColumns, GLfloat spacing);

void renderSphere();




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
	glfwSetKeyCallback(window, key_callback);

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

	////////////////////GAMMA-CORRECTION////////////////////
	//glEnable(GL_FRAMEBUFFER_SRGB);

    // build and compile shaders
    // -------------------------
	
	//////////////////////////////////WARNING!!!/////////////////////////////////////
	//we have to set texture blocks for ALL samplerCubes in shaders even without textures or 
	//even it won't be used in shader further
	//I did it in Shader class in method SetSamplerCubesBlocks()
	//////////////////////////////////WARNING!!!/////////////////////////////////////

    Shader shader("Shaders/Vertex Shader Model.glsl", "Shaders/Fragment Shader Model.glsl",
		"Shaders/Geometry Shader Model.glsl");
	//shader.SetSamplerCubesBlocks();

    Shader postProcShader("Shaders/Vertex Shader PP.glsl", "Shaders/Fragment Shader PP.glsl");
	Shader skyboxShader("Shaders/Cubemap Vertex Shader.glsl", "Shaders/Cubemap Fragment Shader.glsl");
	 
	Shader modelShader("Shaders/Vertex Shader Model.glsl", "Shaders/Fragment Shader Model.glsl",
		"Shaders/Geometry Shader Model.glsl");
	Shader modelShaderNormals("Shaders/Vertex Shader Model.glsl", "Shaders/Fragment Shader Model.glsl",
		"Shaders/Geometry Shader Model Normals.glsl");

	Shader skullShader("Shaders/Vertex Shader Model.glsl", "Shaders/Fragment Shader Model.glsl",
		"Shaders/Geometry Shader Model.glsl");

	Shader dirLightDepthShader("Shaders/Dir Light Depth VS.glsl", "Shaders/Dir Light Depth FS.glsl");

	Shader pointLightDepthShader("Shaders/Point Light Depth VS.glsl", "Shaders/Point Light Depth FS.glsl",
		"Shaders/Point Light Depth GS.glsl");

	Shader HDR_Shader("Shaders/Vertex Shader PP.glsl", "Shaders/HDR FS.glsl");

	Shader blurShader("Shaders/Blur VS.glsl", "Shaders/Blur FS.glsl");

	Shader geomPassShader("Shaders/Geometry Pass VS.glsl", "Shaders/Geometry Pass FS.glsl");
	Shader lightPassShader("Shaders/Lightning Pass VS.glsl", "Shaders/Lightning Pass FS.glsl");
	Shader lampShader("Shaders/Single Color VS.glsl", "Shaders/Single Color FS.glsl");

	Shader AO_Shader("Shaders/AO VS.glsl", "Shaders/AO FS.glsl");
	Shader AO_Blur_Shader("Shaders/AO VS.glsl", "Shaders/AO Blur FS.glsl");

	Shader PBR_Shader("Shaders/PBR VS.glsl", "Shaders/PBR FS.glsl");
	//////////////////////////////UNIFORM BUFFER//////////////////////////////
	//2x matrices 4x4
	unsigned int uboBlock;
	glGenBuffers(1, &uboBlock);
	glBindBuffer(GL_UNIFORM_BUFFER, uboBlock);
	glBufferData(GL_UNIFORM_BUFFER, 128, NULL, GL_STATIC_DRAW); // выдел€ем 128 байт пам€ти
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	glBindBufferBase(
		GL_UNIFORM_BUFFER,
		0,				//binding point
		uboBlock);		//uniform buffer ID

	//postProc and skybox shaders are not need this uniform buffer
	shader.BindUniformBuffer(
		"Matrices",		//uniform block name
		0);				//binding point
	
	modelShader.BindUniformBuffer("Matrices", 0);
	modelShaderNormals.BindUniformBuffer("Matrices", 0);

	skullShader.BindUniformBuffer("Matrices", 0);

	geomPassShader.BindUniformBuffer("Matrices", 0);
	lightPassShader.BindUniformBuffer("Matrices", 0);
	lampShader.BindUniformBuffer("Matrices", 0);

	PBR_Shader.BindUniformBuffer("Matrices", 0);
	/////////////////////////////////////SETTINGS UNIFORM BUFFER////////////////////////////////////	
	glGenBuffers(1, &settingsUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, settingsUBO);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(Settings), NULL, GL_STATIC_DRAW); 
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	glBindBufferBase(
		GL_UNIFORM_BUFFER,
		1,					//binding point
		settingsUBO);		//uniform buffer ID
	
	shader.BindUniformBuffer(
		"Settings",		//uniform block name
		1);				//binding point

	modelShader.BindUniformBuffer("Settings", 1);
	modelShaderNormals.BindUniformBuffer("Settings", 1);
	postProcShader.BindUniformBuffer("Settings", 1);
	skyboxShader.BindUniformBuffer("Settings", 1);

	skullShader.BindUniformBuffer("Settings", 1);

	HDR_Shader.BindUniformBuffer("Settings", 1);
	////////////////////////////////////////////////////////////////////////////////////////////////

	/////////////////////////////////////LIGHTS UNIFORM BUFFER////////////////////////////////////	

	glGenBuffers(1, &lightsUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, lightsUBO);
	glBufferData(GL_UNIFORM_BUFFER, lights.lightsSize, NULL, GL_STATIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	glBindBufferBase(
		GL_UNIFORM_BUFFER,
		2,					//binding point
		lightsUBO);		//uniform buffer ID

	shader.BindUniformBuffer(
		"Lights",		//uniform block name
		2);				//binding point

	modelShader.BindUniformBuffer("Lights", 2);
	skullShader.BindUniformBuffer("Lights", 2);
	lightPassShader.BindUniformBuffer("Lights", 2);
	PBR_Shader.BindUniformBuffer("Lights", 2);

	glGenBuffers(1, &pointLightPositionsUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, pointLightPositionsUBO);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(glm::vec4) * NUM_POINT_LIGHTS, NULL, GL_STATIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	glBindBufferBase(
		GL_UNIFORM_BUFFER,
		3,					//binding point
		pointLightPositionsUBO);		//uniform buffer ID

	shader.BindUniformBuffer(
		"PointLightPositions",		//uniform block name
		3);				//binding point

	modelShader.BindUniformBuffer("PointLightPositions", 3);
	skullShader.BindUniformBuffer("PointLightPositions", 3);	

	glGenBuffers(1, &dirLightDirectionUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, dirLightDirectionUBO);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(glm::vec4), NULL, GL_STATIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	glBindBufferBase(
		GL_UNIFORM_BUFFER,
		4,					//binding point
		dirLightDirectionUBO);		//uniform buffer ID

	shader.BindUniformBuffer(
		"DirLightDirection",		//uniform block name
		4);				//binding point

	modelShader.BindUniformBuffer("DirLightDirection", 4);
	skullShader.BindUniformBuffer("DirLightDirection", 4);

	glGenBuffers(1, &spotLightDirectionUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, spotLightDirectionUBO);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(glm::vec4), NULL, GL_STATIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	glBindBufferBase(
		GL_UNIFORM_BUFFER,
		5,					//binding point
		spotLightDirectionUBO);		//uniform buffer ID

	shader.BindUniformBuffer(
		"SpotLightDirection",		//uniform block name
		5);				//binding point

	modelShader.BindUniformBuffer("SpotLightDirection", 5);
	skullShader.BindUniformBuffer("SpotLightDirection", 5);

	glGenBuffers(1, &pointLightRadiusUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, pointLightRadiusUBO);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(pointLightRadiuses), NULL, GL_STATIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	glBindBufferBase(
		GL_UNIFORM_BUFFER,
		6,							//binding point
		pointLightRadiusUBO);		//uniform buffer ID

	lightPassShader.BindUniformBuffer(
		"PointLightRadiuses",		//uniform block name
		6);							//binding point
	PBR_Shader.BindUniformBuffer("PointLightRadiuses", 6);
	
	////////////////////////////////////////////////////////////////////////////////////////////////


    // cube VAO
    GLuint cubeVAO, cubeVBO, cubeTanVBO, cubeBitanVBO;
	GenCubeVAO(cubeVAO, cubeVBO, cubeTanVBO, cubeBitanVBO);
    // plane VAO
	GLuint planeVAO, planeVBO, planeTanVBO, planeBitanVBO;
	GenPlaneVAO(planeVAO, planeVBO, planeTanVBO, planeBitanVBO);

    // screen quad VAO
	GLuint postProcVAO, postProcVBO;
	GenPostProcVAO(postProcVAO, postProcVBO);

    // load textures
    // -------------
	GLuint cubeTexture = loadTexture("Textures/container2.png", bLoadSRGB);
	GLuint cubeSpecTexture = loadTexture("Textures/container2_specular.png");
	GLuint floorTexture = loadTexture("Textures/container.jpg", bLoadSRGB);
	//GLuint wallTexture = loadTexture("Textures/brickwall.jpg", bLoadSRGB);
	//GLuint wallNormalMap = loadTexture("Textures/brickwall_normal.jpg");
	GLuint wallTexture = loadTexture("Textures/bricks2.jpg", bLoadSRGB);
	GLuint wallNormalMap = loadTexture("Textures/bricks2_normal.jpg");
	GLuint wallHeightMap = loadTexture("Textures/bricks2_disp.jpg");
    // shader configuration
    // --------------------
    shader.UseProgram();
    //shader.SetInt("material.diffuse[0]", 0);
	//shader.SetInt("material.specular[0]", 1);
	shader.SetFloat("material.shininess", 32.f);

	modelShader.UseProgram();
	modelShader.SetFloat("material.shininess", 32.f);

	skullShader.UseProgram();
	skullShader.SetFloat("material.shininess", 32.f);

	postProcShader.UseProgram();
	postProcShader.SetInt("screenTexture", 0);
	postProcShader.SetInt("screenTextureMS", 1);

    //////////////////////////////////////// framebuffer configuration /////////////////////////////////////
    // -------------------------
	GLuint samples = 4;

	glm::vec2 uvOffset(1.0f / SCR_WIDTH, 1.0f / SCR_HEIGHT);
	postProcShader.SetVec2("uvOffset", uvOffset);	

	GLuint framebuffer = 0;
	GLuint framebufferAA = 0;
    
    // create a color attachment texture
	GLuint textureColorbuffer = 0;
	GLuint textureColorbufferMS = 0;
	GLuint renderBuffer = 0;
	GLuint renderBufferMS = 0;

	///////////////////////BlitFramebuffer/////////////////////
	//second post-processing framebuffer
	GLuint intermediateFBO = 0;
	//color attachment texture
	GLuint screenBlitTexture = 0;

	//Setup Frame Buffer	
	SetupFrameBuffer(framebuffer, textureColorbuffer, renderBuffer);
	
	////////////////////TEXTURE ANTI-ALIASING////////////////////		

	SetupFrameBufferMS(framebufferAA, textureColorbufferMS, renderBufferMS, samples,
		intermediateFBO, screenBlitTexture);

	postProcShader.SetInt("samples", samples);
	postProcShader.SetVec2("dimensions", glm::vec2(SCR_WIDTH, SCR_HEIGHT));
	
	/////////////////////////////////////////////////////////////
	
    
	

	

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
	GLuint cubemapTexture = loadCubemap(faces, bLoadSRGB);

	// skybox VAO
	GLuint skyboxVAO, skyboxVBO;
	GenSkyboxVAO(skyboxVAO, skyboxVBO);
	
	/////////////////////////////////////////////////REFLECT CUBEMAP//////////////////////////////////////////
	

	/////////////////////////////////////MODEL////////////////////////////////////
	Model model("Models/backpack/backpack.obj", cubemapTexture, 1.f, false);
	modelShader.UseProgram();
	glm::mat4 mod = glm::mat4(1.0f);	
	mod = glm::translate(mod, glm::vec3(2.0f, 2.0f, -3.0f));
	mod = glm::rotate(mod, glm::radians( (GLfloat)glfwGetTime()), glm::vec3(1.0, 1.0, 1.0));
	modelShader.SetMat4("model", mod);	


	modelShaderNormals.UseProgram();
	modelShaderNormals.SetMat4("model", mod);

	skullShader.UseProgram();
	skullShader.SetMat4("model", mod);
	///////////////////////////////////////LIGHTS////////////////////////////////////
		
	std::vector<PointLight> pointLights;
	std::vector<glm::mat4> pointLightsModelMatrices;

	AddDirectedLight();

	AddPointLights(pointLights, pointLightsModelMatrices);

	AddSpotLight();

	///////////////////////////////////INSTANCING//////////////////////////////////

	//Shader skullShader = modelShader;
	
	Model skull("Models/Skull/Skull.obj", 0, 1.f, false);

	GLuint modelMatricesVBO = 0;

	GLuint amount = 31;
	glm::mat4 *modelMatrices = new glm::mat4[amount];
	FillModelMatrices(amount, modelMatrices);

	UpdateVAO(skull, amount, modelMatrices, modelMatricesVBO);


	///////////////////////////////////SHADOWS//////////////////////////////////////

	//Depth map for directional light
	GLuint dirLightFBO;
	GLuint dirLightDepthMapTex;//Depth map texture
	SetupDirLightFBO(dirLightFBO, dirLightDepthMapTex);

	
	GLuint pointLightsNum = pointLights.size();
	//depth maps for point lights
	std::vector<GLuint> pointLightFBOs;
	std::vector<GLuint> pointLightDepthCubemaps;

	SetupPointLightsFBOs(pointLightFBOs, pointLightDepthCubemaps, pointLightsNum);

	float aspect = (float)POINT_SHADOW_WIDTH / (float)POINT_SHADOW_HEIGHT;
	float nearPlane = 0.0f;
	float farPlane = 25.0f;
	glm::mat4 pointShadowProj = glm::perspective(glm::radians(90.0f), aspect, nearPlane, farPlane);

	pointLightDepthShader.UseProgram();
	pointLightDepthShader.SetFloat("far_plane", farPlane);

	shader.UseProgram();
	shader.SetFloat("far_plane", farPlane);

	
	////////////////////////////////////////// HDR & BLOOM ////////////////////////////////////////

	GLuint HDR_FBO = 0;
	//color attachment texture
	GLuint HDR_ColorBuffers[2];
	//depth, stencil attachment texture
	GLuint HDR_RenderBuffer = 0;

	//Setup Frame Buffer	
	SetupHDRFrameBuffer(HDR_FBO, HDR_ColorBuffers, HDR_RenderBuffer);

	HDR_Shader.UseProgram();
	HDR_Shader.SetMat4("model", postProcModel);

	GLuint pingpongFBOs[2];
	GLuint pingpongColorsBuffers[2];

	SetupPingPongFBOs(pingpongFBOs, pingpongColorsBuffers);

	GLuint screenQuadVAO = 0;
	GLuint screenQuadVBO = 0;
	GenScreenQuadVAO(screenQuadVAO, screenQuadVBO);

	/////////////model matrices for cubes drawing/////////////////
	std::vector<glm::mat4> cubeModelMatrices;
	FillModelMatrices(cubeModelMatrices);


	//after installing new OS appears screen clipping
	//which disappears after changing window size
	glfwSetWindowSize(window, SCR_WIDTH + 1, SCR_HEIGHT + 1);
	glfwSetWindowSize(window, SCR_WIDTH - 1, SCR_HEIGHT - 1);
	
	//////////////////////////////////DEFERRED RENDERING///////////////////////////////
	GLuint gBuffer = 0;
	
	GLuint gPosition = 0, gModelNormal = 0, gMaterialNormal = 0,
		gAlbedoSpec = 0, gTangent = 0, gBitangent = 0;
	GLuint gRenderBuffer = 0;
	
	Setup_G_Buffer(gBuffer, gPosition, gModelNormal, gMaterialNormal,
		gAlbedoSpec, gTangent, gBitangent, gRenderBuffer);
	
	GLuint modelMatricesVBO_DR = 0, normalMatricesVBO_DR = 0;
	GLuint dimensions = 5;
	GLfloat scale_DR = 5.f;
	glm::mat4 *modelMatrices_DR = new glm::mat4[dimensions * dimensions];
	glm::mat4 *normalMatrices_DR = new glm::mat4[dimensions * dimensions];
	FillModelMatrices_DR(dimensions, scale_DR, modelMatrices_DR, normalMatrices_DR);

	UpdateVAO(model, dimensions * dimensions, modelMatrices_DR, modelMatricesVBO_DR);
	UpdateNormalMatricesVAO(model, dimensions * dimensions, normalMatrices_DR, normalMatricesVBO_DR);

	GLuint DR_VAO = 0, DR_VBO = 0;
	Gen_DR_VAO(DR_VAO, DR_VBO);

	std::vector<PointLight> pointLights_DR;
	std::vector<glm::mat4> pointLightsModelMatrices_DR;

	AddPointLights_DR(pointLights_DR, pointLightsModelMatrices_DR, dimensions*scale_DR);

	///////////////////////////////////////////////////////////////////////////////////


	////////////////////////////////////AO////////////////////////////////////
	std::vector<glm::vec3> ssaoKernel;
	GLuint noiseTexture = 0;
	Gen_AO_Kernel(ssaoKernel);
	Gen_AO_NoiseTexture(noiseTexture);

	GLuint ssaoFBO = 0;	
	GLuint ssaoColorBuffer = 0;
	Gen_AO_FBO(ssaoFBO, ssaoColorBuffer);

	GLuint ssaoBlurFBO = 0, ssaoColorBufferBlur = 0;
	Gen_AO_BlurFBO(ssaoBlurFBO, ssaoColorBufferBlur);

	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////PBR/////////////////////////////////////
	PBR_Shader.UseProgram();
	PBR_Shader.SetInt("albedoMap", 0);
	PBR_Shader.SetInt("normalMap", 1);
	PBR_Shader.SetInt("metallicMap", 2);
	PBR_Shader.SetInt("roughnessMap", 3);
	PBR_Shader.SetInt("aoMap", 4);

	// load PBR material textures
	// --------------------------
	GLuint albedo		= loadTexture("Textures/rustediron/rustediron2_basecolor.png", bLoadSRGB);
	GLuint normal		= loadTexture("Textures/rustediron/rustediron2_normal.png");
	GLuint metallic		= loadTexture("Textures/rustediron/rustediron2_metallic.png");
	GLuint roughness	= loadTexture("Textures/rustediron/rustediron2_roughness.png");
	GLuint ao			= loadTexture("Textures/rustediron/rustediron2_ao.png");

	
	int nrRows = 7;
	int nrColumns = 7;
	float spacing = 2.5;

	//////////////////////////////////////////////////////////////////////////

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

		if (settings.bDeferredRendering)
		{
			glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

			UpdateMatrices(uboBlock);
			
			//////////////////////////////gBuffer////////////////////////////////
			glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);

			glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

			geomPassShader.UseProgram();		
			geomPassShader.SetBool("binvertUVs", true);
			model.Draw(geomPassShader, true, dimensions*dimensions);
			geomPassShader.SetBool("binvertUVs", false);

			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			///////////////////////////////AO//////////////////////////////
			glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);

			glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

			Draw_AO_Quad(AO_Shader, DR_VAO,
				gPosition, gModelNormal,
				gTangent, gBitangent,
				noiseTexture, ssaoKernel);

			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO);

			glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

			Draw_AO_Blur_Quad(AO_Blur_Shader, DR_VAO,
				ssaoColorBuffer);

			glBindFramebuffer(GL_FRAMEBUFFER, 0);


			///////////////////////SCENE////////////////////////

			Draw_DR_Quad(lightPassShader, DR_VAO,
				gPosition, gModelNormal, gMaterialNormal,
				gAlbedoSpec, gTangent, gBitangent, ssaoColorBuffer);

			//copy depth buffer from gBuffer into default frame buffer
			glBindFramebuffer(GL_READ_FRAMEBUFFER, gBuffer);
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0); // буфер глубины по-умолчанию
			glBlitFramebuffer(
				0, 0, SCR_WIDTH, SCR_HEIGHT, 0, 0, SCR_WIDTH, SCR_HEIGHT, GL_DEPTH_BUFFER_BIT, GL_NEAREST
			);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			SetPointLights(pointLights_DR);
			UpdatePointLights_DR(pointLights_DR, pointLightsModelMatrices_DR, lampShader, cubeVAO);

			lightPassShader.UseProgram();
			for (int i = 0; i < NUM_POINT_LIGHTS; i++)
			{
				char num[3];
				_itoa_s(i, num, 10);
				lightPassShader.SetVec4(std::string("positions[") + num + "]", lights.pointLights[i].position);
			}

			///////////////////////////////////PBR/////////////////////////////////
			Render_PBR_Spheres(PBR_Shader,
				albedo, normal, metallic, roughness, ao,
				nrRows, nrColumns, spacing);

		}
		else
		{
			

			//if shadows aren't drawing - use default parameters for functions calls
			GLuint dirLightDepthMapTexID = 0;
			std::vector<GLuint>* pointLightDepthCubemapsPtr = nullptr;

			/////////////////////////////////////////////SHADOWS//////////////////////////////////////////////
			if (settings.bShadows && settings.bDirectionalLight)
			{
				//Scene drawing for shadows
				glViewport(0, 0, DIR_SHADOW_WIDTH, DIR_SHADOW_HEIGHT);
				glBindFramebuffer(GL_FRAMEBUFFER, dirLightFBO);
				glCullFace(GL_BACK);
				DrawSceneForDirShadows(dirLightDepthShader, cubeVAO, planeVAO, cubeModelMatrices);
				glCullFace(GL_FRONT);

				// 2. рисуем сцену как обычно с тен€ми (использу€ карту глубины)
				glBindFramebuffer(GL_FRAMEBUFFER, 0);
				glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);

				dirLightDepthMapTexID = dirLightDepthMapTex;

				shader.UseProgram();
				shader.SetMat4("dirLightSpaceMatrix", lightSpaceMatrices.dirLightSpaceMatrix);
			}

			if (settings.bShadows && settings.bPointLights)
			{
				//Scene drawing for shadows
				glViewport(0, 0, POINT_SHADOW_WIDTH, POINT_SHADOW_HEIGHT);
				DrawSceneForPointShadows(pointLightDepthShader, pointLightFBOs,
					pointShadowProj, pointLightsNum,
					cubeVAO, planeVAO, cubeModelMatrices);

				// 2. рисуем сцену как обычно с тен€ми (использу€ карту глубины)
				glBindFramebuffer(GL_FRAMEBUFFER, 0);
				glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);

				pointLightDepthCubemapsPtr = &pointLightDepthCubemaps;
			}

			////////////////////////////////////////Common Scene drawing/////////////////////////////////////////
			glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
			DrawScene(shader, skyboxShader, cubeVAO, cubeModelMatrices, planeVAO, skyboxVAO, uboBlock,
				cubeTexture, cubeSpecTexture,
				floorTexture, cubemapTexture,
				wallTexture, wallNormalMap, wallHeightMap,
				dirLightDepthMapTexID,
				pointLightDepthCubemapsPtr);


			/////////////////////////////////////////////////FRAME BUFFER///////////////////////////////////////////////
			// render
			// ------
			// bind to framebuffer and draw scene as we normally would to color texture 

			glBindFramebuffer(GL_FRAMEBUFFER, settings.bAntiAliasing ? framebufferAA : framebuffer);

			glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
			DrawScene(shader, skyboxShader, cubeVAO, cubeModelMatrices, planeVAO, skyboxVAO, uboBlock,
				cubeTexture, cubeSpecTexture,
				floorTexture, cubemapTexture,
				wallTexture, wallNormalMap, wallHeightMap,
				dirLightDepthMapTexID,
				pointLightDepthCubemapsPtr);

			if (settings.bPointLights)
				DrawPointLights(pointLights, pointLightsModelMatrices,
					shader, cubeVAO);

			// now bind back to default framebuffer and draw a quad plane with the attached framebuffer color texture
			if (settings.bAntiAliasing)
			{
				// 2. now blit multisampled buffer(s) to normal colorbuffer of intermediate FBO. Image is stored in screenTexture
				glBindFramebuffer(GL_READ_FRAMEBUFFER, framebufferAA);
				glBindFramebuffer(GL_DRAW_FRAMEBUFFER, intermediateFBO);
				glBlitFramebuffer(0, 0, SCR_WIDTH, SCR_HEIGHT, 0, 0, SCR_WIDTH, SCR_HEIGHT, GL_COLOR_BUFFER_BIT, GL_NEAREST);

			}

			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			/////////////////////////////////////////////////////////////////////////////////////////////////////////////

			/////////////////////////////////////HDR & BLOOM///////////////////////////////////
			if (settings.bHDR)
			{
				shader.UseProgram();
				shader.SetBool("bHDR", true);
				if (settings.bBloom)
					shader.SetBool("bBloom", true);

				glBindFramebuffer(GL_FRAMEBUFFER, HDR_FBO);

				glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
				DrawScene(shader, skyboxShader, cubeVAO, cubeModelMatrices, planeVAO, skyboxVAO, uboBlock,
					cubeTexture, cubeSpecTexture,
					floorTexture, cubemapTexture,
					wallTexture, wallNormalMap, wallHeightMap,
					dirLightDepthMapTexID,
					pointLightDepthCubemapsPtr);

				if (settings.bPointLights)
					DrawPointLights(pointLights, pointLightsModelMatrices,
						shader, cubeVAO);

				glBindFramebuffer(GL_FRAMEBUFFER, 0);

				shader.UseProgram();
				shader.SetBool("bHDR", false);
				shader.SetBool("bBloom", false);

				if (settings.bBloom)
				{
					bool horizontal = true, first_iteration = true;
					int amount = 10;
					blurShader.UseProgram();
					for (unsigned int i = 0; i < amount; i++)
					{
						glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBOs[horizontal]);
						blurShader.SetInt("horizontal", horizontal);
						/*glBindTexture(
							GL_TEXTURE_2D, first_iteration ? HDR_ColorBuffers[1] : pingpongColorsBuffers[!horizontal]
						);*/

						DrawBlurQuad(blurShader, screenQuadVAO,
							first_iteration ? HDR_ColorBuffers[1] : pingpongColorsBuffers[!horizontal]);

						horizontal = !horizontal;
						if (first_iteration)
							first_iteration = false;
					}
					glBindFramebuffer(GL_FRAMEBUFFER, 0);
				}
			}
			///////////////////////////////////////////////////////////////////////////

			DrawReflectCube(shader, cubeVAO, cubemapTexture, cubeTexture, cubeSpecTexture, camera.Position);

			DrawRefractCube(shader, cubeVAO, cubemapTexture, camera.Position);


			//Draw model
			modelShader.UseProgram();
			modelShader.SetVec3("cameraPos", camera.Position);
			modelShader.SetFloat("time", glfwGetTime());
			modelShader.SetMat4("model", mod);
			modelShader.SetBool("bReflect", false);
			modelShader.SetBool("binvertUVs", true);
			model.Draw(modelShader);
			modelShader.SetBool("bReflect", false);
			modelShader.SetBool("binvertUVs", false);

			///////////////////////////////Draw model normals//////////////////////////////
			if (settings.bShowNormals)
			{
				modelShaderNormals.UseProgram();
				modelShaderNormals.SetBool("bShowNormals", true);
				modelShaderNormals.SetBool("binvertUVs", true);
				model.Draw(modelShaderNormals);
				modelShaderNormals.SetBool("bShowNormals", false);
				modelShaderNormals.SetBool("binvertUVs", false);
			}

			///////////////////////////////////INSTANCING//////////////////////////////////
			//render of planet and asteroids
			if (settings.bInstancing)
			{
				skullShader.UseProgram();
				skullShader.SetVec3("cameraPos", camera.Position);
				skullShader.SetFloat("time", glfwGetTime());
				skullShader.SetMat4("model", mod);
				skullShader.SetBool("bReflect", false);
				skullShader.SetBool("bInstancing", true);
				skull.Draw(skullShader, true, amount);
				skullShader.SetBool("bInstancing", false);
			}

			//////////////////////////////////POST PROCESS EFFECT//////////////////////////
			postProcShader.UseProgram();
			postProcShader.SetBool("bUseKernel", settings.bPostProcess);
			postProcShader.SetBool("bAntiAliasing", settings.bAntiAliasing);
			postProcShader.SetBool("bBlit", settings.bBlit);

			//Show scene with HDR
			if (settings.bHDR)
			{
				HDR_Shader.UseProgram();
				HDR_Shader.SetFloat("exposure", 0.5f);

				DrawHDR(HDR_Shader, postProcVAO, HDR_ColorBuffers, pingpongColorsBuffers);
			}
			//show depth texture
			else if (settings.bShadows && settings.bDirectionalLight && settings.bShowDirLightDepthMap)
				DrawPostProc(postProcShader, postProcVAO, dirLightDepthMapTex, textureColorbufferMS, screenBlitTexture);
			else
				DrawPostProc(postProcShader, postProcVAO, textureColorbuffer, textureColorbufferMS, screenBlitTexture);


			//////////////////////////////////////LIGHTS//////////////////////////////////			

			if (settings.bPointLights)
			{
				SetPointLights(pointLights);
				UpdatePointLights(pointLights, pointLightsModelMatrices, shader, cubeVAO);
			}

			if (settings.bSpotLight)
				UpdateSpotLight();
		}
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

	



	glDeleteFramebuffers(1, &framebuffer);
	glDeleteFramebuffers(1, &intermediateFBO);
	glDeleteFramebuffers(1, &framebufferAA);

	glDeleteBuffers(1, &modelMatricesVBO);
	delete[] modelMatrices;

	glDeleteFramebuffers(1, &dirLightFBO);

	for (int i = 0; i < pointLightFBOs.size(); i++)
		glDeleteFramebuffers(1, &pointLightFBOs[i]);

	glDeleteVertexArrays(1, &screenQuadVAO);
	glDeleteBuffers(1, &screenQuadVBO);


	//////////////////////////DEFERRED RENDERING///////////////////////////
	glDeleteBuffers(1, &modelMatricesVBO_DR);
	glDeleteBuffers(1, &normalMatricesVBO_DR);
	delete[] modelMatrices_DR;
	delete[] normalMatrices_DR;

	glDeleteVertexArrays(1, &DR_VAO);
	glDeleteBuffers(1, &DR_VBO);
	///////////////////////////////////////////////////////////////////////

	glDeleteFramebuffers(1, &ssaoFBO);
	glDeleteFramebuffers(1, &ssaoBlurFBO);

    glfwTerminate();
    return 0;
}

void DrawScene(Shader & shader, Shader & skyboxShader,
	GLuint cubeVAO, const std::vector<glm::mat4>& cubeModelMatrices,
	GLuint planeVAO, GLuint skyboxVAO, GLuint uboBlock,
	GLuint cubeDiffTexture, GLuint cubeSpecTexture,
	GLuint floorTexture, GLuint cubemapTexture,
	GLuint wallTexture, GLuint wallNormalMap, GLuint wallHeightMap,
	GLuint dirLightDepthMapTex, const std::vector<GLuint>* pointLightDepthCubemaps)
{
	


	shader.UseProgram();

	glm::mat4 model = glm::mat4(1.0f);
	glm::mat4 view = camera.GetViewMatrix();
	glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 1000.0f);
	
	////////////////////////////////////////////UNIFORM BUFFER VALUES///////////////////////////////////////////////
	UpdateMatrices(uboBlock);
	

	shader.SetBool("bStencil", false);

	glEnable(GL_DEPTH_TEST); // enable depth testing (is disabled for rendering screen-space quad)

	

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
	DrawCubes(shader, cubeVAO,
		cubeModelMatrices, pointLightDepthCubemaps,
		cubeDiffTexture, cubeSpecTexture,
		wallTexture, wallNormalMap, wallHeightMap,
		dirLightDepthMapTex);

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
	DrawCubes(shader, cubeVAO,
		cubeModelMatrices, nullptr,
		0, 0, 0, 0, 0, 0,
		glm::vec3(scale), true, borderColor);

	
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
	DrawFloor(shader, planeVAO,
		floorTexture, cubeSpecTexture,
		wallTexture, wallNormalMap, wallHeightMap,
		dirLightDepthMapTex, pointLightDepthCubemaps);

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

//* Use key_callback if single press needs
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	

	if (key == GLFW_KEY_F && action == GLFW_PRESS)
	{
		settings.bGammaCorrection = !settings.bGammaCorrection;
		settings.UpdateSettings();
	}
	if (key == GLFW_KEY_I && action == GLFW_PRESS)
	{
		settings.bInstancing = !settings.bInstancing;
		settings.UpdateSettings();
	}
	if (key == GLFW_KEY_N && action == GLFW_PRESS)
	{
		settings.bShowNormals = !settings.bShowNormals;
		if (settings.bShowNormals)
			settings.bExplode = false;
		settings.UpdateSettings();
	}
	if (key == GLFW_KEY_M && action == GLFW_PRESS)
	{
		settings.bExplode = !settings.bExplode;
		if (settings.bExplode)
			settings.bShowNormals = false;
		settings.UpdateSettings();
	}
	
	if (key == GLFW_KEY_1 && action == GLFW_PRESS)
	{
		settings.bPointLights = !settings.bPointLights;
		//do not using shadows without lights
		if (!(settings.bPointLights || settings.bDirectionalLight))
			settings.bShadows = false;
		//reset influencing settings
		if (!settings.bDirectionalLight)
		{
			settings.bShowDirLightDepthMap = false;
		}
		settings.UpdateSettings();
	}
	if (key == GLFW_KEY_2 && action == GLFW_PRESS)
	{
		settings.bDirectionalLight = !settings.bDirectionalLight;
		//do not using shadows without lights
		if (!(settings.bPointLights || settings.bDirectionalLight))
			settings.bShadows = false;
		//reset influencing settings
		if (!settings.bDirectionalLight)
		{
			settings.bShowDirLightDepthMap = false;
		}

		settings.UpdateSettings();
	}
	
	if (key == GLFW_KEY_3 && action == GLFW_PRESS)
	{
		settings.bSpotLight = !settings.bSpotLight;
		//do not using shadows without lights
		if (!(settings.bPointLights || settings.bDirectionalLight))
			settings.bShadows = false;
		settings.UpdateSettings();
	}	
	if (key == GLFW_KEY_P && action == GLFW_PRESS)
	{
		settings.bPostProcess = !settings.bPostProcess;
		if (settings.bPostProcess)
		{
			settings.bAntiAliasing = false;
			settings.bBlit = false;
			settings.bShowDirLightDepthMap = false;
		}
		settings.UpdateSettings();
	}
	if (key == GLFW_KEY_U && action == GLFW_PRESS)
	{
		settings.bAntiAliasing = !settings.bAntiAliasing;
		settings.bBlit = settings.bAntiAliasing;
		if (settings.bAntiAliasing)
		{			
			settings.bPostProcess = false;
			settings.bShowDirLightDepthMap = false;
		}
		settings.UpdateSettings();
	}
	if (key == GLFW_KEY_O && action == GLFW_PRESS)
	{
		//do not using shadows without lights
		if (settings.bPointLights || settings.bDirectionalLight)
		{
			settings.bShadows = !settings.bShadows;
			if (!settings.bShadows)
			{				
				settings.bShowDirLightDepthMap = false;
			}
			settings.UpdateSettings();
		}
	}
	if (key == GLFW_KEY_L && action == GLFW_PRESS)
	{
		//do not using depth map without shadows
		if (settings.bShadows)
		{
			settings.bShowDirLightDepthMap = !settings.bShowDirLightDepthMap;
			
			//reset influencing settings
			if (settings.bShowDirLightDepthMap)
			{
				settings.bAntiAliasing = false;
				settings.bBlit = false;
				settings.bPostProcess = false;
			}

			settings.UpdateSettings();
		}
	}
	if (key == GLFW_KEY_M && action == GLFW_PRESS)
	{
		settings.bUseNormalMap = !settings.bUseNormalMap;
		if (!settings.bUseNormalMap)
			settings.bUseParallaxMapping = false;

		settings.UpdateSettings();
	}

	if (key == GLFW_KEY_B && action == GLFW_PRESS)
	{
		settings.bUseParallaxMapping = !settings.bUseParallaxMapping;
		if (settings.bUseParallaxMapping)
			settings.bUseNormalMap = true;

		settings.UpdateSettings();
	}

	if (key == GLFW_KEY_H && action == GLFW_PRESS)
	{
		settings.bHDR = !settings.bHDR;		
		if (!settings.bHDR)
			settings.bBloom = false;

		settings.UpdateSettings();
	}
	if (key == GLFW_KEY_G && action == GLFW_PRESS)
	{
		settings.bBloom = !settings.bBloom;
		if (settings.bBloom)
			settings.bHDR = true;

		settings.UpdateSettings();
	}

	if (key == GLFW_KEY_T && action == GLFW_PRESS)
	{
		settings.bDeferredRendering = !settings.bDeferredRendering;
		
		settings.UpdateSettings();
	}
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
	SCR_WIDTH = width;
	SCR_HEIGHT = height;
    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
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
unsigned int loadTexture(char const * path, bool bSRGB)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
		GLenum internalFormat;
		if (nrComponents == 1)
		{
			format = GL_RED;
			internalFormat = GL_RED;
		}
		else if (nrComponents == 3)
		{
			format = GL_RGB;
			internalFormat = bSRGB ? GL_SRGB : GL_RGB;
		}
		else if (nrComponents == 4)
		{
			format = GL_RGBA;
			internalFormat = bSRGB ? GL_SRGB_ALPHA : GL_RGBA;
		}

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, data);
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

void DrawCubes(Shader & shader, GLuint VAO,
	const std::vector<glm::mat4>& cubeModelMatrices, const std::vector<GLuint>* pointLightDepthCubemaps,
	GLuint diffTexture, GLuint specTexture,
	GLuint wallTexture, GLuint wallNormalMap, GLuint wallHeightMap,
	GLuint dirLightDepthMapTex,
	glm::vec3 scale, bool bStencil, glm::vec4 borderColor)
{
	shader.UseProgram();

	shader.SetBool("bStencil", bStencil);
	shader.SetVec4("borderColor", borderColor);
	
	glBindVertexArray(VAO);

	GLuint texCount = 0;
	if (settings.bUseNormalMap || settings.bUseParallaxMapping)
	{
		if (wallTexture != 0)
		{
			glActiveTexture(GL_TEXTURE0 + texCount);
			glBindTexture(GL_TEXTURE_2D, wallTexture);
			shader.SetInt("material.diffuse[0]", texCount);
			texCount++;
		}

		if (wallNormalMap != 0)
		{
			glActiveTexture(GL_TEXTURE0 + texCount);
			glBindTexture(GL_TEXTURE_2D, wallNormalMap);
			shader.SetInt("material.normal[0]", texCount);
			//shader.SetInt("material.specular[0]", 0);
			shader.SetBool("bHasNormalMap", true);
			texCount++;
		}

		if (wallHeightMap != 0)
		{
			glActiveTexture(GL_TEXTURE0 + texCount);
			glBindTexture(GL_TEXTURE_2D, wallHeightMap);
			shader.SetInt("material.height[0]", texCount);
			shader.SetBool("bHasHeightMap", true);
			texCount++;
		}
	}
	else
	{
		if (diffTexture != 0)
		{
			glActiveTexture(GL_TEXTURE0 + texCount);
			glBindTexture(GL_TEXTURE_2D, diffTexture);
			shader.SetInt("material.diffuse[0]", texCount);
			texCount++;
		}

		if (specTexture != 0)
		{
			glActiveTexture(GL_TEXTURE0 + texCount);
			glBindTexture(GL_TEXTURE_2D, specTexture);
			shader.SetInt("material.specular[0]", texCount);
			//shader.SetInt("material.normal[0]", 0);
			texCount++;
		}
	}
	if (dirLightDepthMapTex != 0)
	{
		glActiveTexture(GL_TEXTURE0 + texCount);
		glBindTexture(GL_TEXTURE_2D, dirLightDepthMapTex);
		shader.SetInt("dirLight_ShadowMap", texCount);
		texCount++;		
	}

	if (pointLightDepthCubemaps != nullptr)
	{
		for (int i = 0; i < (*pointLightDepthCubemaps).size(); i++)
		{
			glActiveTexture(GL_TEXTURE0 +5+ texCount);
			glBindTexture(GL_TEXTURE_CUBE_MAP, (*pointLightDepthCubemaps)[i]);
			char num[3];
			_itoa_s(i, num, 10);

			shader.SetInt(std::string("pointLight_ShadowMaps[") + num + "]", 5 + texCount);

			texCount++;
		}
	}

	for (int i = 0; i < cubeModelMatrices.size(); i++)
	{
		glm::mat4 model = cubeModelMatrices[i];
		model = glm::scale(model, scale);
		shader.SetMat4("model", model);
		glDrawArrays(GL_TRIANGLES, 0, 36);
	}
	

	glBindVertexArray(0);

	shader.SetBool("bStencil", false);
	shader.SetBool("bHasNormalMap", false);
	shader.SetBool("bHasHeightMap", false);
}

void DrawFloor(Shader & shader, GLuint VAO,
	GLuint diffTexture, GLuint specTexture,
	GLuint wallTexture, GLuint wallNormalMap, GLuint wallHeightMap,
	GLuint dirLightDepthMapTex,
	const std::vector<GLuint>* pointLightDepthCubemaps)
{
	//glDisable(GL_CULL_FACE);

	shader.UseProgram();

	glBindVertexArray(VAO);

	GLuint texCount = 0;
	if (settings.bUseNormalMap || settings.bUseParallaxMapping)
	{
		if (wallTexture != 0)
		{
			glActiveTexture(GL_TEXTURE0 + texCount);
			glBindTexture(GL_TEXTURE_2D, wallTexture);
			shader.SetInt("material.diffuse[0]", texCount);
			texCount++;
		}

		if (wallNormalMap != 0)
		{
			glActiveTexture(GL_TEXTURE0 + texCount);
			glBindTexture(GL_TEXTURE_2D, wallNormalMap);
			shader.SetInt("material.normal[0]", texCount);
			//shader.SetInt("material.specular[0]", 0);
			shader.SetBool("bHasNormalMap", true);
			texCount++;
		}

		if (wallHeightMap != 0)
		{
			glActiveTexture(GL_TEXTURE0 + texCount);
			glBindTexture(GL_TEXTURE_2D, wallHeightMap);
			shader.SetInt("material.height[0]", texCount);
			shader.SetBool("bHasHeightMap", true);
			texCount++;
		}
	}
	else
	{
		if (diffTexture != 0)
		{
			glActiveTexture(GL_TEXTURE0 + texCount);
			glBindTexture(GL_TEXTURE_2D, diffTexture);
			shader.SetInt("material.diffuse[0]", texCount);
			texCount++;
		}

		if (specTexture != 0)
		{
			glActiveTexture(GL_TEXTURE0 + texCount);
			glBindTexture(GL_TEXTURE_2D, specTexture);
			shader.SetInt("material.specular[0]", texCount);
			//shader.SetInt("material.normal[0]", 0);
			texCount++;
		}
	}

	if (dirLightDepthMapTex != 0)
	{
		glActiveTexture(GL_TEXTURE0 + texCount);
		glBindTexture(GL_TEXTURE_2D, dirLightDepthMapTex);
		shader.SetInt("dirLight_ShadowMap", texCount);
		texCount++;		
	}

	if (pointLightDepthCubemaps != nullptr)
	{
		for (int i = 0; i < (*pointLightDepthCubemaps).size(); i++)
		{
			glActiveTexture(GL_TEXTURE0 + 5 + texCount);
			glBindTexture(GL_TEXTURE_CUBE_MAP, (*pointLightDepthCubemaps)[i]);
			char num[3];
			_itoa_s(i, num, 10);

			shader.SetInt(std::string("pointLight_ShadowMaps[") + num + "]", 5 + texCount);

			texCount++;
		}
	}

	shader.SetMat4("model", glm::mat4(1.0f));
	glDrawArrays(GL_TRIANGLES, 0, 6);

	glBindVertexArray(0);

	shader.SetBool("bHasNormalMap", false);
	shader.SetBool("bHasHeightMap", false);
}

void SetKernelValue(Shader & postProcShader, GLint cellID)
{
	char num[3];
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
	GLuint textureColorbuffer, GLuint textureColorbufferMS, GLuint screenBlitTexture)
{
	postProcShader.UseProgram();

	glBindVertexArray(postProcVAO);

	glActiveTexture(GL_TEXTURE0);
	if(!settings.bBlit)
		glBindTexture(GL_TEXTURE_2D, textureColorbuffer);	// use the color attachment texture as the texture of the quad plane
	else
		glBindTexture(GL_TEXTURE_2D, screenBlitTexture);
	postProcShader.SetInt("screenTexture", 0);

	if (settings.bAntiAliasing)
	{
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, textureColorbufferMS);
		postProcShader.SetInt("screenTextureMS", 1);

		if (settings.bBlit)
		{

		}
	}

	if (bPPModelChanged)
	{
		postProcShader.SetMat4("model", postProcModel);
		bPPModelChanged = false;
	}
	
	if (settings.bPostProcess)
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

void DrawHDR(Shader & HDR_Shader, GLuint postProcVAO,
	GLuint colorBuffers[], GLuint pingpongColorsBuffers[])
{
	HDR_Shader.UseProgram();

	if (bPPModelChanged)
	{
		HDR_Shader.SetMat4("model", postProcModel);
		bPPModelChanged = false;
	}

	glBindVertexArray(postProcVAO);

	glActiveTexture(GL_TEXTURE0);
	
	glBindTexture(GL_TEXTURE_2D, colorBuffers[0]);	// use the color attachment texture as the texture of the quad plane
	
	HDR_Shader.SetInt("screenTexture", 0);

	if (settings.bBloom)
	{
		HDR_Shader.SetBool("bBloom", true);

		glActiveTexture(GL_TEXTURE1);

		glBindTexture(GL_TEXTURE_2D, pingpongColorsBuffers[0]);	// use the color attachment texture as the texture of the quad plane

		HDR_Shader.SetInt("bloomBlur", 1);
	}
	
	glDrawArrays(GL_TRIANGLES, 0, 6);

	glBindVertexArray(0);

	HDR_Shader.SetBool("bBloom", false);
}

void GenScreenQuadVAO(GLuint& screenQuadVAO, GLuint& screenQuadVBO)
{
	//have to be written in CW order
	float screenQuadVertices[] = { // vertex attributes for a quad that fills the entire screen in Normalized Device Coordinates.
		// positions   // texCoords
		-1.0f, -1.0f,  0.0f, 0.0f,
		-1.0f,  1.0f,  0.0f, 1.0f,
		 1.0f, -1.0f,  1.0f, 0.0f,

		 1.0f, -1.0f,  1.0f, 0.0f,
		-1.0f,  1.0f,  0.0f, 1.0f,
		 1.0f,  1.0f,  1.0f, 1.0f
	};

	glGenVertexArrays(1, &screenQuadVAO);
	glGenBuffers(1, &screenQuadVBO);
	glBindVertexArray(screenQuadVAO);
	glBindBuffer(GL_ARRAY_BUFFER, screenQuadVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(screenQuadVertices), &screenQuadVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
	glBindVertexArray(0);
}

void DrawBlurQuad(Shader & blurShader, GLuint postProcVAO,
	GLuint colorBuffer)
{
	blurShader.UseProgram();	

	glBindVertexArray(postProcVAO);

	glActiveTexture(GL_TEXTURE0);

	glBindTexture(GL_TEXTURE_2D, colorBuffer);	// use the color attachment texture as the texture of the quad plane

	blurShader.SetInt("image", 0);

	glDrawArrays(GL_TRIANGLES, 0, 6);

	glBindVertexArray(0);
}

GLuint loadCubemap(const vector<std::string>& faces, bool bSRGB)
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
			GLenum internalFormat = bSRGB ? GL_SRGB : GL_RGB;
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
				0, internalFormat, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
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
	// ... задание видовой и проекционной матриц

	shader.SetMat4("view", view);
	shader.SetMat4("projection", projection);

	glBindVertexArray(VAO);

	glActiveTexture(GL_TEXTURE31);
	glBindTexture(GL_TEXTURE_CUBE_MAP, texture);
	shader.SetInt("skybox", 31);

	glDrawArrays(GL_TRIANGLES, 0, 36);

	glBindVertexArray(0);
	
}

void DrawReflectCube(Shader & shader, GLuint VAO,
	GLuint cubeMapTexture, GLuint cubeDiffTexture, GLuint cubeSpecTexture, glm::vec3 cameraPos)
{
	shader.UseProgram();

	shader.SetBool("bReflect", true);	

	shader.SetVec3("cameraPos", cameraPos);

	glBindVertexArray(VAO);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, cubeDiffTexture);
	shader.SetInt("material.diffuse[0]", 0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, cubeSpecTexture);
	shader.SetInt("material.specular[0]", 1);

	//glEnable(GL_TEXTURE_CUBE_MAP);
	glActiveTexture(GL_TEXTURE31);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapTexture);
	shader.SetInt("skybox", 31);
	
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(2.0f, 5.0f, 0.0f));	
	shader.SetMat4("model", model);
	glDrawArrays(GL_TRIANGLES, 0, 36);

	glBindVertexArray(0);

	shader.SetBool("bReflect", false);
	
	glEnable(GL_TEXTURE_2D);
}

void DrawRefractCube(Shader & shader, GLuint VAO, GLuint cubeMapTexture, glm::vec3 cameraPos)
{
	shader.UseProgram();

	shader.SetBool("bRefract", true);
	
	shader.SetVec3("cameraPos", cameraPos);

	glBindVertexArray(VAO);	

	//glEnable(GL_TEXTURE_CUBE_MAP);
	glActiveTexture(GL_TEXTURE31);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapTexture);
	shader.SetInt("skybox", 31);

	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, 5.0f, 0.0f));
	shader.SetMat4("model", model);
	glDrawArrays(GL_TRIANGLES, 0, 36);

	glBindVertexArray(0);

	shader.SetBool("bRefract", false);

	glEnable(GL_TEXTURE_2D);
}

void GenCubeVAO(GLuint& cubeVAO, GLuint& cubeVBO, GLuint& tanVBO, GLuint& bitanVBO)
{
	// set up vertex data (and buffer(s)) and configure vertex attributes
	// ------------------------------------------------------------------
	float cubeVertices[] = {
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
	glGenBuffers(1, &cubeVBO);
	glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), &cubeVertices, GL_STATIC_DRAW);

	glGenVertexArrays(1, &cubeVAO);
	
	glBindVertexArray(cubeVAO);	
	
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);//pos
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));//uv
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));//norm

	glBindVertexArray(0);

	glm::vec3 cubeTangents[36];
	glm::vec3 cubeBitangents[36];
	//float cubeTangents[108];
	//float cubeBiangents[108];

	int verticesSize = sizeof(cubeVertices) / sizeof(float);
	int posOffset = 0;
	int uvOffset = 3;
	int step = 8;
	int vertsPerFace = 6;

	GenTangentsAndBitangents(cubeVertices,
		verticesSize, step, posOffset, uvOffset, vertsPerFace,
		cubeTangents, cubeBitangents);

	GLsizei arrSize = sizeof(cubeTangents);
	AddTangentsAndBitangentsAttributes(cubeVAO, tanVBO, bitanVBO,
		cubeTangents, cubeBitangents, arrSize);
	
}



void GenPlaneVAO(GLuint& planeVAO, GLuint& planeVBO, GLuint& tanVBO, GLuint& bitanVBO)
{
	float planeVertices[] = {
		// positions			// texture Coords		//norm
		 50.0f, -0.5f,  50.0f,	20.0f, 0.0f,			0.0f,  1.0f,  0.0f,
		-50.0f, -0.5f,  50.0f,  0.0f, 0.0f,				0.0f,  1.0f,  0.0f,
		-50.0f, -0.5f, -50.0f,  0.0f, 20.0f,			0.0f,  1.0f,  0.0f,

		 50.0f, -0.5f,  50.0f,  20.0f, 0.0f,			0.0f,  1.0f,  0.0f,
		-50.0f, -0.5f, -50.0f,  0.0f, 20.0f,			0.0f,  1.0f,  0.0f,
		 50.0f, -0.5f, -50.0f,  20.0f, 20.0f,			0.0f,  1.0f,  0.0f
	};

	glGenVertexArrays(1, &planeVAO);
	glGenBuffers(1, &planeVBO);
	glBindVertexArray(planeVAO);
	glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), &planeVertices, GL_STATIC_DRAW);
	
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);//pos
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));//uv
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));//norm

	glBindVertexArray(0);

	glm::vec3 tangents[6];
	glm::vec3 bitangents[6];
	//float cubeTangents[108];
	//float cubeBiangents[108];

	int verticesSize = sizeof(planeVertices) / sizeof(float);
	int posOffset = 0;
	int uvOffset = 3;
	int step = 8;
	int vertsPerFace = 6;

	GenTangentsAndBitangents(planeVertices,
		verticesSize, step, posOffset, uvOffset, vertsPerFace,
		tangents, bitangents);

	GLsizei arrSize = sizeof(tangents);
	AddTangentsAndBitangentsAttributes(planeVAO, tanVBO, bitanVBO,
		tangents, bitangents, arrSize);
}

void GenTangentsAndBitangents(float vertices[],
	int verticesSize, int verticesStep, int posOffset, int uvOffset, int vertsPerFace,
	glm::vec3 tangents[], glm::vec3 bitangents[])
{
	for (int vertID = 0; vertID < verticesSize; vertID += verticesStep * vertsPerFace)
	{
		int posX_Index = vertID + posOffset;
		int uvX_Index = vertID + uvOffset;
		glm::vec3 positions[3];
		glm::vec2 UVs[3];

		for (int i = 0; i < 3; i++)
		{
			positions[i] = glm::vec3(
				vertices[posX_Index + 0],
				vertices[posX_Index + 1],
				vertices[posX_Index + 2]);

			UVs[i] = glm::vec2(
				vertices[uvX_Index + 0],
				vertices[uvX_Index + 1]);

			posX_Index += verticesStep;
			uvX_Index += verticesStep;
		}

		glm::vec3 edge1 = positions[1] - positions[0];
		glm::vec3 edge2 = positions[2] - positions[0];
		glm::vec2 deltaUV1 = UVs[1] - UVs[0];
		glm::vec2 deltaUV2 = UVs[2] - UVs[0];

		float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

		glm::vec3 tangent;
		glm::vec3 bitangent;

		tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
		tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
		tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
		tangent = glm::normalize(tangent);

		bitangent.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
		bitangent.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
		bitangent.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);
		bitangent = glm::normalize(bitangent);

		int faceID = vertID / verticesStep / vertsPerFace;
		/*int tanX_Index = faceID * vertsPerFace * 3;
		for (int vertCount = 0; vertCount < vertsPerFace; vertCount++)
		{
			for (int i = 0; i < 3; i++)
			{
				tangents[tanX_Index + i] = tangent[i];
				bitangents[tanX_Index + i] = bitangent[i];
			}
			tanX_Index += 3;
		}*/
		int tan_Index = faceID * vertsPerFace;
		for (int vertCount = 0; vertCount < vertsPerFace; vertCount++)
		{			
			tangents[tan_Index + vertCount] = tangent;
			bitangents[tan_Index + vertCount] = bitangent;
		}
	}
}

void AddTangentsAndBitangentsAttributes(GLuint& VAO, GLuint& tanVBO, GLuint& bitanVBO,
	glm::vec3 tangents[], glm::vec3 bitangents[], GLsizei arrSize)
{
	glBindVertexArray(VAO);


	glGenBuffers(1, &tanVBO);
	glBindBuffer(GL_ARRAY_BUFFER, tanVBO);
	glBufferData(GL_ARRAY_BUFFER, arrSize, &tangents[0], GL_STATIC_DRAW);

	// настройка атрибутов
	GLsizei vec3Size = sizeof(glm::vec3);
	glEnableVertexAttribArray(4);
	glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, vec3Size, (void*)0);

	glGenBuffers(1, &bitanVBO);
	glBindBuffer(GL_ARRAY_BUFFER, bitanVBO);
	glBufferData(GL_ARRAY_BUFFER, arrSize, &bitangents[0], GL_STATIC_DRAW);

	glEnableVertexAttribArray(5);
	glVertexAttribPointer(5, 3, GL_FLOAT, GL_FALSE, vec3Size, (void*)0);


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




void UpdateVAO(Model & model, GLuint amount, glm::mat4 *modelMatrices, GLuint& modelMatricesVBO)
{
	// создаем VBO	
	glGenBuffers(1, &modelMatricesVBO);
	glBindBuffer(GL_ARRAY_BUFFER, modelMatricesVBO);
	glBufferData(GL_ARRAY_BUFFER, amount * sizeof(glm::mat4), &modelMatrices[0], GL_STATIC_DRAW);

	for (unsigned int i = 0; i < model.meshes.size(); i++)
	{
		unsigned int VAO = model.meshes[i].VAO;
		glBindVertexArray(VAO);
		// настройка атрибутов
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

	srand(glfwGetTime()); // задаем seed дл€ генератора случ. чисел
	float radius = 50.0;
	float offset = 2.5f;
	for (unsigned int i = 1; i < amount; i++)
	{
		glm::mat4 model(1.0f);
		// 1. перенос: расположить вдоль окружности радиусом 'radius' 
		// и добавить смещение в пределах [-offset, offset]
		float angle = (float)i / (float)amount * 360.0f;
		float displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
		float x = sin(angle) * radius + displacement;
		displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
		// высоту пол€ держим заметно меньшей, чем размеры в плоскости XZ
		float y = displacement * 0.4f;
		displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
		float z = cos(angle) * radius + displacement;
		model = glm::translate(model, glm::vec3(x, y, z));

		// 2. масштабирование: случайное масштабирование в пределах (0.05, 0.25f)
		float scale = (rand() % 20) / 100.0f + 0.05;
		model = glm::scale(model, glm::vec3(scale));

		// 3. поворот: поворот на случайный угол вдоль 
		float rotAngle = (rand() % 360);
		model = glm::rotate(model, rotAngle, glm::vec3(0.4f, 0.6f, 0.8f));

		// 4. добавл€ем в массив матриц
		modelMatrices[i] = model;
	}
}

void SetupFrameBuffer(GLuint& framebuffer, GLuint& colorBuffer, GLuint& renderBuffer)
{
	glGenFramebuffers(1, &framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

	glGenTextures(1, &colorBuffer);
	glBindTexture(GL_TEXTURE_2D, colorBuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorBuffer, 0);
	//glDrawBuffer(GL_COLOR_ATTACHMENT0);

	// create a renderbuffer object for depth and stencil attachment (we won't be sampling these)
	//GLuint rbo;
	glGenRenderbuffers(1, &renderBuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, renderBuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, SCR_WIDTH, SCR_HEIGHT); // use a single renderbuffer object for both a depth AND stencil buffer.
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, renderBuffer); // now actually attach it

	GLenum Status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

	if (Status != GL_FRAMEBUFFER_COMPLETE) {
		cout << "ERROR::FRAMEBUFFER::" << Status << endl;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

}

void SetupFrameBufferMS(GLuint& framebuffer, GLuint& colorBufferMS, GLuint& renderBufferMS, GLint samples,
	GLuint& intermediateFBO, GLuint& screenTexture)
{
	glGenFramebuffers(1, &framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

	glGenTextures(1, &colorBufferMS);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, colorBufferMS);
	glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, GL_RGB, SCR_WIDTH, SCR_HEIGHT, GL_TRUE);
	glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, colorBufferMS, 0);

	//glDrawBuffer(GL_COLOR_ATTACHMENT1);

	
	glGenRenderbuffers(1, &renderBufferMS);
	glBindRenderbuffer(GL_RENDERBUFFER, renderBufferMS);
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_DEPTH24_STENCIL8, SCR_WIDTH, SCR_HEIGHT);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, renderBufferMS); // now actually attach it

	GLenum Status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

	if (Status != GL_FRAMEBUFFER_COMPLETE) {
		cout << "ERROR::FRAMEBUFFER::" << Status << endl;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);



	//intermediateFBO setup
	glGenFramebuffers(1, &intermediateFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, intermediateFBO);

	glGenTextures(1, &screenTexture);
	glBindTexture(GL_TEXTURE_2D, screenTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, screenTexture, 0);	// we only need a color buffer

	Status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

	if (Status != GL_FRAMEBUFFER_COMPLETE) {
		cout << "ERROR::FRAMEBUFFER::" << Status << endl;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

}

void AddDirectedLight()
{
	lights.directionalLight.direction = glm::vec4(-0.2f, -1.0f, -0.3f, 0.0f);
	lights.directionalLight.ambient = glm::vec4(0.15f, 0.15f, 0.15f, 0.0f);
	lights.directionalLight.diffuse = glm::vec4(0.5f, 0.5f, 0.5f, 0.0f); // darken the light a bit to fit the scene
	lights.directionalLight.specular = glm::vec4(0.5f, 0.5f, 0.5f, 0.0f);	

	lights.UpdateLights();
}

void AddPointLights(std::vector<PointLight>& pointLights, std::vector<glm::mat4>& pointLightsModelMatrices)
{
	pointLights.push_back(
		PointLight(
			glm::vec4(1.2f, 1.0f, 2.0f, 0.0f),
			glm::vec4(0.1f, 0.1f, 0.1f, 1.0f),
			glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
			//glm::vec4(20.0f, 20.0f, 20.0f, 20.0f),
			glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)));
	pointLights.push_back(
		PointLight(
			glm::vec4(2.3f, -2.3f, -2.0f, 0.0f),
			glm::vec4(0.1f, 0.0f, 0.0f, 1.0f),
			glm::vec4(1.0f, 0.0f, 0.0f, 1.0f),
			glm::vec4(1.0f, 0.0f, 0.0f, 1.0f)));
	pointLights.push_back(
		PointLight(
			glm::vec4(-2.0f, 2.0f, -2.0f, 0.0f),
			glm::vec4(0.0f, 0.1f, 0.0f, 1.0f),
			glm::vec4(0.0f, 10.0f, 0.0f, 1.0f),
			glm::vec4(0.0f, 1.0f, 0.0f, 1.0f)));
	pointLights.push_back(
		PointLight(
			glm::vec4(0.0f, 0.0f, -2.0f, 0.0f),
			glm::vec4(0.0f, 0.0f, 0.1f, 1.0f),
			glm::vec4(0.0f, 0.0f, 1.0f, 1.0f),
			glm::vec4(0.0f, 0.0f, 1.0f, 1.0f)));

	for (GLint i = 0; i < pointLights.size(); i++)
	{
		pointLightsModelMatrices.push_back(glm::mat4());
	}

	SetPointLights(pointLights);
}

void SetPointLights(std::vector<PointLight>& pointLights)
{	
	for (GLint i = 0; i < pointLights.size(); i++)
	{
		lights.pointLights[i] = pointLights[i];		
	}
	for (GLint i = pointLights.size(); i < NUM_POINT_LIGHTS; i++)
	{
		lights.pointLights[i].bEnabled = false;
	}

	lights.UpdateLights();
}

void AddSpotLight()
{
	lights.spotLight.cutOff = glm::cos(glm::radians(12.5f));
	lights.spotLight.outerCutOff = glm::cos(glm::radians(17.5f));

	lights.spotLight.ambient = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
	lights.spotLight.diffuse = glm::vec4(1.0f, 1.0f, 1.0f, 0.0f); // darken the light a bit to fit the scene
	lights.spotLight.specular = glm::vec4(1.0f, 1.0f, 1.0f, 0.0f);

	lights.UpdateLights();
}

void UpdateSpotLight()
{	
	lights.spotLight.position = glm::vec4(camera.Position, 0.0f);
	lights.spotLight.direction = glm::vec4(camera.Front, 0.0f);	

	lights.UpdateLights();
}

void UpdatePointLights(std::vector<PointLight>& pointLights,
	std::vector<glm::mat4>& pointLightsModelMatrices,
	Shader& lampShader, GLuint lampVAO)
{

	for (int lightId = 0; lightId < pointLights.size(); lightId++)
	{
		GLfloat axisCoef = 1.f / (pointLights.size()) * lightId;

		glm::mat4 model(1.0f);
		//model = glm::rotate(model, ((sin((GLfloat)glfwGetTime()) / 2) + 0.5f)/* * glm::radians(20.0f )*/,
			//glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::rotate(model, ((GLfloat)glfwGetTime() * glm::radians(10.0f*(lightId + 1))),
			glm::vec3(axisCoef, 1.0f - axisCoef, axisCoef));

		glm::vec3 lampPosition = pointLights[lightId].position;

		//rotate lamp position before model's translation and rotation
		glm::vec4 transLightPos = model * glm::vec4(lampPosition, 1.f);

		model = glm::translate(model, lampPosition);
		model = glm::scale(model, glm::vec3(0.2f));
		
		pointLightsModelMatrices[lightId] = model;		

		//Set light position
		lights.pointLights[lightId].position = transLightPos;
		//pointLights[lightId].currPosition = transLightPos;
	}

	lights.UpdateLights();

	DrawPointLights(pointLights, pointLightsModelMatrices,
		lampShader, lampVAO);
}

void DrawPointLights(std::vector<PointLight>& pointLights, 
	std::vector<glm::mat4>& pointLightsModelMatrices,
	Shader& lampShader, GLuint lampVAO)
{
	for (int lightId = 0; lightId < pointLights.size(); lightId++)
	{
		glm::mat4 model = pointLightsModelMatrices[lightId];

		//Draw Lamp
		lampShader.UseProgram();

		lampShader.SetMat4("model", model);
		lampShader.SetBool("bStencil", true);
		glm::vec4 lampColor = glm::vec4(pointLights[lightId].diffuse/*, 1.0*/);
		lampShader.SetVec4("borderColor", lampColor);


		glBindVertexArray(lampVAO);
		glDrawArrays(GL_TRIANGLES, 0, 36);
		glBindVertexArray(0);

		lampShader.SetBool("bStencil", false);	
	}
}

GLsizeiptr CalcStructSizeUBO(GLsizeiptr structSize)
{
	GLsizeiptr vec4Size = sizeof(glm::vec4);
	GLfloat vec4NumF = (GLfloat)structSize / (GLfloat)vec4Size;
	GLfloat vec4NumI = (GLfloat)((GLint)vec4NumF);

	//
	if (vec4NumI < vec4NumF)//struct Lights is not multiple of glm::vec4
	{
		structSize = sizeof(glm::vec4) * (vec4NumI + 1);
	}

	return structSize;
}

void SetupDirLightFBO(GLuint& dirLightFBO, GLuint& dirLightDepthMapTex)
{
	glGenFramebuffers(1, &dirLightFBO);

		
	glGenTextures(1, &dirLightDepthMapTex);
	glBindTexture(GL_TEXTURE_2D, dirLightDepthMapTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
		DIR_SHADOW_WIDTH, DIR_SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

	glBindFramebuffer(GL_FRAMEBUFFER, dirLightFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, dirLightDepthMapTex, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);

	GLenum Status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

	if (Status != GL_FRAMEBUFFER_COMPLETE) {
		cout << "ERROR::FRAMEBUFFER::DirLightFBO" << Status << endl;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);


}

void DrawSceneForDirShadows(Shader & dirLightDepthShader,
	GLuint cubeVAO, GLuint planeVAO, const std::vector<glm::mat4>& cubeModelMatrices)
{
	glClear(GL_DEPTH_BUFFER_BIT);

	GLfloat near_plane = 1.0f, far_plane = 7.5f;
	glm::mat4 lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, near_plane, far_plane);	

	glm::vec4 position = -lights.directionalLight.direction * 5.f;

	glm::mat4 lightView = glm::lookAt(
		glm::vec3(position),	//cam pos
		glm::vec3(lights.directionalLight.direction),	//cam dir
		glm::vec3(0.0f, 1.0f, 0.0f));					//up vec


	lightSpaceMatrices.dirLightSpaceMatrix = lightProjection * lightView;

	dirLightDepthShader.UseProgram();

	dirLightDepthShader.SetMat4("dirLightSpaceMatrix", lightSpaceMatrices.dirLightSpaceMatrix);


	//Draw cubes
	DrawCubes(dirLightDepthShader, cubeVAO, cubeModelMatrices);

	//draw floor
	DrawFloor(dirLightDepthShader, planeVAO);
}

void FillModelMatrices(std::vector<glm::mat4>& cubeModelMatrices)
{
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(-1.0f, 0.0f, -1.0f));
	cubeModelMatrices.push_back(model);

	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(2.0f, 0.0f, 0.0f));
	cubeModelMatrices.push_back(model);

	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(2.0f, 0.0f, 2.0f));
	model = glm::rotate(model, glm::radians(45.f), glm::vec3(1.0f));
	model = glm::scale(model, glm::vec3(0.5f));
	cubeModelMatrices.push_back(model);

	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(-1.0f, 2.0f, 2.0f));
	model = glm::rotate(model, glm::radians(30.f), glm::vec3(1.0f));
	model = glm::scale(model, glm::vec3(1.3f));
	cubeModelMatrices.push_back(model);
	
}


void SetupPointLightsFBOs(std::vector<GLuint>& pointLightFBOs, std::vector<GLuint>& pointLightDepthCubemaps,
	GLuint buffersNum)
{
	for (GLuint i = 0; i < buffersNum; i++)
	{
		pointLightFBOs.push_back(0);
		pointLightDepthCubemaps.push_back(0);


		glGenFramebuffers(1, &pointLightFBOs[i]);

		glGenTextures(1, &pointLightDepthCubemaps[i]);
		glBindTexture(GL_TEXTURE_CUBE_MAP, pointLightDepthCubemaps[i]);

		for (unsigned int j = 0; j < CUBE_FACES; ++j)
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + j, 0, GL_DEPTH_COMPONENT,
				POINT_SHADOW_WIDTH, POINT_SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

		glBindFramebuffer(GL_FRAMEBUFFER, pointLightFBOs[i]);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, pointLightDepthCubemaps[i], 0);
		glDrawBuffer(GL_NONE);
		glReadBuffer(GL_NONE);

		GLenum Status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

		char num[3];
		_itoa_s(i, num, 10);

		if (Status != GL_FRAMEBUFFER_COMPLETE) {
			cout << "ERROR::FRAMEBUFFER::PointLightFBO["<< num << "]" << Status << endl;
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
}

void SetShaderValues(Shader & pointLightDepthShader, GLuint pointLightID)
{
	pointLightDepthShader.UseProgram();

	pointLightDepthShader.SetVec3("lightPos", lights.pointLights[pointLightID].position);

	for (GLuint i = 0; i < CUBE_FACES; i++)
	{
		char num[3];
		_itoa_s(i, num, 10);
		
		//set matrices
		pointLightDepthShader.SetMat4(std::string("shadowMatrices[") + num + "]",
			lightSpaceMatrices.pointLightSpaceMatrices[pointLightID][i]);
	}
}

void DrawSceneForPointShadows(Shader & pointLightDepthShader,
	const std::vector<GLuint>& pointLightFBOs, const glm::mat4& pointShadowProj, GLuint pointLightsNum,
	GLuint cubeVAO, GLuint planeVAO, const std::vector<glm::mat4>& cubeModelMatrices)
{
	for (GLuint i = 0; i < pointLightsNum; i++)
	{
		std::vector<glm::mat4> matrices = { glm::mat4(), glm::mat4(), glm::mat4(),
											glm::mat4(), glm::mat4(), glm::mat4() };

		lightSpaceMatrices.pointLightSpaceMatrices.push_back(matrices);

		glm::vec3 lightPos = glm::vec3(lights.pointLights[i].position);

		lightSpaceMatrices.pointLightSpaceMatrices[i][0] = (pointShadowProj *
			glm::lookAt(lightPos, lightPos + glm::vec3(1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0)));

		lightSpaceMatrices.pointLightSpaceMatrices[i][1] = (pointShadowProj *
			glm::lookAt(lightPos, lightPos + glm::vec3(-1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0)));

		lightSpaceMatrices.pointLightSpaceMatrices[i][2] = (pointShadowProj *
			glm::lookAt(lightPos, lightPos + glm::vec3(0.0, 1.0, 0.0), glm::vec3(0.0, 0.0, 1.0)));

		lightSpaceMatrices.pointLightSpaceMatrices[i][3] = (pointShadowProj *
			glm::lookAt(lightPos, lightPos + glm::vec3(0.0, -1.0, 0.0), glm::vec3(0.0, 0.0, -1.0)));

		lightSpaceMatrices.pointLightSpaceMatrices[i][4] = (pointShadowProj *
			glm::lookAt(lightPos, lightPos + glm::vec3(0.0, 0.0, 1.0), glm::vec3(0.0, -1.0, 0.0)));

		lightSpaceMatrices.pointLightSpaceMatrices[i][5] = (pointShadowProj *
			glm::lookAt(lightPos, lightPos + glm::vec3(0.0, 0.0, -1.0), glm::vec3(0.0, -1.0, 0.0)));

		glBindFramebuffer(GL_FRAMEBUFFER, pointLightFBOs[i]);

		glClear(GL_DEPTH_BUFFER_BIT);

		glCullFace(GL_BACK);
		
		SetShaderValues(pointLightDepthShader, i);

		//Draw cubes
		DrawCubes(pointLightDepthShader, cubeVAO, cubeModelMatrices);

		//draw floor
		DrawFloor(pointLightDepthShader, planeVAO);

		glCullFace(GL_FRONT);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

}


void SetupHDRFrameBuffer(GLuint& framebuffer, GLuint colorBuffers[], GLuint& renderBuffer)
{
	glGenFramebuffers(1, &framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

	glGenTextures(2, colorBuffers);
	for (unsigned int i = 0; i < 2; i++)
	{
		glBindTexture(GL_TEXTURE_2D, colorBuffers[i]);
		glTexImage2D(
			GL_TEXTURE_2D, 0, GL_RGB16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_FLOAT, NULL
		);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		// присоединение текстуры к фреймбуферу
		glFramebufferTexture2D(
			GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, colorBuffers[i], 0
		);
	}

	unsigned int attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
	glDrawBuffers(2, attachments);	

	// create a renderbuffer object for depth and stencil attachment (we won't be sampling these)
	//GLuint rbo;
	glGenRenderbuffers(1, &renderBuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, renderBuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, SCR_WIDTH, SCR_HEIGHT); // use a single renderbuffer object for both a depth AND stencil buffer.
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, renderBuffer); // now actually attach it

	GLenum Status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

	if (Status != GL_FRAMEBUFFER_COMPLETE) {
		cout << "ERROR::FRAMEBUFFER::" << Status << endl;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

}

void SetupPingPongFBOs(GLuint pingpongFBOs[], GLuint pingpongColors[])
{
	glGenFramebuffers(2, pingpongFBOs);
	glGenTextures(2, pingpongColors);
	for (unsigned int i = 0; i < 2; i++)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBOs[i]);
		glBindTexture(GL_TEXTURE_2D, pingpongColors[i]);
		glTexImage2D(
			GL_TEXTURE_2D, 0, GL_RGB16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_FLOAT, NULL
		);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glFramebufferTexture2D(
			GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingpongColors[i], 0
		);		
		
		GLenum Status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

		if (Status != GL_FRAMEBUFFER_COMPLETE) {
			cout << "ERROR::FRAMEBUFFER::Ping-Pong[" << i << "]::" << Status << endl;
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	

}

void Setup_G_Buffer(GLuint& gBuffer,
	GLuint& gPosition, GLuint& gModelNormal, GLuint& gMaterialNormal,
	GLuint& gAlbedoSpec, GLuint& gTangent, GLuint& gBitangent,
	GLuint& gRenderBuffer)
{
	glGenFramebuffers(1, &gBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
	
	// буфер позиций
	glGenTextures(1, &gPosition);
	glBindTexture(GL_TEXTURE_2D, gPosition);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPosition, 0);

	// буфер нормалей
	glGenTextures(1, &gModelNormal);
	glBindTexture(GL_TEXTURE_2D, gModelNormal);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gModelNormal, 0);

	// буфер нормалей
	glGenTextures(1, &gMaterialNormal);
	glBindTexture(GL_TEXTURE_2D, gMaterialNormal);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gMaterialNormal, 0);

	// буфер дл€ цвета + коэффициента зеркального отражени€
	glGenTextures(1, &gAlbedoSpec);
	glBindTexture(GL_TEXTURE_2D, gAlbedoSpec);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, gAlbedoSpec, 0);

	glGenTextures(1, &gTangent);
	glBindTexture(GL_TEXTURE_2D, gTangent);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT4, GL_TEXTURE_2D, gTangent, 0);

	glGenTextures(1, &gBitangent);
	glBindTexture(GL_TEXTURE_2D, gBitangent);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT5, GL_TEXTURE_2D, gBitangent, 0);

	

	
	// укажем OpenGL, какие буферы мы будем использовать при рендеринге
	unsigned int attachments[6] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1,
		GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3, GL_COLOR_ATTACHMENT4, GL_COLOR_ATTACHMENT5 };
	glDrawBuffers(6, attachments);

	glGenRenderbuffers(1, &gRenderBuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, gRenderBuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, SCR_WIDTH, SCR_HEIGHT); // use a single renderbuffer object for both a depth AND stencil buffer.
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, gRenderBuffer); // now actually attach it

	GLenum Status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

	if (Status != GL_FRAMEBUFFER_COMPLETE) {
		cout << "ERROR::FRAMEBUFFER::" << Status << endl;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


void FillModelMatrices_DR(GLuint dimensions, GLfloat scale, glm::mat4 *modelMatrices, glm::mat4 *normalMatrices)
{
	glm::mat4 modelP(1.0f);
	modelP = glm::translate(modelP, glm::vec3(0.0f, -3.0f, 0.0f));
	modelP = glm::rotate(modelP, glm::radians(-90.f), glm::vec3(1.0f, 0.0f, 0.0f));
	modelP = glm::scale(modelP, glm::vec3(1.0f, 1.0f, 1.0f));

	modelMatrices[0] = modelP;

	srand(glfwGetTime()); // задаем seed дл€ генератора случ. чисел
	
	for ( int i = 0; i < dimensions; i++)
	{
		for (int j = 0; j < dimensions; j++)
		{
			glm::mat4 model(1.0f);

			model = glm::translate(model, glm::vec3((float)(i * scale), 0.f, -(float)(j * scale)));

			// 3. поворот: поворот на случайный угол вдоль 
			float rotAngle = (rand() % 360);
			model = glm::rotate(model, rotAngle, glm::vec3(0.4f, 0.6f, 0.8f));

			//glm::mat4 view = camera.GetViewMatrix();

			// 4. добавл€ем в массив матриц
			modelMatrices[i*dimensions + j] = /*view **/ model;

			
			//normalMatrices[i*dimensions + j] = glm::transpose(glm::inverse(view * model));
		}
	}
}

//* updating view and projection matrices
void UpdateMatrices(const GLuint& uboBlock)
{
	glm::mat4 view = camera.GetViewMatrix();
	glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 1000.0f);

	////////////////////////////////////////////UNIFORM BUFFER VALUES///////////////////////////////////////////////
	glBindBuffer(GL_UNIFORM_BUFFER, uboBlock);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), &view[0][0]);
	glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat4), &projection[0][0]);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void Gen_DR_VAO(GLuint& DR_VAO, GLuint& DR_VBO)
{
	//have to be written in CW order
	float DR_Vertices[] = { // vertex attributes for a quad that fills the entire screen in Normalized Device Coordinates.
		// positions   // texCoords
		-1.0f, -1.0f,  0.0f, 0.0f,
		-1.0f,  1.0f,  0.0f, 1.0f,
		 1.0f, -1.0f,  1.0f, 0.0f,

		 1.0f, -1.0f,  1.0f, 0.0f,
		-1.0f,  1.0f,  0.0f, 1.0f,
		 1.0f,  1.0f,  1.0f, 1.0f
	};

	glGenVertexArrays(1, &DR_VAO);
	glGenBuffers(1, &DR_VBO);
	glBindVertexArray(DR_VAO);
	glBindBuffer(GL_ARRAY_BUFFER, DR_VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(DR_Vertices), &DR_Vertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
	glBindVertexArray(0);
}

void Draw_DR_Quad(Shader & lightPassShader, GLuint DR_VAO,
	GLuint& gPosition, GLuint& gModelNormal, GLuint& gMaterialNormal,
	GLuint& gAlbedoSpec, GLuint& gTangent, GLuint& gBitangent,
	GLuint& ssaoColorBuffer)
{
	lightPassShader.UseProgram();

	glBindVertexArray(DR_VAO);

	glActiveTexture(GL_TEXTURE0);	
	glBindTexture(GL_TEXTURE_2D, gPosition);
	lightPassShader.SetInt("gPosition", 0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, gModelNormal);
	lightPassShader.SetInt("gModelNormal", 1);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, gMaterialNormal);
	lightPassShader.SetInt("gMaterialNormal", 2);

	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, gAlbedoSpec);
	lightPassShader.SetInt("gAlbedoSpec", 3);

	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, gTangent);
	lightPassShader.SetInt("gTangent", 4);

	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_2D, gBitangent);
	lightPassShader.SetInt("gBitangent", 5);

	glActiveTexture(GL_TEXTURE6);
	glBindTexture(GL_TEXTURE_2D, ssaoColorBuffer);
	lightPassShader.SetInt("ssaoColorBuffer", 6);
	
	lightPassShader.SetVec3("cameraPos", camera.Position);


	glDrawArrays(GL_TRIANGLES, 0, 6);

	glBindVertexArray(0);
}

void UpdateNormalMatricesVAO(Model & model, GLuint amount, glm::mat4 *normalMatrices, GLuint& normalMatricesVBO)
{
	// создаем VBO	
	glGenBuffers(1, &normalMatricesVBO);
	glBindBuffer(GL_ARRAY_BUFFER, normalMatricesVBO);
	glBufferData(GL_ARRAY_BUFFER, amount * sizeof(glm::mat4), &normalMatrices[0], GL_STATIC_DRAW);

	for (unsigned int i = 0; i < model.meshes.size(); i++)
	{
		unsigned int VAO = model.meshes[i].VAO;
		glBindVertexArray(VAO);
		// настройка атрибутов
		GLsizei vec4Size = sizeof(glm::vec4);
		glEnableVertexAttribArray(10);
		glVertexAttribPointer(10, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)0);
		glEnableVertexAttribArray(11);
		glVertexAttribPointer(11, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(vec4Size));
		glEnableVertexAttribArray(12);
		glVertexAttribPointer(12, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(2 * vec4Size));
		glEnableVertexAttribArray(13);
		glVertexAttribPointer(13, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(3 * vec4Size));

		glVertexAttribDivisor(10, 1);
		glVertexAttribDivisor(11, 1);
		glVertexAttribDivisor(12, 1);
		glVertexAttribDivisor(13, 1);

		glBindVertexArray(0);
	}
}

void AddPointLights_DR(std::vector<PointLight>& pointLights, std::vector<glm::mat4>& pointLightsModelMatrices,
	GLfloat areaSize)
{

	for (int i = 0; i < NUM_POINT_LIGHTS; i++)
	{
		float posX = rand() % ((int)areaSize * 10) / 10.f;
		float posZ = rand() % ((int)areaSize * 10) / 10.f;
		float colX = rand() % 100 / 100.f;
		float colY = rand() % 100 / 100.f;
		float colZ = rand() % 100 / 100.f;

		PointLight pointLight(
			glm::vec4(posX, 0.0f, -posZ, 0.0f),
			glm::vec4(0.3f*colX, 0.3f*colY, 0.3f*colZ, 1.0f),
			glm::vec4(colX, colY, colZ, 1.0f),
			glm::vec4(colX, colY, colZ, 1.0f));

		
		pointLight.constant = 1.0;
		pointLight.linear = 0.07;
		pointLight.quadratic = 0.08;

		pointLights.push_back(pointLight);

		float lightMax = 
			std::fmaxf(std::fmaxf(pointLight.diffuse.r, pointLight.diffuse.g), pointLight.diffuse.b);
		float radius =
			(-pointLight.linear + 
				std::sqrtf(pointLight.linear * pointLight.linear - 4 * pointLight.quadratic * 
				(pointLight.constant - (256.0 / 5.0) * lightMax)))
			/ (2 * pointLight.quadratic);

		pointLightRadiuses[i] = glm::vec4(radius);

	}

	UpdatePointLightRadiusUBO();

	for (GLint i = 0; i < pointLights.size(); i++)
	{
		pointLightsModelMatrices.push_back(glm::mat4());
	}

	SetPointLights(pointLights);
}

void UpdatePointLights_DR(std::vector<PointLight>& pointLights,
	std::vector<glm::mat4>& pointLightsModelMatrices,
	Shader& lampShader, GLuint lampVAO)
{

	for (int lightId = 0; lightId < pointLights.size(); lightId++)
	{
		GLfloat axisCoef = 1.f / (pointLights.size()) * lightId;

		glm::mat4 model(1.0f);

		glm::vec3 lampPosition = pointLights[lightId].position;		
		
		model = glm::translate(model, lampPosition);
		
		float angle = (GLfloat)glfwGetTime() * glm::radians(30.f);
		glm::vec3 axis = glm::vec3(axisCoef, 1 - axisCoef, axisCoef);
		model = glm::rotate(model, angle, axis);

		
		model = glm::translate(model, glm::vec3(0.0, 2.0, 0.0));
		model = glm::scale(model, glm::vec3(0.2f));

		glm::vec4 transLightPos = glm::vec4(
			 lampPosition + glm::rotate((glm::vec3(0.0, 2.0, 0.0)), angle, axis)
			, 1.f);
		//transLightPos = glm::vec4(5 * lightId, 2.0, -5 * lightId, 1.0);

		pointLightsModelMatrices[lightId] = model;

		//Set light position
		lights.pointLights[lightId].position = transLightPos;
		
	}

	lights.UpdateLights();

	DrawPointLights(pointLights, pointLightsModelMatrices,
		lampShader, lampVAO);
}

void Gen_AO_Kernel(std::vector<glm::vec3>& ssaoKernel)
{
	std::uniform_real_distribution<GLfloat> randomFloats(0.0, 1.0); // generates random floats between 0.0 and 1.0
	std::default_random_engine generator;
	for (unsigned int i = 0; i < 64; ++i)
	{
		glm::vec3 sample(
			randomFloats(generator) * 2.0 - 1.0,
			randomFloats(generator) * 2.0 - 1.0,
			randomFloats(generator)
		);
		sample = glm::normalize(sample);
		sample *= randomFloats(generator);
		float scale = float(i) / 64.0;

		// scale samples s.t. they're more aligned to center of kernel
		scale = lerp(0.1f, 1.0f, scale * scale);
		sample *= scale;
		ssaoKernel.push_back(sample);
	}
}

void Gen_AO_NoiseTexture(GLuint& noiseTexture)
{
	std::uniform_real_distribution<GLfloat> randomFloats(0.0, 1.0); // generates random floats between 0.0 and 1.0
	std::default_random_engine generator;

	// generate noise texture
	// ----------------------
	std::vector<glm::vec3> ssaoNoise;
	for (unsigned int i = 0; i < 16; i++)
	{
		// rotate around z-axis (in tangent space)
		glm::vec3 noise(
			randomFloats(generator) * 2.0 - 1.0,
			randomFloats(generator) * 2.0 - 1.0,
			0.0f);
		ssaoNoise.push_back(noise);
	}

	glGenTextures(1, &noiseTexture);
	glBindTexture(GL_TEXTURE_2D, noiseTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, 4, 4, 0, GL_RGB, GL_FLOAT, &ssaoNoise[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

void Gen_AO_FBO(GLuint& ssaoFBO, GLuint& ssaoColorBuffer)
{	
	glGenFramebuffers(1, &ssaoFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);

	glGenTextures(1, &ssaoColorBuffer);
	glBindTexture(GL_TEXTURE_2D, ssaoColorBuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoColorBuffer, 0);

	GLenum Status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

	if (Status != GL_FRAMEBUFFER_COMPLETE) {
		cout << "ERROR::FRAMEBUFFER::" << Status << endl;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Gen_AO_BlurFBO(GLuint& ssaoBlurFBO, GLuint& ssaoColorBufferBlur)
{
	glGenFramebuffers(1, &ssaoBlurFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO);

	glGenTextures(1, &ssaoColorBufferBlur);
	glBindTexture(GL_TEXTURE_2D, ssaoColorBufferBlur);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoColorBufferBlur, 0);

	GLenum Status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

	if (Status != GL_FRAMEBUFFER_COMPLETE) {
		cout << "ERROR::FRAMEBUFFER::" << Status << endl;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Draw_AO_Quad(Shader & AO_Shader, GLuint AO_VAO,
	GLuint& gPosition, GLuint& gNormal,
	GLuint& gTangent, GLuint& gBitangent,
	GLuint& noiseTexture, std::vector<glm::vec3>& ssaoKernel)
{
	AO_Shader.UseProgram();

	glBindVertexArray(AO_VAO);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gPosition);
	AO_Shader.SetInt("gPosition", 0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, gNormal);
	AO_Shader.SetInt("gNormal", 1);	

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, gTangent);
	AO_Shader.SetInt("gTangent", 2);

	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, gBitangent);
	AO_Shader.SetInt("gBitangent", 3);
	
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, noiseTexture);
	AO_Shader.SetInt("texNoise", 4);

	glm::mat4 projection = 
		glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 1000.0f);
	glm::mat4 view = camera.GetViewMatrix();

	AO_Shader.SetMat4("projection", projection);
	AO_Shader.SetMat4("view", view);
	
	for (int i = 0; i < ssaoKernel.size(); i++)
	{
		char num[4];
		_itoa_s(i, num, 10);
		AO_Shader.SetVec3(std::string("samples[") + num + "]", ssaoKernel[i]);
	}

	glDrawArrays(GL_TRIANGLES, 0, 6);

	glBindVertexArray(0);
}

void Draw_AO_Blur_Quad(Shader & AO_Shader, GLuint AO_VAO,
	GLuint& ssaoColorBuffer)
{
	AO_Shader.UseProgram();

	glBindVertexArray(AO_VAO);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, ssaoColorBuffer);
	AO_Shader.SetInt("ssaoInput", 0);

	
	glDrawArrays(GL_TRIANGLES, 0, 6);

	glBindVertexArray(0);
}

void Render_PBR_Spheres(Shader& PBR_Shader, 
	GLuint& albedo, GLuint& normal, GLuint& metallic, GLuint& roughness, GLuint& ao,
	GLint nrRows, GLint nrColumns, GLfloat spacing )
{
	PBR_Shader.UseProgram();	
	PBR_Shader.SetVec3("cameraPos", camera.Position);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, albedo);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, normal);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, metallic);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, roughness);
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, ao);

	// render rows*column number of spheres with material properties defined by textures (they all have the same material properties)
	glm::mat4 model = glm::mat4(1.0f);
	for (int row = 0; row < nrRows; ++row)
	{
		for (int col = 0; col < nrColumns; ++col)
		{
			model = glm::mat4(1.0f);
			model = glm::translate(model, glm::vec3(
				(float)(col - (nrColumns / 2)) * spacing,
				(float)(row - (nrRows / 2)) * spacing,
				0.0f
			));
			PBR_Shader.SetMat4("model", model);
			renderSphere();
		}
	}	
}

// renders (and builds at first invocation) a sphere
// -------------------------------------------------
unsigned int sphereVAO = 0;
unsigned int indexCount;
void renderSphere()
{
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
		const float PI = 3.14159265359;
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
		for (unsigned int y = 0; y < Y_SEGMENTS; ++y)
		{
			if (!oddRow) // even rows: y == 0, y == 2; and so on
			{
				for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
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
		indexCount = indices.size();

		std::vector<float> data;
		for (std::size_t i = 0; i < positions.size(); ++i)
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
		float stride = (3 + 2 + 3) * sizeof(float);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)(5 * sizeof(float)));
	}

	glBindVertexArray(sphereVAO);
	glDrawElements(GL_TRIANGLE_STRIP, indexCount, GL_UNSIGNED_INT, 0);
}