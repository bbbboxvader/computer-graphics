#include <GL/glew.h>
#include <GL/glfw3.h>
#include <iostream>
#include <random>

// 실습 7: 점, 선, 삼각형, 사각형 그리기

enum ShapeType {
	POINT_SHAPE,
	LINE_SHAPE,
	TRIANGLE_SHAPE,
	RECTANGLE_SHAPE
};

struct Shape {
	int type;
	float X;
	float Y;
	float size;
	float R;
	float G;
	float B;
};

struct Vertex {
	float x;
	float y;
	float z;
	float R;
	float G;
	float B;
};

Shape shapes[50];
int shapeCount = 0;
int selectedShape = -1;

GLuint shaderProgram = 0;
GLuint VAO = 0;
GLuint VBO = 0;

std::random_device rd;
std::mt19937 gen(rd());
std::uniform_real_distribution<float> positionDist(-0.75f, 0.75f);
std::uniform_real_distribution<float> sizeDist(0.10f, 0.22f);
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
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * 6, nullptr, GL_DYNAMIC_DRAW);

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

void SetVertex(Vertex& vertex, float x, float y, const Shape& shape)
{
	vertex.x = x;
	vertex.y = y;
	vertex.z = 0.0f;
	vertex.R = shape.R;
	vertex.G = shape.G;
	vertex.B = shape.B;
}

// 도형 종류에 맞는 정점을 만든다.
int MakeVertices(const Shape& shape, Vertex vertices[6], GLenum& drawMode)
{
	float half = shape.size / 2.0f;

	if (shape.type == POINT_SHAPE) {
		SetVertex(vertices[0], shape.X, shape.Y, shape);
		drawMode = GL_POINTS;
		return 1;
	}

	if (shape.type == LINE_SHAPE) {
		SetVertex(vertices[0], shape.X - half, shape.Y, shape);
		SetVertex(vertices[1], shape.X + half, shape.Y, shape);
		drawMode = GL_LINES;
		return 2;
	}

	if (shape.type == TRIANGLE_SHAPE) {
		SetVertex(vertices[0], shape.X, shape.Y + half, shape);
		SetVertex(vertices[1], shape.X - half, shape.Y - half, shape);
		SetVertex(vertices[2], shape.X + half, shape.Y - half, shape);
		drawMode = GL_TRIANGLES;
		return 3;
	}

	// 사각형은 삼각형 2개로 만든다.
	SetVertex(vertices[0], shape.X - half, shape.Y + half, shape);
	SetVertex(vertices[1], shape.X - half, shape.Y - half, shape);
	SetVertex(vertices[2], shape.X + half, shape.Y - half, shape);

	SetVertex(vertices[3], shape.X - half, shape.Y + half, shape);
	SetVertex(vertices[4], shape.X + half, shape.Y - half, shape);
	SetVertex(vertices[5], shape.X + half, shape.Y + half, shape);
	drawMode = GL_TRIANGLES;
	return 6;
}

void MakeShape(int type)
{
	if (shapeCount >= 50)
		return;

	Shape& shape = shapes[shapeCount];
	shape.type = type;
	shape.X = positionDist(gen);
	shape.Y = positionDist(gen);
	shape.size = sizeDist(gen);
	shape.R = colorDist(gen);
	shape.G = colorDist(gen);
	shape.B = colorDist(gen);

	selectedShape = shapeCount;
	shapeCount++;

	std::cout << "현재 도형 개수: " << shapeCount << std::endl;
}

void KeepInside(Shape& shape)
{
	float half = shape.size / 2.0f;

	if (shape.X < -1.0f + half)
		shape.X = -1.0f + half;
	if (shape.X > 1.0f - half)
		shape.X = 1.0f - half;
	if (shape.Y < -1.0f + half)
		shape.Y = -1.0f + half;
	if (shape.Y > 1.0f - half)
		shape.Y = 1.0f - half;
}

