#include <GL/glew.h>
#include <GL/glfw3.h>
#include <algorithm>
#include <iostream>
#include <random>

std::random_device rd;
std::mt19937 gen(rd());

constexpr int MaxOriginalBoxCount = 10;
constexpr int MaxPieceCount = 100;

struct Box {
	float X;
	float Y;
	float size;

	float R;
	float G;
	float B;

	bool active;
};

struct Piece {
	float X;
	float Y;
	float size;

	float R;
	float G;
	float B;

	float directionX;
	float directionY;
	float moveSpeed;
	float shrinkSpeed;
	float colorDirection;

	bool active;
};

Box boxes[MaxOriginalBoxCount];
Piece pieces[MaxPieceCount];

int boxCount = 0;
int pieceCount = 0;
bool pressR = false;

std::uniform_real_distribution<float> colorDist(0.0f, 1.0f);
std::uniform_real_distribution<float> sizeDist(0.18f, 0.36f);
std::uniform_real_distribution<float> moveSpeedDist(0.0025f, 0.0060f);
std::uniform_real_distribution<float> shrinkSpeedDist(0.0003f, 0.0007f);
std::uniform_int_distribution<int> firstBoxCountDist(5, 10);
std::uniform_int_distribution<int> animationDist(1, 4);
std::uniform_int_distribution<int> directionDist(0, 7);
std::uniform_int_distribution<int> colorDirectionDist(0, 1);

constexpr float DiagonalValue = 0.70710678f;

const float EightDirectionX[8] = {
	1.0f,
	DiagonalValue,
	0.0f,
	-DiagonalValue,
	-1.0f,
	-DiagonalValue,
	0.0f,
	DiagonalValue
};

const float EightDirectionY[8] = {
	0.0f,
	DiagonalValue,
	1.0f,
	DiagonalValue,
	0.0f,
	-DiagonalValue,
	-1.0f,
	-DiagonalValue
};

// 마우스의 픽셀 좌표를 OpenGL 좌표로 변경한다.
void GetOpenGLMousePosition(
	GLFWwindow* window,
	double mouseX,
	double mouseY,
	float& openGLX,
	float& openGLY)
{
	int width;
	int height;
	glfwGetWindowSize(window, &width, &height);

	openGLX =
		static_cast<float>(mouseX / width) * 2.0f - 1.0f;

	openGLY =
		1.0f - static_cast<float>(mouseY / height) * 2.0f;
}

void MakeBox(Box& box)
{
	box.size = sizeDist(gen);
	float halfSize = box.size / 2.0f;

	std::uniform_real_distribution<float> positionDist(
		-1.0f + halfSize,
		1.0f - halfSize
	);

	box.X = positionDist(gen);
	box.Y = positionDist(gen);

	box.R = colorDist(gen);
	box.G = colorDist(gen);
	box.B = colorDist(gen);

	box.active = true;
}

void ResetScene()
{
	for (int i = 0; i < MaxOriginalBoxCount; i++) {
		boxes[i] = Box{};
	}

	for (int i = 0; i < MaxPieceCount; i++) {
		pieces[i] = Piece{};
	}

	boxCount = firstBoxCountDist(gen);
	pieceCount = 0;

	for (int i = 0; i < boxCount; i++) {
		MakeBox(boxes[i]);
	}
}

void DrawBox(const Box& box)
{
	if (!box.active)
		return;

	float halfSize = box.size / 2.0f;
	glColor3f(box.R, box.G, box.B);

	glRectf(
		box.X - halfSize,
		box.Y - halfSize,
		box.X + halfSize,
		box.Y + halfSize
	);
}

void DrawPiece(const Piece& piece)
{
	if (!piece.active)
		return;

	float halfSize = piece.size / 2.0f;
	glColor3f(piece.R, piece.G, piece.B);

	glRectf(
		piece.X - halfSize,
		piece.Y - halfSize,
		piece.X + halfSize,
		piece.Y + halfSize
	);
}

bool IsMouseInsideBox(float mouseX, float mouseY, const Box& box)
{
	float halfSize = box.size / 2.0f;

	bool insideX =
		mouseX >= box.X - halfSize &&
		mouseX <= box.X + halfSize;

	bool insideY =
		mouseY >= box.Y - halfSize &&
		mouseY <= box.Y + halfSize;

	return insideX && insideY;
}

void AddPiece(
	const Box& original,
	float startX,
	float startY,
	float pieceSize,
	float directionX,
	float directionY,
	float moveSpeed,
	float colorDirection)
{
	if (pieceCount >= MaxPieceCount)
		return;

	Piece& piece = pieces[pieceCount];

	piece.X = startX;
	piece.Y = startY;
	piece.size = pieceSize;

	piece.R = original.R;
	piece.G = original.G;
	piece.B = original.B;

	piece.directionX = directionX;
	piece.directionY = directionY;
	piece.moveSpeed = moveSpeed;
	piece.shrinkSpeed = shrinkSpeedDist(gen);
	piece.colorDirection = colorDirection;
	piece.active = true;

	pieceCount++;
}

