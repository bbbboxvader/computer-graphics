#include <GL/glew.h>
#include <GL/glfw3.h>
#include <algorithm>
#include <iostream>
#include <random>

std::random_device rd;
std::mt19937 gen(rd());

constexpr int MaxBoxCount = 50;
constexpr int MaxNewBoxCount = 10;
constexpr float SmallBoxSize = 0.08f;
constexpr float OriginalEraserSize = SmallBoxSize * 2.0f;
constexpr float MinimumEraserSize = SmallBoxSize / 10.0f;
constexpr float EraserShrinkStep =
	(OriginalEraserSize - MinimumEraserSize) / MaxNewBoxCount;

struct Box {
	float X;
	float Y;
	float size;

	float R;
	float G;
	float B;

	bool visible;
};

Box boxes[MaxBoxCount];
int boxCount = 0;
int newBoxCount = 0;

bool erasing = false;
bool pressR = false;

float eraserX = 0.0f;
float eraserY = 0.0f;
float eraserBaseSize = OriginalEraserSize;
float eraserSize = OriginalEraserSize;
float eraserR = 0.0f;
float eraserG = 0.0f;
float eraserB = 0.0f;

std::uniform_real_distribution<float> colorDist(0.0f, 1.0f);
std::uniform_real_distribution<float> positionDist(
	-1.0f + SmallBoxSize / 2.0f,
	1.0f - SmallBoxSize / 2.0f
);
std::uniform_int_distribution<int> firstBoxCountDist(20, 40);

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

// 사각형 하나를 지정한 위치에 만든다.
void MakeBox(Box& box, float x, float y)
{
	box.X = x;
	box.Y = y;
	box.size = SmallBoxSize;

	box.R = colorDist(gen);
	box.G = colorDist(gen);
	box.B = colorDist(gen);

	box.visible = true;
}

// 처음 시작할 때 20~40개의 사각형을 만든다.
void ResetScene()
{
	for (int i = 0; i < MaxBoxCount; i++) {
		boxes[i] = Box{};
	}

	boxCount = firstBoxCountDist(gen);
	newBoxCount = 0;
	erasing = false;

	eraserBaseSize = OriginalEraserSize;
	eraserSize = eraserBaseSize;
	eraserR = 0.0f;
	eraserG = 0.0f;
	eraserB = 0.0f;

	for (int i = 0; i < boxCount; i++) {
		MakeBox(
			boxes[i],
			positionDist(gen),
			positionDist(gen)
		);
	}
}

void DrawBox(const Box& box)
{
	if (!box.visible)
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

void DrawEraser()
{
	if (!erasing)
		return;

	float halfSize = eraserSize / 2.0f;
	glColor3f(eraserR, eraserG, eraserB);

	glRectf(
		eraserX - halfSize,
		eraserY - halfSize,
		eraserX + halfSize,
		eraserY + halfSize
	);
}

// 두 사각형이 서로 겹치는지 검사한다.
bool IsOverlapping(
	float firstX,
	float firstY,
	float firstSize,
	float secondX,
	float secondY,
	float secondSize)
{
	float firstHalf = firstSize / 2.0f;
	float secondHalf = secondSize / 2.0f;

	bool insideX =
		firstX + firstHalf >= secondX - secondHalf &&
		firstX - firstHalf <= secondX + secondHalf;

	bool insideY =
		firstY + firstHalf >= secondY - secondHalf &&
		firstY - firstHalf <= secondY + secondHalf;

	return insideX && insideY;
}

// 지우개와 겹친 사각형을 숨긴다.
void EraseTouchedBoxes()
{
	for (int i = 0; i < boxCount; i++) {
		if (!boxes[i].visible)
			continue;

		if (IsOverlapping(
			eraserX,
			eraserY,
			eraserSize,
			boxes[i].X,
			boxes[i].Y,
			boxes[i].size))
		{
			boxes[i].visible = false;

			// 사각형을 지울 때마다 지우개를 조금 크게 만든다.
			eraserSize += SmallBoxSize * 0.25f;
			eraserSize = std::min(eraserSize, 2.0f);

			// 지운 사각형의 색상으로 지우개 색상을 변경한다.
			eraserR = boxes[i].R;
			eraserG = boxes[i].G;
			eraserB = boxes[i].B;
		}
	}
}

// 숨겼던 사각형을 원래 위치에 다시 나타나게 한다.
void RestoreErasedBoxes()
{
	for (int i = 0; i < boxCount; i++) {
		boxes[i].visible = true;
	}
}

void MouseButtonCallback(
	GLFWwindow* window,
	int button,
	int action,
	int mods)
{
	if (action != GLFW_PRESS && action != GLFW_RELEASE)
		return;

	double mouseX;
	double mouseY;
	glfwGetCursorPos(window, &mouseX, &mouseY);

	float x;
	float y;
	GetOpenGLMousePosition(window, mouseX, mouseY, x, y);

	// 왼쪽 버튼을 누르면 검정색 지우개를 만든다.
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		RestoreErasedBoxes();

		erasing = true;
		eraserX = x;
		eraserY = y;
		eraserSize = eraserBaseSize;
		eraserR = 0.0f;
		eraserG = 0.0f;
		eraserB = 0.0f;

		EraseTouchedBoxes();
	}

	// 왼쪽 버튼을 놓으면 지우개를 없애고 사각형을 복구한다.
	else if (
		button == GLFW_MOUSE_BUTTON_LEFT &&
		action == GLFW_RELEASE)
	{
		erasing = false;
		RestoreErasedBoxes();
	}

	// 오른쪽 버튼을 누르면 그 위치에 새 사각형을 만든다.
	else if (
		button == GLFW_MOUSE_BUTTON_RIGHT &&
		action == GLFW_PRESS)
	{
		if (
			boxCount < MaxBoxCount &&
			newBoxCount < MaxNewBoxCount)
		{
			MakeBox(boxes[boxCount], x, y);
			boxCount++;
			newBoxCount++;

			// 새 사각형이 생길 때마다 기본 지우개 크기를 줄인다.
			eraserBaseSize -= EraserShrinkStep;
			eraserBaseSize = std::max(
				eraserBaseSize,
				MinimumEraserSize
			);

			// 지우개가 보이는 중이라면 현재 크기도 바로 줄인다.
			if (erasing) {
				eraserSize -= EraserShrinkStep;
				eraserSize = std::max(
					eraserSize,
					MinimumEraserSize
				);
			}
			else {
				eraserSize = eraserBaseSize;
			}
		}
	}
}

void CursorPositionCallback(
	GLFWwindow* window,
	double mouseX,
	double mouseY)
{
	if (!erasing)
		return;

	GetOpenGLMousePosition(
		window,
		mouseX,
		mouseY,
		eraserX,
		eraserY
	);

	EraseTouchedBoxes();
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
		"Practice 5 - Eraser",
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
	glfwSetCursorPosCallback(window, CursorPositionCallback);

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

		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		for (int i = 0; i < boxCount; i++) {
			DrawBox(boxes[i]);
		}

		DrawEraser();

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}
