#version 330 core
out vec4 FragColor;
  
in vec2 TexCoords;

uniform sampler2D screenTexture;

uniform bool bUseKernel;
uniform float kernel[9];
uniform vec2 uvOffset;

void main()
{ 
	vec4 col = vec4(0.0);
	if(!bUseKernel)
		col = texture(screenTexture, TexCoords);
	else
	{	
		int kernelID = 0;
		for(int i = -1; i <= 1; i++)
		{
			for(int j = -1; j <= 1; j++)
			{
				col += vec4(vec3(texture(screenTexture, TexCoords.st + vec2(uvOffset.x * j, uvOffset.y * i))) * kernel[kernelID], 1.0);
				kernelID++;				
			}
		}			
	}
	FragColor = col;
}