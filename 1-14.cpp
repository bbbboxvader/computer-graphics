#include "PracticeCommon.h"
#include <algorithm>

// 실습 14
// 육면체와 사각뿔을 선택하고, 은면 제거·와이어·회전·이동을 키보드로 바꾼다.

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

enum ObjectType
{
	CUBE_OBJECT,
	PYRAMID_OBJECT
};

GLuint shaderProgramID = 0;
GLuint VAO = 0;
GLuint VBO = 0;

GLint angleXLocation = -1;
GLint angleYLocation = -1;
GLint offsetLocation = -1;

ObjectType objectType = CUBE_OBJECT;
bool hiddenSurface = true;
bool wireframe = false;

float objectAngleX = 30.0f;
float objectAngleY = 30.0f;
float objectX = 0.0f;
float objectY = 0.0f;

// 0: 정지, 1: x축 자전, 2: y축 자전
int animationAxis = 0;
float animationDirection = 1.0f;

const Point3D cubePoints[8] = {
	{ -0.42f, -0.42f, -0.42f },
	{  0.42f, -0.42f, -0.42f },
	{  0.42f,  0.42f, -0.42f },
	{ -0.42f,  0.42f, -0.42f },
	{ -0.42f, -0.42f,  0.42f },
	{  0.42f, -0.42f,  0.42f },
	{  0.42f,  0.42f,  0.42f },
	{ -0.42f,  0.42f,  0.42f }
};

const int cubeFaces[6][4] = {
	{ 4, 5, 6, 7 },
	{ 0, 3, 2, 1 },
	{ 0, 4, 7, 3 },
	{ 1, 2, 6, 5 },
	{ 3, 7, 6, 2 },
	{ 0, 1, 5, 4 }
};

// 꼭짓점마다 색을 다르게 주지 않고, 면마다 단순한 색 하나를 사용한다.
// 한 면을 만드는 모든 정점에 같은 색을 넣으면 그라데이션이 생기지 않는다.
const Color3 cubeFaceColors[6] = {
	{ 0.10f, 0.25f, 1.00f }, // 앞면: 파랑
	{ 1.00f, 0.90f, 0.05f }, // 뒷면: 노랑
	{ 1.00f, 0.10f, 0.10f }, // 왼쪽 면: 빨강
	{ 0.05f, 0.90f, 0.20f }, // 오른쪽 면: 초록
	{ 0.00f, 0.90f, 0.90f }, // 윗면: 청록
	{ 1.00f, 0.00f, 0.85f }  // 아랫면: 자홍
};

const Point3D pyramidPoints[5] = {
	{  0.00f,  0.55f,  0.00f },
	{ -0.48f, -0.42f,  0.48f },
	{  0.48f, -0.42f,  0.48f },
	{  0.48f, -0.42f, -0.48f },
	{ -0.48f, -0.42f, -0.48f }
};

const int pyramidSides[4][3] = {
	{ 0, 1, 2 },
	{ 0, 2, 3 },
	{ 0, 3, 4 },
	{ 0, 4, 1 }
};

const int pyramidBase[4] = { 1, 4, 3, 2 };

const Color3 pyramidFaceColors[5] = {
	{ 0.10f, 0.25f, 1.00f }, // 앞쪽 옆면: 파랑
	{ 1.00f, 0.10f, 0.10f }, // 오른쪽 옆면: 빨강
	{ 0.05f, 0.90f, 0.20f }, // 뒤쪽 옆면: 초록
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
	Vertex axes[6] = {
		{ -0.95f, 0.0f, 0.0f, 1.0f, 0.15f, 0.15f },
		{  0.95f, 0.0f, 0.0f, 1.0f, 0.15f, 0.15f },
		{ 0.0f, -0.95f, 0.0f, 0.15f, 1.0f, 0.15f },
		{ 0.0f,  0.95f, 0.0f, 0.15f, 1.0f, 0.15f },
		{ 0.0f, 0.0f, -0.95f, 0.20f, 0.35f, 1.0f },
		{ 0.0f, 0.0f,  0.95f, 0.20f, 0.35f, 1.0f }
	};
	DrawVertices(VBO, GL_LINES, axes, 6);
}

void DrawTriangleFace(const Point3D points[], Color3 color, const int face[3])
{
	Vertex vertices[3] = {
		MakeVertex3D(points[face[0]], color),
		MakeVertex3D(points[face[1]], color),
		MakeVertex3D(points[face[2]], color)
	};
	DrawVertices(VBO, wireframe ? GL_LINE_LOOP : GL_TRIANGLES, vertices, 3);
}

void DrawQuadFace(const Point3D points[], Color3 color, const int face[4])
{
	if (wireframe) {
		Vertex outline[4] = {
			MakeVertex3D(points[face[0]], color),
			MakeVertex3D(points[face[1]], color),
			MakeVertex3D(points[face[2]], color),
			MakeVertex3D(points[face[3]], color)
		};
		DrawVertices(VBO, GL_LINE_LOOP, outline, 4);
		return;
	}

	Vertex filled[6] = {
		MakeVertex3D(points[face[0]], color),
		MakeVertex3D(points[face[1]], color),
		MakeVertex3D(points[face[2]], color),
		MakeVertex3D(points[face[0]], color),
		MakeVertex3D(points[face[2]], color),
		MakeVertex3D(points[face[3]], color)
	};
	DrawVertices(VBO, GL_TRIANGLES, filled, 6);
}

