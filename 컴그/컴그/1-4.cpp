#include <GL/glew.h>
#include <GL/glfw3.h>
#include <iostream>
#include <random>

std::random_device rd; //난수 시드 생성
std::mt19937 gen(rd()); //난수 생성 엔진

std::uniform_real_distribution<float> colorDist(0.0f, 1.0f);

std::uniform_real_distribution<float> springDist(-1.0f, 1.0f);

float OriginSize = 0.3f;
float BoxSize = 0.1f;

struct Box {
	float size;
	
	float R;
	float G;
	float B;

	float X;
	float Y;

	float sizeDirection;
	float HorizontalDirection;

	float OriginX;
	float OriginY;

	float boingX;
	float boingY;

	int MoveDirection;
};
Box box[5];
int BoxCount=0;

bool ColorA = false;
bool SizeA = false;
bool Animation = false;
bool MoveHorizontalA = false;
bool MoveClockA = false;
bool DiagonalA = false;

bool Press1 = false;
bool Press2 = false;
bool Press3 = false;
bool Press4 = false;
bool Press5 = false;

void MakeRandomDirection(Box& box);

void MakeBox(Box&box, float mouseX, float mouseY) {

	box.R = colorDist(gen);
	box.G = colorDist(gen);
	box.B = colorDist(gen);

	box.X = mouseX;
	box.Y = mouseY;

	box.OriginX = mouseX;
	box.OriginY = mouseY;

	box.size = OriginSize;

	box.sizeDirection = 1.0f;
	box.HorizontalDirection = 1.0f;

	box.MoveDirection = 0;

	MakeRandomDirection(box);
}

void DrawBox(Box& box) {
	glColor3f(box.R, box.G, box.B);

	glRectf(
		box.X - box.size / 2.0f,
		box.Y - box.size / 2.0f,
		box.X + box.size / 2.0f,
		box.Y + box.size / 2.0f
	);
}

// 마우스 위치를 opengl값으로 계산해 주는 함수
void GetOpenGLMousePosition(
	GLFWwindow* window,
	double mouseX,
	double mouseY,
	float& openGLX,
	float& openGLY)
{
	int width;
	int height;

	glfwGetWindowSize(
		window,
		&width,
		&height
	);

	openGLX =
		static_cast<float>(mouseX / width)
		* 2.0f - 1.0f;

	openGLY =
		1.0f
		- static_cast<float>(mouseY / height)
		* 2.0f;
}
//마우스 클릭
void MouseButtonCallback(
	GLFWwindow* window,
	int button,
	int action,
	int mods)
{
	if (button != GLFW_MOUSE_BUTTON_LEFT)
		return;

	if (action == GLFW_PRESS) {
		double mouseX;
		double mouseY;

		glfwGetCursorPos(
			window,
			&mouseX,
			&mouseY
		);

		float x;
		float y;

		GetOpenGLMousePosition(
			window,
			mouseX,
			mouseY,
			x,
			y
		);
		if (BoxCount < 5) {
			MakeBox(box[BoxCount], x, y);
			BoxCount++;
		}
	}
}

// 초기화
void RemoveBox()
{
	
	for (int i = 0; i < BoxCount; i++) {
		box[i] = Box{};
	}
	BoxCount=0;

	// 해당 그룹에 조각이 하나도 남지 않은 경우

}

// 5번 랜덤 색
void RandomColor(Box& box) {

	box.R = colorDist(gen);
	box.G = colorDist(gen);
	box.B = colorDist(gen);
}

// 4 크기변화
void ChangeSize(Box& box) {
	// 프레임마다 변경할 크기
	float sizeSpeed = 0.003f;

	// 현재 방향으로 크기 변경
	box.size += sizeSpeed * box.sizeDirection;

	// 최대 크기에 도착하면 작아지는 방향으로 변경
	if (box.size >= 0.5f) {
		box.size = 0.5f;
		box.sizeDirection = -1.0f;
	}

	// 최소 크기에 도착하면 커지는 방향으로 변경
	else if (box.size <= 0.1f) {
		box.size = 0.1f;
		box.sizeDirection = 1.0f;
	}
}

