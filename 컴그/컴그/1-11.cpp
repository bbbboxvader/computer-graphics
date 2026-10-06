#include "PracticeCommon.h"
#include <ctime>

// 실습 11: 20 x 20 보드 위에서 주인공이 뱀처럼 한 칸씩 이동한다.

const int BOARD_SIZE = 20;
const int OBSTACLE_COUNT = 65;
const float BOARD_LEFT = -0.90f;
const float BOARD_TOP = 0.90f;
const float CELL_SIZE = 1.80f / BOARD_SIZE;

struct BoardObject
{
	int row;
	int column;
	ShapeType kind;
	float size;
	float r;
	float g;
	float b;
	bool active;
};

BoardObject hero;
BoardObject obstacles[OBSTACLE_COUNT];
bool automaticMove = false;
bool finished = false;
float moveTimer = 0.0f;
float collisionTimer = 0.0f;

GLuint shaderProgramID = 0;
GLuint VAO = 0;
GLuint VBO = 0;

Point2D CellCenter(int row, int column)
{
	Point2D center;
	center.x = BOARD_LEFT + (column + 0.5f) * CELL_SIZE;
	center.y = BOARD_TOP - (row + 0.5f) * CELL_SIZE;
	return center;
}

bool CellAlreadyUsed(int row, int column, int usedCount)
{
	for (int i = 0; i < usedCount; ++i) {
		if (obstacles[i].row == row && obstacles[i].column == column)
			return true;
	}
	return false;
}

void ResetGame()
{
	hero = { 0, 0, SQUARE, CELL_SIZE * 0.72f,
		0.10f, 0.35f, 0.95f, true };
	finished = false;
	automaticMove = false;
	moveTimer = 0.0f;
	collisionTimer = 0.0f;

	for (int i = 0; i < OBSTACLE_COUNT; ++i) {
		int row;
		int column;
		do {
			row = std::rand() % BOARD_SIZE;
			column = std::rand() % BOARD_SIZE;
		} while ((row == 0 && column == 0) || CellAlreadyUsed(row, column, i));

		ShapeType kind = static_cast<ShapeType>(std::rand() % 3);
		obstacles[i] = { row, column, kind,
			RandomFloat(CELL_SIZE * 0.48f, CELL_SIZE * 0.78f),
			RandomFloat(0.15f, 0.95f), RandomFloat(0.15f, 0.95f),
			RandomFloat(0.15f, 0.95f), true };
	}
}

void SwapHeroAndObstacle(BoardObject& obstacle)
{
	ShapeType oldKind = hero.kind;
	float oldSize = hero.size;
	float oldR = hero.r;
	float oldG = hero.g;
	float oldB = hero.b;

	hero.kind = obstacle.kind;
	hero.size = obstacle.size;
	hero.r = obstacle.r;
	hero.g = obstacle.g;
	hero.b = obstacle.b;

	obstacle.kind = oldKind;
	obstacle.size = oldSize;
	obstacle.r = oldR;
	obstacle.g = oldG;
	obstacle.b = oldB;
	collisionTimer = 0.28f;
}

void CheckCollision()
{
	for (int i = 0; i < OBSTACLE_COUNT; ++i) {
		if (obstacles[i].active && obstacles[i].row == hero.row &&
			obstacles[i].column == hero.column) {
			SwapHeroAndObstacle(obstacles[i]);
			return;
		}
	}
}

void MoveOneCell()
{
	if (finished)
		return;

	// 짝수 줄은 오른쪽, 홀수 줄은 왼쪽으로 가는 뱀 모양 이동이다.
	if (hero.row % 2 == 0) {
		if (hero.column < BOARD_SIZE - 1)
			++hero.column;
		else if (hero.row < BOARD_SIZE - 1)
			++hero.row;
	}
	else {
		if (hero.column > 0)
			--hero.column;
		else if (hero.row < BOARD_SIZE - 1)
			++hero.row;
	}

	CheckCollision();
	if (hero.row == BOARD_SIZE - 1 && hero.column == 0) {
		finished = true;
		automaticMove = false;
	}
}

void UpdateScene(float deltaTime)
{
	if (collisionTimer > 0.0f)
		collisionTimer -= deltaTime;

	if (!automaticMove || finished)
		return;

	moveTimer += deltaTime;
	if (moveTimer >= 0.12f) {
		moveTimer = 0.0f;
		MoveOneCell();
	}
}

void DrawGrid()
{
	Vertex lines[(BOARD_SIZE + 1) * 4];
	int count = 0;
	for (int i = 0; i <= BOARD_SIZE; ++i) {
		float x = BOARD_LEFT + i * CELL_SIZE;
		lines[count++] = MakeVertex(x, -0.90f, 0.75f, 0.82f, 0.90f);
		lines[count++] = MakeVertex(x,  0.90f, 0.75f, 0.82f, 0.90f);

		float y = BOARD_TOP - i * CELL_SIZE;
		lines[count++] = MakeVertex(-0.90f, y, 0.75f, 0.82f, 0.90f);
		lines[count++] = MakeVertex( 0.90f, y, 0.75f, 0.82f, 0.90f);
	}
	DrawVertices(VBO, GL_LINES, lines, count);
}

void DrawBoardObject(const BoardObject& object)
{
	if (!object.active)
		return;
	Point2D center = CellCenter(object.row, object.column);
	Vertex vertices[6];
	int count = MakeFilledShape(vertices, object.kind, center.x, center.y,
		object.size, 0.0f, object.r, object.g, object.b);
	DrawVertices(VBO, GL_TRIANGLES, vertices, count);
}

void DrawCollisionEffect()
{
	if (collisionTimer <= 0.0f)
		return;
	Point2D center = CellCenter(hero.row, hero.column);
	Vertex outline[6];
	int count = MakeShapeOutline(outline, SQUARE, center.x, center.y,
		CELL_SIZE * 0.92f, 0.0f, 1.0f, 0.35f, 0.05f);
	glLineWidth(4.0f);
	DrawVertices(VBO, GL_LINE_LOOP, outline, count);
	glLineWidth(1.0f);
}

void DrawScene()
{
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glUseProgram(shaderProgramID);
	glBindVertexArray(VAO);

	DrawGrid();
	for (int i = 0; i < OBSTACLE_COUNT; ++i)
		DrawBoardObject(obstacles[i]);
	DrawBoardObject(hero);
	DrawCollisionEffect();
}

void KeyCallback(GLFWwindow* window, int key, int, int action, int)
{
	if (action != GLFW_PRESS)
		return;

	if (key == GLFW_KEY_SPACE || key == GLFW_KEY_RIGHT)
		MoveOneCell();
	else if (key == GLFW_KEY_A) {
		automaticMove = !automaticMove;
		moveTimer = 0.0f;
	}
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

	GLFWwindow* window = glfwCreateWindow(800, 800, "Practice 11", nullptr, nullptr);
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
