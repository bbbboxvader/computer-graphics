#include "PracticeCommon.h"
#include <ctime>

// 실습 13
// 숫자 키로 육면체 또는 사각뿔의 한 면을 골라 그린다.
// C키는 육면체의 서로 다른 두 면, T키는 사각뿔의 바닥과 옆면 하나를 그린다.

struct Point3D
{
	float x;
	float y;
	float z;
};

struct Color3
{
	float r;
	float g;
	float b;
};

enum DrawMode
{
	SHOW_CUBE_ALL,
	SHOW_CUBE_ONE,
	SHOW_PYRAMID_ONE,
	SHOW_PYRAMID_ALL,
	SHOW_CUBE_RANDOM_TWO,
	SHOW_PYRAMID_BASE_AND_SIDE
};

GLuint shaderProgramID = 0;
GLuint VAO = 0;
GLuint VBO = 0;

GLint angleXLocation = -1;
GLint angleYLocation = -1;
GLint offsetLocation = -1;

DrawMode drawMode = SHOW_CUBE_ALL;
int selectedCubeFace = 0;
int selectedPyramidSide = 0;
int randomCubeFaceA = 0;
int randomCubeFaceB = 1;
int randomPyramidSide = 0;

const Point3D cubePoints[8] = {
	{ -0.42f, -0.42f, -0.42f }, // 0: 왼쪽 아래 뒤
	{  0.42f, -0.42f, -0.42f }, // 1: 오른쪽 아래 뒤
	{  0.42f,  0.42f, -0.42f }, // 2: 오른쪽 위 뒤
	{ -0.42f,  0.42f, -0.42f }, // 3: 왼쪽 위 뒤
	{ -0.42f, -0.42f,  0.42f }, // 4: 왼쪽 아래 앞
	{  0.42f, -0.42f,  0.42f }, // 5: 오른쪽 아래 앞
	{  0.42f,  0.42f,  0.42f }, // 6: 오른쪽 위 앞
	{ -0.42f,  0.42f,  0.42f }  // 7: 왼쪽 위 앞
};

// 각 행은 사각형 한 면의 꼭짓점 네 개이다.
// 바깥에서 보았을 때 반시계 방향이 되도록 적었다.
const int cubeFaces[6][4] = {
	{ 4, 5, 6, 7 }, // 앞
	{ 0, 3, 2, 1 }, // 뒤
	{ 0, 4, 7, 3 }, // 왼쪽
	{ 1, 2, 6, 5 }, // 오른쪽
	{ 3, 7, 6, 2 }, // 위
	{ 0, 1, 5, 4 }  // 아래
};

const Color3 cubeFaceColors[6] = {
	{ 0.10f, 0.25f, 1.00f }, // 파랑
	{ 1.00f, 0.90f, 0.05f }, // 노랑
	{ 1.00f, 0.10f, 0.10f }, // 빨강
	{ 0.05f, 0.90f, 0.20f }, // 초록
	{ 0.00f, 0.90f, 0.90f }, // 청록
	{ 1.00f, 0.00f, 0.85f }  // 자홍
};

const Point3D pyramidPoints[5] = {
	{  0.00f,  0.55f,  0.00f }, // 0: 꼭대기
	{ -0.48f, -0.42f,  0.48f }, // 1: 왼쪽 앞
	{  0.48f, -0.42f,  0.48f }, // 2: 오른쪽 앞
	{  0.48f, -0.42f, -0.48f }, // 3: 오른쪽 뒤
	{ -0.48f, -0.42f, -0.48f }  // 4: 왼쪽 뒤
};

const int pyramidSides[4][3] = {
	{ 0, 1, 2 }, // 앞
	{ 0, 2, 3 }, // 오른쪽
	{ 0, 3, 4 }, // 뒤
	{ 0, 4, 1 }  // 왼쪽
};

const int pyramidBase[4] = { 1, 4, 3, 2 };

const Color3 pyramidColors[5] = {
	{ 0.10f, 0.25f, 1.00f }, // 앞쪽 옆면: 파랑
	{ 1.00f, 0.10f, 0.10f }, // 오른쪽 옆면: 빨강
	{ 0.00f, 0.90f, 0.20f }, // 뒤쪽 옆면
	{ 1.00f, 0.00f, 0.85f }, // 왼쪽 옆면: 자홍
	{ 1.00f, 0.95f, 0.05f }  // 바닥면: 노랑
};

Vertex MakeVertex3D(Point3D point, Color3 color)
{
	return { point.x, point.y, point.z, color.r, color.g, color.b };
}

void SendTransform(float angleX, float angleY, float moveX, float moveY)
{
	glUniform1f(angleXLocation, angleX);
	glUniform1f(angleYLocation, angleY);
	glUniform3f(offsetLocation, moveX, moveY, 0.0f);
}

void DrawAxes()
{
	Vertex xAxis[2] = {
		{ -0.95f, 0.0f, 0.0f, 1.0f, 0.25f, 0.25f },
		{  0.95f, 0.0f, 0.0f, 1.0f, 0.25f, 0.25f }
	};
	Vertex yAxis[2] = {
		{ 0.0f, -0.95f, 0.0f, 0.20f, 1.0f, 0.20f },
		{ 0.0f,  0.95f, 0.0f, 0.20f, 1.0f, 0.20f }
	};
	Vertex zAxis[2] = {
		{ 0.0f, 0.0f, -0.95f, 0.25f, 0.40f, 1.0f },
		{ 0.0f, 0.0f,  0.95f, 0.25f, 0.40f, 1.0f }
	};

	DrawVertices(VBO, GL_LINES, xAxis, 2);
	DrawVertices(VBO, GL_LINES, yAxis, 2);
	DrawVertices(VBO, GL_LINES, zAxis, 2);
}

