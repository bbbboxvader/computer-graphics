#pragma once

#include <GL/glew.h>
#include <GL/glfw3.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

// 네 실습에서 공통으로 사용하는 아주 기본적인 자료형과 함수들이다.
// PDF에 나온 배열, 구조체, VAO, VBO, glDrawArrays 방식만 사용한다.

const float PI = 3.1415926535f;

struct Vertex
{
	float x;
	float y;
	float z;
	float r;
	float g;
	float b;
};

enum ShapeType
{
	SQUARE = 0,
	TRIANGLE = 1,
	INVERTED_TRIANGLE = 2,
	RIGHT_TRIANGLE = 3
};

struct Point2D
{
	float x;
	float y;
};

Vertex MakeVertex(float x, float y, float r, float g, float b)
{
	Vertex vertex;
	vertex.x = x;
	vertex.y = y;
	vertex.z = 0.0f;
	vertex.r = r;
	vertex.g = g;
	vertex.b = b;
	return vertex;
}

Point2D RotatePoint(float x, float y, float angle)
{
	Point2D result;
	result.x = x * std::cos(angle) - y * std::sin(angle);
	result.y = x * std::sin(angle) + y * std::cos(angle);
	return result;
}
float Distance(float x1, float y1, float x2, float y2)
{
	float dx = x1 - x2;
	float dy = y1 - y2;
	return std::sqrt(dx * dx + dy * dy);
}

float RandomFloat(float minimum, float maximum)
{
	float zeroToOne = static_cast<float>(std::rand()) / RAND_MAX;
	return minimum + (maximum - minimum) * zeroToOne;
}

std::string ReadTextFile(const char* fileName)
{
	std::ifstream file(fileName);
	if (!file.is_open()) {
		std::cerr << "파일을 열 수 없습니다: " << fileName << std::endl;
		return "";
	}

	std::stringstream text;
	text << file.rdbuf();
	return text.str();
}

GLuint CompileShader(GLenum shaderType, const char* fileName)
{
	std::string shaderText = ReadTextFile(fileName);
	if (shaderText.empty())
		return 0;

	const char* source = shaderText.c_str();
	GLuint shader = glCreateShader(shaderType);
	glShaderSource(shader, 1, &source, nullptr);
	glCompileShader(shader);

	GLint success = GL_FALSE;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (success == GL_FALSE) {
		char errorMessage[512];
		glGetShaderInfoLog(shader, 512, nullptr, errorMessage);
		std::cerr << "셰이더 컴파일 실패: " << fileName << "\n";
		std::cerr << errorMessage << std::endl;
		glDeleteShader(shader);
		return 0;
	}

	return shader;
}

GLuint MakeShaderProgram(const char* vertexFile, const char* fragmentFile)
{
	GLuint vertexShader = CompileShader(GL_VERTEX_SHADER, vertexFile);
	GLuint fragmentShader = CompileShader(GL_FRAGMENT_SHADER, fragmentFile);
	if (vertexShader == 0 || fragmentShader == 0)
		return 0;

	GLuint program = glCreateProgram();
	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);
	glLinkProgram(program);

	GLint success = GL_FALSE;
	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if (success == GL_FALSE) {
		char errorMessage[512];
		glGetProgramInfoLog(program, 512, nullptr, errorMessage);
		std::cerr << "셰이더 프로그램 연결 실패\n";
		std::cerr << errorMessage << std::endl;
		glDeleteProgram(program);
		program = 0;
	}

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
	return program;
}

void InitBuffer(GLuint& VAO, GLuint& VBO, int maximumVertexCount)
{
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(
		GL_ARRAY_BUFFER,
		sizeof(Vertex) * maximumVertexCount,
		nullptr,
		GL_DYNAMIC_DRAW
	);

	// layout(location = 0): 정점의 x, y, z 위치
	glVertexAttribPointer(
		0, 3, GL_FLOAT, GL_FALSE,
		sizeof(Vertex), reinterpret_cast<void*>(0)
	);
	glEnableVertexAttribArray(0);

	// layout(location = 1): 정점의 r, g, b 색상
	glVertexAttribPointer(
		1, 3, GL_FLOAT, GL_FALSE,
		sizeof(Vertex), reinterpret_cast<void*>(sizeof(float) * 3)
	);
	glEnableVertexAttribArray(1);

	glBindVertexArray(0);
}

