#include "PracticeCommon.h"
#include <ctime>

// 실습 10: 왼쪽의 조각을 마우스로 끌어 오른쪽의 같은 모양에 맞춘다.

const int MAX_SLOTS = 20;
const int MAX_PIECES = 24;

struct Slot
{
	ShapeType kind;
	float x;
	float y;
	float size;
	float angle;
	bool filled;
};

struct Piece
{
	ShapeType kind;
	float x;
	float y;
	float startX;
	float startY;
	float size;
	float angle;
	float r;
	float g;
	float b;
	int targetSlot;
	bool placed;
};

Slot slots[MAX_SLOTS];
Piece pieces[MAX_PIECES];
int slotCount = 0;
int pieceCount = 0;
int draggedPiece = -1;
float dragDifferenceX = 0.0f;
float dragDifferenceY = 0.0f;

GLuint shaderProgramID = 0;
GLuint VAO = 0;
GLuint VBO = 0;

void AddSlot(ShapeType kind, float x, float y, float size, float angle)
{
	if (slotCount >= MAX_SLOTS)
		return;

	slots[slotCount] = { kind, x, y, size, angle, false };
	++slotCount;
}

void MakeTargetShapes()
{
	slotCount = 0;

	// 모양판 1: 작은 사각형 네 개
	AddSlot(SQUARE, 0.22f, 0.72f, 0.10f, 0.0f);
	AddSlot(SQUARE, 0.36f, 0.72f, 0.10f, 0.0f);
	AddSlot(SQUARE, 0.22f, 0.58f, 0.10f, 0.0f);
	AddSlot(SQUARE, 0.36f, 0.58f, 0.10f, 0.0f);

	// 모양판 2: 서로 떨어진 직각삼각형 네 개
	AddSlot(RIGHT_TRIANGLE, 0.62f, 0.72f, 0.13f, 0.0f);
	AddSlot(RIGHT_TRIANGLE, 0.80f, 0.72f, 0.13f, PI * 0.5f);
	AddSlot(RIGHT_TRIANGLE, 0.62f, 0.54f, 0.13f, -PI * 0.5f);
	AddSlot(RIGHT_TRIANGLE, 0.80f, 0.54f, 0.13f, PI);

	// 모양판 3: 나란히 놓인 직각삼각형 두 개
	AddSlot(RIGHT_TRIANGLE, 0.22f, 0.15f, 0.18f, 0.0f);
	AddSlot(RIGHT_TRIANGLE, 0.43f, 0.15f, 0.18f, PI);

	// 모양판 4: 집 모양(사각형 위에 삼각형)
	AddSlot(SQUARE, 0.73f, 0.08f, 0.18f, 0.0f);
	AddSlot(TRIANGLE, 0.73f, 0.27f, 0.18f, 0.0f);

	// 모양판 5: 산처럼 나란한 삼각형 세 개
	AddSlot(TRIANGLE, 0.30f, -0.52f, 0.17f, 0.0f);
	AddSlot(TRIANGLE, 0.52f, -0.52f, 0.17f, 0.0f);
	AddSlot(TRIANGLE, 0.74f, -0.52f, 0.17f, 0.0f);
}

void SwapPieces(int first, int second)
{
	Piece temporary = pieces[first];
	pieces[first] = pieces[second];
	pieces[second] = temporary;
}

void ResetGame()
{
	MakeTargetShapes();
	pieceCount = slotCount + 2 + std::rand() % 3;

	for (int i = 0; i < slotCount; ++i) {
		float red = RandomFloat(0.15f, 0.95f);
		float green = RandomFloat(0.15f, 0.95f);
		float blue = RandomFloat(0.15f, 0.95f);
		pieces[i] = { slots[i].kind, 0.0f, 0.0f, 0.0f, 0.0f,
			slots[i].size, slots[i].angle, red, green, blue, i, false };
	}

	// 목표에 없는 방해용 조각도 2~4개 넣는다.
	for (int i = slotCount; i < pieceCount; ++i) {
		ShapeType randomKind = static_cast<ShapeType>(std::rand() % 4);
		pieces[i] = { randomKind, 0.0f, 0.0f, 0.0f, 0.0f, 0.11f, 0.0f,
			RandomFloat(0.15f, 0.95f), RandomFloat(0.15f, 0.95f),
			RandomFloat(0.15f, 0.95f), -1, false };
	}

	// 배열 순서를 섞으면 매번 조각의 종류가 다른 위치에 나타난다.
	for (int i = pieceCount - 1; i > 0; --i)
		SwapPieces(i, std::rand() % (i + 1));

	// 5열 격자에 하나씩 놓으므로 시작할 때 서로 겹치지 않는다.
	for (int i = 0; i < pieceCount; ++i) {
		int column = i % 5;
		int row = i / 5;
		pieces[i].x = -0.90f + column * 0.18f;
		pieces[i].y = 0.76f - row * 0.43f;
		pieces[i].startX = pieces[i].x;
		pieces[i].startY = pieces[i].y;
	}

	draggedPiece = -1;
}

