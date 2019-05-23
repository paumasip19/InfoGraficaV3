#version 330
in vec3 in_Position;
in vec3 in_Normal;

out vec3 vert_Normal;
out vec3 FragPos;

uniform mat4 objMat; //model
uniform mat4 mv_Mat; //view
uniform mat4 mvpMat; // projection * view

void main() 
{
	gl_Position = mvpMat * objMat * vec4(in_Position, 1.0);
	vert_Normal = mat3(transpose(inverse(objMat))) * in_Normal;
	FragPos = vec3(objMat * vec4(in_Position, 1.0f));
}