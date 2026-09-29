#include <GL/glew.h>
#include <GL/glfw3.h>
#include <iostream>
#include <random>

// 실습 8: 화면을 4분할하여 삼각형 그리기

struct Triangle {
	float X;
	float Y;
	float size;
	float R;
	float G;
	float B;
	bool active;
	bool growing;
};

struct Vertex {
	float x;
	float y;
	float z;
	float R;
	float G;
	float B;
};

Triangle triangles[4];
bool fillMode = true;

GLuint shaderProgram = 0;
GLuint VAO = 0;
GLuint VBO = 0;

std::random_device rd;
std::mt19937 gen(rd());

// 새 삼각형의 크기와 색상을 정할 난수 범위
std::uniform_real_distribution<float> sizeDist(0.16f, 0.42f);
std::uniform_real_distribution<float> colorDist(0.1f, 1.0f);

GLuint CompileShader(GLenum shaderType, const char* shaderSource)
{
	GLuint shader = glCreateShader(shaderType);
	glShaderSource(shader, 1, &shaderSource, nullptr);
	glCompileShader(shader);

	GLint success;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

	if (!success) {
		char message[512];
		glGetShaderInfoLog(shader, 512, nullptr, message);
		std::cout << "셰이더 컴파일 실패!\n" << message << std::endl;
		glDeleteShader(shader);
		return 0;
	}

	return shader;
}

GLuint MakeShaderProgram()
{
	const char* vertexSource =
		"#version 330 core\n"
		"layout (location = 0) in vec3 vPosition;\n"
		"layout (location = 1) in vec3 vColor;\n"
		"out vec3 outColor;\n"
		"void main()\n"
		"{\n"
		"    gl_Position = vec4(vPosition, 1.0);\n"
		"    outColor = vColor;\n"
		"}\n";

	const char* fragmentSource =
		"#version 330 core\n"
		"in vec3 outColor;\n"
		"out vec4 FragColor;\n"
		"void main()\n"
		"{\n"
		"    FragColor = vec4(outColor, 1.0);\n"
		"}\n";

	GLuint vertexShader = CompileShader(GL_VERTEX_SHADER, vertexSource);
	GLuint fragmentShader = CompileShader(GL_FRAGMENT_SHADER, fragmentSource);

	if (vertexShader == 0 || fragmentShader == 0)
		return 0;

	GLuint program = glCreateProgram();
	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);
	glLinkProgram(program);

	GLint success;
	glGetProgramiv(program, GL_LINK_STATUS, &success);

	if (!success) {
		char message[512];
		glGetProgramInfoLog(program, 512, nullptr, message);
		std::cout << "셰이더 연결 실패!\n" << message << std::endl;
		glDeleteProgram(program);
		program = 0;
	}

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
	return program;
}