void DrawPiece(const Piece& piece)
{
	Vertex vertices[6];
	int count = MakeFilledShape(vertices, piece.kind, piece.x, piece.y,
		piece.size, piece.angle, piece.r, piece.g, piece.b);
	DrawVertices(VBO, GL_TRIANGLES, vertices, count);
}

void DrawSlot(const Slot& slot)
{
	Vertex vertices[6];
	int count = MakeShapeOutline(vertices, slot.kind, slot.x, slot.y,
		slot.size, slot.angle, 0.15f, 0.45f, 0.75f);
	DrawVertices(VBO, GL_LINE_LOOP, vertices, count);
}

void DrawScene()
{
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glUseProgram(shaderProgramID);
	glBindVertexArray(VAO);

	// 왼쪽 조각 구역과 오른쪽 모양판 구역을 나누는 선이다.
	Vertex divider[2] = {
		MakeVertex(0.0f, -0.95f, 0.65f, 0.75f, 0.85f),
		MakeVertex(0.0f,  0.95f, 0.65f, 0.75f, 0.85f)
	};
	DrawVertices(VBO, GL_LINES, divider, 2);

	for (int i = 0; i < slotCount; ++i)
		DrawSlot(slots[i]);
	for (int i = 0; i < pieceCount; ++i)
		DrawPiece(pieces[i]);
}

int FindPiece(float mouseX, float mouseY)
{
	// 뒤에 그린 조각부터 검사하면 눈에 보이는 맨 위 조각이 선택된다.
	for (int i = pieceCount - 1; i >= 0; --i) {
		if (!pieces[i].placed && PointInsideShape(mouseX, mouseY,
			pieces[i].x, pieces[i].y, pieces[i].size))
			return i;
	}
	return -1;
}

void MouseButtonCallback(GLFWwindow* window, int button, int action, int)
{
	if (button != GLFW_MOUSE_BUTTON_LEFT)
		return;

	double mousePixelX;
	double mousePixelY;
	glfwGetCursorPos(window, &mousePixelX, &mousePixelY);
	Point2D mouse = MouseToOpenGL(window, mousePixelX, mousePixelY);

	if (action == GLFW_PRESS) {
		draggedPiece = FindPiece(mouse.x, mouse.y);
		if (draggedPiece != -1) {
			dragDifferenceX = pieces[draggedPiece].x - mouse.x;
			dragDifferenceY = pieces[draggedPiece].y - mouse.y;
		}
	}
	else if (action == GLFW_RELEASE && draggedPiece != -1) {
		Piece& piece = pieces[draggedPiece];
		bool correct = false;

		if (piece.targetSlot >= 0) {
			Slot& target = slots[piece.targetSlot];
			if (!target.filled && Distance(piece.x, piece.y, target.x, target.y) < 0.10f) {
				piece.x = target.x;
				piece.y = target.y;
				piece.angle = target.angle;
				piece.placed = true;
				target.filled = true;
				correct = true;
			}
		}

		if (!correct) {
			piece.x = piece.startX;
			piece.y = piece.startY;
		}
		draggedPiece = -1;
	}
}

void CursorPositionCallback(GLFWwindow* window, double mousePixelX, double mousePixelY)
{
	if (draggedPiece == -1)
		return;

	Point2D mouse = MouseToOpenGL(window, mousePixelX, mousePixelY);
	pieces[draggedPiece].x = mouse.x + dragDifferenceX;
	pieces[draggedPiece].y = mouse.y + dragDifferenceY;
}

void KeyCallback(GLFWwindow* window, int key, int, int action, int)
{
	if (action != GLFW_PRESS)
		return;
	if (key == GLFW_KEY_R)
		ResetGame();
	else if (key == GLFW_KEY_Q || key == GLFW_KEY_ESCAPE)
		glfwSetWindowShouldClose(window, GLFW_TRUE);
}

int main()
{
	std::srand(static_cast<unsigned int>(std::time(nullptr)));
	if (glfwInit() == GLFW_FALSE)
		return -1;

	GLFWwindow* window = glfwCreateWindow(800, 800, "Practice 10", nullptr, nullptr);
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
	glfwSetMouseButtonCallback(window, MouseButtonCallback);
	glfwSetCursorPosCallback(window, CursorPositionCallback);
	shaderProgramID = MakeShaderProgram("vertex-basic.glsl", "fragment-basic.glsl");
	if (shaderProgramID == 0) {
		glfwTerminate();
		return -1;
	}
	InitBuffer(VAO, VBO, 100);
	ResetGame();

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
