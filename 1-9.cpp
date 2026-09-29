#include <GL/glew.h>
#include <GL/glfw3.h>
#include <iostream>
#include <random>
#include <cmath>

// 실습 9: 실습 8의 삼각형 움직이기

struct Triangle {
	float X;
	float Y;
	float size;
	float R;
	float G;
	float B;
	bool active;
	bool growing;

	float speed;
	float moveX;
	float moveY;

	float centerX;
	float centerY;
	float angle;
	float radius;
	float turnDirection;
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
int animationMode = 1;

GLuint shaderProgram = 0;
GLuint VAO = 0;
GLuint VBO = 0;

std::random_device rd;
std::mt19937 gen(rd());
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

void ChangeRandomColor(Triangle& triangle)
{
	triangle.R = colorDist(gen);
	triangle.G = colorDist(gen);
	triangle.B = colorDist(gen);
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
	triangle.active = true;
	triangle.growing = true;

	if (index == 0) {
		triangle.size = 0.26f;
		triangle.R = 0.2f;
		triangle.G = 0.55f;
		triangle.B = 0.9f;
	}
	else if (index == 1) {
		triangle.size = 0.22f;
		triangle.R = 1.0f;
		triangle.G = 0.75f;
		triangle.B = 0.0f;
	}
	else if (index == 2) {
		triangle.size = 0.18f;
		triangle.R = 1.0f;
		triangle.G = 0.1f;
		triangle.B = 0.1f;
	}
	else {
		triangle.size = 0.30f;
		triangle.R = 0.9f;
		triangle.G = 0.35f;
		triangle.B = 0.05f;
	}

	// 사분면마다 다른 속도와 방향을 준다.
	triangle.speed = 0.28f + index * 0.07f;
	triangle.moveX = index % 2 == 0
		? triangle.speed
		: -triangle.speed;
	triangle.moveY = index < 2
		? -triangle.speed * 0.8f
		: triangle.speed * 0.8f;

	triangle.centerX = triangle.X;
	triangle.centerY = triangle.Y;
	triangle.angle = index * 1.57f;
	triangle.radius = 0.0f;
	triangle.turnDirection = index % 2 == 0 ? 1.0f : -1.0f;

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
}

void SetAnimationMode(int mode)
{
	animationMode = mode;

	for (int i = 0; i < 4; i++) {
		Triangle& triangle = triangles[i];
		triangle.moveX = i % 2 == 0
			? triangle.speed
			: -triangle.speed;
		triangle.moveY = i < 2
			? -triangle.speed * 0.8f
			: triangle.speed * 0.8f;

		if (mode == 4) {
			triangle.centerX = triangle.X;
			triangle.centerY = triangle.Y;
			triangle.radius = 0.0f;
			triangle.angle = i * 1.57f;
		}
	}

	std::cout << "애니메이션 번호: " << mode << std::endl;
}

void MoveBounce(Triangle& triangle, float deltaTime)
{
	float half = triangle.size / 2.0f;

	triangle.X += triangle.moveX * deltaTime;
	triangle.Y += triangle.moveY * deltaTime;

	if (triangle.X <= -1.0f + half) {
		triangle.X = -1.0f + half;
		triangle.moveX = -triangle.moveX;
		ChangeRandomColor(triangle);
	}
	else if (triangle.X >= 1.0f - half) {
		triangle.X = 1.0f - half;
		triangle.moveX = -triangle.moveX;
		ChangeRandomColor(triangle);
	}

	if (triangle.Y <= -1.0f + half) {
		triangle.Y = -1.0f + half;
		triangle.moveY = -triangle.moveY;
		ChangeRandomColor(triangle);
	}
	else if (triangle.Y >= 1.0f - half) {
		triangle.Y = 1.0f - half;
		triangle.moveY = -triangle.moveY;
		ChangeRandomColor(triangle);
	}
}

void MoveHorizontalZigzag(Triangle& triangle, float deltaTime)
{
	float half = triangle.size / 2.0f;
	triangle.X += triangle.moveX * deltaTime;

	if (triangle.X <= -1.0f + half) {
		triangle.X = -1.0f + half;
		triangle.moveX = triangle.speed;
		triangle.Y -= 0.12f;
		ChangeRandomColor(triangle);
	}
	else if (triangle.X >= 1.0f - half) {
		triangle.X = 1.0f - half;
		triangle.moveX = -triangle.speed;
		triangle.Y -= 0.12f;
		ChangeRandomColor(triangle);
	}

	if (triangle.Y < -1.0f + half)
		triangle.Y = 1.0f - half;
}

void MoveVerticalZigzag(Triangle& triangle, float deltaTime)
{
	float half = triangle.size / 2.0f;
	triangle.Y += triangle.moveY * deltaTime;

	if (triangle.Y <= -1.0f + half) {
		triangle.Y = -1.0f + half;
		triangle.moveY = triangle.speed;
		triangle.X += 0.12f;
		ChangeRandomColor(triangle);
	}
	else if (triangle.Y >= 1.0f - half) {
		triangle.Y = 1.0f - half;
		triangle.moveY = -triangle.speed;
		triangle.X += 0.12f;
		ChangeRandomColor(triangle);
	}

	if (triangle.X > 1.0f - half)
		triangle.X = -1.0f + half;
}

void MoveSpiral(Triangle& triangle, float deltaTime)
{
	float half = triangle.size / 2.0f;

	triangle.angle +=
		triangle.turnDirection * triangle.speed * 5.0f * deltaTime;
	triangle.radius += 0.08f * deltaTime;

	triangle.X =
		triangle.centerX + std::cos(triangle.angle) * triangle.radius;
	triangle.Y =
		triangle.centerY + std::sin(triangle.angle) * triangle.radius;

	bool hitWall =
		triangle.X <= -1.0f + half ||
		triangle.X >= 1.0f - half ||
		triangle.Y <= -1.0f + half ||
		triangle.Y >= 1.0f - half;

	if (hitWall) {
		if (triangle.X < -1.0f + half)
			triangle.X = -1.0f + half;
		if (triangle.X > 1.0f - half)
			triangle.X = 1.0f - half;
		if (triangle.Y < -1.0f + half)
			triangle.Y = -1.0f + half;
		if (triangle.Y > 1.0f - half)
			triangle.Y = 1.0f - half;

		ChangeRandomColor(triangle);
		triangle.centerX = triangle.X;
		triangle.centerY = triangle.Y;
		triangle.radius = 0.0f;
		triangle.turnDirection = -triangle.turnDirection;
	}
}

void UpdateAnimation(float deltaTime)
{
	for (int i = 0; i < 4; i++) {
		if (!triangles[i].active)
			continue;

		if (animationMode == 1)
			MoveBounce(triangles[i], deltaTime);
		else if (animationMode == 2)
			MoveHorizontalZigzag(triangles[i], deltaTime);
		else if (animationMode == 3)
			MoveVerticalZigzag(triangles[i], deltaTime);
		else if (animationMode == 4)
			MoveSpiral(triangles[i], deltaTime);
	}
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

	if (key == GLFW_KEY_A)
		fillMode = true;
	if (key == GLFW_KEY_B)
		fillMode = false;
	if (key == GLFW_KEY_C)
		ResetTriangles();

	if (key == GLFW_KEY_1)
		SetAnimationMode(1);
	if (key == GLFW_KEY_2)
		SetAnimationMode(2);
	if (key == GLFW_KEY_3)
		SetAnimationMode(3);
	if (key == GLFW_KEY_4)
		SetAnimationMode(4);
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
	SetVertex(lines[0], -1.0f, 0.0f, 0.8f, 0.8f, 0.8f);
	SetVertex(lines[1], 1.0f, 0.0f, 0.8f, 0.8f, 0.8f);
	SetVertex(lines[2], 0.0f, -1.0f, 0.8f, 0.8f, 0.8f);
	SetVertex(lines[3], 0.0f, 1.0f, 0.8f, 0.8f, 0.8f);
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
		800, 600, "Practice 9", nullptr, nullptr
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

	double previousTime = glfwGetTime();

	while (!glfwWindowShouldClose(window)) {
		double currentTime = glfwGetTime();
		float deltaTime =
			static_cast<float>(currentTime - previousTime);
		previousTime = currentTime;

		UpdateAnimation(deltaTime);
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
