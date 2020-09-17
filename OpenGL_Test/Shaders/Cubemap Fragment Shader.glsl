#version 330 core
out vec4 FragColor;

in vec3 TexCoords;

uniform samplerCube skybox;

layout (std140) uniform Settings
{
	bool bGammaCorrection;       
};


void main()
{    
	vec4 col = vec4(0.0);
    col = texture(skybox, TexCoords);

	if(bGammaCorrection)
	{
		float gamma = 2.2;
		col = pow(col, vec4(1.0/gamma));
	}
	FragColor = col;
}