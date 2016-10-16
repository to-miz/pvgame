static const char* noLightingVertexShaderSource = R"(
#version 330 core

uniform mat4 worldViewProj;
uniform float screenDepthOffset;

in vec3 position;
in vec4 color;
in vec2 texCoords0;

out vec4 fragColor;
out vec2 fragTexCoords0;

void main()
{
	vec4 screenPos = worldViewProj * vec4( position, 1 );
	screenPos.z -= screenDepthOffset;
    gl_Position = screenPos;
	fragColor = color;
	fragTexCoords0 = texCoords0;
}
)";
static const char* noLightingFragmentShaderSource = R"(
#version 330 core

precision highp float; // Video card drivers require this line to function properly

uniform sampler2D texture0;

in vec4 fragColor;
in vec2 fragTexCoords0;

out vec4 outColor;

void main()
{
	vec4 tex = texture2D( texture0, fragTexCoords0 );
	outColor = tex * fragColor.bgra;
}
)";

static const char* ingameVertexShaderSource = R"(
#version 330 core

uniform mat4 worldViewProj;
uniform mat4 model;
uniform float screenDepthOffset;

in vec3 position;
in vec4 color;
in vec2 texCoords0;
in vec4 normal0;

out vec3 fragPos;
out vec4 fragColor;
out vec2 fragTexCoords0;
out vec4 fragNormal;

void main()
{
	vec4 posW = vec4( position, 1 );
	vec4 screenPos = worldViewProj * posW;
	screenPos.z -= screenDepthOffset;
	gl_Position = screenPos;

	fragPos = ( model * posW ).xyz;
	fragColor = color;
	fragTexCoords0 = texCoords0;
	fragNormal = model * normal0;
}
)";
static const char* ingameFragmentShaderSource = R"(
#version 330 core

precision highp float; // Video card drivers require this line to function properly

uniform float ambientStrength;
uniform vec4 lightColor;
uniform vec3 lightPosition;
uniform sampler2D texture0;

in vec3 fragPos;
in vec4 fragColor;
in vec2 fragTexCoords0;
in vec4 fragNormal;

out vec4 outColor;

void main()
{
	vec3 lightDir = normalize( lightPosition - fragPos );
	float diff = max( dot(fragNormal, vec4( lightDir, 0)), 0 );
	vec4 diffuse = diff * lightColor;
	vec4 tex = texture2D( texture0, fragTexCoords0 );
	vec4 ambientColor = vec4( ambientStrength, ambientStrength, ambientStrength, 1 );
	vec4 lightResult = clamp( diffuse + ambientColor, 0, 1 );
	lightResult.a = 1;
	outColor = tex * fragColor.bgra * lightResult;
}
)";