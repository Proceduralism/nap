#version 150 core

uniform mat4 projectionMatrix;
uniform mat4 viewMatrix;
uniform mat4 modelMatrix;

in vec3	in_Position;
in vec3	in_UV0;

out vec3 pass_Uvs;

void main(void)
{
    gl_Position = projectionMatrix * viewMatrix * modelMatrix * vec4(in_Position, 1.0);
	//gl_Position = vec4(in_Position, 1.0f);
	pass_Uvs = in_UV0;
}