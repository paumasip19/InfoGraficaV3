#version 330
in vec3 in_Position;
in vec3 in_Normal;

out vec4 vert_Normal;

uniform mat4 objMat; //model
uniform mat4 mv_Mat; //view
uniform mat4 mvpMat; // projection * view

void main() 
{
	gl_Position = mvpMat * objMat * vec4(in_Position, 1.0);
	vert_Normal = mv_Mat * objMat * vec4(in_Normal, 0.0);
}