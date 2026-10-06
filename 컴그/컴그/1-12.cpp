#include "PracticeSelect.h"

#if ACTIVE_PRACTICE == 12

#include "Simple2D.h"

#include <random>
#include <vector>

using namespace Simple2D;

// 실습 12: 두 세로 공간의 사각형이 중앙 구역에 모이면
// Enter 키로 오른쪽으로 보내 차례대로 쌓는다.

constexpr float SQUARE_SIZE = 0.12f;
constexpr float LANE_BOTTOM = -0.82f;
constexpr float LANE_TOP = 0.82f;
constexpr float SYNC_BOTTOM = -0.14f;
constexpr float SYNC_TOP = 0.14f;
constexpr float STACK_X = 0.73f;
constexpr float STACK_BOTTOM = -0.84f;

enum class MoveState
{
	VerticalMoving,
	MovingFirstToStack,
	MovingSecondToStack
};

struct MovingSquare
{
	Vec2 position;
	float direction;
	float speed;
	Color color;
	Vec2 target;
};

struct StackedSquare
{
	Vec2 position;
	Color color;
};

Renderer renderer;
std::mt19937 randomEngine(std::random_device{}());
MovingSquare leftSquare{};
MovingSquare rightSquare{};
std::vector<StackedSquare> stackedSquares;
MoveState moveState = MoveState::VerticalMoving;

Color RandomColor()
{
	std::uniform_real_distribution<float> color(0.15f, 0.95f);
	return { color(randomEngine), color(randomEngine), color(randomEngine) };
}

Vec2 NextStackPosition()
{
	float gap = 0.012f;
	float y = STACK_BOTTOM
		+ SQUARE_SIZE * 0.5f
		+ static_cast<float>(stackedSquares.size()) * (SQUARE_SIZE + gap);
	return { STACK_X, y };
}

void CreateNewPair()
{
	std::uniform_real_distribution<float> startY(-0.70f, 0.70f);
	std::uniform_real_distribution<float> speed(0.28f, 0.55f);
	std::uniform_int_distribution<int> direction(0, 1);

	leftSquare = {
		{ -0.53f, startY(randomEngine) },
		direction(randomEngine) == 0 ? -1.0f : 1.0f,
		speed(randomEngine),
		RandomColor(),
		{}
	};
	rightSquare = {
		{ 0.08f, startY(randomEngine) },
		direction(randomEngine) == 0 ? -1.0f : 1.0f,
		speed(randomEngine),
		RandomColor(),
		{}
	};
	moveState = MoveState::VerticalMoving;
}

void ResetPractice()
{
	stackedSquares.clear();
	CreateNewPair();
	std::cout
		<< "실습 12: 두 사각형이 점선 구역에 함께 있을 때 ENTER, "
		<< "R=리셋, Q=종료\n";
}

bool IsInsideSyncArea(const MovingSquare& square)
{
	return square.position.y >= SYNC_BOTTOM
		&& square.position.y <= SYNC_TOP;
}

void UpdateVerticalSquare(MovingSquare& square, float deltaTime)
{
	float half = SQUARE_SIZE * 0.5f;
	square.position.y += square.direction * square.speed * deltaTime;

	if (square.position.y + half >= LANE_TOP) {
		square.position.y = LANE_TOP - half;
		square.direction = -1.0f;
	}
	else if (square.position.y - half <= LANE_BOTTOM) {
		square.position.y = LANE_BOTTOM + half;
		square.direction = 1.0f;
	}
}

bool MoveToward(Vec2& position, Vec2 target, float speed, float deltaTime)
{
	float dx = target.x - position.x;
	float dy = target.y - position.y;
	float distance = std::sqrt(dx * dx + dy * dy);
	float amount = speed * deltaTime;

	if (distance <= amount || distance < 0.001f) {
		position = target;
		return true;
	}

	position.x += dx / distance * amount;
	position.y += dy / distance * amount;
	return false;
}

void Update(float deltaTime)
{
	if (moveState == MoveState::VerticalMoving) {
		UpdateVerticalSquare(leftSquare, deltaTime);
		UpdateVerticalSquare(rightSquare, deltaTime);
		return;
	}

	if (moveState == MoveState::MovingFirstToStack) {
		if (MoveToward(leftSquare.position, leftSquare.target, 0.95f, deltaTime)) {
			stackedSquares.push_back({ leftSquare.position, leftSquare.color });
			rightSquare.target = NextStackPosition();
			moveState = MoveState::MovingSecondToStack;
		}
		return;
	}

	if (MoveToward(rightSquare.position, rightSquare.target, 0.95f, deltaTime)) {
		stackedSquares.push_back({ rightSquare.position, rightSquare.color });

		// 화면 높이를 넘을 만큼 쌓이면 새 탑을 만들기 위해 자동 리셋한다.
		if (stackedSquares.size() >= 12) {
			std::cout << "탑이 가득 차서 새 탑을 시작합니다.\n";
			stackedSquares.clear();
		}
		CreateNewPair();
	}
}

