#version 330 core
layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

in VS_OUT {    
	vec3 gs_normal;

	vec2 fs_texCoords;
	vec3 fs_normal;
	vec3 fs_position;

} gs_in[];

out vec2 TexCoords;
out vec3 Normal;
out vec3 Position;


uniform float time;

uniform bool bExplode = false;

vec4 explode(vec4 position, vec3 normal)
{ 
	float magnitude = 2.0;
    vec3 direction = normal * ((sin(time) + 1.0) / 2.0) * magnitude; 
    return position + vec4(direction, 0.0);
}

vec3 GetNormal() 
{ 
	vec3 a = vec3(gl_in[0].gl_Position) - vec3(gl_in[1].gl_Position);
	vec3 b = vec3(gl_in[2].gl_Position) - vec3(gl_in[1].gl_Position);
	return normalize(cross(a, b));
}

void main() {    
    vec3 tri_normal = GetNormal();

	for(int i = 0; i < gs_in.length(); i++)
	{ 
		if(bExplode)
			gl_Position = explode(gl_in[i].gl_Position, tri_normal);
		else
			gl_Position = gl_in[i].gl_Position;

		TexCoords = gs_in[i].fs_texCoords;
		Normal =	gs_in[i].fs_normal;
		Position =	gs_in[i].fs_position;

		EmitVertex();
	}
   
    EndPrimitive();
}