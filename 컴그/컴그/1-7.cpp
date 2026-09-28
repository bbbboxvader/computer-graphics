#include <GL/glew.h>
#include <GL/glfw3.h>
#include <iostream>

// 셰이더 프로그램 번호
GLuint shaderProgram = 0;

// 정점 배열과 정점 버퍼
GLuint VAO = 0;
GLuint VBO = 0;

// 정점 한 개는 위치 3개와 색상 3개를 사용한다.
struct Vertex {
	float x;
	float y;
	float z;

	float R;
	float G;
	float B;
};

// 셰이더 하나를 컴파일하는 함수
GLuint CompileShader(
	GLenum shaderType,
	const char* shaderSource)
{
	// 셰이더 생성
	GLuint shader = glCreateShader(shaderType);

	// 셰이더 코드를 전달
	glShaderSource(
		shader,
		1,
		&shaderSource,
		nullptr
	);

	// 셰이더 컴파일
	glCompileShader(shader);

	// 컴파일 성공 여부 확인
	GLint success = 0;

	glGetShaderiv(
		shader,
		GL_COMPILE_STATUS,
		&success
	);

	if (success == GL_FALSE) {
		char errorMessage[512];

		glGetShaderInfoLog(
			shader,
			512,
			nullptr,
			errorMessage
		);

		std::cout
			<< "셰이더 컴파일 실패!\n"
			<< errorMessage
			<< std::endl;

		glDeleteShader(shader);
		return 0;
	}

	return shader;
}

// 버텍스 셰이더와 프래그먼트 셰이더를 연결하는 함수
GLuint MakeShaderProgram()
{
	// 정점 위치와 색상을 처리하는 버텍스 셰이더
	const char* vertexSource =
		"#version 330 core\n"
		"layout (location = 0) in vec3 vPosition;\n"
		"layout (location = 1) in vec3 vColor;\n"
		"out vec3 outColor;\n"
		"void main()\n"
		"{\n"
		"    gl_Position = vec4(vPosition, 1.0);\n"
		"    outColor = vColor;\n"
		"}\n";

	// 화면에 색상을 출력하는 프래그먼트 셰이더
	const char* fragmentSource =
		"#version 330 core\n"
		"in vec3 outColor;\n"
		"out vec4 FragColor;\n"
		"void main()\n"
		"{\n"
		"    FragColor = vec4(outColor, 1.0);\n"
		"}\n";

	// 셰이더 컴파일
	GLuint vertexShader =
		CompileShader(
			GL_VERTEX_SHADER,
			vertexSource
		);

	if (vertexShader == 0)
		return 0;

	GLuint fragmentShader =
		CompileShader(
			GL_FRAGMENT_SHADER,
			fragmentSource
		);

	if (fragmentShader == 0) {
		glDeleteShader(vertexShader);
		return 0;
	}

	// 셰이더 프로그램 생성
	GLuint program = glCreateProgram();

	// 프로그램에 셰이더 연결
	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);

	// 프로그램 링크
	glLinkProgram(program);

	// 링크 성공 여부 확인
	GLint success = 0;

	glGetProgramiv(
		program,
		GL_LINK_STATUS,
		&success
	);

	if (success == GL_FALSE) {
		char errorMessage[512];

		glGetProgramInfoLog(
			program,
			512,
			nullptr,
			errorMessage
		);

		std::cout
			<< "셰이더 프로그램 연결 실패!\n"
			<< errorMessage
			<< std::endl;

		glDeleteProgram(program);
		program = 0;
	}

	// 프로그램 연결이 끝났으므로 개별 셰이더 삭제
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	return program;
}

// VAO와 VBO를 만드는 함수
void InitBuffer()
{
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);

	// 정점 100개를 저장할 공간
	glBufferData(
		GL_ARRAY_BUFFER,
		sizeof(Vertex) * 100,
		nullptr,
		GL_DYNAMIC_DRAW
	);

	// 정점 위치 설정
	glVertexAttribPointer(
		0,
		3,
		GL_FLOAT,
		GL_FALSE,
		sizeof(Vertex),
		(void*)0
	);

	glEnableVertexAttribArray(0);

	// 정점 색상 설정
	glVertexAttribPointer(
		1,
		3,
		GL_FLOAT,
		GL_FALSE,
		sizeof(Vertex),
		(void*)(sizeof(float) * 3)
	);

	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

