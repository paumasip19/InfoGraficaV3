#include <GL\glew.h>
#include <glm\gtc\type_ptr.hpp>
#include <glm\gtc\matrix_transform.hpp>
#include <cstdio>
#include <cassert>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>

#include <imgui\imgui.h>
#include <imgui\imgui_impl_sdl_gl3.h>

#include "GL_framework.h"

float accum{0};

int ex1{ 0 };
int ex2{ 0 };
int ex3{ 0 };

bool ex1Enabled{ true };
bool ex2Enabled{ false };
bool ex3Enabled{ false };

bool loadOBJ(const char *path, std::vector<glm::vec3> &out_vertices, std::vector<glm::vec2> &out_uvs, std::vector<glm::vec3> &out_normals)
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
	const float FOV = glm::radians(65.f);
	const float zNear = 0.01f;
	const float zFar = 50.f;

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
	if (height != 0) RV::_projection = glm::perspective(RV::FOV, (float)width / (float)height, RV::zNear, RV::zFar);
	else RV::_projection = glm::perspective(RV::FOV, 0.f, RV::zNear, RV::zFar);
}

void GLmousecb(MouseEvent ev) {
	if (RV::prevMouse.waspressed && RV::prevMouse.button == ev.button) {
		float diffx = ev.posx - RV::prevMouse.lastx;
		float diffy = ev.posy - RV::prevMouse.lasty;
		switch (ev.button) {
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
	}
	else {
		RV::prevMouse.button = ev.button;
		RV::prevMouse.waspressed = true;
	}
	RV::prevMouse.lastx = ev.posx;
	RV::prevMouse.lasty = ev.posy;
}

//////////////////////////////////////////////////
GLuint compileShader(const GLchar* shaderStr, GLenum shaderType, const char* name = "") {

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

GLuint compileAndLoadShader(const GLchar* path, GLenum shaderType, const char* name = "") {

	std::string shaderStr;
	std::ifstream shaderFile;

	try {
		shaderFile.open(path);
		std::stringstream shaderStream;
		shaderStream << shaderFile.rdbuf();
		shaderFile.close();
		shaderStr = shaderStream.str();
		//std::cout << shaderStr << std::endl;
	}
	catch (std::ifstream::failure e)
	{
		std::cout << "error shader " << std::endl;
	}

	const GLchar * shaderChar = shaderStr.c_str();

	GLuint shader = glCreateShader(shaderType);
	glShaderSource(shader, 1, &shaderChar, NULL);
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

	void setupAxis() {
		glGenVertexArrays(1, &AxisVao);
		glBindVertexArray(AxisVao);
		glGenBuffers(3, AxisVbo);

		glBindBuffer(GL_ARRAY_BUFFER, AxisVbo[0]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 24, AxisVerts, GL_STATIC_DRAW);
		glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 0, 0);
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

		AxisShader[0] = compileAndLoadShader("shaders/axis.vs", GL_VERTEX_SHADER, "AxisVert");
		AxisShader[1] = compileAndLoadShader("shaders/axis.fs", GL_FRAGMENT_SHADER, "AxisFrag");

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
	GLuint cubeShaders[3];
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
		//std::cout << cube_fragShader << std::endl;
		cubeShaders[0] = compileAndLoadShader("shaders/vertex.vs", GL_VERTEX_SHADER, "cubeVert");
		cubeShaders[1] = compileAndLoadShader("shaders/fragment.fs", GL_FRAGMENT_SHADER, "cubeFrag");
		//cubeShaders[2] = compileAndLoadShader("shaders/geometry.gs", GL_GEOMETRY_SHADER, "cubeGeo");

		cubeProgram = glCreateProgram();
		glAttachShader(cubeProgram, cubeShaders[0]);
		glAttachShader(cubeProgram, cubeShaders[1]);
		//glAttachShader(cubeProgram, cubeShaders[2]);
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
		//glDeleteShader(cubeShaders[2]);
	}
	void updateCube(const glm::mat4& transform) {
		objMat = transform;
	}
	void drawCube() {
		glEnable(GL_PRIMITIVE_RESTART);
		glBindVertexArray(cubeVao);
		glUseProgram(cubeProgram);
		glUniformMatrix4fv(glGetUniformLocation(cubeProgram, "objMat"), 1, GL_FALSE, glm::value_ptr(objMat));
		glUniformMatrix4fv(glGetUniformLocation(cubeProgram, "mv_Mat"), 1, GL_FALSE, glm::value_ptr(RenderVars::_modelView));
		glUniformMatrix4fv(glGetUniformLocation(cubeProgram, "mvpMat"), 1, GL_FALSE, glm::value_ptr(RenderVars::_MVP));
		glUniform4f(glGetUniformLocation(cubeProgram, "color"), 0.1f, 1.f, 1.f, 0.f);
		glDrawElements(GL_TRIANGLE_STRIP, numVerts, GL_UNSIGNED_BYTE, 0);

		glUseProgram(0);
		glBindVertexArray(0);
		glDisable(GL_PRIMITIVE_RESTART);
	}
}

/////////////////////////////////////////////////

struct ObjectProps 
{
	GLuint objVao;
	GLuint objVbo[3];
	GLuint objShaders[3];
	GLuint objProgram;
	glm::mat4 objMat = glm::mat4(1.f);

	std::vector< glm::vec3 > vertices, normals;
	std::vector< glm::vec2 > uvs;
};

namespace Object {

	void setupObject(ObjectProps &props, GLchar * modelPath, GLchar * vsPath, GLchar * fsPath) {

		loadOBJ(modelPath, props.vertices, props.uvs, props.normals);

		glGenVertexArrays(1, &props.objVao);
		glBindVertexArray(props.objVao);
		glGenBuffers(3, props.objVbo);

		glBindBuffer(GL_ARRAY_BUFFER, props.objVbo[0]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3)*props.vertices.size(), props.vertices.data(), GL_STATIC_DRAW);
		glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(0);

		glBindBuffer(GL_ARRAY_BUFFER, props.objVbo[1]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3)*props.normals.size(), props.normals.data(), GL_STATIC_DRAW);
		glVertexAttribPointer((GLuint)1, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(1);

		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		//std::cout << cube_fragShader << std::endl;
		props.objShaders[0] = compileAndLoadShader(vsPath, GL_VERTEX_SHADER);
		props.objShaders[1] = compileAndLoadShader(fsPath, GL_FRAGMENT_SHADER);
		//cubeShaders[2] = compileAndLoadShader("shaders/geometry.gs", GL_GEOMETRY_SHADER, "cubeGeo");

		props.objProgram = glCreateProgram();
		glAttachShader(props.objProgram, props.objShaders[0]);
		glAttachShader(props.objProgram, props.objShaders[1]);
		//glAttachShader(cubeProgram, cubeShaders[2]);
		glBindAttribLocation(props.objProgram, 0, "in_Position");
		glBindAttribLocation(props.objProgram, 1, "in_Normal");
		linkProgram(props.objProgram);
	}
	void cleanupObject(ObjectProps &props) {
		glDeleteBuffers(3, props.objVbo);
		glDeleteVertexArrays(1, &props.objVao);

		glDeleteProgram(props.objProgram);
		glDeleteShader(props.objShaders[0]);
		glDeleteShader(props.objShaders[1]);
		//glDeleteShader(cubeShaders[2]);
	}
	void updateObject(const glm::mat4& transform, ObjectProps props) {
		props.objMat = transform;
	}
	void drawObject(ObjectProps &props) {
		glEnable(GL_PRIMITIVE_RESTART);
		glBindVertexArray(props.objVao);
		glUseProgram(props.objProgram);
		glUniformMatrix4fv(glGetUniformLocation(props.objProgram, "objMat"), 1, GL_FALSE, glm::value_ptr(props.objMat));
		glUniformMatrix4fv(glGetUniformLocation(props.objProgram, "mv_Mat"), 1, GL_FALSE, glm::value_ptr(RenderVars::_modelView));
		glUniformMatrix4fv(glGetUniformLocation(props.objProgram, "mvpMat"), 1, GL_FALSE, glm::value_ptr(RenderVars::_MVP));
		glUniform4f(glGetUniformLocation(props.objProgram, "color"), 0.1f, 1.f, 1.f, 0.f);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, props.vertices.size());

		glUseProgram(0);
		glBindVertexArray(0);
		glDisable(GL_PRIMITIVE_RESTART);
	}
};