// 클릭한 사각형을 무작위 애니메이션으로 분열시킨다.
void SplitBox(int boxIndex)
{
	Box original = boxes[boxIndex];
	boxes[boxIndex].active = false;

	int animation = animationDist(gen);
	float moveSpeed = moveSpeedDist(gen);

	// 1이면 밝아지고, -1이면 어두워진다.
	float colorDirection =
		colorDirectionDist(gen) == 0 ? -1.0f : 1.0f;

	if (animation == 4) {
		// 네 번째 애니메이션은 8방향으로 8개가 퍼진다.
		float pieceSize = original.size / 3.0f;
		float startDistance = pieceSize * 0.5f;

		for (int i = 0; i < 8; i++) {
			AddPiece(
				original,
				original.X + EightDirectionX[i] * startDistance,
				original.Y + EightDirectionY[i] * startDistance,
				pieceSize,
				EightDirectionX[i],
				EightDirectionY[i],
				moveSpeed,
				colorDirection
			);
		}

		return;
	}

	// 1~3번 애니메이션은 사각형을 4개로 나눈다.
	float pieceSize = original.size / 2.0f;
	float offset = pieceSize / 2.0f;

	float startX[4] = {
		original.X - offset,
		original.X + offset,
		original.X - offset,
		original.X + offset
	};

	float startY[4] = {
		original.Y + offset,
		original.Y + offset,
		original.Y - offset,
		original.Y - offset
	};

	float directionX[4];
	float directionY[4];

	if (animation == 1) {
		// 왼쪽, 오른쪽, 위쪽, 아래쪽으로 이동한다.
		directionX[0] = -1.0f;
		directionY[0] = 0.0f;

		directionX[1] = 1.0f;
		directionY[1] = 0.0f;

		directionX[2] = 0.0f;
		directionY[2] = 1.0f;

		directionX[3] = 0.0f;
		directionY[3] = -1.0f;
	}
	else if (animation == 2) {
		// 네 개의 대각선 방향으로 이동한다.
		directionX[0] = -DiagonalValue;
		directionY[0] = DiagonalValue;

		directionX[1] = DiagonalValue;
		directionY[1] = DiagonalValue;

		directionX[2] = -DiagonalValue;
		directionY[2] = -DiagonalValue;

		directionX[3] = DiagonalValue;
		directionY[3] = -DiagonalValue;
	}
	else {
		// 네 조각이 무작위로 선택한 한 방향으로 함께 이동한다.
		int selectedDirection = directionDist(gen);

		for (int i = 0; i < 4; i++) {
			directionX[i] = EightDirectionX[selectedDirection];
			directionY[i] = EightDirectionY[selectedDirection];
		}
	}

	for (int i = 0; i < 4; i++) {
		AddPiece(
			original,
			startX[i],
			startY[i],
			pieceSize,
			directionX[i],
			directionY[i],
			moveSpeed,
			colorDirection
		);
	}
}

void UpdatePieces()
{
	constexpr float colorChangeSpeed = 0.002f;
	constexpr float disappearSize = 0.01f;

	for (int i = 0; i < pieceCount; i++) {
		Piece& piece = pieces[i];

		if (!piece.active)
			continue;

		// 분열될 때 정해진 방향으로 이동한다.
		piece.X += piece.directionX * piece.moveSpeed;
		piece.Y += piece.directionY * piece.moveSpeed;

		// 이동하면서 크기가 점점 작아진다.
		piece.size -= piece.shrinkSpeed;

		// 원래 색상에서 흰색 또는 검정색 방향으로 변한다.
		piece.R = std::clamp(
			piece.R + colorChangeSpeed * piece.colorDirection,
			0.0f,
			1.0f
		);

		piece.G = std::clamp(
			piece.G + colorChangeSpeed * piece.colorDirection,
			0.0f,
			1.0f
		);

		piece.B = std::clamp(
			piece.B + colorChangeSpeed * piece.colorDirection,
			0.0f,
			1.0f
		);

		// 정해진 크기보다 작아지면 완전히 사라진다.
		if (piece.size <= disappearSize) {
			piece.active = false;
		}
	}
}

void MouseButtonCallback(
	GLFWwindow* window,
	int button,
	int action,
	int mods)
{
	if (
		button != GLFW_MOUSE_BUTTON_LEFT ||
		action != GLFW_PRESS)
	{
		return;
	}

	double mouseX;
	double mouseY;
	glfwGetCursorPos(window, &mouseX, &mouseY);

	float x;
	float y;
	GetOpenGLMousePosition(window, mouseX, mouseY, x, y);

	// 나중에 그린 사각형부터 검사한다.
	for (int i = boxCount - 1; i >= 0; i--) {
		if (!boxes[i].active)
			continue;

		if (IsMouseInsideBox(x, y, boxes[i])) {
			SplitBox(i);
			return;
		}
	}
}

int main()
{
	if (!glfwInit()) {
		std::cerr << "GLFW initialization failed." << std::endl;
		return -1;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);

	GLFWwindow* window = glfwCreateWindow(
		800,
		600,
		"Practice 6 - Splitting Boxes",
		nullptr,
		nullptr
	);

	if (!window) {
		std::cerr << "Window creation failed." << std::endl;
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);
	glfwSetMouseButtonCallback(window, MouseButtonCallback);

	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK) {
		std::cerr << "GLEW initialization failed." << std::endl;
		glfwDestroyWindow(window);
		glfwTerminate();
		return -1;
	}

	glViewport(0, 0, 800, 600);
	ResetScene();

	while (!glfwWindowShouldClose(window)) {
		if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
			glfwSetWindowShouldClose(window, true);
		}

		bool currentR =
			glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS;

		if (currentR && !pressR) {
			ResetScene();
		}
		pressR = currentR;

		UpdatePieces();

		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		for (int i = 0; i < boxCount; i++) {
			DrawBox(boxes[i]);
		}

		for (int i = 0; i < pieceCount; i++) {
			DrawPiece(pieces[i]);
		}

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}
