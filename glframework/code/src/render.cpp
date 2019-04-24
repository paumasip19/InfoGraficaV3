#include <GL\glew.h>
#include <cstdio>
#include <cassert>

#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <glm\gtc\type_ptr.hpp>
#include <glm\gtc\matrix_transform.hpp>
#include <iostream>


#include <imgui\imgui.h>
#include <imgui\imgui_impl_sdl_gl3.h>

#include "GL_framework.h"

bool dolly = false;
float modifiedFOV = 65.f;

float renderW, renderH;

///////// fw decl
namespace ImGui {
	void Render();
}
namespace Axis {
void setupAxis();
void cleanupAxis();
void drawAxis();
}
////////////////

namespace RenderVars {
	const float FOV = glm::radians(65.f); //Como de cerca esta la camara
	const float zNear = 1.f; //Como de cerca deja de renderizar
	const float zFar = 50.f; //Como de lejos deja de renderizar

	glm::mat4 _projection;
	glm::mat4 _modelView;
	glm::mat4 _MVP;
	glm::mat4 _inv_modelview;
	glm::vec4 _cameraPoint;

	struct prevMouse {
		float lastx, lasty;
		MouseEvent::Button button = MouseEvent::Button::None;
		bool waspressed = false;
	} prevMouse;

	float panv[3] = { 0.f, -5.f, -15.f };
	float rota[2] = { 0.f, 0.f };
}
namespace RV = RenderVars;

void GLResize(int width, int height) {
	glViewport(0, 0, width, height);
	if(height != 0) RV::_projection = glm::perspective(RV::FOV, (float)width / (float)height, RV::zNear, RV::zFar);
	else RV::_projection = glm::perspective(RV::FOV, 0.f, RV::zNear, RV::zFar);
	renderW = (float)width;
	renderH = (float)height;
}

void GLmousecb(MouseEvent ev) {
	if(RV::prevMouse.waspressed && RV::prevMouse.button == ev.button) {
		float diffx = ev.posx - RV::prevMouse.lastx;
		float diffy = ev.posy - RV::prevMouse.lasty;
		switch(ev.button) {
		case MouseEvent::Button::Left: // ROTATE
			RV::rota[0] += diffx * 0.005f;
			RV::rota[1] += diffy * 0.005f;
			break;
		case MouseEvent::Button::Right: // MOVE XY
			RV::panv[0] += diffx * 0.03f;
			RV::panv[1] -= diffy * 0.03f;
			break;
		case MouseEvent::Button::Middle: // MOVE Z
			RV::panv[2] += diffy * 0.05f;
			break;
		default: break;
		}
	} else {
		RV::prevMouse.button = ev.button;
		RV::prevMouse.waspressed = true;
	}
	RV::prevMouse.lastx = ev.posx;
	RV::prevMouse.lasty = ev.posy;
}

//////////////////////////////////////////////////
GLuint compileShader(const char* shaderStr, GLenum shaderType, const char* name="") {
	GLuint shader = glCreateShader(shaderType);
	glShaderSource(shader, 1, &shaderStr, NULL);
	glCompileShader(shader);
	GLint res;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &res);
	if (res == GL_FALSE) {
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &res);
		char *buff = new char[res];
		glGetShaderInfoLog(shader, res, &res, buff);
		fprintf(stderr, "Error Shader %s: %s", name, buff);
		delete[] buff;
		glDeleteShader(shader);
		return 0;
	}
	return shader;
}
void linkProgram(GLuint program) {
	glLinkProgram(program);
	GLint res;
	glGetProgramiv(program, GL_LINK_STATUS, &res);
	if (res == GL_FALSE) {
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &res);
		char *buff = new char[res];
		glGetProgramInfoLog(program, res, &res, buff);
		fprintf(stderr, "Error Link: %s", buff);
		delete[] buff;
	}
}

bool loadOBJ(const char * path,
	std::vector < glm::vec3 > & out_vertices,
	std::vector < glm::vec2 > & out_uvs,
	std::vector < glm::vec3 > & out_normals
) 
{
	std::vector< unsigned int > vertexIndices, uvIndices, normalIndices;
	std::vector< glm::vec3 > temp_vertices;
	std::vector< glm::vec2 > temp_uvs;
	std::vector< glm::vec3 > temp_normals;

	FILE * file;
	if (fopen_s(&file, path, "r") != NULL)
	{
		printf("Impossible to open the file !\n");
		return false;
	}

	while (1)
	{
		char lineHeader[128];
		// read the first word of the line
		int res = fscanf_s(file, "%s", lineHeader, 128);
		if (res == EOF) break; // EOF = End Of File. Quit the loop.

		if (strcmp(lineHeader, "v") == 0) {
			glm::vec3 vertex;
			fscanf_s(file, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z);
			temp_vertices.push_back(vertex);
		}
		else if (strcmp(lineHeader, "vt") == 0) {
			glm::vec2 uv;
			fscanf_s(file, "%f %f\n", &uv.x, &uv.y);
			temp_uvs.push_back(uv);
		}
		else if (strcmp(lineHeader, "vn") == 0) {
			glm::vec3 normal;
			fscanf_s(file, "%f %f %f\n", &normal.x, &normal.y, &normal.z);
			temp_normals.push_back(normal);
		}
		else if (strcmp(lineHeader, "f") == 0) {
			std::string vertex1, vertex2, vertex3;
			unsigned int vertexIndex[3], uvIndex[3], normalIndex[3];
			int matches = fscanf_s(file, "%d/%d/%d %d/%d/%d %d/%d/%d\n", &vertexIndex[0], &uvIndex[0], &normalIndex[0], &vertexIndex[1], &uvIndex[1], &normalIndex[1], &vertexIndex[2], &uvIndex[2], &normalIndex[2]);
			if (matches != 9) {
				printf("File can't be read by our simple parser : ( Try exporting with other options\n");
				return false;
			}
			vertexIndices.push_back(vertexIndex[0]);
			vertexIndices.push_back(vertexIndex[1]);
			vertexIndices.push_back(vertexIndex[2]);
			uvIndices.push_back(uvIndex[0]);
			uvIndices.push_back(uvIndex[1]);
			uvIndices.push_back(uvIndex[2]);
			normalIndices.push_back(normalIndex[0]);
			normalIndices.push_back(normalIndex[1]);
			normalIndices.push_back(normalIndex[2]);
		}
	}

	for (unsigned int i = 0; i < vertexIndices.size(); i++) {
		unsigned int vertexIndex = vertexIndices[i];
		glm::vec3 vertex = temp_vertices[vertexIndex - 1];
		out_vertices.push_back(vertex);
	}

	for (unsigned int i = 0; i < uvIndices.size(); i++) {
		unsigned int uvIndex = uvIndices[i];
		glm::vec2 uv = temp_uvs[uvIndex - 1];
		out_uvs.push_back(uv);
	}

	for (unsigned int i = 0; i < normalIndices.size(); i++) {
		unsigned int normalIndex = normalIndices[i];
		glm::vec3 normal = temp_normals[normalIndex - 1];
		out_normals.push_back(normal);
	}

	fclose(file);
}

