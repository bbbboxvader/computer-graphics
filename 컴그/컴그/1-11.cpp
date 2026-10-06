#include "PracticeSelect.h"

#if ACTIVE_PRACTICE == 11

#include "Simple2D.h"

#include <array>
#include <random>

using namespace Simple2D;

// 실습 11: 20 x 20 보드에서 주인공을 좌우 지그재그로 이동시키기

constexpr int BOARD_SIZE = 20;
constexpr int CELL_COUNT = BOARD_SIZE * BOARD_SIZE;
constexpr float BOARD_LEFT = -0.92f;
constexpr float BOARD_RIGHT = 0.92f;
constexpr float BOARD_BOTTOM = -0.92f;
constexpr float BOARD_TOP = 0.92f;
constexpr float CELL_SIZE = (BOARD_RIGHT - BOARD_LEFT) / BOARD_SIZE;

struct CellShape
{
	bool active;
	ShapeKind kind;
	Color color;
	float size;
};

Renderer renderer;
std::array<CellShape, CELL_COUNT> obstacles{};
std::mt19937 randomEngine(std::random_device{}());

int pathIndex = 0;
ShapeKind heroKind = ShapeKind::Square;
Color heroColor{ 0.15f, 0.45f, 0.95f };
float heroSize = CELL_SIZE * 0.68f;

bool automaticMove = false;
bool reachedLastCell = false;
float automaticTimer = 0.0f;
float stepInterval = 0.09f;

float collisionTimer = 0.0f;
int collisionCell = -1;

int CellIndexFromPath(int index)
{
	int row = index / BOARD_SIZE;
	int positionInRow = index % BOARD_SIZE;
	int column = row % 2 == 0
		? positionInRow
		: BOARD_SIZE - 1 - positionInRow;
	return row * BOARD_SIZE + column;
}

Vec2 CellCenter(int cellIndex)
{
	int row = cellIndex / BOARD_SIZE;
	int column = cellIndex % BOARD_SIZE;
	return {
		BOARD_LEFT + (column + 0.5f) * CELL_SIZE,
		BOARD_TOP - (row + 0.5f) * CELL_SIZE
	};
}

Color RandomColor()
{
	std::uniform_real_distribution<float> color(0.12f, 0.95f);
	return { color(randomEngine), color(randomEngine), color(randomEngine) };
}

void ResetPractice()
{
	for (CellShape& obstacle : obstacles)
		obstacle.active = false;

	pathIndex = 0;
	heroKind = ShapeKind::Square;
	heroColor = { 0.15f, 0.45f, 0.95f };
	heroSize = CELL_SIZE * 0.68f;
	automaticMove = false;
	reachedLastCell = false;
	automaticTimer = 0.0f;
	collisionTimer = 0.0f;
	collisionCell = -1;

	std::uniform_int_distribution<int> randomCell(1, CELL_COUNT - 2);
	std::uniform_int_distribution<int> randomKind(0, 2);
	std::uniform_real_distribution<float> randomSize(0.45f, 0.78f);

	// 장애물은 겹치지 않게 충분한 개수(70개)를 배치한다.
	int made = 0;
	while (made < 70) {
		int cell = randomCell(randomEngine);
		if (obstacles[cell].active)
			continue;

		int kindNumber = randomKind(randomEngine);
		ShapeKind kind = ShapeKind::Triangle;
		if (kindNumber == 1)
			kind = ShapeKind::Square;
		else if (kindNumber == 2)
			kind = ShapeKind::InvertedTriangle;

		obstacles[cell] = {
			true,
			kind,
			RandomColor(),
			CELL_SIZE * randomSize(randomEngine)
		};
		++made;
	}

	std::cout
		<< "실습 11: SPACE/오른쪽 화살표=한 칸, A=자동 이동, "
		<< "R=리셋, Q=종료\n";
}

void MoveOneCell()
{
	if (reachedLastCell)
		return;

	++pathIndex;
	if (pathIndex >= CELL_COUNT - 1) {
		pathIndex = CELL_COUNT - 1;
		reachedLastCell = true;
		automaticMove = false;
		std::cout << "마지막 칸에 도착했습니다!\n";
	}

	int cell = CellIndexFromPath(pathIndex);
	CellShape& obstacle = obstacles[cell];
	if (!obstacle.active)
		return;

	// 충돌하면 주인공과 장애물의 모양, 색상, 크기를 서로 교환한다.
	std::swap(heroKind, obstacle.kind);
	std::swap(heroColor, obstacle.color);
	std::swap(heroSize, obstacle.size);

	collisionTimer = 0.45f;
	collisionCell = cell;
}

void Update(float deltaTime)
{
	if (collisionTimer > 0.0f)
		collisionTimer -= deltaTime;

	if (!automaticMove || reachedLastCell)
		return;

	automaticTimer += deltaTime;
	while (automaticTimer >= stepInterval && !reachedLastCell) {
		automaticTimer -= stepInterval;
		MoveOneCell();
	}
}

void DrawBoardLines()
{
	std::vector<Vertex> lines;
	Color gridColor{ 0.64f, 0.72f, 0.79f };
	lines.reserve((BOARD_SIZE + 1) * 4);

	for (int i = 0; i <= BOARD_SIZE; ++i) {
		float x = BOARD_LEFT + i * CELL_SIZE;
		lines.push_back(MakeVertex({ x, BOARD_BOTTOM }, gridColor));
		lines.push_back(MakeVertex({ x, BOARD_TOP }, gridColor));

		float y = BOARD_BOTTOM + i * CELL_SIZE;
		lines.push_back(MakeVertex({ BOARD_LEFT, y }, gridColor));
		lines.push_back(MakeVertex({ BOARD_RIGHT, y }, gridColor));
	}
	renderer.Draw(GL_LINES, lines);
}

void DrawScene()
{
	renderer.BeginFrame({ 0.97f, 0.97f, 0.95f });
	glLineWidth(1.0f);
	DrawBoardLines();

	for (int cell = 0; cell < CELL_COUNT; ++cell) {
		const CellShape& obstacle = obstacles[cell];
		if (!obstacle.active)
			continue;

		Vec2 center = CellCenter(cell);
		renderer.Draw(
			GL_TRIANGLES,
			MakeShape(obstacle.kind, center, obstacle.size, obstacle.color)
		);
	}

	int heroCell = CellIndexFromPath(pathIndex);
	Vec2 heroCenter = CellCenter(heroCell);
	renderer.Draw(
		GL_TRIANGLES,
		MakeShape(heroKind, heroCenter, heroSize, heroColor)
	);

	// 충돌 후 잠깐 보이는 두 겹의 주황색 테두리 효과
	if (collisionTimer > 0.0f && collisionCell >= 0) {
		float ratio = collisionTimer / 0.45f;
		float size1 = CELL_SIZE * (0.9f + (1.0f - ratio) * 0.55f);
		Vec2 center = CellCenter(collisionCell);
		glLineWidth(3.0f);
		renderer.Draw(
			GL_LINE_LOOP,
			MakeRectangleOutline(center, size1, size1, { 1.0f, 0.25f, 0.05f })
		);
		glLineWidth(1.0f);
	}

	renderer.EndFrame();
}

void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (action != GLFW_PRESS)
		return;

	if (key == GLFW_KEY_SPACE || key == GLFW_KEY_RIGHT)
		MoveOneCell();
	if (key == GLFW_KEY_A) {
		automaticMove = !automaticMove;
		std::cout << "자동 이동: " << (automaticMove ? "켜짐" : "꺼짐") << '\n';
	}
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
		850, 850,
		"Practice 11 - SPACE step / A auto / R reset / Q quit",
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