void DrawVertices(GLuint VBO, GLenum drawMode, Vertex vertices[], int count)
{
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vertex) * count, vertices);
	glDrawArrays(drawMode, 0, count);
}

int MakeFilledShape(
	Vertex result[],
	ShapeType kind,
	float centerX,
	float centerY,
	float size,
	float angle,
	float r,
	float g,
	float b)
{
	Point2D local[4];
	int pointCount = 0;

	if (kind == SQUARE) {
		float half = size * 0.5f;
		local[0] = { -half, -half };
		local[1] = {  half, -half };
		local[2] = {  half,  half };
		local[3] = { -half,  half };
		pointCount = 4;
	}
	else if (kind == TRIANGLE) {
		local[0] = { 0.0f, size * 0.58f };
		local[1] = { -size * 0.5f, -size * 0.42f };
		local[2] = {  size * 0.5f, -size * 0.42f };
		pointCount = 3;
	}
	else if (kind == INVERTED_TRIANGLE) {
		local[0] = { 0.0f, -size * 0.58f };
		local[1] = { -size * 0.5f, size * 0.42f };
		local[2] = {  size * 0.5f, size * 0.42f };
		pointCount = 3;
	}
	else {
		local[0] = { -size * 0.5f, -size * 0.5f };
		local[1] = {  size * 0.5f, -size * 0.5f };
		local[2] = { -size * 0.5f,  size * 0.5f };
		pointCount = 3;
	}

	Point2D world[4];
	for (int i = 0; i < pointCount; ++i) {
		Point2D rotated = RotatePoint(local[i].x, local[i].y, angle);
		world[i] = { centerX + rotated.x, centerY + rotated.y };
	}

	if (kind == SQUARE) {
		result[0] = MakeVertex(world[0].x, world[0].y, r, g, b);
		result[1] = MakeVertex(world[1].x, world[1].y, r, g, b);
		result[2] = MakeVertex(world[2].x, world[2].y, r, g, b);
		result[3] = MakeVertex(world[0].x, world[0].y, r, g, b);
		result[4] = MakeVertex(world[2].x, world[2].y, r, g, b);
		result[5] = MakeVertex(world[3].x, world[3].y, r, g, b);
		return 6;
	}

	for (int i = 0; i < 3; ++i)
		result[i] = MakeVertex(world[i].x, world[i].y, r, g, b);
	return 3;
}

int MakeShapeOutline(
	Vertex result[],
	ShapeType kind,
	float centerX,
	float centerY,
	float size,
	float angle,
	float r,
	float g,
	float b)
{
	Vertex filled[6];
	int filledCount = MakeFilledShape(
		filled, kind, centerX, centerY, size, angle, r, g, b
	);

	if (kind == SQUARE) {
		result[0] = filled[0];
		result[1] = filled[1];
		result[2] = filled[2];
		result[3] = filled[5];
		return 4;
	}

	result[0] = filled[0];
	result[1] = filled[1];
	result[2] = filled[2];
	return filledCount;
}

bool PointInsideShape(
	float pointX,
	float pointY,
	float centerX,
	float centerY,
	float size)
{
	return std::abs(pointX - centerX) <= size * 0.55f &&
		std::abs(pointY - centerY) <= size * 0.60f;
}

Point2D MouseToOpenGL(GLFWwindow* window, double mouseX, double mouseY)
{
	int width = 1;
	int height = 1;
	glfwGetWindowSize(window, &width, &height);

	Point2D result;
	result.x = static_cast<float>(mouseX / width) * 2.0f - 1.0f;
	result.y = 1.0f - static_cast<float>(mouseY / height) * 2.0f;
	return result;
}
