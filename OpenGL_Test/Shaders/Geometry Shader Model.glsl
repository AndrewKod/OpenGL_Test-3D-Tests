#version 330 core
layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

#define NUM_POINT_LIGHTS 16

in VS_OUT {    
	vec3 gs_normal;

	vec2 fs_texCoords;
	vec3 fs_normal;
	vec3 fs_position;

	vec3 fs_fragPos;

	vec4 fs_fragPosDirLightSpace;

	vec3 fs_tanFragPos;
	vec3 fs_tanCameraPos;
	vec3 fs_tanPointLightPositions[NUM_POINT_LIGHTS];
	vec3 fs_tanDirLightDirection;
	vec3 fs_tanSpotLightDirection;

} gs_in[];



out GS_OUT {
    vec2 TexCoords;
	vec3 Normal;
	vec3 Position;
	vec3 FragPos;/*fragment shader's fragment coords*/
	vec4 FragPosDirLightSpace;

	vec3 tanFragPos;
	vec3 tanCameraPos;
	vec3 tanPointLightPositions[NUM_POINT_LIGHTS];
	vec3 tanDirLightDirection;
	vec3 tanSpotLightDirection;

} gs_out;

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

		gs_out.TexCoords =	gs_in[i].fs_texCoords;
		gs_out.Normal =		gs_in[i].fs_normal;
		gs_out.Position =	gs_in[i].fs_position;

		gs_out.FragPos =	gs_in[i].fs_fragPos;

		gs_out.FragPosDirLightSpace = gs_in[i].fs_fragPosDirLightSpace;

		gs_out.tanFragPos = gs_in[i].fs_tanFragPos;
		gs_out.tanCameraPos = gs_in[i].fs_tanCameraPos;
		gs_out.tanPointLightPositions = gs_in[i].fs_tanPointLightPositions;
		gs_out.tanDirLightDirection = gs_in[i].fs_tanDirLightDirection;
		gs_out.tanSpotLightDirection =  gs_in[i].fs_tanSpotLightDirection;

		EmitVertex();
	}
   
    EndPrimitive();
}