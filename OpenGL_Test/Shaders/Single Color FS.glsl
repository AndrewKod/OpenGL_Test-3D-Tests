#version 330 core

out vec4 FragColor;

uniform vec4 borderColor;

void main()
{
	FragColor = borderColor;
}