#version 330 core
out float FragColor;
  
in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;//model normal
uniform sampler2D gTangent;
uniform sampler2D gBitangent;
uniform sampler2D texNoise;

uniform int kernelSize = 64;
uniform float radius = 0.5;
uniform float bias = 0.025;
uniform vec3 samples[64];
uniform mat4 view;
uniform mat4 projection;


const vec2 noiseScale = vec2(800.0/4.0, 600.0/4.0); 

void main()
{
//	vec3 N = texture(gNormal, TexCoords).rgb;
//	vec3 T = texture(gTangent, TexCoords).rgb;
//  vec3 B = texture(gBitangent, TexCoords).rgb;	
//	mat3 TBN = transpose(mat3(T, B, N)); 
   
	vec3 fragPos   = (view * texture(gPosition, TexCoords)).xyz;//frag pos in view space
	vec3 normal    = (transpose(inverse(view ))* texture(gNormal, TexCoords)).rgb;//model normal in view space
	vec3 randomVec = texture(texNoise, TexCoords * noiseScale).xyz;

	vec3 tangent	= normalize(randomVec - normal * dot(randomVec, normal));
	vec3 bitangent	= cross(normal, tangent);
	mat3 AO_TBN		= mat3(tangent, bitangent, normal);

	float occlusion = 0.0;
	for(int i = 0; i < kernelSize; ++i)
	{
		vec3 sample = AO_TBN * (samples[i]);
		sample = fragPos + sample * radius; 
    
		vec4 offset = vec4(sample, 1.0);
		offset      = projection * offset;		//transfer to clip space
		offset.xyz /= offset.w;					//perspective division 
		//transfer into [0.0, 1.0] interval
		//for using as tex coords further
		offset.xyz  = offset.xyz * 0.5 + 0.5;	

		float sampleDepth = (view * texture(gPosition, offset.xy)).z;

		float rangeCheck = smoothstep(0.0, 1.0, radius / abs(fragPos.z - sampleDepth));
		occlusion       += (sampleDepth >= sample.z + bias ? 1.0 : 0.0) * rangeCheck; 
	}  

	occlusion = 1.0 - (occlusion / kernelSize);
	FragColor = occlusion;

}