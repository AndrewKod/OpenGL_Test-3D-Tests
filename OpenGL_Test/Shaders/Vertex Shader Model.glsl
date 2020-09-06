#version 330 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;
layout (location = 2) in vec2 texCoords;
layout (location = 3) in vec3 normal;

layout (location = 4) in vec3 tangent;
layout (location = 5) in vec3 bitangent;

out vec2 TexCoords;
out vec3 Normal;
out vec3 Position;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

layout (std140) uniform Matrices
{
	mat4 view1;
    mat4 projection1;    
};

void main()
{
	
	Normal = mat3(transpose(inverse(model))) * normal;
	Position = vec3(model * vec4(position, 1.0));	
    TexCoords = texCoords;    

    gl_Position = projection * view * model * vec4(position, 1.0);
	
}