#include "PracticeSelect.h"

#if ACTIVE_PRACTICE == 10

#include "Simple2D.h"

#include <algorithm>
#include <array>
#include <random>
#include <string>
#include <vector>

using namespace Simple2D;

// 실습 10: 왼쪽의 작은 도형을 오른쪽 모양판에 드래그해서 맞추기

struct Slot
{
	ShapeKind kind;
	Vec2 center;
	float size;
	float angle;
	int boardIndex;
	bool filled;
};

struct Piece
{
	ShapeKind kind;
	Vec2 position;
	float size;
	float angle;
	Color color;
	int targetSlot;
	bool placed;
};

struct Board
{
	Vec2 center;
	float width;
	float height;
	std::vector<int> slots;
	bool complete;
};

Renderer renderer;
std::vector<Slot> slots;
std::vector<Piece> pieces;
std::vector<Board> boards;

std::mt19937 randomEngine(std::random_device{}());
std::uniform_real_distribution<float> randomX(-0.91f, -0.12f);
std::uniform_real_distribution<float> randomY(-0.88f, 0.88f);
std::uniform_real_distribution<float> randomColor(0.15f, 0.95f);

int draggedPiece = -1;
Vec2 dragDifference{ 0.0f, 0.0f };
Vec2 dragStartPosition{ 0.0f, 0.0f };

Color RandomColor()
{
	return {
		randomColor(randomEngine),
		randomColor(randomEngine),
		randomColor(randomEngine)
	};
}

Vec2 FindFreeLeftPosition(float size)
{
	float margin = size * 0.65f;
	std::uniform_real_distribution<float> x(-0.94f + margin, -0.06f - margin);
	std::uniform_real_distribution<float> y(-0.94f + margin, 0.94f - margin);

	Vec2 candidate{};
	for (int attempt = 0; attempt < 150; ++attempt) {
		candidate = { x(randomEngine), y(randomEngine) };
		bool overlaps = false;
		for (const Piece& piece : pieces) {
			float wantedDistance = (size + piece.size) * 0.58f;
			if (Distance(candidate, piece.position) < wantedDistance) {
				overlaps = true;
				break;
			}
		}
		if (!overlaps)
			return candidate;
	}

	// 조각이 많아 빈 곳을 찾지 못하면 마지막 후보를 사용한다.
	return candidate;
}

int AddBoard(Vec2 center, float width, float height)
{
	boards.push_back({ center, width, height, {}, false });
	return static_cast<int>(boards.size()) - 1;
}

Vec2 CenterRightTriangleAtCorner(Vec2 corner, float size, float angle)
{
	// RightTriangle의 직각 꼭짓점은 회전 전 (-size/2, -size/2)이다.
	// 네 삼각형의 직각 꼭짓점을 같은 점에 맞추면 겹치지 않는 바람개비가 된다.
	Vec2 localCorner = Rotate({ -size * 0.5f, -size * 0.5f }, angle);
	return {
		corner.x - localCorner.x,
		corner.y - localCorner.y
	};
}

void AddSlot(
	int boardIndex,
	ShapeKind kind,
	Vec2 center,
	float size,
	float angle = 0.0f)
{
	int slotIndex = static_cast<int>(slots.size());
	slots.push_back({ kind, center, size, angle, boardIndex, false });
	boards[boardIndex].slots.push_back(slotIndex);

	// 모양판에 꼭 필요한 조각을 왼쪽 임의 위치에 하나 만든다.
	pieces.push_back({
		kind,
		FindFreeLeftPosition(size),
		size,
		angle,
		RandomColor(),
		-1,
		false
	});
}

void CheckBoardComplete(int boardIndex)
{
	Board& board = boards[boardIndex];
	board.complete = true;
	for (int slotIndex : board.slots) {
		if (!slots[slotIndex].filled) {
			board.complete = false;
			break;
		}
	}

	if (board.complete)
		std::cout << "모양판 " << boardIndex + 1 << " 완성!\n";
}

