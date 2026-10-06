#include "PracticeCommon.h"

// 실습 9: 두 삼각형을 네 가지 방법으로 움직인다.
// 어려운 클래스나 vector 대신, PDF에 나온 구조체와 배열을 사용했다.

const int TRIANGLE_COUNT = 2;
const int SPIRAL_POINT_COUNT = 401;

struct Triangle
{
	float x;
	float y;
	float size;
	float r;
	float g;
	float b;
	float speedX;
	float speedY;
	float spiralProgress;
	float spiralDirection;
	float spiralSpeed;
};

Triangle triangles[TRIANGLE_COUNT];
int animationMode = 1;
bool fillMode = true;

GLuint shaderProgramID = 0;
GLuint VAO = 0;
GLuint VBO = 0;

Point2D GetSpiralPoint(float progress)
{
	// progress가 0이면 중심, 1이면 나선의 가장 바깥쪽이다.
	float angle = progress * PI * 8.0f;
	float radius = progress * 0.72f;
	Point2D point;
	point.x = std::cos(angle) * radius;
	point.y = std::sin(angle) * radius;
	return point;
}

void ResetTriangles()
{
	triangles[0] = { -0.50f, -0.15f, 0.20f, 1.0f, 0.75f, 0.05f,
		0.42f, 0.31f, 0.0f, 1.0f, 0.12f };
	triangles[1] = { 0.45f, 0.22f, 0.20f, 0.25f, 0.85f, 0.35f,
		-0.33f, -0.40f, 0.0f, 1.0f, 0.18f };
}

void PrepareAnimation(int newMode)
{
	// 이미 실행 중인 번호를 또 누르면 아무것도 바꾸지 않는다.
	if (animationMode == newMode)
		return;

	animationMode = newMode;

	if (newMode == 2) {
		triangles[0].x = -0.75f;
		triangles[0].y = 0.62f;
		triangles[1].x = 0.75f;
		triangles[1].y = 0.40f;
		triangles[0].speedX = 0.55f;
		triangles[1].speedX = -0.42f;
	}
	else if (newMode == 3) {
		// 두 삼각형 모두 왼쪽에서 오른쪽으로 가면서 위아래로 튕긴다.
		triangles[0].x = -0.82f;
		triangles[0].y = -0.55f;
		triangles[1].x = -0.82f;
		triangles[1].y = 0.52f;
		triangles[0].speedX = 0.38f;
		triangles[0].speedY = 0.92f;
		triangles[1].speedX = 0.28f;
		triangles[1].speedY = -0.75f;
	}
	else if (newMode == 4) {
		// 4번은 반드시 두 삼각형 모두 중심 (0, 0)에서 시작한다.
		for (int i = 0; i < TRIANGLE_COUNT; ++i) {
			triangles[i].x = 0.0f;
			triangles[i].y = 0.0f;
			triangles[i].spiralProgress = 0.0f;
			triangles[i].spiralDirection = 1.0f;
		}
	}
}

void MoveBounce(Triangle& triangle, float deltaTime)
{
	triangle.x += triangle.speedX * deltaTime;
	triangle.y += triangle.speedY * deltaTime;
	float limit = 1.0f - triangle.size * 0.55f;

	if (triangle.x > limit || triangle.x < -limit) {
		triangle.speedX = -triangle.speedX;
		triangle.x = std::clamp(triangle.x, -limit, limit);
	}
	if (triangle.y > limit || triangle.y < -limit) {
		triangle.speedY = -triangle.speedY;
		triangle.y = std::clamp(triangle.y, -limit, limit);
	}
}

void MoveHorizontalZigzag(Triangle& triangle, float deltaTime)
{
	triangle.x += triangle.speedX * deltaTime;
	float limit = 1.0f - triangle.size * 0.55f;

	if (triangle.x > limit || triangle.x < -limit) {
		triangle.speedX = -triangle.speedX;
		triangle.x = std::clamp(triangle.x, -limit, limit);
		triangle.y -= 0.20f;
		if (triangle.y < -0.78f)
			triangle.y = 0.78f;
	}
}

