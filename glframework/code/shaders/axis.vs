#version 330

in vec3 in_Position;
in vec4 in_Color;

out vec4 vert_color;

uniform mat4 mvpMat;

void main() 
{
	vert_color = in_Color;
	gl_Position = mvpMat * vec4(in_Position, 1.0);
}