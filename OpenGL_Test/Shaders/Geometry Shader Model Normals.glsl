#version 330 core
layout (triangles) in;
layout (line_strip, max_vertices = 6) out;

in VS_OUT {
    vec3 gs_normal;

	vec2 fs_texCoords;
	vec3 fs_normal;
	vec3 fs_position;

} gs_in[];

const float MAGNITUDE = 0.2;

void GenerateLine(int index)
{
    gl_Position = gl_in[index].gl_Position;
    EmitVertex();
    gl_Position = gl_in[index].gl_Position + vec4(gs_in[index].gs_normal, 0.0) * MAGNITUDE;
    EmitVertex();
    EndPrimitive();
}

void main() {    
    GenerateLine(0); 
    GenerateLine(1); 
    GenerateLine(2);
   
    EndPrimitive();
}