#pragma region Objects
namespace Noria {
	ObjectProps props;
	GLchar * modelPath("models/Noria.obj");
	GLchar * vsPath("shaders/vertex.vs");
	GLchar * fsPath("shaders/fragment.fs");
}

namespace Cabina {
	ObjectProps props;
	GLchar * modelPath("models/CabinaNoria.obj");
	GLchar * vsPath("shaders/vertex.vs");
	GLchar * fsPath("shaders/fragment.fs");
}

namespace Base {
	ObjectProps props;
	GLchar * modelPath("models/BaseNoria.obj");
	GLchar * vsPath("shaders/vertex.vs");
	GLchar * fsPath("shaders/fragment.fs");
}

namespace Trump {
	ObjectProps props;
	GLchar * modelPath("models/Trump.obj");
	GLchar * vsPath("shaders/vertex.vs");
	GLchar * fsPath("shaders/trump.fs");
}

namespace Chicken {
	ObjectProps props;
	GLchar * modelPath("models/Pollo.obj");
	GLchar * vsPath("shaders/vertex.vs");
	GLchar * fsPath("shaders/fragment.fs");
}

#pragma endregion

#pragma region Inits

void initEx1() 
{
	accum = 0;

	Object::setupObject(Cabina::props, Cabina::modelPath, Cabina::vsPath, Cabina::fsPath);
	Object::setupObject(Noria::props, Noria::modelPath, Noria::vsPath, Noria::fsPath);
	Object::setupObject(Base::props, Base::modelPath, Base::vsPath, Base::fsPath);
	Object::setupObject(Trump::props, Trump::modelPath, Trump::vsPath, Trump::fsPath);
	Object::setupObject(Chicken::props, Chicken::modelPath, Chicken::vsPath, Chicken::fsPath);

	Base::props.objMat = glm::translate(Base::props.objMat, { 0,-8,0 });

};

