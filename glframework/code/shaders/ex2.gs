#version 330
	layout(triangles) in;
	layout(triangle_strip, max_vertices = 15) out;
	in vec4 vert_Normal[];
	out vec4 vert_g_Normal;
	out vec4 frag_pos;
	uniform mat4 mv_Mat;
	uniform mat4 projMat;
	uniform float time;
	float offset = 0.5;
	uniform int use_sten;
	void main() {
		for (int i = 0; i < 3; ++i) {
			vec4 vert = gl_in[i].gl_Position;
			if (use_sten == 1) {
				vert += vert_Normal[i] * 0.05;
				vec4 va = gl_in[0].gl_Position;
				vec4 vb = gl_in[1].gl_Position;
				vec4 vc = gl_in[2].gl_Position;
				vec4 cent = (va + vb + vc) / 3.0;
				vert += (gl_in[i].gl_Position - cent) * 0.5;
			}
			gl_Position = projMat * vert;
			vert_g_Normal = vert_Normal[i];
			EmitVertex();
		}
		EndPrimitive();
	}