void MoveShape(Shape& shape, float moveX, float moveY)
{
	shape.X += moveX;
	shape.Y += moveY;
	KeepInside(shape);
}

void MoveSelected(float moveX, float moveY)
{
	if (selectedShape == -1)
		return;

	MoveShape(shapes[selectedShape], moveX, moveY);
}

void MoveAll(float moveX, float moveY)
{
	for (int i = 0; i < shapeCount; i++)
		MoveShape(shapes[i], moveX, moveY);
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

	if (key == GLFW_KEY_P)
		MakeShape(POINT_SHAPE);
	if (key == GLFW_KEY_E)
		MakeShape(LINE_SHAPE);
	if (key == GLFW_KEY_T)
		MakeShape(TRIANGLE_SHAPE);
	if (key == GLFW_KEY_R)
		MakeShape(RECTANGLE_SHAPE);

	float move = 0.05f;

	if (key == GLFW_KEY_W)
		MoveSelected(0.0f, move);
	if (key == GLFW_KEY_A)
		MoveSelected(-move, 0.0f);
	if (key == GLFW_KEY_S)
		MoveSelected(0.0f, -move);
	if (key == GLFW_KEY_D)
		MoveSelected(move, 0.0f);

	if (key == GLFW_KEY_I)
		MoveSelected(-move, move);
	if (key == GLFW_KEY_J)
		MoveSelected(move, move);
	if (key == GLFW_KEY_K)
		MoveSelected(-move, -move);
	if (key == GLFW_KEY_L)
		MoveSelected(move, -move);

	if (key == GLFW_KEY_1)
		MoveAll(-move, 0.0f);
	if (key == GLFW_KEY_2)
		MoveAll(move, 0.0f);
	if (key == GLFW_KEY_3)
		MoveAll(0.0f, move);
	if (key == GLFW_KEY_4)
		MoveAll(0.0f, -move);

	if (key == GLFW_KEY_C) {
		shapeCount = 0;
		selectedShape = -1;
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

bool IsInsideShape(const Shape& shape, float x, float y)
{
	float half = shape.size / 2.0f;

	if (shape.type == POINT_SHAPE)
		half = 0.05f;

	if (shape.type == LINE_SHAPE) {
		return
			x >= shape.X - half &&
			x <= shape.X + half &&
			y >= shape.Y - 0.04f &&
			y <= shape.Y + 0.04f;
	}

	return
		x >= shape.X - half &&
		x <= shape.X + half &&
		y >= shape.Y - half &&
		y <= shape.Y + half;
}

void MouseButtonCallback(
	GLFWwindow* window,
	int button,
	int action,
	int mods)
{
	if (button != GLFW_MOUSE_BUTTON_LEFT || action != GLFW_PRESS)
		return;

	double mouseX;
	double mouseY;
	glfwGetCursorPos(window, &mouseX, &mouseY);

	float x;
	float y;
	GetOpenGLMousePosition(window, mouseX, mouseY, x, y);

	selectedShape = -1;

	// 나중에 그린 도형부터 검사한다.
	for (int i = shapeCount - 1; i >= 0; i--) {
		if (IsInsideShape(shapes[i], x, y)) {
			selectedShape = i;
			std::cout << "선택한 도형 번호: " << i << std::endl;
			break;
		}
	}
}

void DrawScene()
{
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	glUseProgram(shaderProgram);
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);

	for (int i = 0; i < shapeCount; i++) {
		Vertex vertices[6];
		GLenum drawMode;
		int vertexCount = MakeVertices(shapes[i], vertices, drawMode);

		glBufferSubData(
			GL_ARRAY_BUFFER,
			0,
			sizeof(Vertex) * vertexCount,
			vertices
		);

		glDrawArrays(drawMode, 0, vertexCount);
	}

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
		800, 600, "Practice 7", nullptr, nullptr
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
	glPointSize(8.0f);
	glLineWidth(3.0f);

	glfwSetKeyCallback(window, KeyCallback);
	glfwSetMouseButtonCallback(window, MouseButtonCallback);
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
