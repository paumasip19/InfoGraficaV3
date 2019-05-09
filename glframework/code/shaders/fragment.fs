#version 330
in vec4 vert_Normal;
out vec4 out_Color;

uniform mat4 mv_Mat;
uniform vec4 color;

void main() 
{	
	out_Color = vec4(color.xyz * dot(vert_Normal, mv_Mat*vec4(0.0, 1.0, 0.0, 0.0)) + color.xyz * 0.3, 1.0 );
}