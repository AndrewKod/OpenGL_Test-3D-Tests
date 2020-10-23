#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;//hdrBuffer
uniform float exposure;

void main()
{
	gl_FragDepth = 0.0;

	const float gamma = 2.2;
    vec3 hdrColor = texture(screenTexture, TexCoords).rgb;

    vec3 mapped = vec3(1.0) - exp(-hdrColor * exposure);
    mapped = pow(mapped, vec3(1.0 / gamma));

    FragColor = vec4(mapped, 1.0);
	//FragColor = texture(screenTexture, TexCoords);
}