////////////////////////////////////////////////// AXIS
namespace Axis {
GLuint AxisVao;
GLuint AxisVbo[3];
GLuint AxisShader[2];
GLuint AxisProgram;

float AxisVerts[] = {
	0.0, 0.0, 0.0,
	1.0, 0.0, 0.0,
	0.0, 0.0, 0.0,
	0.0, 1.0, 0.0,
	0.0, 0.0, 0.0,
	0.0, 0.0, 1.0
};
float AxisColors[] = {
	1.0, 0.0, 0.0, 1.0,
	1.0, 0.0, 0.0, 1.0,
	0.0, 1.0, 0.0, 1.0,
	0.0, 1.0, 0.0, 1.0,
	0.0, 0.0, 1.0, 1.0,
	0.0, 0.0, 1.0, 1.0
};
GLubyte AxisIdx[] = {
	0, 1,
	2, 3,
	4, 5
};
const char* Axis_vertShader =
"#version 330\n\
in vec3 in_Position;\n\
in vec4 in_Color;\n\
out vec4 vert_color;\n\
uniform mat4 mvpMat;\n\
void main() {\n\
	vert_color = in_Color;\n\
	gl_Position = mvpMat * vec4(in_Position, 1.0);\n\
}";
const char* Axis_fragShader =
"#version 330\n\
in vec4 vert_color;\n\
out vec4 out_Color;\n\
void main() {\n\
	out_Color = vert_color;\n\
}";

void setupAxis() {
	glGenVertexArrays(1, &AxisVao);
	glBindVertexArray(AxisVao);
	glGenBuffers(3, AxisVbo);

	glBindBuffer(GL_ARRAY_BUFFER, AxisVbo[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 24, AxisVerts, GL_STATIC_DRAW);
	glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, AxisVbo[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 24, AxisColors, GL_STATIC_DRAW);
	glVertexAttribPointer((GLuint)1, 4, GL_FLOAT, false, 0, 0);
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, AxisVbo[2]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLubyte) * 6, AxisIdx, GL_STATIC_DRAW);

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	AxisShader[0] = compileShader(Axis_vertShader, GL_VERTEX_SHADER, "AxisVert");
	AxisShader[1] = compileShader(Axis_fragShader, GL_FRAGMENT_SHADER, "AxisFrag");

	AxisProgram = glCreateProgram();
	glAttachShader(AxisProgram, AxisShader[0]);
	glAttachShader(AxisProgram, AxisShader[1]);
	glBindAttribLocation(AxisProgram, 0, "in_Position");
	glBindAttribLocation(AxisProgram, 1, "in_Color");
	linkProgram(AxisProgram);
}
void cleanupAxis() {
	glDeleteBuffers(3, AxisVbo);
	glDeleteVertexArrays(1, &AxisVao);

	glDeleteProgram(AxisProgram);
	glDeleteShader(AxisShader[0]);
	glDeleteShader(AxisShader[1]);
}
void drawAxis() {
	glBindVertexArray(AxisVao);
	glUseProgram(AxisProgram);
	glUniformMatrix4fv(glGetUniformLocation(AxisProgram, "mvpMat"), 1, GL_FALSE, glm::value_ptr(RV::_MVP));
	glDrawElements(GL_LINES, 6, GL_UNSIGNED_BYTE, 0);

	glUseProgram(0);
	glBindVertexArray(0);
}
}

////////////////////////////////////////////////// CUBE
namespace Cube {
GLuint cubeVao;
GLuint cubeVbo[3];
GLuint cubeShaders[2];
GLuint cubeProgram;
glm::mat4 objMat = glm::mat4(1.f);

extern const float halfW = 0.5f;
int numVerts = 24 + 6; // 4 vertex/face * 6 faces + 6 PRIMITIVE RESTART

