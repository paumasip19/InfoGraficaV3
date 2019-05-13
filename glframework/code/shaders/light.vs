#version 330
in vec3 in_Position;

uniform mat4 objMat; //model
uniform mat4 mv_Mat; //view
uniform mat4 mvpMat; // projection * view

void main() 
{
	gl_Position = mvpMat * objMat * vec4(in_Position, 1.0);
}