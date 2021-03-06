#version 110

uniform vec3		LightPos;
uniform float		LightRadius;

attribute vec3		SurfNormal;
attribute float		SurfDist;

varying vec3		Normal;
varying float		Dist;
varying vec3		VertToLight;

void main()
{
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;

	Normal = SurfNormal; 
	Dist = dot(LightPos, SurfNormal) - SurfDist;
	VertToLight.xyz = LightPos.xyz - gl_Vertex.xyz;
}