// 마우스 좌표를 OpenGL 좌표로 변환하는 함수
void GetOpenGLMousePosition(
	GLFWwindow* window,
	double mouseX,
	double mouseY,
	float& openGLX,
	float& openGLY)
{
	int width;
	int height;

	glfwGetFramebufferSize(
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

// 키보드 콜백 함수
void KeyCallback(
	GLFWwindow* window,
	int key,
	int scancode,
	int action,
	int mods)
{
	// 키를 한 번 눌렀을 때만 실행
	if (action != GLFW_PRESS)
		return;

	// Q를 누르면 종료
	if (key == GLFW_KEY_Q) {
		glfwSetWindowShouldClose(window, true);
	}

	// P: 점 만들기
	if (key == GLFW_KEY_P) {
		std::cout << "P: 점 만들기" << std::endl;

	}

	// E: 선 만들기
	if (key == GLFW_KEY_E) {
		std::cout << "E: 선 만들기" << std::endl;
	}

	// T: 삼각형 만들기
	if (key == GLFW_KEY_T) {
		std::cout << "T: 삼각형 만들기" << std::endl;
	}

	// R: 사각형 만들기
	if (key == GLFW_KEY_R) {
		std::cout << "R: 사각형 만들기" << std::endl;
	}

	// C: 모든 도형 삭제
	if (key == GLFW_KEY_C) {
		std::cout << "C: 모든 도형 삭제" << std::endl;
	}
}

// 마우스 버튼 콜백 함수
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

	std::cout
		<< "마우스 위치: "
		<< x << ", " << y
		<< std::endl;

	// 이곳에서 클릭한 도형을 검사한다.
}

// 윈도우 크기가 바뀌었을 때 실행
void FramebufferSizeCallback(
	GLFWwindow* window,
	int width,
	int height)
{
	glViewport(0, 0, width, height);
}

// 화면을 그리는 함수
void DrawScene()
{
	// 배경색을 흰색으로 설정
	glClearColor(
		1.0f,
		1.0f,
		1.0f,
		1.0f
	);

	glClear(GL_COLOR_BUFFER_BIT);

	// 셰이더 사용
	glUseProgram(shaderProgram);

	// VAO 사용
	glBindVertexArray(VAO);

	// 이곳에서 도형을 그린다.
	//
	// 점:
	// glDrawArrays(GL_POINTS, 시작번호, 1);
	//
	// 선:
	// glDrawArrays(GL_LINES, 시작번호, 2);
	//
	// 삼각형:
	// glDrawArrays(GL_TRIANGLES, 시작번호, 3);
	//
	// 사각형:
	// glDrawArrays(GL_TRIANGLES, 시작번호, 6);

	glBindVertexArray(0);
}

int main()
{
	// GLFW 초기화
	if (!glfwInit()) {
		std::cerr
			<< "GLFW 초기화 실패!"
			<< std::endl;

		return -1;
	}

	// OpenGL 3.3 사용
	glfwWindowHint(
		GLFW_CONTEXT_VERSION_MAJOR,
		3
	);

	glfwWindowHint(
		GLFW_CONTEXT_VERSION_MINOR,
		3
	);

	// GLSL을 사용하기 위한 Core Profile
	glfwWindowHint(
		GLFW_OPENGL_PROFILE,
		GLFW_OPENGL_CORE_PROFILE
	);

	// 윈도우 생성
	GLFWwindow* window =
		glfwCreateWindow(
			800,
			600,
			"Practice 7",
			nullptr,
			nullptr
		);

	if (window == nullptr) {
		std::cerr
			<< "윈도우 생성 실패!"
			<< std::endl;

		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);

	// GLEW 초기화
	glewExperimental = GL_TRUE;

	if (glewInit() != GLEW_OK) {
		std::cerr
			<< "GLEW 초기화 실패!"
			<< std::endl;

		glfwDestroyWindow(window);
		glfwTerminate();

		return -1;
	}

	// 현재 프레임버퍼 크기로 뷰포트 설정
	int width;
	int height;

	glfwGetFramebufferSize(
		window,
		&width,
		&height
	);

	glViewport(0, 0, width, height);

	// 콜백 함수 등록
	glfwSetKeyCallback(
		window,
		KeyCallback
	);

	glfwSetMouseButtonCallback(
		window,
		MouseButtonCallback
	);

	glfwSetFramebufferSizeCallback(
		window,
		FramebufferSizeCallback
	);

	// 셰이더 프로그램 생성
	shaderProgram = MakeShaderProgram();

	if (shaderProgram == 0) {
		glfwDestroyWindow(window);
		glfwTerminate();

		return -1;
	}

	// VAO와 VBO 생성
	InitBuffer();

	// 점 크기와 선 두께
	glPointSize(7.0f);
	glLineWidth(2.0f);

	// 메인 반복문
	while (!glfwWindowShouldClose(window)) {
		DrawScene();

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// OpenGL 객체 삭제
	glDeleteBuffers(1, &VBO);
	glDeleteVertexArrays(1, &VAO);
	glDeleteProgram(shaderProgram);

	// 종료 처리
	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}