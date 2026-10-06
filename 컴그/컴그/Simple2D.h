#pragma once

#include <GL/glew.h>
#include <GL/glfw3.h>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>

namespace Simple2D
{
constexpr float PI = 3.14159265358979323846f;

struct Vec2
{
	float x;
	float y;
};

struct Color
{
	float r;
	float g;
	float b;
};

struct Vertex
{
	float x;
	float y;
	float z;
	float r;
	float g;
	float b;
};

enum class ShapeKind
{
	Square,
	Triangle,
	InvertedTriangle,
	RightTriangle
};

inline Vertex MakeVertex(Vec2 position, Color color)
{
	return {
		position.x, position.y, 0.0f,
		color.r, color.g, color.b
	};
}

inline Vec2 Rotate(Vec2 point, float angle)
{
	float cosine = std::cos(angle);
	float sine = std::sin(angle);
	return {
		point.x * cosine - point.y * sine,
		point.x * sine + point.y * cosine
	};
}

inline Vec2 Add(Vec2 left, Vec2 right)
{
	return { left.x + right.x, left.y + right.y };
}

inline float Distance(Vec2 left, Vec2 right)
{
	float dx = left.x - right.x;
	float dy = left.y - right.y;
	return std::sqrt(dx * dx + dy * dy);
}

inline GLuint CompileShader(GLenum shaderType, const char* source)
{
	GLuint shader = glCreateShader(shaderType);
	glShaderSource(shader, 1, &source, nullptr);
	glCompileShader(shader);

	GLint success = GL_FALSE;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (success == GL_TRUE)
		return shader;

	GLchar message[1024]{};
	glGetShaderInfoLog(shader, 1024, nullptr, message);
	std::cerr << "Shader compile failed:\n" << message << std::endl;
	glDeleteShader(shader);
	return 0;
}

inline GLuint CreateShaderProgram()
{
	const char* vertexSource =
		"#version 330 core\n"
		"layout(location = 0) in vec3 vPosition;\n"
		"layout(location = 1) in vec3 vColor;\n"
		"out vec3 passColor;\n"
		"void main()\n"
		"{\n"
		"    gl_Position = vec4(vPosition, 1.0);\n"
		"    passColor = vColor;\n"
		"}\n";

	const char* fragmentSource =
		"#version 330 core\n"
		"in vec3 passColor;\n"
		"out vec4 FragColor;\n"
		"void main()\n"
		"{\n"
		"    FragColor = vec4(passColor, 1.0);\n"
		"}\n";

	GLuint vertexShader = CompileShader(GL_VERTEX_SHADER, vertexSource);
	GLuint fragmentShader = CompileShader(GL_FRAGMENT_SHADER, fragmentSource);
	if (vertexShader == 0 || fragmentShader == 0)
		return 0;

	GLuint program = glCreateProgram();
	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);
	glLinkProgram(program);

	GLint success = GL_FALSE;
	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if (success != GL_TRUE) {
		GLchar message[1024]{};
		glGetProgramInfoLog(program, 1024, nullptr, message);
		std::cerr << "Shader link failed:\n" << message << std::endl;
		glDeleteProgram(program);
		program = 0;
	}

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
	return program;
}

class Renderer
{
public:
	bool Initialize(int maximumVertices = 32768)
	{
		program = CreateShaderProgram();
		if (program == 0)
			return false;

		capacity = maximumVertices;
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(
			GL_ARRAY_BUFFER,
			static_cast<GLsizeiptr>(sizeof(Vertex) * capacity),
			nullptr,
			GL_DYNAMIC_DRAW
		);

		glVertexAttribPointer(
			0, 3, GL_FLOAT, GL_FALSE,
			sizeof(Vertex), reinterpret_cast<void*>(0)
		);
		glEnableVertexAttribArray(0);

		glVertexAttribPointer(
			1, 3, GL_FLOAT, GL_FALSE,
			sizeof(Vertex),
			reinterpret_cast<void*>(sizeof(float) * 3)
		);
		glEnableVertexAttribArray(1);

		glBindVertexArray(0);
		return true;
	}

