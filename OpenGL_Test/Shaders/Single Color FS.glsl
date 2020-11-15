#version 330 core

out vec4 FragColor;

uniform vec4 borderColor;

uniform bool bPBR = false;

void main()
{
	FragColor = borderColor/(bPBR ? 1.0 : 300.0);
}