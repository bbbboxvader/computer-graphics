#include <GL/glew.h>
#include <GL/glfw3.h>
#include <iostream>

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