					   //   4---------7
					   //  /|        /|
					   // / |       / |
					   //5---------6  |
					   //|  0------|--3
					   //| /       | /
					   //|/        |/
					   //1---------2
glm::vec3 verts[] = {
	glm::vec3(-halfW, -halfW, -halfW),
	glm::vec3(-halfW, -halfW,  halfW),
	glm::vec3(halfW, -halfW,  halfW),
	glm::vec3(halfW, -halfW, -halfW),
	glm::vec3(-halfW,  halfW, -halfW),
	glm::vec3(-halfW,  halfW,  halfW),
	glm::vec3(halfW,  halfW,  halfW),
	glm::vec3(halfW,  halfW, -halfW)
};
glm::vec3 norms[] = {
	glm::vec3(0.f, -1.f,  0.f),
	glm::vec3(0.f,  1.f,  0.f),
	glm::vec3(-1.f,  0.f,  0.f),
	glm::vec3(1.f,  0.f,  0.f),
	glm::vec3(0.f,  0.f, -1.f),
	glm::vec3(0.f,  0.f,  1.f)
};

glm::vec3 cubeVerts[] = {
	verts[1], verts[0], verts[2], verts[3],
	verts[5], verts[6], verts[4], verts[7],
	verts[1], verts[5], verts[0], verts[4],
	verts[2], verts[3], verts[6], verts[7],
	verts[0], verts[4], verts[3], verts[7],
	verts[1], verts[2], verts[5], verts[6]
};
glm::vec3 cubeNorms[] = {
	norms[0], norms[0], norms[0], norms[0],
	norms[1], norms[1], norms[1], norms[1],
	norms[2], norms[2], norms[2], norms[2],
	norms[3], norms[3], norms[3], norms[3],
	norms[4], norms[4], norms[4], norms[4],
	norms[5], norms[5], norms[5], norms[5]
};
GLubyte cubeIdx[] = {
	0, 1, 2, 3, UCHAR_MAX,
	4, 5, 6, 7, UCHAR_MAX,
	8, 9, 10, 11, UCHAR_MAX,
	12, 13, 14, 15, UCHAR_MAX,
	16, 17, 18, 19, UCHAR_MAX,
	20, 21, 22, 23, UCHAR_MAX
};

const char* cube_vertShader =
"#version 330\n\
in vec3 in_Position;\n\
in vec3 in_Normal;\n\
out vec4 vert_Normal;\n\
uniform mat4 objMat;\n\
uniform mat4 mv_Mat;\n\
uniform mat4 mvpMat;\n\
void main() {\n\
	gl_Position = mvpMat * objMat * vec4(in_Position, 1.0);\n\
	vert_Normal = mv_Mat * objMat * vec4(in_Normal, 0.0);\n\
}";
const char* cube_fragShader =
"#version 330\n\
in vec4 vert_Normal;\n\
out vec4 out_Color;\n\
uniform mat4 mv_Mat;\n\
uniform vec4 color;\n\
void main() {\n\
	out_Color = vec4(color.xyz * dot(vert_Normal, mv_Mat*vec4(0.0, 1.0, 0.0, 0.0)) + color.xyz * 0.3, 1.0 );\n\
}";

void setupCube() {
	glGenVertexArrays(1, &cubeVao);
	glBindVertexArray(cubeVao);
	glGenBuffers(3, cubeVbo);

	glBindBuffer(GL_ARRAY_BUFFER, cubeVbo[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVerts), cubeVerts, GL_STATIC_DRAW);
	glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, cubeVbo[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cubeNorms), cubeNorms, GL_STATIC_DRAW);
	glVertexAttribPointer((GLuint)1, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(1);

	glPrimitiveRestartIndex(UCHAR_MAX);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cubeVbo[2]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cubeIdx), cubeIdx, GL_STATIC_DRAW);

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	cubeShaders[0] = compileShader(cube_vertShader, GL_VERTEX_SHADER, "cubeVert");
	cubeShaders[1] = compileShader(cube_fragShader, GL_FRAGMENT_SHADER, "cubeFrag");

	cubeProgram = glCreateProgram();
	glAttachShader(cubeProgram, cubeShaders[0]);
	glAttachShader(cubeProgram, cubeShaders[1]);
	glBindAttribLocation(cubeProgram, 0, "in_Position");
	glBindAttribLocation(cubeProgram, 1, "in_Normal");
	linkProgram(cubeProgram);
}
void cleanupCube() {
	glDeleteBuffers(3, cubeVbo);
	glDeleteVertexArrays(1, &cubeVao);

	glDeleteProgram(cubeProgram);
	glDeleteShader(cubeShaders[0]);
	glDeleteShader(cubeShaders[1]);
}
void updateCube(const glm::mat4& transform) {
	objMat = transform;
}
void drawCube(float time) {
	glEnable(GL_PRIMITIVE_RESTART);
	glBindVertexArray(cubeVao);
	glUseProgram(cubeProgram);

	glUniformMatrix4fv(glGetUniformLocation(cubeProgram, "mv_Mat"), 1, GL_FALSE, glm::value_ptr(RenderVars::_modelView));
	glUniformMatrix4fv(glGetUniformLocation(cubeProgram, "mvpMat"), 1, GL_FALSE, glm::value_ptr(RenderVars::_MVP));

	objMat = glm::mat4(1.f);
	objMat[3][3] = 1.f;

	glm::mat4 obj1 = glm::translate(objMat, glm::vec3(-2.f, 0.f, 0.f));	//Cubo 1 movido a la izquierda
	glUniformMatrix4fv(glGetUniformLocation(cubeProgram, "objMat"), 1, GL_FALSE, glm::value_ptr(obj1));	
	glUniform4f(glGetUniformLocation(cubeProgram, "color"), 0.1f, 1.f, 1.f, 0.f);
	glDrawElements(GL_TRIANGLE_STRIP, numVerts, GL_UNSIGNED_BYTE, 0);


	glm::mat4 obj2 = glm::rotate(obj1, time, glm::vec3(0.f, 1.f, 0.f)); //Cubo 2 rota en el eje de las Y sobre el obj1
	
	obj2 = glm::scale(obj2, glm::vec3(sin(time) + 2, sin(time) + 2, sin(time) + 2)); //Hace el escalado
	obj2 = glm::rotate(obj2, time, glm::vec3(0.f, 1.f, 0.f)); // Rota sobre si mismo

	obj2 = glm::translate(obj2, glm::vec3(2.f, 0.f, 0.f)); //Se mueve a la derecha


	glUniformMatrix4fv(glGetUniformLocation(cubeProgram, "objMat"), 1, GL_FALSE, glm::value_ptr(obj2));
	glUniform4f(glGetUniformLocation(cubeProgram, "color"), (float)sin(time) * 0.5f + 0.5f, (float)cos(time)* 0.5f + 0.5f, 0.0f, 1.0f);
	glDrawElements(GL_TRIANGLE_STRIP, numVerts, GL_UNSIGNED_BYTE, 0);
	
	glUseProgram(0);
	glBindVertexArray(0);
	glDisable(GL_PRIMITIVE_RESTART);
}
}


glm::vec3 oC = { 0.5f, 0.5f, 0.5f };
glm::vec3 lPos = { 10.f, 15.f, 0.f };
glm::vec3 lColor = { 1.f, 1.f, 1.f };
float exponent = 32.f;
float ambientStrength = 0.1f;
float specularStrength = 0.5f;

namespace Object1 {
	GLuint objectVao;
	GLuint objectVbo[2];
	GLuint objectShaders[2];
	GLuint objectProgram;
	glm::mat4 objMat = glm::mat4(1.f);

	const char* object_vertShader =
		"#version 330\n\
in vec3 in_Position;\n\
in vec3 in_Normal;\n\
out vec3 vert_Normal;\n\
out vec3 frag_Pos;\n\
out vec3 LightPos;\n\
uniform vec3 light;\n\
uniform mat4 objMat;\n\
uniform mat4 mv_Mat;\n\
uniform mat4 projection;\n\
void main() {\n\
	gl_Position = projection * objMat * vec4(in_Position, 1.0);\n\
	frag_Pos = vec3(mv_Mat * objMat * vec4(in_Position, 1.0));\n\
	vert_Normal = mat3(transpose(inverse(mv_Mat * objMat))) *in_Normal;\n\
	LightPos = vec3(mv_Mat * vec4(light,1.0));\n\
}";
	const char* object_fragShader =
		"#version 330\n\
in vec3 vert_Normal;\n\
in vec3 frag_Pos;\n\
in vec3 LightPos;\n\
out vec4 frag_Color;\n\
uniform vec3 lightColor;\n\
uniform mat4 mv_Mat;\n\
uniform vec3 objectColor;\n\
uniform vec3 viewPos;\n\
uniform float ex;\n\
uniform float ambientStrength;\n\
uniform float specularStrength;\n\
void main() {\n\
	//ambient \n\
	vec3 ambient = ambientStrength * lightColor;\n\
	//diffuse\n\
	vec3 norm = normalize(vert_Normal);\n\
	vec3 lightDir = normalize(LightPos - frag_Pos);\n\
	float diff = max(dot(norm, lightDir), 0.0);\n\
	if(diff < 0.2) diff = 0;\n\
	if(diff >= 0.2 && diff < 0.4) diff = 0.2;\n\
	if(diff >= 0.4 && diff < 0.5) diff = 0.4;\n\
	if(diff >= 0.5) diff = 1;\n\
	vec3 diffuse = diff * lightColor;\n\
	//specular\n\
	vec3 viewDir = normalize(-frag_Pos);\n\
	vec3 reflectDir = reflect(-lightDir, norm);\n\
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), ex);\n\
	if(spec < 0.2) spec = 0;\n\
	if(spec >= 0.2 && spec < 0.4) spec = 0.2;\n\
	if(spec >= 0.4 && spec < 0.5) spec = 0.4;\n\
	if(spec >= 0.5) spec = 1;\n\
	vec3 specular = specularStrength * spec * lightColor;\n\
	vec3 result = (ambient + diffuse + specular) * objectColor;\n\
	frag_Color = vec4(result, 1.0);\n\
}";

