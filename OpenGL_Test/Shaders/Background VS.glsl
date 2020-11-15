#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNorm;
layout (location = 2) in vec2 aTexCoords;

//uniform mat4 view;
//uniform mat4 projection;

layout (std140) uniform Matrices
{
	mat4 view;
    mat4 projection;    
};

out vec3 TexCoords;

void main()
{
    TexCoords = aPos;

	mat4 rotView = mat4(mat3(view));
	vec4 clipPos = projection * rotView * vec4(TexCoords, 1.0);

	gl_Position = clipPos.xyww;
}