void initEx2() {

}

void initEx3() {

}

#pragma endregion

void GLinit(int width, int height) {
	glViewport(0, 0, width, height);
	glClearColor(0.2f, 0.2f, 0.2f, 1.f);
	glClearDepth(1.f);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_DEPTH_TEST);
	//glEnable(GL_CULL_FACE);

	RV::_projection = glm::perspective(RV::FOV, (float)width / (float)height, RV::zNear, RV::zFar);

	// Setup shaders & geometry
	Axis::setupAxis();
	/////////////////////////////////////////////////////TODO
	// Do your init code here
	// ...
	// ...
	// ...
	/////////////////////////////////////////////////////////

	initEx1();

}

#pragma region ExerciseCleanups

void Ex1Cleanup() {

	Object::cleanupObject(Noria::props);
	Object::cleanupObject(Cabina::props);
	Object::cleanupObject(Base::props);
	Object::cleanupObject(Trump::props);
	Object::cleanupObject(Chicken::props);
}

void Ex2Cleanup() {

}

void Ex3Cleanup() {

}


#pragma endregion

void GLcleanup() {
	Axis::cleanupAxis();

	/////////////////////////////////////////////////////TODO
	// Do your cleanup code here
	// ...
	// ...
	// ...
	/////////////////////////////////////////////////////////
}

#pragma region renders

