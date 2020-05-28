#version 450 core

// model view projection matrices UBO
uniform nap
{
	uniform mat4 projectionMatrix;
	uniform mat4 viewMatrix;
	uniform mat4 modelMatrix;
} mvp;

// Vertex inputs
in vec3	in_Position;
in vec3	in_UV0;

// Outputs
out vec3 pass_Uvs;

void main(void)
{
	// Calculate position
    gl_Position = mvp.projectionMatrix * mvp.viewMatrix * mvp.modelMatrix * vec4(in_Position, 1.0);

	// Pass color and uv's 
	pass_Uvs = in_UV0;
}