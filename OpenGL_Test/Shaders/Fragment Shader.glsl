#version 330 core

out vec4 FragColor;

in vec2 TexCoords;
in vec3 Normal;
in vec3 Position;

uniform sampler2D texture1;
uniform samplerCube skybox;

uniform bool bStencil = false;
uniform vec4 borderColor;

uniform bool bReflect = false;
uniform bool bRefract = false;

uniform vec3 cameraPos;

layout (std140) uniform Settings
{
	bool bGammaCorrection;       
};

void main()
{   
	vec4 col = vec4(0.0);

	if(bReflect)
	{
		vec3 I = normalize(Position - cameraPos);
		vec3 R = reflect(I, normalize(Normal));
		col = vec4(texture(skybox, R).rgb, 1.0);
		/*col = texture(texture1,vec2(TexCoords.x, 1.0 - TexCoords.y));*/
	}
	else if(bRefract)
	{
		float ratio = 1.00 / 1.52;
		vec3 I = normalize(Position - cameraPos);
		vec3 R = refract(I, normalize(Normal), ratio);
		col = vec4(texture(skybox, R).rgb, 1.0);
	}
	else if(bStencil)
	{
		col = borderColor;
	}	
	else
	{
		vec4 texColor = texture(texture1,vec2(TexCoords.x, 1.0 - TexCoords.y));
		if(texColor.a < 0.1)
			discard;

		col = texColor;
	}

	if(bGammaCorrection)
	{
		float gamma = 2.2;
		col = pow(col, vec4(gamma));
	}
	FragColor = col;
}