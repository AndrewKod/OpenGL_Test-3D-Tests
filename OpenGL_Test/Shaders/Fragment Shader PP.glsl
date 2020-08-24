#version 330 core
out vec4 FragColor;
  
in vec2 TexCoords;

uniform sampler2D screenTexture;

uniform bool bPostProc;
uniform float kernel[9];

void main()
{ 
    FragColor = texture(screenTexture, TexCoords);
}