	void BeginFrame(Color background)
	{
		glClearColor(background.r, background.g, background.b, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		glUseProgram(program);
		glBindVertexArray(vao);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
	}

	void Draw(GLenum mode, const std::vector<Vertex>& vertices)
	{
		if (vertices.empty())
			return;

		if (static_cast<int>(vertices.size()) > capacity) {
			std::cerr << "Too many vertices for the shared VBO." << std::endl;
			return;
		}

		glBufferSubData(
			GL_ARRAY_BUFFER,
			0,
			static_cast<GLsizeiptr>(sizeof(Vertex) * vertices.size()),
			vertices.data()
		);
		glDrawArrays(mode, 0, static_cast<GLsizei>(vertices.size()));
	}

	void EndFrame()
	{
		glBindVertexArray(0);
	}

	void Shutdown()
	{
		if (vbo != 0)
			glDeleteBuffers(1, &vbo);
		if (vao != 0)
			glDeleteVertexArrays(1, &vao);
		if (program != 0)
			glDeleteProgram(program);
		vbo = vao = program = 0;
	}

private:
	GLuint program = 0;
	GLuint vao = 0;
	GLuint vbo = 0;
	int capacity = 0;
};

inline std::vector<Vertex> MakeLine(Vec2 start, Vec2 end, Color color)
{
	return {
		MakeVertex(start, color),
		MakeVertex(end, color)
	};
}

inline std::vector<Vertex> MakeRectangleOutline(
	Vec2 center,
	float width,
	float height,
	Color color)
{
	float halfWidth = width * 0.5f;
	float halfHeight = height * 0.5f;
	return {
		MakeVertex({ center.x - halfWidth, center.y - halfHeight }, color),
		MakeVertex({ center.x + halfWidth, center.y - halfHeight }, color),
		MakeVertex({ center.x + halfWidth, center.y + halfHeight }, color),
		MakeVertex({ center.x - halfWidth, center.y + halfHeight }, color)
	};
}

inline std::vector<Vertex> MakeFilledRectangle(
	Vec2 center,
	float width,
	float height,
	Color color,
	float angle = 0.0f)
{
	float halfWidth = width * 0.5f;
	float halfHeight = height * 0.5f;
	Vec2 local[4] = {
		{ -halfWidth, -halfHeight },
		{  halfWidth, -halfHeight },
		{  halfWidth,  halfHeight },
		{ -halfWidth,  halfHeight }
	};
	for (Vec2& point : local)
		point = Add(center, Rotate(point, angle));

	return {
		MakeVertex(local[0], color),
		MakeVertex(local[1], color),
		MakeVertex(local[2], color),
		MakeVertex(local[0], color),
		MakeVertex(local[2], color),
		MakeVertex(local[3], color)
	};
}

inline std::vector<Vertex> MakeShape(
	ShapeKind kind,
	Vec2 center,
	float size,
	Color color,
	float angle = 0.0f)
{
	if (kind == ShapeKind::Square)
		return MakeFilledRectangle(center, size, size, color, angle);

	Vec2 local[3]{};
	if (kind == ShapeKind::Triangle) {
		local[0] = { 0.0f, size * 0.58f };
		local[1] = { -size * 0.5f, -size * 0.42f };
		local[2] = { size * 0.5f, -size * 0.42f };
	}
	else if (kind == ShapeKind::InvertedTriangle) {
		local[0] = { 0.0f, -size * 0.58f };
		local[1] = { -size * 0.5f, size * 0.42f };
		local[2] = { size * 0.5f, size * 0.42f };
	}
	else {
		local[0] = { -size * 0.5f, -size * 0.5f };
		local[1] = { size * 0.5f, -size * 0.5f };
		local[2] = { -size * 0.5f, size * 0.5f };
	}

	for (Vec2& point : local)
		point = Add(center, Rotate(point, angle));

	return {
		MakeVertex(local[0], color),
		MakeVertex(local[1], color),
		MakeVertex(local[2], color)
	};
}

inline std::vector<Vertex> MakeShapeOutline(
	ShapeKind kind,
	Vec2 center,
	float size,
	Color color,
	float angle = 0.0f)
{
	if (kind == ShapeKind::Square) {
		std::vector<Vertex> rectangle =
			MakeRectangleOutline(center, size, size, color);
		if (angle == 0.0f)
			return rectangle;

		for (Vertex& vertex : rectangle) {
			Vec2 local{ vertex.x - center.x, vertex.y - center.y };
			Vec2 rotated = Add(center, Rotate(local, angle));
			vertex.x = rotated.x;
			vertex.y = rotated.y;
		}
		return rectangle;
	}

	std::vector<Vertex> filled = MakeShape(kind, center, size, color, angle);
	return { filled[0], filled[1], filled[2] };
}

inline Vec2 MouseToOpenGL(GLFWwindow* window, double mouseX, double mouseY)
{
	int width = 1;
	int height = 1;
	glfwGetWindowSize(window, &width, &height);
	return {
		static_cast<float>(mouseX / width) * 2.0f - 1.0f,
		1.0f - static_cast<float>(mouseY / height) * 2.0f
	};
}

inline bool PointInShape(
	ShapeKind kind,
	Vec2 point,
	Vec2 center,
	float size)
{
	float dx = std::abs(point.x - center.x);
	float dy = std::abs(point.y - center.y);
	if (kind == ShapeKind::Square)
		return dx <= size * 0.5f && dy <= size * 0.5f;

	// 삼각형은 초보 실습에서 선택하기 편하도록 바운딩 박스로 판정한다.
	return dx <= size * 0.55f && dy <= size * 0.60f;
}
}