	std::vector< glm::vec3 > vertices, normals;
	std::vector< glm::vec2 > uvs;

	void setupObject() {

		loadOBJ("pistola1.obj", vertices, uvs, normals);
		
		glGenVertexArrays(1, &objectVao);
		glBindVertexArray(objectVao);
		glGenBuffers(2, objectVbo);

		glBindBuffer(GL_ARRAY_BUFFER, objectVbo[0]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3)*vertices.size(), vertices.data(), GL_STATIC_DRAW);
		glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(0);

		glBindBuffer(GL_ARRAY_BUFFER, objectVbo[1]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3)*normals.size(), normals.data(), GL_STATIC_DRAW);
		glVertexAttribPointer((GLuint)1, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(1);

		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		objectShaders[0] = compileShader(object_vertShader, GL_VERTEX_SHADER, "objectVert");
		objectShaders[1] = compileShader(object_fragShader, GL_FRAGMENT_SHADER, "objectFrag");

		objectProgram = glCreateProgram();
		glAttachShader(objectProgram, objectShaders[0]);
		glAttachShader(objectProgram, objectShaders[1]);
		glBindAttribLocation(objectProgram, 0, "in_Position");
		glBindAttribLocation(objectProgram, 1, "in_Normal");
		linkProgram(objectProgram);
	}
	void cleanupObject() {
		glDeleteBuffers(2, objectVbo);
		glDeleteVertexArrays(1, &objectVao);

		glDeleteProgram(objectProgram);
		glDeleteShader(objectShaders[0]);
		glDeleteShader(objectShaders[1]);
	}
	void drawObject(float time) {
		glBindVertexArray(objectVao);
		glUseProgram(objectProgram);
		
		glUniformMatrix4fv(glGetUniformLocation(objectProgram, "mv_Mat"), 1, GL_FALSE, glm::value_ptr(RenderVars::_modelView));
		glUniformMatrix4fv(glGetUniformLocation(objectProgram, "projection"), 1, GL_FALSE, glm::value_ptr(RenderVars::_MVP));
		
		glUniformMatrix3fv(glGetUniformLocation(objectProgram, "viewPos"), 1, GL_FALSE, glm::value_ptr(RV::_cameraPoint));
		//std::cout << RV::_cameraPoint.x <<" "<< RV::_cameraPoint.y <<" "<< RV::_cameraPoint.z << std::endl;
		
		glm::mat4 obj = glm::translate(objMat, glm::vec3(-1.f, 0.f, -15.f));
		glUniformMatrix4fv(glGetUniformLocation(objectProgram, "objMat"), 1, GL_FALSE, glm::value_ptr(obj));
		glUniform3f(glGetUniformLocation(objectProgram, "objectColor"), oC.x, oC.y, oC.z);
		glUniform3f(glGetUniformLocation(objectProgram, "light"), lPos.x, lPos.y, lPos.z);
		glUniform3f(glGetUniformLocation(objectProgram, "lightColor"), lColor.x, lColor.y, lColor.z);

		glUniform1f(glGetUniformLocation(objectProgram, "ex"), exponent);
		glUniform1f(glGetUniformLocation(objectProgram, "ambientStrength"), ambientStrength);
		glUniform1f(glGetUniformLocation(objectProgram, "specularStrength"), specularStrength);

		glDrawArrays(GL_TRIANGLES, 0, vertices.size());
		
		glUseProgram(0);
		glBindVertexArray(0);
	}
}

namespace Object2 {
	GLuint objectVao;
	GLuint objectVbo[2];
	GLuint objectShaders[2];
	GLuint objectProgram;
	glm::mat4 objMat = glm::mat4(1.f);

	const char* object_vertShader =
		"#version 330\n\
in vec3 in_Position;\n\
in vec3 in_Normal;\n\
out vec3 vert_Normal;\n\
out vec3 frag_Pos;\n\
out vec3 LightPos;\n\
uniform vec3 light;\n\
uniform mat4 objMat;\n\
uniform mat4 mv_Mat;\n\
uniform mat4 projection;\n\
void main() {\n\
	gl_Position = projection * objMat * vec4(in_Position, 1.0);\n\
	frag_Pos = vec3(mv_Mat * objMat * vec4(in_Position, 1.0));\n\
	vert_Normal = mat3(transpose(inverse(mv_Mat * objMat))) *in_Normal;\n\
	LightPos = vec3(mv_Mat * vec4(light,1.0));\n\
}";
	const char* object_fragShader =
		"#version 330\n\
in vec3 vert_Normal;\n\
in vec3 frag_Pos;\n\
in vec3 LightPos;\n\
out vec4 frag_Color;\n\
uniform vec3 lightColor;\n\
uniform mat4 mv_Mat;\n\
uniform vec3 objectColor;\n\
uniform vec3 viewPos;\n\
uniform float ex;\n\
uniform float ambientStrength;\n\
uniform float specularStrength;\n\
void main() {\n\
	//ambient \n\
	vec3 ambient = ambientStrength * lightColor;\n\
	//diffuse\n\
	vec3 norm = normalize(vert_Normal);\n\
	vec3 lightDir = normalize(LightPos - frag_Pos);\n\
	float diff = max(dot(norm, lightDir), 0.0);\n\
	if(diff < 0.2) diff = 0;\n\
	if(diff >= 0.2 && diff < 0.4) diff = 0.2;\n\
	if(diff >= 0.4 && diff < 0.5) diff = 0.4;\n\
	if(diff >= 0.5) diff = 1;\n\
	vec3 diffuse = diff * lightColor;\n\
	//specular\n\
	vec3 viewDir = normalize(-frag_Pos);\n\
	vec3 reflectDir = reflect(-lightDir, norm);\n\
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), ex);\n\
	if(spec < 0.2) spec = 0;\n\
	if(spec >= 0.2 && spec < 0.4) spec = 0.2;\n\
	if(spec >= 0.4 && spec < 0.5) spec = 0.4;\n\
	if(spec >= 0.5) spec = 1;\n\
	vec3 specular = specularStrength * spec * lightColor;\n\
	vec3 result = (ambient + diffuse + specular) * objectColor;\n\
	frag_Color = vec4(result, 1.0);\n\
}";

	std::vector< glm::vec3 > vertices, normals;
	std::vector< glm::vec2 > uvs;

	void setupObject() {



		loadOBJ("trash.obj", vertices, uvs, normals);

		glGenVertexArrays(1, &objectVao);
		glBindVertexArray(objectVao);
		glGenBuffers(2, objectVbo);

		glBindBuffer(GL_ARRAY_BUFFER, objectVbo[0]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3)*vertices.size(), vertices.data(), GL_STATIC_DRAW);
		glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(0);

