#include <GL/glew.h>
#include <GL/glfw3.h>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>

#include "PracticeSelect.h"
#include "Simple2D.h"

#if ACTIVE_PRACTICE == 9

using namespace Simple2D;

struct Triangle
{
	Vec2 position;
	float size;
	Color color;
	float speedX;
	float speedY;
	float angle;
	float angularSpeed;
	float radius;
	float radialDirection;
	float turnDirection;
};

Renderer renderer;
Triangle triangles[2];
std::vector<Vertex> spiralPath;

bool fillMode = true;
int animationMode = 1;

void ResetTriangles()
{
	triangles[0] = {
		{ -0.55f, 0.35f }, 0.22f, { 0.18f, 0.55f, 0.92f },
		0.55f, 0.42f, 0.0f, 1.85f, 0.0f, 1.0f, 1.0f
	};
	triangles[1] = {
		{ 0.50f, -0.35f }, 0.18f, { 1.00f, 0.72f, 0.05f },
		-0.43f, 0.58f, 0.0f, 1.45f, 0.0f, 1.0f, -1.0f
	};

	spiralPath.clear();
}

void PrepareHorizontalMovement()
{
	triangles[0].speedX = std::abs(triangles[0].speedX);
	triangles[1].speedX = -std::abs(triangles[1].speedX);
}

void PrepareDiagonalMovement()
{
	// 두 삼각형 모두 왼쪽에서 오른쪽으로만 전진한다.
	// y 방향만 위·아래 벽에서 뒤집기 때문에 긴 지그재그 파형이 생긴다.
	for (int i = 0; i < 2; ++i) {
		Triangle& triangle = triangles[i];
		triangle.position.x = -1.0f + triangle.size * 0.5f;
		triangle.position.y = i == 0
			? 1.0f - triangle.size * 0.5f
			: -1.0f + triangle.size * 0.5f;
		triangle.speedX = i == 0 ? 0.42f : 0.34f;
		triangle.speedY = i == 0 ? -0.95f : 0.82f;
	}
}

Vec2 GetSpiralPosition(float progress)
{
	// progress가 0이면 반지름도 0이므로 정확히 화면 중심 (0, 0)이다.
	const float angle = progress * PI * 8.0f;
	const float radius = progress * 0.72f;
	return {
		std::cos(angle) * radius,
		std::sin(angle) * radius
	};
}

void PrepareCenterSpiral()
{
	// 4번을 누르는 순간 전체 이동 경로를 미리 계산한다.
	// 따라서 삼각형이 움직이며 선을 만드는 것이 아니라, 완성된 선 위를 움직인다.
	spiralPath.clear();
	constexpr int pointCount = 500;
	for (int i = 0; i <= pointCount; ++i) {
		float progress = static_cast<float>(i) / pointCount;
		spiralPath.push_back(MakeVertex(
			GetSpiralPosition(progress),
			{ 0.45f, 0.55f, 0.68f }
		));
	}

	for (int i = 0; i < 2; ++i) {
		Triangle& triangle = triangles[i];
		triangle.position = { 0.0f, 0.0f };
		triangle.radius = 0.0f;             // 선 위에서의 진행률(0~1)
		triangle.radialDirection = 1.0f;    // 1: 바깥쪽, -1: 중심 쪽
		triangle.angularSpeed = i == 0 ? 0.10f : 0.14f;
	}
}

void SetAnimationMode(int newMode)
{
	// 이미 실행 중인 번호를 다시 눌렀다면 아무것도 초기화하지 않는다.
	// 그래서 위치와 진행 방향이 끊기지 않고 그대로 이어진다.
	if (animationMode == newMode)
		return;

	animationMode = newMode;

	if (animationMode == 2)
		PrepareHorizontalMovement();
	else if (animationMode == 3)
		PrepareDiagonalMovement();
	else if (animationMode == 4)
		PrepareCenterSpiral();

	std::cout << "Animation mode: " << animationMode << std::endl;
}

void MoveBounce(Triangle& triangle, float deltaTime)
{
	const float half = triangle.size * 0.5f;
	triangle.position.x += triangle.speedX * deltaTime;
	triangle.position.y += triangle.speedY * deltaTime;

	if (triangle.position.x <= -1.0f + half) {
		triangle.position.x = -1.0f + half;
		triangle.speedX = std::abs(triangle.speedX);
	}
	else if (triangle.position.x >= 1.0f - half) {
		triangle.position.x = 1.0f - half;
		triangle.speedX = -std::abs(triangle.speedX);
	}

	if (triangle.position.y <= -1.0f + half) {
		triangle.position.y = -1.0f + half;
		triangle.speedY = std::abs(triangle.speedY);
	}
	else if (triangle.position.y >= 1.0f - half) {
		triangle.position.y = 1.0f - half;
		triangle.speedY = -std::abs(triangle.speedY);
	}
}

void MoveHorizontalZigzag(Triangle& triangle, float deltaTime)
{
	const float half = triangle.size * 0.5f;
	triangle.position.x += triangle.speedX * deltaTime;

	if (triangle.position.x <= -1.0f + half) {
		triangle.position.x = -1.0f + half;
		triangle.speedX = std::abs(triangle.speedX);
		triangle.position.y -= 0.14f;
	}
	else if (triangle.position.x >= 1.0f - half) {
		triangle.position.x = 1.0f - half;
		triangle.speedX = -std::abs(triangle.speedX);
		triangle.position.y -= 0.14f;
	}

	if (triangle.position.y < -1.0f + half)
		triangle.position.y = 1.0f - half;
}

