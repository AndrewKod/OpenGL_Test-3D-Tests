#version 330 core
in vec4 FragPos;

uniform vec3 lightPos;
uniform float far_plane;

void main()
{
    //distance between fragment and light source 
    float lightDistance = length(FragPos.xyz - lightPos);
    
	//transfer to [0, 1] range by division by far_plane
    lightDistance = lightDistance / far_plane;
    
    gl_FragDepth = lightDistance;
}  