		glBindBuffer(GL_ARRAY_BUFFER, objectVbo[1]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3)*normals.size(), normals.data(), GL_STATIC_DRAW);
		glVertexAttribPointer((GLuint)1, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(1);

		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		objectShaders[0] = compileShader(object_vertShader, GL_VERTEX_SHADER, "objectVert");
		objectShaders[1] = compileShader(object_fragShader, GL_FRAGMENT_SHADER, "objectFrag");

		objectProgram = glCreateProgram();
		glAttachShader(objectProgram, objectShaders[0]);
		glAttachShader(objectProgram, objectShaders[1]);
		glBindAttribLocation(objectProgram, 0, "in_Position");
		glBindAttribLocation(objectProgram, 1, "in_Normal");
		linkProgram(objectProgram);
	}
	void cleanupObject() {
		glDeleteBuffers(2, objectVbo);
		glDeleteVertexArrays(1, &objectVao);

		glDeleteProgram(objectProgram);
		glDeleteShader(objectShaders[0]);
		glDeleteShader(objectShaders[1]);
	}
	void drawObject(float time) {
		glBindVertexArray(objectVao);
		glUseProgram(objectProgram);

		glUniformMatrix4fv(glGetUniformLocation(objectProgram, "mv_Mat"), 1, GL_FALSE, glm::value_ptr(RenderVars::_modelView));
		glUniformMatrix4fv(glGetUniformLocation(objectProgram, "projection"), 1, GL_FALSE, glm::value_ptr(RenderVars::_MVP));

		glUniformMatrix3fv(glGetUniformLocation(objectProgram, "viewPos"), 1, GL_FALSE, glm::value_ptr(RV::_cameraPoint));
		//std::cout << RV::_cameraPoint.x <<" "<< RV::_cameraPoint.y <<" "<< RV::_cameraPoint.z << std::endl;

		glm::mat4 obj = glm::translate(objMat, glm::vec3(4.f, 0.f, -10.f));
		glUniformMatrix4fv(glGetUniformLocation(objectProgram, "objMat"), 1, GL_FALSE, glm::value_ptr(obj));
		glUniform3f(glGetUniformLocation(objectProgram, "objectColor"), oC.x, oC.y, oC.z);
		glUniform3f(glGetUniformLocation(objectProgram, "light"), lPos.x, lPos.y, lPos.z);
		glUniform3f(glGetUniformLocation(objectProgram, "lightColor"), lColor.x, lColor.y, lColor.z);

		glUniform1f(glGetUniformLocation(objectProgram, "ex"), exponent);
		glUniform1f(glGetUniformLocation(objectProgram, "ambientStrength"), ambientStrength);
		glUniform1f(glGetUniformLocation(objectProgram, "specularStrength"), specularStrength);

		glDrawArrays(GL_TRIANGLES, 0, vertices.size());

		glUseProgram(0);
		glBindVertexArray(0);
	}
}

namespace Object3 {
	GLuint objectVao;
	GLuint objectVbo[2];
	GLuint objectShaders[2];
	GLuint objectProgram;
	glm::mat4 objMat = glm::mat4(1.f);

	const char* object_vertShader =
		"#version 330\n\
in vec3 in_Position;\n\
in vec3 in_Normal;\n\
out vec3 vert_Normal;\n\
out vec3 frag_Pos;\n\
out vec3 LightPos;\n\
uniform vec3 light;\n\
uniform mat4 objMat;\n\
uniform mat4 mv_Mat;\n\
uniform mat4 projection;\n\
void main() {\n\
	gl_Position = projection * objMat * vec4(in_Position, 1.0);\n\
	frag_Pos = vec3(mv_Mat * objMat * vec4(in_Position, 1.0));\n\
	vert_Normal = mat3(transpose(inverse(mv_Mat * objMat))) *in_Normal;\n\
	LightPos = vec3(mv_Mat * vec4(light,1.0));\n\
}";
	const char* object_fragShader =
		"#version 330\n\
in vec3 vert_Normal;\n\
in vec3 frag_Pos;\n\
in vec3 LightPos;\n\
out vec4 frag_Color;\n\
uniform vec3 lightColor;\n\
uniform mat4 mv_Mat;\n\
uniform vec3 objectColor;\n\
uniform vec3 viewPos;\n\
uniform float ex;\n\
uniform float ambientStrength;\n\
uniform float specularStrength;\n\
void main() {\n\
	//ambient \n\
	vec3 ambient = ambientStrength * lightColor;\n\
	//diffuse\n\
	vec3 norm = normalize(vert_Normal);\n\
	vec3 lightDir = normalize(LightPos - frag_Pos);\n\
	float diff = max(dot(norm, lightDir), 0.0);\n\
	if(diff < 0.2) diff = 0;\n\
	if(diff >= 0.2 && diff < 0.4) diff = 0.2;\n\
	if(diff >= 0.4 && diff < 0.5) diff = 0.4;\n\
	if(diff >= 0.5) diff = 1;\n\
	vec3 diffuse = diff * lightColor;\n\
	//specular\n\
	vec3 viewDir = normalize(-frag_Pos);\n\
	vec3 reflectDir = reflect(-lightDir, norm);\n\
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), ex);\n\
	if(spec < 0.2) spec = 0;\n\
	if(spec >= 0.2 && spec < 0.4) spec = 0.2;\n\
	if(spec >= 0.4 && spec < 0.5) spec = 0.4;\n\
	if(spec >= 0.5) spec = 1;\n\
	vec3 specular = specularStrength * spec * lightColor;\n\
	vec3 result = (ambient + diffuse + specular) * objectColor;\n\
	frag_Color = vec4(result, 1.0);\n\
}";

	std::vector< glm::vec3 > vertices, normals;
	std::vector< glm::vec2 > uvs;

	void setupObject() {

		loadOBJ("wolf.obj", vertices, uvs, normals);

		glGenVertexArrays(1, &objectVao);
		glBindVertexArray(objectVao);
		glGenBuffers(2, objectVbo);

		glBindBuffer(GL_ARRAY_BUFFER, objectVbo[0]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3)*vertices.size(), vertices.data(), GL_STATIC_DRAW);
		glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(0);