void DrawQuadFace(const Point3D points[], const int face[4], Color3 color)
{
	Vertex vertices[6] = {
		MakeVertex3D(points[face[0]], color),
		MakeVertex3D(points[face[1]], color),
		MakeVertex3D(points[face[2]], color),
		MakeVertex3D(points[face[0]], color),
		MakeVertex3D(points[face[2]], color),
		MakeVertex3D(points[face[3]], color)
	};
	DrawVertices(VBO, GL_TRIANGLES, vertices, 6);
}

void DrawTriangleFace(const Point3D points[], const int face[3], Color3 color)
{
	Vertex vertices[3] = {
		MakeVertex3D(points[face[0]], color),
		MakeVertex3D(points[face[1]], color),
		MakeVertex3D(points[face[2]], color)
	};
	DrawVertices(VBO, GL_TRIANGLES, vertices, 3);
}

void DrawCubeFace(int face)
{
	DrawQuadFace(cubePoints, cubeFaces[face], cubeFaceColors[face]);
}

void DrawPyramidSide(int side)
{
	DrawTriangleFace(pyramidPoints, pyramidSides[side], pyramidColors[side]);
}

void DrawPyramidBase()
{
	DrawQuadFace(pyramidPoints, pyramidBase, pyramidColors[4]);
}

void DrawPyramidAll()
{
	for (int side = 0; side < 4; ++side)
		DrawPyramidSide(side);

	DrawPyramidBase();
}

void DrawScene()
{
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram(shaderProgramID);
	glBindVertexArray(VAO);

	// 좌표축은 물체의 회전과 이동에 영향을 받지 않는다.
	SendTransform(0.0f, 0.0f, 0.0f, 0.0f);
	glLineWidth(1.5f);
	DrawAxes();

	// 실습 조건에 따라 물체를 x축과 y축으로 각각 30도 회전한다.
	SendTransform(30.0f, 30.0f, 0.0f, 0.0f);

	if (drawMode == SHOW_CUBE_ALL) {
		for (int face = 0; face < 6; ++face)
			DrawCubeFace(face);
	}
	else if (drawMode == SHOW_CUBE_ONE) {
		DrawCubeFace(selectedCubeFace);
	}
	else if (drawMode == SHOW_PYRAMID_ONE) {
		DrawPyramidSide(selectedPyramidSide);
	}
	else if (drawMode == SHOW_PYRAMID_ALL) {
		DrawPyramidAll();
	}
	else if (drawMode == SHOW_CUBE_RANDOM_TWO) {
		DrawCubeFace(randomCubeFaceA);
		DrawCubeFace(randomCubeFaceB);
	}
	else if (drawMode == SHOW_PYRAMID_BASE_AND_SIDE) {
		DrawPyramidBase();
		DrawPyramidSide(randomPyramidSide);
	}
}

void KeyCallback(GLFWwindow* window, int key, int, int action, int)
{
	if (action != GLFW_PRESS)
		return;

	if (key >= GLFW_KEY_1 && key <= GLFW_KEY_6) {
		drawMode = SHOW_CUBE_ONE;
		selectedCubeFace = key - GLFW_KEY_1;
	}
	else if (key >= GLFW_KEY_7 && key <= GLFW_KEY_9) {
		drawMode = SHOW_PYRAMID_ONE;
		selectedPyramidSide = key - GLFW_KEY_7;
	}
	else if (key == GLFW_KEY_0) {
		drawMode = SHOW_PYRAMID_ONE;
		selectedPyramidSide = 3;
	}
	else if (key == GLFW_KEY_P) {
		// 사진처럼 사각뿔의 모든 면을 합친 완성형을 보여 준다.
		drawMode = SHOW_PYRAMID_ALL;
	}
	else if (key == GLFW_KEY_C) {
		drawMode = SHOW_CUBE_RANDOM_TWO;
		randomCubeFaceA = std::rand() % 6;
		do {
			randomCubeFaceB = std::rand() % 6;
		} while (randomCubeFaceA == randomCubeFaceB);
	}
	else if (key == GLFW_KEY_T) {
		drawMode = SHOW_PYRAMID_BASE_AND_SIDE;
		randomPyramidSide = std::rand() % 4;
	}
	else if (key == GLFW_KEY_Q || key == GLFW_KEY_ESCAPE) {
		glfwSetWindowShouldClose(window, GLFW_TRUE);
	}
}

int main()
{
	std::srand(static_cast<unsigned int>(std::time(nullptr)));

	if (glfwInit() == GLFW_FALSE)
		return -1;

	GLFWwindow* window = glfwCreateWindow(800, 800, "Practice 13", nullptr, nullptr);
	if (window == nullptr) {
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);
	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK) {
		glfwDestroyWindow(window);
		glfwTerminate();
		return -1;
	}

	glfwSetKeyCallback(window, KeyCallback);

	shaderProgramID = MakeShaderProgram("vertex-3d.glsl", "fragment-basic.glsl");
	if (shaderProgramID == 0) {
		glfwDestroyWindow(window);
		glfwTerminate();
		return -1;
	}

	InitBuffer(VAO, VBO, 100);
	angleXLocation = glGetUniformLocation(shaderProgramID, "uAngleX");
	angleYLocation = glGetUniformLocation(shaderProgramID, "uAngleY");
	offsetLocation = glGetUniformLocation(shaderProgramID, "uOffset");

	// 앞쪽 조각이 뒤쪽 조각을 가리도록 깊이 검사를 켠다.
	glEnable(GL_DEPTH_TEST);

	while (glfwWindowShouldClose(window) == GLFW_FALSE) {
		DrawScene();
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glDeleteBuffers(1, &VBO);
	glDeleteVertexArrays(1, &VAO);
	glDeleteProgram(shaderProgramID);
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}