void ResetPractice()
{
	slots.clear();
	pieces.clear();
	boards.clear();
	draggedPiece = -1;

	// 1번 모양판: 작은 사각형 4개
	int board = AddBoard({ 0.27f, 0.70f }, 0.26f, 0.26f);
	AddSlot(board, ShapeKind::Square, { 0.22f, 0.75f }, 0.075f);
	AddSlot(board, ShapeKind::Square, { 0.32f, 0.75f }, 0.075f);
	AddSlot(board, ShapeKind::Square, { 0.22f, 0.65f }, 0.075f);
	AddSlot(board, ShapeKind::Square, { 0.32f, 0.65f }, 0.075f);

	// 2번 모양판: 직각삼각형 4개가 중심점에서 만나되 서로 겹치지 않는다.
	board = AddBoard({ 0.72f, 0.70f }, 0.26f, 0.26f);
	const Vec2 pinwheelCenter{ 0.72f, 0.70f };
	const float pinwheelSize = 0.12f;
	for (int i = 0; i < 4; ++i) {
		float angle = PI * 0.5f * i;
		AddSlot(
			board,
			ShapeKind::RightTriangle,
			CenterRightTriangleAtCorner(pinwheelCenter, pinwheelSize, angle),
			pinwheelSize,
			angle
		);
	}

	// 3번 모양판: 직각삼각형 2개로 사각형 만들기
	board = AddBoard({ 0.27f, 0.18f }, 0.26f, 0.26f);
	AddSlot(board, ShapeKind::RightTriangle, { 0.27f, 0.18f }, 0.20f, 0.0f);
	AddSlot(board, ShapeKind::RightTriangle, { 0.27f, 0.18f }, 0.20f, PI);

	// 4번 모양판: 사각형과 정삼각형으로 집 만들기
	board = AddBoard({ 0.72f, 0.18f }, 0.26f, 0.30f);
	AddSlot(board, ShapeKind::Square, { 0.72f, 0.10f }, 0.14f);
	AddSlot(board, ShapeKind::Triangle, { 0.72f, 0.30f }, 0.17f);

	// 5번 모양판: 정삼각형 3개로 산 모양 만들기(학생이 추가한 모양판)
	board = AddBoard({ 0.50f, -0.55f }, 0.40f, 0.32f);
	AddSlot(board, ShapeKind::Triangle, { 0.38f, -0.60f }, 0.14f);
	AddSlot(board, ShapeKind::Triangle, { 0.62f, -0.60f }, 0.14f);
	AddSlot(board, ShapeKind::Triangle, { 0.50f, -0.40f }, 0.14f);

	// 문제의 "랜덤한 개수"를 보이기 위한 여분 조각 2~5개.
	std::uniform_int_distribution<int> extraCount(2, 5);
	std::uniform_int_distribution<int> extraKind(0, 2);
	int count = extraCount(randomEngine);
	for (int i = 0; i < count; ++i) {
		ShapeKind kind = ShapeKind::Square;
		int kindNumber = extraKind(randomEngine);
		if (kindNumber == 1)
			kind = ShapeKind::Triangle;
		else if (kindNumber == 2)
			kind = ShapeKind::RightTriangle;

		pieces.push_back({
			kind,
			FindFreeLeftPosition(0.07f),
			0.07f,
			0.0f,
			RandomColor(),
			-1,
			false
		});
	}

	std::shuffle(pieces.begin(), pieces.end(), randomEngine);
	std::cout << "실습 10 리셋: 조각을 드래그해서 같은 모양판에 놓으세요.\n";
}

void DrawPiece(const Piece& piece)
{
	renderer.Draw(
		GL_TRIANGLES,
		MakeShape(piece.kind, piece.position, piece.size, piece.color, piece.angle)
	);
	renderer.Draw(
		GL_LINE_LOOP,
		MakeShapeOutline(
			piece.kind, piece.position, piece.size,
			{ 0.10f, 0.30f, 0.55f }, piece.angle
		)
	);
}