		glBindBuffer(GL_ARRAY_BUFFER, objectVbo[1]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3)*normals.size(), normals.data(), GL_STATIC_DRAW);
		glVertexAttribPointer((GLuint)1, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(1);

		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		objectShaders[0] = compileShader(object_vertShader, GL_VERTEX_SHADER, "objectVert");
		objectShaders[1] = compileShader(object_fragShader, GL_FRAGMENT_SHADER, "objectFrag");

		objectProgram = glCreateProgram();
		glAttachShader(objectProgram, objectShaders[0]);
		glAttachShader(objectProgram, objectShaders[1]);
		glBindAttribLocation(objectProgram, 0, "in_Position");
		glBindAttribLocation(objectProgram, 1, "in_Normal");
		linkProgram(objectProgram);
	}
	void cleanupObject() {
		glDeleteBuffers(2, objectVbo);
		glDeleteVertexArrays(1, &objectVao);

		glDeleteProgram(objectProgram);
		glDeleteShader(objectShaders[0]);
		glDeleteShader(objectShaders[1]);
	}
	void drawObject(float time) {
		glBindVertexArray(objectVao);
		glUseProgram(objectProgram);

		glUniformMatrix4fv(glGetUniformLocation(objectProgram, "mv_Mat"), 1, GL_FALSE, glm::value_ptr(RenderVars::_modelView));
		glUniformMatrix4fv(glGetUniformLocation(objectProgram, "projection"), 1, GL_FALSE, glm::value_ptr(RenderVars::_MVP));

		glUniformMatrix3fv(glGetUniformLocation(objectProgram, "viewPos"), 1, GL_FALSE, glm::value_ptr(RV::_cameraPoint));
		//std::cout << RV::_cameraPoint.x <<" "<< RV::_cameraPoint.y <<" "<< RV::_cameraPoint.z << std::endl;

		glm::mat4 obj = glm::translate(objMat, glm::vec3(7.f, 0.f, -8.f));
		glUniformMatrix4fv(glGetUniformLocation(objectProgram, "objMat"), 1, GL_FALSE, glm::value_ptr(obj));
		glUniform3f(glGetUniformLocation(objectProgram, "objectColor"), oC.x, oC.y, oC.z);
		glUniform3f(glGetUniformLocation(objectProgram, "light"), lPos.x, lPos.y, lPos.z);
		glUniform3f(glGetUniformLocation(objectProgram, "lightColor"), lColor.x, lColor.y, lColor.z);

		glUniform1f(glGetUniformLocation(objectProgram, "ex"), exponent);
		glUniform1f(glGetUniformLocation(objectProgram, "ambientStrength"), ambientStrength);
		glUniform1f(glGetUniformLocation(objectProgram, "specularStrength"), specularStrength);

		glDrawArrays(GL_TRIANGLES, 0, vertices.size());

		glUseProgram(0);
		glBindVertexArray(0);
	}
}

namespace Object4 {
	GLuint objectVao;
	GLuint objectVbo[2];
	GLuint objectShaders[2];
	GLuint objectProgram;
	glm::mat4 objMat = glm::mat4(1.f);

	const char* object_vertShader =
		"#version 330\n\
in vec3 in_Position;\n\
in vec3 in_Normal;\n\
out vec3 vert_Normal;\n\
out vec3 frag_Pos;\n\
out vec3 LightPos;\n\
uniform vec3 light;\n\
uniform mat4 objMat;\n\
uniform mat4 mv_Mat;\n\
uniform mat4 projection;\n\
void main() {\n\
	gl_Position = projection * objMat * vec4(in_Position, 1.0);\n\
	frag_Pos = vec3(mv_Mat * objMat * vec4(in_Position, 1.0));\n\
	vert_Normal = mat3(transpose(inverse(mv_Mat * objMat))) *in_Normal;\n\
	LightPos = vec3(mv_Mat * vec4(light,1.0));\n\
}";
	const char* object_fragShader =
		"#version 330\n\
in vec3 vert_Normal;\n\
in vec3 frag_Pos;\n\
in vec3 LightPos;\n\
out vec4 frag_Color;\n\
uniform vec3 lightColor;\n\
uniform mat4 mv_Mat;\n\
uniform vec3 objectColor;\n\
uniform vec3 viewPos;\n\
uniform float ex;\n\
uniform float ambientStrength;\n\
uniform float specularStrength;\n\
void main() {\n\
	//ambient \n\
	vec3 ambient = ambientStrength * lightColor;\n\
	//diffuse\n\
	vec3 norm = normalize(vert_Normal);\n\
	vec3 lightDir = normalize(LightPos - frag_Pos);\n\
	float diff = max(dot(norm, lightDir), 0.0);\n\
	if(diff < 0.2) diff = 0;\n\
	if(diff >= 0.2 && diff < 0.4) diff = 0.2;\n\
	if(diff >= 0.4 && diff < 0.5) diff = 0.4;\n\
	if(diff >= 0.5) diff = 1;\n\
	vec3 diffuse = diff * lightColor;\n\
	//specular\n\
	vec3 viewDir = normalize(-frag_Pos);\n\
	vec3 reflectDir = reflect(-lightDir, norm);\n\
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), ex);\n\
	if(spec < 0.2) spec = 0;\n\
	if(spec >= 0.2 && spec < 0.4) spec = 0.2;\n\
	if(spec >= 0.4 && spec < 0.5) spec = 0.4;\n\
	if(spec >= 0.5) spec = 1;\n\
	vec3 specular = specularStrength * spec * lightColor;\n\
	vec3 result = (ambient + diffuse + specular) * objectColor;\n\
	frag_Color = vec4(result, 1.0);\n\
}";

	std::vector< glm::vec3 > vertices, normals;
	std::vector< glm::vec2 > uvs;

	void setupObject() {



		loadOBJ("Concha.obj", vertices, uvs, normals);

		glGenVertexArrays(1, &objectVao);
		glBindVertexArray(objectVao);
		glGenBuffers(2, objectVbo);

		glBindBuffer(GL_ARRAY_BUFFER, objectVbo[0]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3)*vertices.size(), vertices.data(), GL_STATIC_DRAW);
		glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(0);

		glBindBuffer(GL_ARRAY_BUFFER, objectVbo[1]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3)*normals.size(), normals.data(), GL_STATIC_DRAW);
		glVertexAttribPointer((GLuint)1, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(1);

		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		objectShaders[0] = compileShader(object_vertShader, GL_VERTEX_SHADER, "objectVert");
		objectShaders[1] = compileShader(object_fragShader, GL_FRAGMENT_SHADER, "objectFrag");

		objectProgram = glCreateProgram();
		glAttachShader(objectProgram, objectShaders[0]);
		glAttachShader(objectProgram, objectShaders[1]);
		glBindAttribLocation(objectProgram, 0, "in_Position");
		glBindAttribLocation(objectProgram, 1, "in_Normal");
		linkProgram(objectProgram);
	}
	void cleanupObject() {
		glDeleteBuffers(2, objectVbo);
		glDeleteVertexArrays(1, &objectVao);

		glDeleteProgram(objectProgram);
		glDeleteShader(objectShaders[0]);
		glDeleteShader(objectShaders[1]);
	}
	void drawObject(float time) {
		glBindVertexArray(objectVao);
		glUseProgram(objectProgram);

		glUniformMatrix4fv(glGetUniformLocation(objectProgram, "mv_Mat"), 1, GL_FALSE, glm::value_ptr(RenderVars::_modelView));
		glUniformMatrix4fv(glGetUniformLocation(objectProgram, "projection"), 1, GL_FALSE, glm::value_ptr(RenderVars::_MVP));

		glUniformMatrix3fv(glGetUniformLocation(objectProgram, "viewPos"), 1, GL_FALSE, glm::value_ptr(RV::_cameraPoint));
		//std::cout << RV::_cameraPoint.x <<" "<< RV::_cameraPoint.y <<" "<< RV::_cameraPoint.z << std::endl;

		glm::mat4 obj = glm::translate(objMat, glm::vec3(-12.f, 0.f, -20.f));
		glUniformMatrix4fv(glGetUniformLocation(objectProgram, "objMat"), 1, GL_FALSE, glm::value_ptr(obj));
		glUniform3f(glGetUniformLocation(objectProgram, "objectColor"), oC.x, oC.y, oC.z);
		glUniform3f(glGetUniformLocation(objectProgram, "light"), lPos.x, lPos.y, lPos.z);
		glUniform3f(glGetUniformLocation(objectProgram, "lightColor"), lColor.x, lColor.y, lColor.z);

		glUniform1f(glGetUniformLocation(objectProgram, "ex"), exponent);
		glUniform1f(glGetUniformLocation(objectProgram, "ambientStrength"), ambientStrength);
		glUniform1f(glGetUniformLocation(objectProgram, "specularStrength"), specularStrength);

		glDrawArrays(GL_TRIANGLES, 0, normals.size());

		glUseProgram(0);
		glBindVertexArray(0);
	}
}