void MoveSharpDiagonalZigzag(Triangle& triangle, float deltaTime)
{
	const float half = triangle.size * 0.5f;

	// x와 y를 동시에 바꾸므로 대각선으로 움직인다.
	triangle.position.x += triangle.speedX * deltaTime;
	triangle.position.y += triangle.speedY * deltaTime;

	// 위·아래 벽에서 y 방향을 즉시 뒤집어 뾰족한 V 모양을 만든다.
	if (triangle.position.y <= -1.0f + half) {
		triangle.position.y = -1.0f + half;
		triangle.speedY = std::abs(triangle.speedY);
	}
	else if (triangle.position.y >= 1.0f - half) {
		triangle.position.y = 1.0f - half;
		triangle.speedY = -std::abs(triangle.speedY);
	}

	// 오른쪽 끝에 도착하면 왼쪽으로 돌아가 같은 지그재그를 반복한다.
	// x 속도의 부호는 바꾸지 않으므로 1번처럼 좌우로 튕기지 않는다.
	if (triangle.position.x >= 1.0f - half)
		triangle.position.x = -1.0f + half;
}

void MoveCenterSpiral(Triangle& triangle, int index, float deltaTime)
{
	(void)index;
	triangle.radius +=
		triangle.radialDirection * triangle.angularSpeed * deltaTime;

	if (triangle.radius >= 1.0f) {
		triangle.radius = 1.0f;
		triangle.radialDirection = -1.0f;
	}
	else if (triangle.radius <= 0.0f) {
		triangle.radius = 0.0f;
		triangle.radialDirection = 1.0f;
	}

	triangle.position = GetSpiralPosition(triangle.radius);
}

void UpdateAnimation(float deltaTime)
{
	// 창을 오래 끌었다가 돌아왔을 때 한 번에 너무 멀리 뛰지 않게 한다.
	deltaTime = std::min(deltaTime, 0.05f);

	for (int i = 0; i < 2; ++i) {
		if (animationMode == 1)
			MoveBounce(triangles[i], deltaTime);
		else if (animationMode == 2)
			MoveHorizontalZigzag(triangles[i], deltaTime);
		else if (animationMode == 3)
			MoveSharpDiagonalZigzag(triangles[i], deltaTime);
		else if (animationMode == 4)
			MoveCenterSpiral(triangles[i], i, deltaTime);
	}
}

void DrawSpiralPath()
{
	// 이동 경로는 4번을 누른 즉시 완성된 모양으로 전부 표시한다.
	if (animationMode != 4)
		return;

	renderer.Draw(GL_LINE_STRIP, spiralPath);
}

void DrawScene()
{
	renderer.BeginFrame({ 1.0f, 1.0f, 1.0f });
	DrawSpiralPath();

	for (const Triangle& triangle : triangles) {
		if (fillMode) {
			renderer.Draw(
				GL_TRIANGLES,
				MakeShape(
					ShapeKind::Triangle,
					triangle.position,
					triangle.size,
					triangle.color
				)
			);
		}
		else {
			renderer.Draw(
				GL_LINE_LOOP,
				MakeShapeOutline(
					ShapeKind::Triangle,
					triangle.position,
					triangle.size,
					triangle.color
				)
			);
		}
	}

	renderer.EndFrame();
}

void KeyCallback(GLFWwindow* window, int key, int, int action, int)
{
	if (action != GLFW_PRESS)
		return;

	if (key == GLFW_KEY_Q || key == GLFW_KEY_ESCAPE)
		glfwSetWindowShouldClose(window, GLFW_TRUE);
	else if (key == GLFW_KEY_A)
		fillMode = true;
	else if (key == GLFW_KEY_B)
		fillMode = false;
	else if (key == GLFW_KEY_C || key == GLFW_KEY_R)
		ResetTriangles();
	else if (key == GLFW_KEY_1)
		SetAnimationMode(1);
	else if (key == GLFW_KEY_2)
		SetAnimationMode(2);
	else if (key == GLFW_KEY_3)
		SetAnimationMode(3);
	else if (key == GLFW_KEY_4)
		SetAnimationMode(4);
}

void FramebufferSizeCallback(GLFWwindow*, int width, int height)
{
	glViewport(0, 0, width, height);
}

int main()
{
	if (!glfwInit()) {
		std::cerr << "GLFW initialization failed." << std::endl;
		return -1;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(
		800, 800,
		"Practice 9 - 1/2/3/4 animation - Q quit",
		nullptr, nullptr
	);
	if (window == nullptr) {
		std::cerr << "Window creation failed." << std::endl;
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);
	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK) {
		std::cerr << "GLEW initialization failed." << std::endl;
		glfwDestroyWindow(window);
		glfwTerminate();
		return -1;
	}

	if (!renderer.Initialize(40000)) {
		glfwDestroyWindow(window);
		glfwTerminate();
		return -1;
	}

	ResetTriangles();
	glLineWidth(2.5f);
	glfwSwapInterval(1);
	glfwSetKeyCallback(window, KeyCallback);
	glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);

	double previousTime = glfwGetTime();
	while (!glfwWindowShouldClose(window)) {
		const double currentTime = glfwGetTime();
		const float deltaTime = static_cast<float>(currentTime - previousTime);
		previousTime = currentTime;

		UpdateAnimation(deltaTime);
		DrawScene();
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	renderer.Shutdown();
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}

#endif