// 2번: 좌우로 반복 이동.
void MoveHorizontal(Box& box)
{
	float moveSpeed = 0.003f;
	float downDistance = 0.03f;
	float halfSize = box.size / 2.0f;
	float floorY = -1.0f + halfSize;

	// 크기가 바뀌어 바닥 아래로 내려갔다면 바닥 위치로 맞춤.
	if (box.Y < floorY)
		box.Y = floorY;

	// HorizontalDirection이 1이면 오른쪽, -1이면 왼쪽으로 이동.
	box.X += moveSpeed * box.HorizontalDirection;

	// 오른쪽 벽에 도착하면 조금 내려간 뒤 왼쪽으로 방향 변경.
	if (box.X + halfSize >= 1.0f) {
		box.X = 1.0f - halfSize;

		if (box.Y > floorY) {
			box.Y -= downDistance;

			if (box.Y < floorY)
				box.Y = floorY;
		}

		box.HorizontalDirection = -1.0f;
	}

	// 왼쪽 벽에 도착하면 조금 내려간 뒤 오른쪽으로 방향 변경.
	else if (box.X - halfSize <= -1.0f) {
		box.X = -1.0f + halfSize;

		if (box.Y > floorY) {
			box.Y -= downDistance;

			if (box.Y < floorY)
				box.Y = floorY;
		}

		box.HorizontalDirection = 1.0f;
	}
}

// 3번 시계방향
void MoveClockWay(Box& box)
{
	float moveSpeed = 0.003f;
	float halfSize = box.size / 2.0f;

	// 오른쪽으로 이동
	if (box.MoveDirection == 0) {
		box.X += moveSpeed;

		// 오른쪽 벽에 도착
		if (box.X + halfSize >= 1.0f) {
			box.X = 1.0f - halfSize;

			// 다음 방향은 아래
			box.MoveDirection = 1;
		}
	}

	// 아래로 이동
	else if (box.MoveDirection == 1) {
		box.Y -= moveSpeed;

		// 아래쪽 벽에 도착
		if (box.Y - halfSize <= -1.0f) {
			box.Y = -1.0f + halfSize;

			// 다음 방향은 왼쪽
			box.MoveDirection = 2;
		}
	}

	// 왼쪽으로 이동
	else if (box.MoveDirection == 2) {
		box.X -= moveSpeed;

		// 왼쪽 벽에 도착
		if (box.X - halfSize <= -1.0f) {
			box.X = -1.0f + halfSize;

			// 다음 방향은 위
			box.MoveDirection = 3;
		}
	}

	// 위로 이동
	else if (box.MoveDirection == 3) {
		box.Y += moveSpeed;

		// 위쪽 벽에 도착
		if (box.Y + halfSize >= 1.0f) {
			box.Y = 1.0f - halfSize;

			// 다음 방향은 오른쪽
			box.MoveDirection = 0;
		}
	}
}

void MoveDiagonal(Box& box)
{
	float moveSpeed = 0.003f;
	float halfSize = box.size / 2.0f;

	// 랜덤 방향으로 이동
	box.X += box.boingX * moveSpeed;
	box.Y += box.boingY * moveSpeed;

	bool hitRight =
		box.X + halfSize >= 1.0f;

	bool hitLeft =
		box.X - halfSize <= -1.0f;

	bool hitTop =
		box.Y + halfSize >= 1.0f;

	bool hitBottom =
		box.Y - halfSize <= -1.0f;

	// 어느 벽이든 부딪히면 새로운 랜덤 방향 생성
	if (hitRight || hitLeft || hitTop || hitBottom) {

		// 벽 밖으로 나간 위치를 바로잡음
		if (hitRight)
			box.X = 1.0f - halfSize;

		if (hitLeft)
			box.X = -1.0f + halfSize;

		if (hitTop)
			box.Y = 1.0f - halfSize;

		if (hitBottom)
			box.Y = -1.0f + halfSize;

		// 새로운 랜덤 방향
		MakeRandomDirection(box);

		// 부딪힌 벽의 안쪽을 향하도록 방향을 보정
		if (hitRight && box.boingX > 0.0f)
			box.boingX = -box.boingX;

		if (hitLeft && box.boingX < 0.0f)
			box.boingX = -box.boingX;

		if (hitTop && box.boingY > 0.0f)
			box.boingY = -box.boingY;

		if (hitBottom && box.boingY < 0.0f)
			box.boingY = -box.boingY;
	}
}
void MakeRandomDirection(Box& box)
{
	box.boingX = springDist(gen);
	box.boingY = springDist(gen);
}
void MoveOriginLocate(Box& box) {
	box.X = box.OriginX;
	box.Y = box.OriginY;
}