namespace MainCharacter {
	GLuint objectVao;
	GLuint objectVbo[2];
	GLuint objectShaders[2];
	GLuint objectProgram;
	glm::mat4 objMat = glm::mat4(1.f);

	const char* object_vertShader =
		"#version 330\n\
in vec3 in_Position;\n\
in vec3 in_Normal;\n\
out vec3 vert_Normal;\n\
out vec3 frag_Pos;\n\
out vec3 LightPos;\n\
uniform vec3 light;\n\
uniform mat4 objMat;\n\
uniform mat4 mv_Mat;\n\
uniform mat4 projection;\n\
void main() {\n\
	gl_Position = projection * objMat * vec4(in_Position, 1.0);\n\
	frag_Pos = vec3(mv_Mat * objMat * vec4(in_Position, 1.0));\n\
	vert_Normal = mat3(transpose(inverse(mv_Mat * objMat))) *in_Normal;\n\
	LightPos = vec3(mv_Mat * vec4(light,1.0));\n\
}";
	const char* object_fragShader =
		"#version 330\n\
in vec3 vert_Normal;\n\
in vec3 frag_Pos;\n\
in vec3 LightPos;\n\
out vec4 frag_Color;\n\
uniform vec3 lightColor;\n\
uniform mat4 mv_Mat;\n\
uniform vec3 objectColor;\n\
uniform vec3 viewPos;\n\
uniform float ex;\n\
uniform float ambientStrength;\n\
uniform float specularStrength;\n\
void main() {\n\
	//ambient \n\
	vec3 ambient = ambientStrength * lightColor;\n\
	//diffuse\n\
	vec3 norm = normalize(vert_Normal);\n\
	vec3 lightDir = normalize(LightPos - frag_Pos);\n\
	float diff = max(dot(norm, lightDir), 0.0);\n\
	if(diff < 0.2) diff = 0;\n\
	if(diff >= 0.2 && diff < 0.4) diff = 0.2;\n\
	if(diff >= 0.4 && diff < 0.5) diff = 0.4;\n\
	if(diff >= 0.5) diff = 1;\n\
	vec3 diffuse = diff * lightColor;\n\
	//specular\n\
	vec3 viewDir = normalize(-frag_Pos);\n\
	vec3 reflectDir = reflect(-lightDir, norm);\n\
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), ex);\n\
	if(spec < 0.2) spec = 0;\n\
	if(spec >= 0.2 && spec < 0.4) spec = 0.2;\n\
	if(spec >= 0.4 && spec < 0.5) spec = 0.4;\n\
	if(spec >= 0.5) spec = 1;\n\
	vec3 specular = specularStrength * spec * lightColor;\n\
	vec3 result = (ambient + diffuse + specular) * objectColor;\n\
	frag_Color = vec4(result, 1.0);\n\
}";

	std::vector< glm::vec3 > vertices, normals;
	std::vector< glm::vec2 > uvs;

	void setupObject() {

		loadOBJ("Sephiroth_KH2.obj", vertices, uvs, normals);

		glGenVertexArrays(1, &objectVao);
		glBindVertexArray(objectVao);
		glGenBuffers(2, objectVbo);

		glBindBuffer(GL_ARRAY_BUFFER, objectVbo[0]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3)*vertices.size(), vertices.data(), GL_STATIC_DRAW);
		glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(0);

		glBindBuffer(GL_ARRAY_BUFFER, objectVbo[1]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3)*normals.size(), normals.data(), GL_STATIC_DRAW);
		glVertexAttribPointer((GLuint)1, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(1);

		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		objectShaders[0] = compileShader(object_vertShader, GL_VERTEX_SHADER, "objectVert");
		objectShaders[1] = compileShader(object_fragShader, GL_FRAGMENT_SHADER, "objectFrag");

		objectProgram = glCreateProgram();
		glAttachShader(objectProgram, objectShaders[0]);
		glAttachShader(objectProgram, objectShaders[1]);
		glBindAttribLocation(objectProgram, 0, "in_Position");
		glBindAttribLocation(objectProgram, 1, "in_Normal");
		linkProgram(objectProgram);
	}
	void cleanupObject() {
		glDeleteBuffers(2, objectVbo);
		glDeleteVertexArrays(1, &objectVao);

		glDeleteProgram(objectProgram);
		glDeleteShader(objectShaders[0]);
		glDeleteShader(objectShaders[1]);
	}
	void drawObject(float time) {
		glBindVertexArray(objectVao);
		glUseProgram(objectProgram);

		glUniformMatrix4fv(glGetUniformLocation(objectProgram, "mv_Mat"), 1, GL_FALSE, glm::value_ptr(RenderVars::_modelView));
		glUniformMatrix4fv(glGetUniformLocation(objectProgram, "projection"), 1, GL_FALSE, glm::value_ptr(RenderVars::_MVP));

		glUniformMatrix3fv(glGetUniformLocation(objectProgram, "viewPos"), 1, GL_FALSE, glm::value_ptr(RV::_cameraPoint));
		//std::cout << RV::_cameraPoint.x <<" "<< RV::_cameraPoint.y <<" "<< RV::_cameraPoint.z << std::endl;

		glm::mat4 obj = glm::translate(objMat, glm::vec3(0.f, 0.f, 0.f));
		glUniformMatrix4fv(glGetUniformLocation(objectProgram, "objMat"), 1, GL_FALSE, glm::value_ptr(obj));
		glUniform3f(glGetUniformLocation(objectProgram, "objectColor"), oC.x, oC.y, oC.z);
		glUniform3f(glGetUniformLocation(objectProgram, "light"), lPos.x, lPos.y, lPos.z);
		glUniform3f(glGetUniformLocation(objectProgram, "lightColor"), lColor.x, lColor.y, lColor.z);

		glUniform1f(glGetUniformLocation(objectProgram, "ex"), exponent);
		glUniform1f(glGetUniformLocation(objectProgram, "ambientStrength"), ambientStrength);
		glUniform1f(glGetUniformLocation(objectProgram, "specularStrength"), specularStrength);

		glDrawArrays(GL_TRIANGLES, 0, vertices.size());

		glUseProgram(0);
		glBindVertexArray(0);
	}
}


