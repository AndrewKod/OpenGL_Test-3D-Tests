#version 330 core
layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gAlbedoSpec;

layout (location = 3) out vec3 gTangent;
layout (location = 4) out vec3 gBitangent;

#define NUM_TEXTURE_MAPS 16
#define NUM_POINT_LIGHTS 16 

struct Material {
    /*vec3 ambient;*//*ambient in most of cases matches with diffuse*/
    sampler2D diffuse[NUM_TEXTURE_MAPS];
    sampler2D specular[NUM_TEXTURE_MAPS];
	sampler2D emissive[NUM_TEXTURE_MAPS];
	sampler2D reflection[NUM_TEXTURE_MAPS];
	sampler2D normal[NUM_TEXTURE_MAPS];
	sampler2D height[NUM_TEXTURE_MAPS];
    float shininess;
}; 
  
uniform Material material;

in VS_OUT {
    
	vec2 texCoords;
	vec3 normal;
	
	vec3 fragPos;	
	
	vec3 tangent;
	vec3 bitangent;
	

} fs_in;

uniform bool binvertUVs = false; 

void main()
{   

	vec2 texCoords = vec2(fs_in.texCoords.x, binvertUVs ? 1.0 - fs_in.texCoords.y : fs_in.texCoords.y);

	gPosition = fs_in.fragPos;
	
	vec3 norm = vec3(texture(material.normal[0], texCoords));	
	norm = normalize(norm * 2.0 - 1.0); 
	gNormal = norm;
	//gNormal = fs_in.normal;

    gAlbedoSpec.rgb = texture(material.diffuse[0], texCoords).rgb;
    gAlbedoSpec.a = texture(material.specular[0], texCoords).r;	
	
	gTangent = fs_in.tangent;
	gBitangent = fs_in.bitangent;

}