void MoveVerticalZigzag(Triangle& triangle, float deltaTime)
{
	// x는 항상 오른쪽으로 간다. y만 위/아래 방향이 바뀐다.
	triangle.x += std::abs(triangle.speedX) * deltaTime;
	triangle.y += triangle.speedY * deltaTime;
	float limit = 1.0f - triangle.size * 0.55f;

	if (triangle.y > limit || triangle.y < -limit) {
		triangle.speedY = -triangle.speedY;
		triangle.y = std::clamp(triangle.y, -limit, limit);
	}
	if (triangle.x > limit)
		triangle.x = -limit;
}

void MoveSpiral(Triangle& triangle, float deltaTime)
{
	triangle.spiralProgress +=
		triangle.spiralDirection * triangle.spiralSpeed * deltaTime;

	if (triangle.spiralProgress >= 1.0f) {
		triangle.spiralProgress = 1.0f;
		triangle.spiralDirection = -1.0f;
	}
	else if (triangle.spiralProgress <= 0.0f) {
		triangle.spiralProgress = 0.0f;
		triangle.spiralDirection = 1.0f;
	}

	Point2D point = GetSpiralPoint(triangle.spiralProgress);
	triangle.x = point.x;
	triangle.y = point.y;
}

void UpdateScene(float deltaTime)
{
	for (int i = 0; i < TRIANGLE_COUNT; ++i) {
		if (animationMode == 1)
			MoveBounce(triangles[i], deltaTime);
		else if (animationMode == 2)
			MoveHorizontalZigzag(triangles[i], deltaTime);
		else if (animationMode == 3)
			MoveVerticalZigzag(triangles[i], deltaTime);
		else
			MoveSpiral(triangles[i], deltaTime);
	}
}

void DrawSpiralGuide()
{
	Vertex line[SPIRAL_POINT_COUNT];
	for (int i = 0; i < SPIRAL_POINT_COUNT; ++i) {
		float progress = static_cast<float>(i) / (SPIRAL_POINT_COUNT - 1);
		Point2D point = GetSpiralPoint(progress);
		line[i] = MakeVertex(point.x, point.y, 0.95f, 0.25f, 0.25f);
	}
	DrawVertices(VBO, GL_LINE_STRIP, line, SPIRAL_POINT_COUNT);
}

void DrawTriangle(const Triangle& triangle)
{
	Vertex vertices[6];
	if (fillMode) {
		int count = MakeFilledShape(vertices, TRIANGLE, triangle.x, triangle.y,
			triangle.size, 0.0f, triangle.r, triangle.g, triangle.b);
		DrawVertices(VBO, GL_TRIANGLES, vertices, count);
	}
	else {
		int count = MakeShapeOutline(vertices, TRIANGLE, triangle.x, triangle.y,
			triangle.size, 0.0f, triangle.r, triangle.g, triangle.b);
		DrawVertices(VBO, GL_LINE_LOOP, vertices, count);
	}
}

void DrawScene()
{
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glUseProgram(shaderProgramID);
	glBindVertexArray(VAO);

	if (animationMode == 4)
		DrawSpiralGuide();

	for (int i = 0; i < TRIANGLE_COUNT; ++i)
		DrawTriangle(triangles[i]);
}

void KeyCallback(GLFWwindow* window, int key, int, int action, int)
{
	if (action != GLFW_PRESS)
		return;

	if (key >= GLFW_KEY_1 && key <= GLFW_KEY_4)
		PrepareAnimation(key - GLFW_KEY_0);
	else if (key == GLFW_KEY_A)
		fillMode = true;
	else if (key == GLFW_KEY_B)
		fillMode = false;
	else if (key == GLFW_KEY_R) {
		ResetTriangles();
		animationMode = 1;
	}
	else if (key == GLFW_KEY_Q || key == GLFW_KEY_ESCAPE)
		glfwSetWindowShouldClose(window, GLFW_TRUE);
}

int main()
{
	if (glfwInit() == GLFW_FALSE)
		return -1;

	GLFWwindow* window = glfwCreateWindow(800, 800, "Practice 9", nullptr, nullptr);
	if (window == nullptr) {
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);
	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK) {
		glfwTerminate();
		return -1;
	}

	glfwSetKeyCallback(window, KeyCallback);
	shaderProgramID = MakeShaderProgram("vertex-basic.glsl", "fragment-basic.glsl");
	if (shaderProgramID == 0) {
		glfwTerminate();
		return -1;
	}
	InitBuffer(VAO, VBO, SPIRAL_POINT_COUNT);
	ResetTriangles();

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
