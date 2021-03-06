#version 330 core
out vec4 FragColor;
  
in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform sampler2DMS screenTextureMS;

uniform bool bUseKernel = false;
uniform float kernel[9];
uniform vec2 uvOffset;

uniform bool bAntiAliasing = false;
uniform bool bBlit = false;
uniform int samples = 0;

//screenTextureMS width and height
uniform vec2 dimensions;

layout (std140) uniform Settings
{
	bool bGammaCorrection;       //Post Proc shader do not needs gamma-correction
};


void main()
{ 
	gl_FragDepth = 0.0;

	vec4 col = vec4(0.0);
	if(!bUseKernel)
	{
		if(!bAntiAliasing)
			col = texture(screenTexture, TexCoords);
		else
		{		
			if(!bBlit)
			{
				//for texelFetch() we need texel coordinates
				ivec2 UV = ivec2(TexCoords*dimensions);			
				col = texelFetch(screenTextureMS, UV, 0);
	//			for(int i = 0; i < samples; i++)
	//			{
	//				col += texelFetch(screenTextureMS, UV, i);
	//			}
			}
			else
			{
				col = texture(screenTexture, TexCoords);
			}
		}
	}
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
	//Post Proc shader do not needs gamma-correction
//	if(bGammaCorrection)
//	{
//		float gamma = 2.2;
//		col = pow(col, vec4(1.0/gamma));
//	}
	FragColor = col;
}