#version 330 core

out vec4 FragColor;

in vec2 TexCoords;
in vec3 Normal;
in vec3 Position;

uniform sampler2D texture1;

uniform bool bStencil = false;

uniform bool bReflect = false;
uniform bool bRefract = false;

uniform vec3 cameraPos;
uniform samplerCube skybox;

void main()
{   
	/*if(bReflect || bRefract)
	{
		vec3 I = normalize(Position - cameraPos);
		vec3 R = reflect(I, normalize(Normal));
		FragColor = vec4(texture(skybox, R).rgb, 1.0);
	}
	else*/ if(bStencil)
	{
		FragColor = vec4(1.0, 1.0, 0.0, 1.0);
	}	
	else
	{
		vec4 texColor = texture(texture1,vec2(TexCoords.x, 1.0 - TexCoords.y));
		if(texColor.a < 0.1)
			discard;

		FragColor = texColor;
	}
}