int main()
{
	//--- GLFW 초기화
	if (!glfwInit()) {
		std::cerr << "GLFW 초기화 실패!" << std::endl;
		return -1;
	}

	//--- OpenGL 버전 설정
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

	//--- glBegin, glRectf 등을 사용하기 위한 호환 프로필
	glfwWindowHint(
		GLFW_OPENGL_PROFILE,
		GLFW_OPENGL_COMPAT_PROFILE
	);

	//--- 윈도우 생성
	GLFWwindow* window = glfwCreateWindow(
		800,
		600,
		"OpenGL Animation",
		nullptr,
		nullptr
	);

	if (!window) {
		std::cerr << "윈도우 생성 실패!" << std::endl;
		glfwTerminate();
		return -1;
	}

	//--- OpenGL 컨텍스트 설정
	glfwMakeContextCurrent(window);

	glfwSetMouseButtonCallback(
		window,
		MouseButtonCallback
	);


	//--- GLEW 초기화
	glewExperimental = GL_TRUE;

	if (glewInit() != GLEW_OK) {
		std::cerr << "GLEW 초기화 실패!" << std::endl;

		glfwDestroyWindow(window);
		glfwTerminate();

		return -1;
	}

	//--- 뷰포트 설정
	glViewport(0, 0, 800, 600);

	//--- 메인 반복문
	while (!glfwWindowShouldClose(window)) {

		//--- q키를 누르면 프로그램 종료
		if (
			glfwGetKey(window, GLFW_KEY_Q)
			== GLFW_PRESS)
		{
			glfwSetWindowShouldClose(window, true);
		}

		else if (
			glfwGetKey(window, GLFW_KEY_R)
			== GLFW_PRESS) {
			RemoveBox();
		}

		else if (
			glfwGetKey(window, GLFW_KEY_S)
			== GLFW_PRESS) {
			Animation = false;
		}

		else if (
			glfwGetKey(window, GLFW_KEY_M)
			== GLFW_PRESS) {
			Animation = false;
			for (int i = 0; i < BoxCount; i++) {
				box[i].MoveDirection = 0;
				box[i].HorizontalDirection = 1.0f;
				MoveOriginLocate(box[i]);
			}
		}
		//1번 키를 누름
		bool CurrentPress1 = (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS);
		if (CurrentPress1 && !Press1) {
			// true이면 false, false이면 true.
			Animation = true;
			DiagonalA = !DiagonalA;
		}
		Press1 = CurrentPress1;
		if (Animation && DiagonalA) {
			for (int i = 0; i < BoxCount; i++) {
				MoveDiagonal(box[i]);
			}
		}


		// 2번 키를 누름.
		bool CurrentPress2 = (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS);
		if (CurrentPress2 && !Press2) {
			// true이면 false, false이면 true.
			Animation = true;
			MoveHorizontalA = !MoveHorizontalA;
		}
		Press2 = CurrentPress2;
		if (Animation && MoveHorizontalA) {
			for (int i = 0; i < BoxCount; i++) {
				MoveHorizontal(box[i]);
			}
		}

		// 3번 키를 누름
		bool CurrentPress3 = (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS);
		if (CurrentPress3 && !Press3) {
			// true이면 false, false이면 true
			Animation = true;
			MoveClockA = !MoveClockA;
		}
		Press3 = CurrentPress3;
		if (Animation && MoveClockA) {
			for (int i = 0; i < BoxCount; i++) {
				MoveClockWay(box[i]);
			}
		}

		 
		// 4번 키를 누름
		bool CurrentPress4 = (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS);
		if (CurrentPress4 && !Press4) {
			// true이면 false, false이면 true
			Animation = true;
			SizeA = !SizeA;

		}
		Press4 = CurrentPress4;
		if (Animation&&SizeA) {
			for (int i = 0; i < BoxCount; i++) {
				ChangeSize(box[i]);
			}
		}
		// 5번 키를 새롭게 누른 순간
		bool CurrentPress5 = (glfwGetKey(window, GLFW_KEY_5) == GLFW_PRESS);
		if (CurrentPress5 && !Press5) {
			// true이면 false, false이면 true
			Animation = true;
			ColorA = !ColorA;
		}

		// 현재 키 상태를 다음 반복에서 사용
		Press5 = CurrentPress5;

		// 색상 변경이 켜져 있으면
		if (Animation&& ColorA) {
			for (int i = 0; i < BoxCount; i++) {
				RandomColor(box[i]);
			}
		}


		//--- 배경색을 짙은 회색으로 설정
		glClearColor(
			0.2f,
			0.2f,
			0.2f,
			1.0f
		);

		//--- 화면 지우기
		glClear(GL_COLOR_BUFFER_BIT);

		// 이 위치에 사각형 그리기와
		// 애니메이션 코드를 작성합니다.
		for (int i = 0; i < BoxCount; i++) {
			DrawBox(box[i]);
		}

		//--- 버퍼 교체
		glfwSwapBuffers(window);

		//--- 입력 이벤트 처리
		glfwPollEvents();
	}

	//--- 종료 처리
	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}