void DrawCube()
{
	for (int face = 0; face < 6; ++face)
		DrawQuadFace(cubePoints, cubeFaceColors[face], cubeFaces[face]);
}

void DrawPyramid()
{
	for (int side = 0; side < 4; ++side)
		DrawTriangleFace(pyramidPoints, pyramidFaceColors[side], pyramidSides[side]);
	DrawQuadFace(pyramidPoints, pyramidFaceColors[4], pyramidBase);
}

void ResetScene()
{
	objectType = CUBE_OBJECT;
	hiddenSurface = true;
	wireframe = false;
	objectAngleX = 30.0f;
	objectAngleY = 30.0f;
	objectX = 0.0f;
	objectY = 0.0f;
	animationAxis = 0;
	animationDirection = 1.0f;
}

void UpdateScene(float deltaTime)
{
	const float degreesPerSecond = 60.0f;
	if (animationAxis == 1)
		objectAngleX += animationDirection * degreesPerSecond * deltaTime;
	else if (animationAxis == 2)
		objectAngleY += animationDirection * degreesPerSecond * deltaTime;

	if (objectAngleX > 360.0f || objectAngleX < -360.0f)
		objectAngleX = 0.0f;
	if (objectAngleY > 360.0f || objectAngleY < -360.0f)
		objectAngleY = 0.0f;
}

void DrawScene()
{
	glClearColor(0.02f, 0.02f, 0.02f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram(shaderProgramID);
	glBindVertexArray(VAO);

	if (hiddenSurface)
		glEnable(GL_DEPTH_TEST);
	else
		glDisable(GL_DEPTH_TEST);

	// 좌표축에는 물체의 회전과 이동 값을 보내지 않는다.
	SendTransform(0.0f, 0.0f, 0.0f, 0.0f);
	glLineWidth(1.4f);
	DrawAxes();

	// 물체에만 현재 회전각과 이동값을 보낸다.
	SendTransform(objectAngleX, objectAngleY, objectX, objectY);
	if (objectType == CUBE_OBJECT)
		DrawCube();
	else
		DrawPyramid();
}

void KeyCallback(GLFWwindow* window, int key, int, int action, int mods)
{
	if (action != GLFW_PRESS && action != GLFW_REPEAT)
		return;

	bool shiftPressed = (mods & GLFW_MOD_SHIFT) != 0;

	if (key == GLFW_KEY_C && action == GLFW_PRESS)
		objectType = CUBE_OBJECT;
	else if (key == GLFW_KEY_P && action == GLFW_PRESS)
		objectType = PYRAMID_OBJECT;
	else if (key == GLFW_KEY_H && action == GLFW_PRESS)
		hiddenSurface = !hiddenSurface;
	else if (key == GLFW_KEY_W && action == GLFW_PRESS)
		wireframe = !shiftPressed;
	else if (key == GLFW_KEY_X && action == GLFW_PRESS) {
		animationAxis = 1;
		animationDirection = shiftPressed ? -1.0f : 1.0f;
	}
	else if (key == GLFW_KEY_Y && action == GLFW_PRESS) {
		animationAxis = 2;
		animationDirection = shiftPressed ? -1.0f : 1.0f;
	}
	else if (key == GLFW_KEY_LEFT)
		objectX -= 0.06f;
	else if (key == GLFW_KEY_RIGHT)
		objectX += 0.06f;
	else if (key == GLFW_KEY_UP)
		objectY += 0.06f;
	else if (key == GLFW_KEY_DOWN)
		objectY -= 0.06f;
	else if (key == GLFW_KEY_S && action == GLFW_PRESS)
		ResetScene();
	else if (key == GLFW_KEY_Q || key == GLFW_KEY_ESCAPE)
		glfwSetWindowShouldClose(window, GLFW_TRUE);

	// 화면에서 완전히 사라지지 않도록 이동 범위를 제한한다.
	objectX = std::clamp(objectX, -0.60f, 0.60f);
	objectY = std::clamp(objectY, -0.60f, 0.60f);
}

int main()
{
	if (glfwInit() == GLFW_FALSE)
		return -1;

	GLFWwindow* window = glfwCreateWindow(800, 800, "Practice 14", nullptr, nullptr);
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
	ResetScene();

	float previousTime = static_cast<float>(glfwGetTime());
	while (glfwWindowShouldClose(window) == GLFW_FALSE) {
		float currentTime = static_cast<float>(glfwGetTime());
		float deltaTime = currentTime - previousTime;
		previousTime = currentTime;
		if (deltaTime > 0.05f)
			deltaTime = 0.05f;

		UpdateScene(deltaTime);
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