static const GLchar * vertex_shader_source[] =
{
			  "#version 330 \n\
			  void main() \n\
			  { \n\
							const vec4 vertices[3] = vec4[3](\n\
							vec4(0.25, -0.25, 0.5, 1.0),\n\
							vec4(0.25, 0.25, 0.5, 1.0),\n\
							vec4(-0.25, -0.25, 0.5, 1.0)); \n\
							gl_Position = \n\
							vertices[gl_VertexID]; \n\
			  }"

};


static const GLchar * fragment_shader_source[] =
{

			  "#version 330\n\
			  \n\
			  out vec4 color; \n\
			  \n\
			  void main() {\n\
 color = vec4(0.0,0.8,1.0,1.0); \n\
}"
};



GLuint compile_shaders(void)

{
	GLuint vertex_shader; //direccion de memoria del vertex shader
	GLuint fragment_shader; //direccion de memoria del fragment shader
	GLuint program;



	vertex_shader = glCreateShader(GL_VERTEX_SHADER); //crea shader tipo vertex
	glShaderSource(vertex_shader, 1, vertex_shader_source, NULL); //Tiene este codigo source
	glCompileShader(vertex_shader); //compila



	fragment_shader = glCreateShader(GL_FRAGMENT_SHADER); //mismo porceso que el vertex pero tipo fragment
	glShaderSource(fragment_shader, 1, fragment_shader_source, NULL);

	glCompileShader(fragment_shader);



	program = glCreateProgram();



	glAttachShader(program, vertex_shader);

	glAttachShader(program, fragment_shader);



	glLinkProgram(program);



	glDeleteShader(vertex_shader);

	glDeleteShader(fragment_shader);



	return program;

}





GLuint myRenderProgram;

GLuint myVao; //vertex array



void GLinit(int width, int height) {

	glViewport(0, 0, width, height);

	glClearColor(0.2f, 0.2f, 0.2f, 1.f);

	glClearDepth(1.f);

	glDepthFunc(GL_LEQUAL);

	glEnable(GL_DEPTH_TEST);

	glEnable(GL_CULL_FACE);



	RV::_projection = glm::perspective(glm::radians(40.f), (float)width / (float)height, RV::zNear, RV::zFar);
	
	renderW = (float)width;
	renderH = (float)height;





	// Setup shaders & geometry

	Axis::setupAxis();

	Cube::setupCube();

	Object1::setupObject();

	Object2::setupObject();

	Object3::setupObject();

	Object4::setupObject();

	MainCharacter::setupObject();


	glGenVertexArrays(1, &myVao);

	myRenderProgram = compile_shaders();

}



void GLcleanup() {

	Axis::cleanupAxis();

	Cube::cleanupCube();

	Object1::cleanupObject();

	Object2::cleanupObject();

	Object3::cleanupObject();

	Object4::cleanupObject();

	MainCharacter::cleanupObject();

	/////////////////////////////////////////////////////TODO

	// Do your cleanup code here

	// ...

	// ...

	// ...

	/////////////////////////////////////////////////////////

}



void GLrender(float dt) {

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	static float accum = 0.f;
	accum += dt;
	if (accum > glm::two_pi<float>())
	{
		accum = 0.f;
	}

	RV::_modelView = glm::mat4(1.f);

	if (!dolly)
	{
		RV::_projection = glm::perspective(RV::FOV, renderW / renderH, RV::zNear, RV::zFar);

		RV::_modelView = glm::translate(RV::_modelView, glm::vec3(RV::panv[0], RV::panv[1], RV::panv[2]));

		RV::_modelView = glm::rotate(RV::_modelView, RV::rota[1], glm::vec3(1.f, 0.f, 0.f));

		RV::_modelView = glm::rotate(RV::_modelView, RV::rota[0], glm::vec3(0.f, 1.f, 0.f));

		RV::_MVP = RV::_projection * RV::_modelView;

	}
	else                  //DOLLY EFFECT
	{
		RV::_projection = glm::perspective(modifiedFOV, renderW / renderH, RV::zNear, RV::zFar);
		RV::_MVP = RV::_projection * RV::_modelView;

		float zInit = -15.f;
		float zPos = zInit + 10 * sin(accum);
		RV::_MVP = glm::translate(RV::_MVP, glm::vec3(0.f, 0.f, zPos));

		modifiedFOV = 2 * (glm::atan((tan(RV::FOV / 2)* -zInit) / -zPos));
		
	}



	


	RV::_inv_modelview = glm::inverse(RV::_modelView);
	glm::vec4 mat = {0, 0, 0, 1};

	RV::_cameraPoint = RV::_inv_modelview * mat;
	   
	//Axis::drawAxis();

	

	//Cube::drawCube(accum);

	Object1::drawObject(accum);

	Object2::drawObject(accum);

	Object3::drawObject(accum);

	Object4::drawObject(accum);

	MainCharacter::drawObject(accum);

	/////////////////////////////////////////////////////TODO

	// Do your render code here

	// ...

	// ...

	/////////////////////////////////////////////////////////

	/*currentTime += dt;
	

	GLfloat color[] = { (float)sin(currentTime) * 0.5f + 0.5f, (float)cos(currentTime)* 0.5f + 0.5f, 0.0f, 1.0f };
	glClearBufferfv ( GL_COLOR, 0, color );*/


	//EX1:
	//glPointSize(40.0f);

	glBindVertexArray(myVao);
	glUseProgram(myRenderProgram);

	//EX1:
	//glDrawArrays(GL_POINTS, 0, 1);

	//EX2:
	//glDrawArrays(GL_TRIANGLES, 0, 3);



	ImGui::Render();

}

void GUI() {
	bool show = true;
	ImGui::Begin("Physics Parameters", &show, 0);

	{
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

		/////////////////////////////////////////////////////TODO
		// Do your GUI code here....
		ImGui::DragFloat3("Object Color", &oC.x, 0.05f, 0.f, 1.f);
		ImGui::DragFloat3("Light Position", &lPos.x, 0.05f, -50.f, 50.f);
		ImGui::DragFloat3("Light Color", &lColor.x, 0.05f, 0, 1 );
		ImGui::DragFloat("Ambient Strength", &ambientStrength, 0.05f, 0.f, 1.f);
		ImGui::DragFloat("Specular Strength", &specularStrength, 0.05f, 0.f, 1.f);
		ImGui::DragFloat("Exponent Light", &exponent, 8.f, 1, 256);

		if (ImGui::Button("Dolly Effect"))
		{
			dolly = true;
		}
		if (ImGui::Button("Normal Camera"))
		{
			dolly = false;
		}
		

		/////////////////////////////////////////////////////////
	}
	// .........................

	ImGui::End();

	// Example code -- ImGui test window. Most of the sample code is in ImGui::ShowTestWindow()
	bool show_test_window = false;
	if (show_test_window) {
		ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiSetCond_FirstUseEver);
		ImGui::ShowTestWindow(&show_test_window);
	}
}