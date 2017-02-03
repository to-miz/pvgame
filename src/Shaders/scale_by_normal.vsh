#version 330 core

uniform mat4 worldViewProj;
uniform float screenDepthOffset;

in vec3 position;
in vec4 color;
in vec4 normal0;

out vec3 fragPos;
out vec4 fragColor;

void main()
{
	vec4 posW = vec4( position, 1 );
	vec4 screenPos = worldViewProj * posW;
	screenPos.z -= screenDepthOffset;
	vec2 offset = normalize( ( worldViewProj * normal0 ).xy );
	screenPos.xy += offset/* * screenPos.z * 0.1f*/;
	gl_Position = screenPos;

	fragPos = posW.xyz;
	fragColor = color;
}