void DrawDashedHorizontalLine(float y, Color color)
{
	std::vector<Vertex> dashes;
	float start = -0.82f;
	float end = 0.31f;
	float dash = 0.055f;
	float gap = 0.035f;
	for (float x = start; x < end; x += dash + gap) {
		float dashEnd = std::min(x + dash, end);
		dashes.push_back(MakeVertex({ x, y }, color));
		dashes.push_back(MakeVertex({ dashEnd, y }, color));
	}
	renderer.Draw(GL_LINES, dashes);
}

void DrawSquare(Vec2 center, Color color)
{
	renderer.Draw(
		GL_TRIANGLES,
		MakeFilledRectangle(center, SQUARE_SIZE, SQUARE_SIZE, color)
	);
	renderer.Draw(
		GL_LINE_LOOP,
		MakeRectangleOutline(
			center, SQUARE_SIZE, SQUARE_SIZE,
			{ 0.12f, 0.36f, 0.62f }
		)
	);
}

void DrawScene()
{
	renderer.BeginFrame({ 0.97f, 0.97f, 0.95f });
	glLineWidth(2.0f);

	Color laneColor{ 0.25f, 0.50f, 0.75f };
	renderer.Draw(
		GL_LINE_LOOP,
		MakeRectangleOutline(
			{ -0.53f, 0.0f }, 0.30f,
			LANE_TOP - LANE_BOTTOM, laneColor
		)
	);
	renderer.Draw(
		GL_LINE_LOOP,
		MakeRectangleOutline(
			{ 0.08f, 0.0f }, 0.30f,
			LANE_TOP - LANE_BOTTOM, laneColor
		)
	);

	// 두 사각형이 동시에 들어와야 하는 중앙 공간
	DrawDashedHorizontalLine(SYNC_TOP, { 0.25f, 0.50f, 0.75f });
	DrawDashedHorizontalLine(SYNC_BOTTOM, { 0.25f, 0.50f, 0.75f });

	// 오른쪽 적재 위치를 알려 주는 세로 기준선
	renderer.Draw(
		GL_LINES,
		MakeLine({ STACK_X, -0.90f }, { STACK_X, 0.90f }, { 0.78f, 0.82f, 0.84f })
	);

	for (const StackedSquare& square : stackedSquares)
		DrawSquare(square.position, square.color);

	// 쌓는 도중에는 먼저 도착한 사각형이 stackedSquares에 들어가므로
	// 아직 이동 중인 사각형만 따로 그린다.
	if (moveState != MoveState::MovingSecondToStack)
		DrawSquare(leftSquare.position, leftSquare.color);
	DrawSquare(rightSquare.position, rightSquare.color);

	renderer.EndFrame();
}

void StartStacking()
{
	if (moveState != MoveState::VerticalMoving)
		return;

	if (!IsInsideSyncArea(leftSquare) || !IsInsideSyncArea(rightSquare)) {
		std::cout << "두 사각형이 모두 점선 구역 안에 있을 때 ENTER를 누르세요.\n";
		return;
	}

	// 왼쪽 사각형을 먼저 보내고, 도착하면 오른쪽 사각형을 보낸다.
	leftSquare.target = NextStackPosition();
	moveState = MoveState::MovingFirstToStack;
}

void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (action != GLFW_PRESS)
		return;
	if (key == GLFW_KEY_ENTER || key == GLFW_KEY_KP_ENTER)
		StartStacking();
	if (key == GLFW_KEY_R)
		ResetPractice();
	if (key == GLFW_KEY_Q || key == GLFW_KEY_ESCAPE)
		glfwSetWindowShouldClose(window, true);
}

void FramebufferSizeCallback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

int main()
{
	if (!glfwInit())
		return -1;

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(
		1000, 720,
		"Practice 12 - ENTER stack / R reset / Q quit",
		nullptr, nullptr
	);
	if (!window) {
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);
	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK || !renderer.Initialize()) {
		glfwDestroyWindow(window);
		glfwTerminate();
		return -1;
	}

	glfwSetKeyCallback(window, KeyCallback);
	glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);
	ResetPractice();

	double previousTime = glfwGetTime();
	while (!glfwWindowShouldClose(window)) {
		double currentTime = glfwGetTime();
		float deltaTime = static_cast<float>(currentTime - previousTime);
		previousTime = currentTime;

		Update(deltaTime);
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