void DrawScene()
{
	renderer.BeginFrame({ 0.97f, 0.97f, 0.95f });
	glLineWidth(2.0f);

	// 왼쪽 조각 영역과 오른쪽 모양판 영역을 나누는 선
	renderer.Draw(GL_LINES, MakeLine({ 0.0f, -0.95f }, { 0.0f, 0.95f }, { 0.35f, 0.55f, 0.75f }));

	// 바깥쪽 사각형 모양판 테두리는 그리지 않는다.
	// 필요한 도형 자리의 윤곽선만 보이므로 각 모양이 서로 분리되어 보인다.
	for (const Slot& slot : slots) {
		if (!slot.filled) {
			renderer.Draw(
				GL_LINE_LOOP,
				MakeShapeOutline(
					slot.kind, slot.center, slot.size,
					{ 0.45f, 0.62f, 0.78f }, slot.angle
				)
			);
		}
	}

	for (const Piece& piece : pieces)
		DrawPiece(piece);

	renderer.EndFrame();
}

void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	if (button != GLFW_MOUSE_BUTTON_LEFT)
		return;

	double mouseX = 0.0;
	double mouseY = 0.0;
	glfwGetCursorPos(window, &mouseX, &mouseY);
	Vec2 mouse = MouseToOpenGL(window, mouseX, mouseY);

	if (action == GLFW_PRESS) {
		// 뒤에 그린 조각부터 검사해야 겹쳤을 때 위쪽 조각이 선택된다.
		for (int i = static_cast<int>(pieces.size()) - 1; i >= 0; --i) {
			Piece& piece = pieces[i];
			// 한 번 정확한 자리에 들어간 조각은 즉시 잠근다.
			if (piece.placed)
				continue;

			if (PointInShape(piece.kind, mouse, piece.position, piece.size)) {
				draggedPiece = i;
				dragDifference = {
					piece.position.x - mouse.x,
					piece.position.y - mouse.y
				};
				dragStartPosition = piece.position;
				break;
			}
		}
	}
	else if (action == GLFW_RELEASE && draggedPiece >= 0) {
		Piece& piece = pieces[draggedPiece];

		// 숨겨진 정답 번호가 아니라 눈에 보이는 종류·크기·방향으로 맞춘다.
		int bestSlot = -1;
		float bestDistance = 1000.0f;
		for (int i = 0; i < static_cast<int>(slots.size()); ++i) {
			const Slot& slot = slots[i];
			if (slot.filled || slot.kind != piece.kind)
				continue;
			if (std::abs(slot.size - piece.size) > 0.001f)
				continue;
			if (std::abs(slot.angle - piece.angle) > 0.001f)
				continue;

			float distance = Distance(piece.position, slot.center);
			float snapDistance = std::max(0.10f, slot.size * 0.80f);
			if (distance <= snapDistance && distance < bestDistance) {
				bestSlot = i;
				bestDistance = distance;
			}
		}

		if (bestSlot >= 0) {
			Slot& slot = slots[bestSlot];
			piece.position = slot.center;
			piece.targetSlot = bestSlot;
			piece.placed = true;
			slot.filled = true;
			CheckBoardComplete(slot.boardIndex);
		}
		else {
			// 틀린 곳에 놓으면 원래 있던 왼쪽 자리로 돌아간다.
			piece.position = dragStartPosition;
		}
		draggedPiece = -1;
	}
}

void CursorPositionCallback(GLFWwindow* window, double mouseX, double mouseY)
{
	if (draggedPiece < 0)
		return;

	Vec2 mouse = MouseToOpenGL(window, mouseX, mouseY);
	pieces[draggedPiece].position = {
		mouse.x + dragDifference.x,
		mouse.y + dragDifference.y
	};
}

void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (action != GLFW_PRESS)
		return;
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
		1000, 760,
		"Practice 10 - Drag pieces / R reset / Q quit",
		nullptr, nullptr
	);
	if (!window) {
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

	if (!renderer.Initialize()) {
		glfwDestroyWindow(window);
		glfwTerminate();
		return -1;
	}

	glfwSetMouseButtonCallback(window, MouseButtonCallback);
	glfwSetCursorPosCallback(window, CursorPositionCallback);
	glfwSetKeyCallback(window, KeyCallback);
	glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);

	ResetPractice();
	while (!glfwWindowShouldClose(window)) {
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