void RenderEx1(float dt) {

	int maxCabines = 20;
	float radius = 7.f;
	float tau = glm::two_pi<float>();
	float freq = 0.1f;
	accum += dt;

	if (glm::cos(tau*freq*accum) >= 0.9999f) accum = 0; //reset accum at the end of the wheelloop

	for (int i = 0; i < maxCabines; i++)
	{
		if (i == 0)
		{
			Noria::props.objMat = glm::rotate(glm::mat4(1.f), accum*freq*tau, glm::vec3(0, 0, 1));
			glm::vec3 position = glm::vec3(radius * glm::cos(tau*freq*accum), radius * glm::sin(tau*freq*accum), 0);
			Trump::props.objMat = Chicken::props.objMat = glm::translate(glm::mat4(1.f), position);
			Trump::props.objMat = glm::translate(Trump::props.objMat, { -0.25f,-0.8f,0 });
			Chicken::props.objMat = glm::translate(Chicken::props.objMat, { 0.25f,-0.8f,0 });

			//Shot reverse Shot
			if ((int)accum % 2 == 0) RV::_modelView = glm::rotate(glm::mat4(1.f), glm::radians(-90.f), glm::vec3{ 0, 1, 0 }); //Trump
			else RV::_modelView = glm::rotate(glm::mat4(1.f), glm::radians(90.f), glm::vec3{ 0, 1, 0 }); //Chicken

			RV::_modelView = glm::translate(RV::_modelView, -position - glm::vec3{ 0, -0.5f, 0 });
			RV::_MVP = RV::_projection * RV::_modelView;
		}

		glm::vec3 position = glm::vec3(radius * glm::cos(tau*freq*accum + (tau*i / maxCabines)), radius * glm::sin(tau*freq*accum + (tau*i / maxCabines)), 0);
		Cabina::props.objMat = glm::translate(glm::mat4(1.f), position);
		Object::drawObject(Cabina::props);
	}


	Object::drawObject(Noria::props);
	Object::drawObject(Base::props);
	Object::drawObject(Trump::props);
	Object::drawObject(Chicken::props);

}

#pragma endregion

void GLrender(float dt) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	/*RV::_modelView = glm::mat4(1.f);
	RV::_modelView = glm::translate(RV::_modelView, glm::vec3(RV::panv[0], RV::panv[1], RV::panv[2]));
	RV::_modelView = glm::rotate(RV::_modelView, RV::rota[1], glm::vec3(1.f, 0.f, 0.f));
	RV::_modelView = glm::rotate(RV::_modelView, RV::rota[0], glm::vec3(0.f, 1.f, 0.f));

	RV::_MVP = RV::_projection * RV::_modelView;*/

	Axis::drawAxis();
	/////////////////////////////////////////////////////TODO
	// Do your render code here
	// ...
	// ...
	// ...
	/////////////////////////////////////////////////////////

	if(ex1Enabled) RenderEx1(dt);

	

	ImGui::Render();

	if (ex1 & 1)
	{
		Ex1Cleanup();
		initEx1();
		ex1Enabled = true;
		ex2Enabled = ex3Enabled = false;
		ex1--;
	}
	else if (ex2 & 1) {
		Ex2Cleanup();
		initEx2();
		ex2Enabled = true;
		ex1Enabled = ex3Enabled = false;
		ex2--;
	}
	else if (ex3 & 1) {
		Ex3Cleanup();
		initEx3();
		ex3Enabled = true;
		ex2Enabled = ex1Enabled = false;
		ex3--;
	}
}

void GUI() {
	bool show = true;
	ImGui::Begin("Physics Parameters", &show, 0);

	{
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

		/////////////////////////////////////////////////////TODO
		// Do your GUI code here....
		// ...
		// ...
		// ...
		/////////////////////////////////////////////////////////

		if (ImGui::Button("Exercise 1")) ex1++;
		if (ImGui::Button("Exercise 2")) ex2++;
		if (ImGui::Button("Exercise 3")) ex3++;
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