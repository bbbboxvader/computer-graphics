#include <GL/glew.h>
#include <GL/glfw3.h>
#include <iostream>
#include <random>

// 사각형 (크기, 색, 위치, 최대 10개, 먼저 나온것인가)
struct Box {
	float R;
	float G;
	float B;

	float Xbox;
	float Ybox;

	float size;

	bool first;
};

//전역변수

// 첫 삼각형 크기는 고정
Box box[10][10];
constexpr float boxSize = 0.2f;

// A키로 만들어진 사각형 그룹 개수
int count = 0;

// A키로 만든 누적 사각형 개수
int aCreatedCount = 0;

// 화면에 존재하는 전체 사각형 개수
int totalBoxCount = 0;

// 선택한 원본 사각형 번호
int selectbox = -1;

// 선택한 조각 번호
int selectpiece = -1;

bool pressAbotten = false;
bool dragging = false;

// 마우스가 사각형의 어느 위치를 잡았는가를 판별하기 위한 값
float dragOffsetX = 0.0f;
float dragOffsetY = 0.0f;

// 각 원본 사각형이 가지고 있는 조각 개수
int pieceCount[10] = { 0 };

std::random_device rd; //난수 시드 생성
std::mt19937 gen(rd()); //난수 생성 엔진

// 색상: 0.0 ~ 1.0
std::uniform_real_distribution<float> colorDist(0.0f, 1.0f);

// 위치: OpenGL 좌표 범위 -1.0 ~ 1.0
std::uniform_real_distribution<float> positionDist(
	-1.0f,
	1.0f - boxSize
);

// 사각형 만들기 (크기 색 위치 랜덤값으로)
//그리기 전 만드는 단계
void MakeBox(Box& box) {

	//색상
	box.R = colorDist(gen);
	box.G = colorDist(gen);
	box.B = colorDist(gen);

	//위치 x y
	box.Xbox = positionDist(gen);
	box.Ybox = positionDist(gen);

	box.size = boxSize;
	box.first = false;
}

//사각형 화면 출력
void DrawBox(const Box& box) {
	glColor3f(box.R, box.G, box.B);

	glRectf(
		box.Xbox,
		box.Ybox,
		box.Xbox + box.size,
		box.Ybox + box.size
	);
}

//겹쳤는지 확인 함수
bool IsOverlapping(
	const Box& firstBox,
	const Box& secondBox)
{
	bool overlapX =
		firstBox.Xbox < secondBox.Xbox + secondBox.size &&
		firstBox.Xbox + firstBox.size > secondBox.Xbox;

	bool overlapY =
		firstBox.Ybox < secondBox.Ybox + secondBox.size &&
		firstBox.Ybox + firstBox.size > secondBox.Ybox;

	return overlapX && overlapY;
}

//사각형 제거 함수
void RemoveBox(
	int removeBoxIndex,
	int removePieceIndex)
{
	// 제거한 조각 뒤의 조각들을 앞으로 이동
	for (
		int j = removePieceIndex;
		j < pieceCount[removeBoxIndex] - 1;
		j++)
	{
		box[removeBoxIndex][j] =
			box[removeBoxIndex][j + 1];
	}

	pieceCount[removeBoxIndex]--;
	totalBoxCount--;

	// 해당 그룹에 조각이 하나도 남지 않은 경우
	if (pieceCount[removeBoxIndex] == 0) {

		// 뒤에 있는 그룹들을 앞으로 이동
		for (
			int i = removeBoxIndex;
			i < count - 1;
			i++)
		{
			for (int j = 0; j < 10; j++) {
				box[i][j] = box[i + 1][j];
			}

			pieceCount[i] =
				pieceCount[i + 1];
		}

		pieceCount[count - 1] = 0;
		count--;
	}
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

// 좌클릭
void MouseLButtonCallback(
	GLFWwindow* window,
	int action,
	int mods)
{
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

		selectbox = -1;
		selectpiece = -1;
		dragging = false;

		// 최근에 만든 사각형부터 검사
		for (int i = count - 1; i >= 0; i--) {
			for (
				int j = pieceCount[i] - 1;
				j >= 0;
				j--)
			{
				bool insideX =
					x >= box[i][j].Xbox &&
					x <= box[i][j].Xbox
					+ box[i][j].size;

				bool insideY =
					y >= box[i][j].Ybox &&
					y <= box[i][j].Ybox
					+ box[i][j].size;

				if (insideX && insideY) {
					selectbox = i;
					selectpiece = j;
					dragging = true;

					// 클릭한 지점을 그대로 유지하기 위한 거리
					dragOffsetX =
						x - box[i][j].Xbox;

					dragOffsetY =
						y - box[i][j].Ybox;

					return;
				}
			}
		}
	}

	// 왼쪽 버튼을 놓으면 드래그 종료
	if (action == GLFW_RELEASE) {
		if (
			dragging &&
			selectbox != -1 &&
			selectpiece != -1)
		{
			int draggedBox = selectbox;
			int draggedPiece = selectpiece;

			// 드래그한 사각형과 겹친 사각형 검사
			for (int i = 0; i < count; i++) {
				for (
					int j = 0;
					j < pieceCount[i];
					j++)
				{
					// 자기 자신과는 겹침 검사하지 않음
					if (
						i == draggedBox &&
						j == draggedPiece)
					{
						continue;
					}

					if (IsOverlapping(
						box[draggedBox][draggedPiece],
						box[i][j]))
					{
						// 상대 사각형 크기를 2배로 만듦
						box[i][j].size *= 2.0f;

						// 드래그했던 사각형 제거
						RemoveBox(
							draggedBox,
							draggedPiece
						);

						std::cout
							<< "사각형 합치기 완료\n";

						std::cout
							<< "현재 전체 사각형 개수: "
							<< totalBoxCount
							<< '\n';

						dragging = false;
						selectbox = -1;
						selectpiece = -1;

						return;
					}
				}
			}
		}

		dragging = false;
		selectbox = -1;
		selectpiece = -1;
	}
}