void InitBuffer()
{
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * 4, nullptr, GL_DYNAMIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(
		1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
		(void*)(sizeof(float) * 3)
	);
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void SetVertex(
	Vertex& vertex,
	float x,
	float y,
	float R,
	float G,
	float B)
{
	vertex.x = x;
	vertex.y = y;
	vertex.z = 0.0f;
	vertex.R = R;
	vertex.G = G;
	vertex.B = B;
}

void KeepInQuadrant(int index)
{
	Triangle& triangle = triangles[index];
	float half = triangle.size / 2.0f;

	bool left = index == 0 || index == 2;
	bool top = index == 0 || index == 1;

	if (left) {
		if (triangle.X < -1.0f + half)
			triangle.X = -1.0f + half;
		if (triangle.X > -half)
			triangle.X = -half;
	}
	else {
		if (triangle.X < half)
			triangle.X = half;
		if (triangle.X > 1.0f - half)
			triangle.X = 1.0f - half;
	}

	if (top) {
		if (triangle.Y < half)
			triangle.Y = half;
		if (triangle.Y > 1.0f - half)
			triangle.Y = 1.0f - half;
	}
	else {
		if (triangle.Y < -1.0f + half)
			triangle.Y = -1.0f + half;
		if (triangle.Y > -half)
			triangle.Y = -half;
	}
}

void MakeTriangle(int index, float x, float y)
{
	Triangle& triangle = triangles[index];
	triangle.X = x;
	triangle.Y = y;

	// 이전 삼각형 대신 새로운 크기와 색상을 넣는다.
	triangle.size = sizeDist(gen);
	triangle.R = colorDist(gen);
	triangle.G = colorDist(gen);
	triangle.B = colorDist(gen);

	triangle.active = true;
	triangle.growing = true;

	KeepInQuadrant(index);
}

void ResetTriangles()
{
	MakeTriangle(0, -0.55f, 0.55f);
	MakeTriangle(1, 0.55f, 0.55f);
	MakeTriangle(2, -0.55f, -0.55f);
	MakeTriangle(3, 0.55f, -0.55f);
}

void ChangeTriangleSize(int index)
{
	Triangle& triangle = triangles[index];

	if (!triangle.active)
		return;

	if (triangle.growing)
		triangle.size += 0.04f;
	else
		triangle.size -= 0.04f;

	if (triangle.size >= 0.46f) {
		triangle.size = 0.46f;
		triangle.growing = false;
	}

	if (triangle.size <= 0.12f) {
		triangle.size = 0.12f;
		triangle.growing = true;
	}

	KeepInQuadrant(index);
}

void GetOpenGLMousePosition(
	GLFWwindow* window,
	double mouseX,
	double mouseY,
	float& openGLX,
	float& openGLY)
{
	int width;
	int height;
	glfwGetWindowSize(window, &width, &height);

	openGLX = static_cast<float>(mouseX / width) * 2.0f - 1.0f;
	openGLY = 1.0f - static_cast<float>(mouseY / height) * 2.0f;
}

int GetQuadrant(float x, float y)
{
	if (x < 0.0f && y >= 0.0f)
		return 0;
	if (x >= 0.0f && y >= 0.0f)
		return 1;
	if (x < 0.0f && y < 0.0f)
		return 2;
	return 3;
}

void MouseButtonCallback(
	GLFWwindow* window,
	int button,
	int action,
	int mods)
{
	if (action != GLFW_PRESS)
		return;

	double mouseX;
	double mouseY;
	glfwGetCursorPos(window, &mouseX, &mouseY);

	float x;
	float y;
	GetOpenGLMousePosition(window, mouseX, mouseY, x, y);

	int index = GetQuadrant(x, y);

	if (button == GLFW_MOUSE_BUTTON_LEFT)
		MakeTriangle(index, x, y);

	if (button == GLFW_MOUSE_BUTTON_RIGHT)
		ChangeTriangleSize(index);
}

void KeyCallback(
	GLFWwindow* window,
	int key,
	int scancode,
	int action,
	int mods)
{
	if (action != GLFW_PRESS)
		return;

	if (key == GLFW_KEY_Q)
		glfwSetWindowShouldClose(window, true);

	// A: 면으로 그리기
	if (key == GLFW_KEY_A)
		fillMode = true;

	// B: 선으로 그리기
	if (key == GLFW_KEY_B)
		fillMode = false;

	// C: 모두 삭제하고 기본 위치에 다시 그리기
	if (key == GLFW_KEY_C)
		ResetTriangles();
}

void DrawVertices(Vertex vertices[], int count, GLenum mode)
{
	glBufferSubData(
		GL_ARRAY_BUFFER,
		0,
		sizeof(Vertex) * count,
		vertices
	);
	glDrawArrays(mode, 0, count);
}

void DrawGuideLines()
{
	Vertex lines[4];

	SetVertex(lines[0], -1.0f, 0.0f, 0.75f, 0.75f, 0.75f);
	SetVertex(lines[1], 1.0f, 0.0f, 0.75f, 0.75f, 0.75f);
	SetVertex(lines[2], 0.0f, -1.0f, 0.75f, 0.75f, 0.75f);
	SetVertex(lines[3], 0.0f, 1.0f, 0.75f, 0.75f, 0.75f);

	DrawVertices(lines, 4, GL_LINES);
}

void DrawTriangle(const Triangle& triangle)
{
	if (!triangle.active)
		return;

	float half = triangle.size / 2.0f;
	Vertex vertices[3];

	SetVertex(
		vertices[0],
		triangle.X,
		triangle.Y + half,
		triangle.R, triangle.G, triangle.B
	);
	SetVertex(
		vertices[1],
		triangle.X - half,
		triangle.Y - half,
		triangle.R, triangle.G, triangle.B
	);
	SetVertex(
		vertices[2],
		triangle.X + half,
		triangle.Y - half,
		triangle.R, triangle.G, triangle.B
	);

	if (fillMode)
		DrawVertices(vertices, 3, GL_TRIANGLES);
	else
		DrawVertices(vertices, 3, GL_LINE_LOOP);
}

void DrawScene()
{
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	glUseProgram(shaderProgram);
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);

	DrawGuideLines();

	for (int i = 0; i < 4; i++)
		DrawTriangle(triangles[i]);

	glBindVertexArray(0);
}

void FramebufferSizeCallback(
	GLFWwindow* window,
	int width,
	int height)
{
	glViewport(0, 0, width, height);
}

int main()
{
	if (!glfwInit()) {
		std::cerr << "GLFW 초기화 실패!" << std::endl;
		return -1;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(
		800, 600, "Practice 8", nullptr, nullptr
	);

	if (!window) {
		std::cerr << "윈도우 생성 실패!" << std::endl;
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);
	glewExperimental = GL_TRUE;

	if (glewInit() != GLEW_OK) {
		std::cerr << "GLEW 초기화 실패!" << std::endl;
		glfwDestroyWindow(window);
		glfwTerminate();
		return -1;
	}

	shaderProgram = MakeShaderProgram();
	if (shaderProgram == 0)
		return -1;

	InitBuffer();
	ResetTriangles();
	glLineWidth(3.0f);

	glfwSetMouseButtonCallback(window, MouseButtonCallback);
	glfwSetKeyCallback(window, KeyCallback);
	glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);

	while (!glfwWindowShouldClose(window)) {
		DrawScene();
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glDeleteBuffers(1, &VBO);
	glDeleteVertexArrays(1, &VAO);
	glDeleteProgram(shaderProgram);
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}
