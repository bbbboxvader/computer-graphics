#include "PracticeCommon.h"
#include <ctime>

// 실습 12: 두 세로 통로의 사각형이 같은 구역에 있을 때 Enter를 누른다.
// 맞으면 둘이 오른쪽 창고로 이동하여 아래부터 차곡차곡 쌓인다.

const int MAX_STACKED_BLOCKS = 30;

struct MovingBlock
{
	float x;
	float y;
	float size;
	float speedY;
	float r;
	float g;
	float b;
	float targetX;
	float targetY;
	bool arrived;
};

struct StackedBlock
{
	float x;
	float y;
	float size;
	float r;
	float g;
	float b;
};

MovingBlock movingBlocks[2];
StackedBlock stackedBlocks[MAX_STACKED_BLOCKS];
int stackedCount = 0;
bool movingToStack = false;

GLuint shaderProgramID = 0;
GLuint VAO = 0;
GLuint VBO = 0;

int GetRegion(float y)
{
	// 통로를 위, 가운데, 아래의 세 구역으로 나눈다.
	if (y > 0.28f)
		return 0;
	if (y > -0.28f)
		return 1;
	return 2;
}

void MakeNewPair()
{
	movingBlocks[0] = { -0.72f, RandomFloat(-0.72f, 0.72f), 0.13f,
		RandomFloat(0.32f, 0.55f),
		RandomFloat(0.15f, 0.95f), RandomFloat(0.15f, 0.95f),
		RandomFloat(0.15f, 0.95f), 0.0f, 0.0f, false };
	movingBlocks[1] = { -0.34f, RandomFloat(-0.72f, 0.72f), 0.13f,
		-RandomFloat(0.32f, 0.55f),
		RandomFloat(0.15f, 0.95f), RandomFloat(0.15f, 0.95f),
		RandomFloat(0.15f, 0.95f), 0.0f, 0.0f, false };
	movingToStack = false;
}

void ResetGame()
{
	stackedCount = 0;
	MakeNewPair();
}

void StartMovingToStack()
{
	if (movingToStack || GetRegion(movingBlocks[0].y) != GetRegion(movingBlocks[1].y))
		return;

	movingToStack = true;
	for (int i = 0; i < 2; ++i) {
		int stackIndex = stackedCount + i;
		int column = stackIndex % 4;
		int row = stackIndex / 4;
		movingBlocks[i].targetX = 0.25f + column * 0.17f;
		movingBlocks[i].targetY = -0.78f + row * 0.15f;
		movingBlocks[i].arrived = false;
	}
}

void MoveTowardsTarget(MovingBlock& block, float deltaTime)
{
	float dx = block.targetX - block.x;
	float dy = block.targetY - block.y;
	float distance = std::sqrt(dx * dx + dy * dy);
	float speed = 0.90f;

	// 이번 프레임에 갈 거리보다 목표가 가까우면 지나치지 말고 정확히 붙인다.
	if (distance <= speed * deltaTime) {
		block.x = block.targetX;
		block.y = block.targetY;
		block.arrived = true;
		return;
	}

	block.x += dx / distance * speed * deltaTime;
	block.y += dy / distance * speed * deltaTime;
}

void SavePairInStack()
{
	for (int i = 0; i < 2 && stackedCount < MAX_STACKED_BLOCKS; ++i) {
		MovingBlock& source = movingBlocks[i];
		stackedBlocks[stackedCount] = { source.x, source.y, source.size,
			source.r, source.g, source.b };
		++stackedCount;
	}

	if (stackedCount <= MAX_STACKED_BLOCKS - 2)
		MakeNewPair();
}

void UpdateScene(float deltaTime)
{
	if (!movingToStack) {
		for (int i = 0; i < 2; ++i) {
			MovingBlock& block = movingBlocks[i];
			block.y += block.speedY * deltaTime;
			if (block.y > 0.80f || block.y < -0.80f) {
				block.speedY = -block.speedY;
				block.y = std::clamp(block.y, -0.80f, 0.80f);
			}
		}
		return;
	}

	MoveTowardsTarget(movingBlocks[0], deltaTime);
	MoveTowardsTarget(movingBlocks[1], deltaTime);
	if (movingBlocks[0].arrived && movingBlocks[1].arrived)
		SavePairInStack();
}

void DrawRectangleOutline(float left, float right, float bottom, float top,
	float r, float g, float b)
{
	Vertex rectangle[4] = {
		MakeVertex(left,  bottom, r, g, b),
		MakeVertex(right, bottom, r, g, b),
		MakeVertex(right, top,    r, g, b),
		MakeVertex(left,  top,    r, g, b)
	};
	DrawVertices(VBO, GL_LINE_LOOP, rectangle, 4);
}

void DrawSquare(float x, float y, float size, float r, float g, float b)
{
	Vertex vertices[6];
	int count = MakeFilledShape(vertices, SQUARE, x, y, size, 0.0f, r, g, b);
	DrawVertices(VBO, GL_TRIANGLES, vertices, count);
}

void DrawScene()
{
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glUseProgram(shaderProgramID);
	glBindVertexArray(VAO);

	// 두 세로 통로
	DrawRectangleOutline(-0.86f, -0.58f, -0.90f, 0.90f, 0.15f, 0.45f, 0.75f);
	DrawRectangleOutline(-0.48f, -0.20f, -0.90f, 0.90f, 0.15f, 0.45f, 0.75f);

	// 세 구역이 어디인지 알아보기 위한 가로 안내선
	Vertex guides[8] = {
		MakeVertex(-0.86f,  0.28f, 0.70f, 0.78f, 0.86f),
		MakeVertex(-0.58f,  0.28f, 0.70f, 0.78f, 0.86f),
		MakeVertex(-0.48f,  0.28f, 0.70f, 0.78f, 0.86f),
		MakeVertex(-0.20f,  0.28f, 0.70f, 0.78f, 0.86f),
		MakeVertex(-0.86f, -0.28f, 0.70f, 0.78f, 0.86f),
		MakeVertex(-0.58f, -0.28f, 0.70f, 0.78f, 0.86f),
		MakeVertex(-0.48f, -0.28f, 0.70f, 0.78f, 0.86f),
		MakeVertex(-0.20f, -0.28f, 0.70f, 0.78f, 0.86f)
	};
	DrawVertices(VBO, GL_LINES, guides, 8);

	for (int i = 0; i < stackedCount; ++i) {
		StackedBlock& block = stackedBlocks[i];
		DrawSquare(block.x, block.y, block.size, block.r, block.g, block.b);
	}
	for (int i = 0; i < 2; ++i) {
		MovingBlock& block = movingBlocks[i];
		DrawSquare(block.x, block.y, block.size, block.r, block.g, block.b);
	}
}

void KeyCallback(GLFWwindow* window, int key, int, int action, int)
{
	if (action != GLFW_PRESS)
		return;
	if (key == GLFW_KEY_ENTER)
		StartMovingToStack();
	else if (key == GLFW_KEY_R)
		ResetGame();
	else if (key == GLFW_KEY_Q || key == GLFW_KEY_ESCAPE)
		glfwSetWindowShouldClose(window, GLFW_TRUE);
}

int main()
{
	std::srand(static_cast<unsigned int>(std::time(nullptr)));
	if (glfwInit() == GLFW_FALSE)
		return -1;

	GLFWwindow* window = glfwCreateWindow(800, 800, "Practice 12", nullptr, nullptr);
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
	InitBuffer(VAO, VBO, 100);
	ResetGame();

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