// 우클릭
void MouseRButtonCallback(
	GLFWwindow* window,
	int action,
	int mods)
{
	// 오른쪽 버튼을 누른 순간만 처리
	if (action != GLFW_PRESS) {
		return;
	}

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

	// 최근에 만든 사각형부터 검사
	for (int i = count - 1; i >= 0; i--) {
		for (
			int j = pieceCount[i] - 1;
			j >= 0;
			j--)
		{
			bool insideX =
				x >= box[i][j].Xbox &&
				x <= box[i][j].Xbox
				+ box[i][j].size;

			bool insideY =
				y >= box[i][j].Ybox &&
				y <= box[i][j].Ybox
				+ box[i][j].size;

			if (insideX && insideY) {

				// 전체 사각형은 최대 20개
				if (totalBoxCount >= 20) {
					std::cout
						<< "사각형은 최대 20개입니다.\n";

					return;
				}

				// 한 그룹에는 최대 10개 조각 저장
				if (pieceCount[i] >= 10) {
					std::cout
						<< "이 사각형은 더 이상 "
						<< "분리할 수 없습니다.\n";

					return;
				}

				// 분리할 사각형 정보 복사
				Box original = box[i][j];

				// 분리된 사각형 크기
				float halfSize =
					original.size / 2.0f;

				// 기존 사각형을 왼쪽 조각으로 변경
				box[i][j].Xbox =
					original.Xbox;

				box[i][j].Ybox =
					original.Ybox;

				box[i][j].size =
					halfSize;

				// 새로운 오른쪽 조각 번호
				int newPieceIndex =
					pieceCount[i];

				// 원본 사각형의 색상과 정보를 복사
				box[i][newPieceIndex] =
					original;

				// 오른쪽에 배치
				box[i][newPieceIndex].Xbox =
					original.Xbox + halfSize;

				box[i][newPieceIndex].Ybox =
					original.Ybox;

				box[i][newPieceIndex].size =
					halfSize;

				pieceCount[i]++;
				totalBoxCount++;

				std::cout
					<< "사각형 분리 완료\n";

				std::cout
					<< "현재 전체 사각형 개수: "
					<< totalBoxCount
					<< '\n';

				return;
			}
		}
	}
}

// 마우스 클릭
void MouseButtonCallback(
	GLFWwindow* window,
	int button,
	int action,
	int mods)
{
	if (
		button != GLFW_MOUSE_BUTTON_LEFT &&
		button != GLFW_MOUSE_BUTTON_RIGHT)
	{
		return;
	}

	if (button == GLFW_MOUSE_BUTTON_LEFT) {
		MouseLButtonCallback(
			window,
			action,
			mods
		);
	}
	else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
		MouseRButtonCallback(
			window,
			action,
			mods
		);
	}
}

