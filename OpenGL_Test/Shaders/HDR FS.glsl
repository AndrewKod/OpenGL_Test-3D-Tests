#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;//hdrBuffer
uniform sampler2D bloomBlur;
uniform float exposure;

uniform bool bBloom = false;

layout (std140) uniform Settings
{
	bool bGammaCorrection;       
};

void main()
{
	gl_FragDepth = 0.0;

	const float gamma = 2.2;
    vec3 hdrColor = texture(screenTexture, TexCoords).rgb;

	if(bBloom)
	{
		vec3 bloomColor = texture(bloomBlur, TexCoords).rgb;
		hdrColor += bloomColor;
	}

    vec3 mapped = vec3(1.0) - exp(-hdrColor * exposure);
    mapped = pow(mapped, vec3(1.0 / gamma));

	if(bGammaCorrection)
	{
		float gamma = 2.2;
		mapped = pow(mapped, vec3(1.0 / gamma));
	}

    FragColor = vec4(mapped, 1.0);
	//FragColor = texture(screenTexture, TexCoords);
}