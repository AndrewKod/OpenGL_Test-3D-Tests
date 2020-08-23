#version 330 core

out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D texture1;

uniform bool bStencil = false;

void main()
{   
	if(bStencil)
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