// 왼쪽 드래그
void CursorPositionCallback(
	GLFWwindow* window,
	double mouseX,
	double mouseY)
{
	if (
		!dragging ||
		selectbox == -1 ||
		selectpiece == -1)
	{
		return;
	}

	float x;
	float y;

	float currentSize =
		box[selectbox][selectpiece].size;

	GetOpenGLMousePosition(
		window,
		mouseX,
		mouseY,
		x,
		y
	);

	float newX =
		x - dragOffsetX;

	float newY =
		y - dragOffsetY;

	// 왼쪽 화면 밖으로 나가지 않도록 제한
	if (newX < -1.0f)
		newX = -1.0f;

	// 오른쪽 화면 밖으로 나가지 않도록 제한
	if (newX > 1.0f - currentSize)
		newX = 1.0f - currentSize;

	// 아래쪽 화면 밖으로 나가지 않도록 제한
	if (newY < -1.0f)
		newY = -1.0f;

	// 위쪽 화면 밖으로 나가지 않도록 제한
	if (newY > 1.0f - currentSize)
		newY = 1.0f - currentSize;

	box[selectbox][selectpiece].Xbox =
		newX;

	box[selectbox][selectpiece].Ybox =
		newY;
}

int main() {
	//--- GLFW 초기화
	if (!glfwInit()) {
		std::cerr
			<< "GLFW 초기화 실패!"
			<< std::endl;

		return -1;
	}

	//--- OpenGL 버전 설정(예: 3.3 Core Profile)
	glfwWindowHint(
		GLFW_CONTEXT_VERSION_MAJOR,
		3
	);

	glfwWindowHint(
		GLFW_CONTEXT_VERSION_MINOR,
		3
	);

	glfwWindowHint(
		GLFW_OPENGL_PROFILE,
		GLFW_OPENGL_COMPAT_PROFILE
	); //COMPAT으로 해야 도형 출력 가능

	//--- 윈도우생성
	GLFWwindow* window =
		glfwCreateWindow(
			800,
			600,
			"OpenGL Window",
			nullptr,
			nullptr
		);

	if (!window) {
		std::cerr
			<< "윈도우생성실패!"
			<< std::endl;

		glfwTerminate();

		return -1;
	}

	//--- 컨텍스트설정
	glfwMakeContextCurrent(window);

	// 마우스 버튼 콜백 등록
	glfwSetMouseButtonCallback(
		window,
		MouseButtonCallback
	);

	// 현재 위치 콜백
	glfwSetCursorPosCallback(
		window,
		CursorPositionCallback
	);

	//--- GLEW 초기화
	glewExperimental = GL_TRUE; // 최신 기능 사용

	if (glewInit() != GLEW_OK) {
		std::cerr
			<< "GLEW 초기화 실패!"
			<< std::endl;

		glfwDestroyWindow(window);
		glfwTerminate();

		return -1;
	}

	//--- 뷰포트설정
	glViewport(0, 0, 800, 600);

	//--- 메인 루프

	// 입력처리
	while (!glfwWindowShouldClose(window)) {

		//esc
		if (
			glfwGetKey(
				window,
				GLFW_KEY_ESCAPE
			) == GLFW_PRESS)
		{
			glfwSetWindowShouldClose(
				window,
				true
			);
		}

		// A
		bool currentA =
			glfwGetKey(
				window,
				GLFW_KEY_A
			) == GLFW_PRESS;

		if (currentA && !pressAbotten) {

			// A로 만들 수 있는 사각형은 최대 10개
			// 전체 사각형은 최대 20개
			if (
				aCreatedCount < 10 &&
				totalBoxCount < 20 &&
				count < 10)
			{
				MakeBox(box[count][0]);

				// 첫 번째 사각형인지 저장
				box[count][0].first =
					(aCreatedCount == 0);

				// 새 사각형은 조각 하나를 가짐
				pieceCount[count] = 1;

				count++;
				aCreatedCount++;
				totalBoxCount++;

				std::cout
					<< "A키 생성 횟수: "
					<< aCreatedCount
					<< '\n';

				std::cout
					<< "현재 전체 사각형 개수: "
					<< totalBoxCount
					<< '\n';
			}
		}

		// 현재 키 상태를 다음 반복에서 사용
		pressAbotten = currentA;

		glClearColor(
			1.0f,
			1.0f,
			1.0f,
			1.0f
		);

		// 화면 지우기
		glClear(GL_COLOR_BUFFER_BIT);

		// 지금까지 만든 모든 사각형을 다시 그림
		for (int i = 0; i < count; i++) {
			for (
				int j = 0;
				j < pieceCount[i];
				j++)
			{
				DrawBox(box[i][j]);
			}
		}

		// 버퍼교체
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	//--- 종료 처리
	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}