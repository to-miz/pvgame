#version 330 core

precision highp float; // Video card drivers require this line to function properly

in vec4 fragColor;

out vec4 outColor;

void main()
{
	outColor = fragColor.bgra;
}