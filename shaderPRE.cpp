//--- 필요한헤더파일선언
#include <GL/glew.h>
#include <GL/glfw3.h>
#include <fstream>
#include <string>
#include <iostream>
//--- 사용자정의함수
void make_vertexShaders();
void make_fragmentShaders();
GLuint make_shaderProgram();
GLvoid drawScene();
GLvoid Reshape(int w, int h);
//--- 필요한변수선언
GLuint VAO;
GLint colorLocation;
GLint width, height;
GLuint shaderProgramID;
GLuint vertexShader;
GLuint fragmentShader;
std::string filetobuf(const char* file)
{
	std::ifstream shaderFile(file);
	if (!shaderFile.is_open())
	{
		return "";
	}
	//--- 파일전체를문자열로읽기
	std::string source((std::istreambuf_iterator<char>(shaderFile)), std::istreambuf_iterator<char>());
	shaderFile.close();
	return source;
}
void make_vertexShaders()
{
	//--- 셰이더코드읽어오기
	std::string vertexSource =
		filetobuf("vertex9-1.glsl");
	const char* source = vertexSource.c_str();
	//--- 셰이더생성하기
	vertexShader = glCreateShader(GL_VERTEX_SHADER);
	//--- 셰이더에코드연결하고컴파일하기
	glShaderSource(vertexShader, 1, &source, NULL);
	glCompileShader(vertexShader);
	GLint result;
	GLchar errorLog[512];
	//--- 에러 체크하기
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &result);
	if (!result)
	{
	glGetShaderInfoLog(vertexShader, 512, NULL, errorLog);
	}
	std::cerr << "ERROR: vertex shader 컴파일 실패\n" << errorLog << std::endl;
	return;
}
	//--- 프래그먼트세이더객체만들기
void make_fragmentShaders()
{
	//--- 셰이더코드읽어오기
	std::string fragmentSource =
	filetobuf("fragement9-1.glsl");
	const char* source = fragmentSource.c_str();
	//--- 셰이더생성하기
	fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	//--- 셰이더에코드연결하고컴파일하기
	glShaderSource(fragmentShader, 1, &source, NULL);
	glCompileShader(fragmentShader);
	GLint result;
	GLchar errorLog[512];
	//--- 에러체크하기
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &result);
	if (!result)
	{
		glGetShaderInfoLog(fragmentShader, 512, NULL, errorLog);
	}
		std::cerr << "ERROR: fragment shader 컴파일 실패\n" << errorLog << std::endl;
		return;
}
GLuint make_shaderProgram()
{
	GLint result;
	GLchar errorLog[512];
	//--- 셰이더프로그램생성
	GLuint shaderID = glCreateProgram();
	//--- 버텍스셰이더와프래그먼트셰이더연결
	glAttachShader(shaderID, vertexShader);
	glAttachShader(shaderID, fragmentShader);
	//--- 셰이더프로그램링크
	glLinkProgram(shaderID);
	//--- 링크가끝났으므로셰이더객체삭제
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
	//--- 링크 성공여부확인
	glGetProgramiv(shaderID, GL_LINK_STATUS, &result);
	if (!result) {
		glGetProgramInfoLog(shaderID, 512, NULL, errorLog);
		std::cerr << "ERROR: shader program 연결 실패\n" << errorLog << std::endl;
		return 0;
	}
	//--- 셰이더프로그램사용
	glUseProgram(shaderID);
	return shaderID;
}
void drawScene()
{
	// 1. 배경색을 흰색으로 설정
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

	// 2. 이전 화면 지우기
	glClear(GL_COLOR_BUFFER_BIT);

	// 3. 사용할 셰이더 프로그램 선택
	glUseProgram(shaderProgramID);

	// 4. 사용할 VAO 선택
	glBindVertexArray(VAO);

	//// -------------------------
	//// 점 하나 그리기
	//// -------------------------
	//glPointSize(12.0f);

	//// 점은 빨간색
	//glUniform3f(
	//	colorLocation,
	//	1.0f, 0.0f, 0.0f
	//);

	//// 0번부터 정점 1개를 점으로 그리기
	//glDrawArrays(GL_POINTS, 0, 1);

	//// -------------------------
	//// 선 하나 그리기
	//// -------------------------
	//glLineWidth(5.0f);

	//// 선은 초록색
	//glUniform3f(
	//	colorLocation,
	//	0.0f, 0.7f, 0.0f
	//);

	//// 1번부터 정점 2개를 선으로 그리기
	//glDrawArrays(GL_LINES, 1, 2);

	//// -------------------------
	//// 삼각형 하나 그리기
	//// -------------------------

	// 삼각형1은 파란색
	glUniform3f(
		colorLocation,
		0.0f, 0.3f, 1.0f
	);
	glDrawArrays(GL_TRIANGLES, 0, 3);


	// 삼각형2는 빨간색
	glUniform3f(
		colorLocation,
		1.0f, 0.0f, 0.0f
	);
	// 3번부터 정점 3개를 삼각형으로 그리기
	glDrawArrays(GL_TRIANGLES, 3, 3);
}

//--- 메인 함수
int main()
{
	width = 800;
	height = 600;

	// 1. GLFW 시작
	if (!glfwInit())
		return -1;

	// 2. OpenGL 3.3 Core Profile 설정
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
		GLFW_OPENGL_CORE_PROFILE
	);

	// 3. 윈도우 생성
	GLFWwindow* window = glfwCreateWindow(
		width,
		height,
		"Point Line Triangle",
		nullptr,
		nullptr
	);

	if (!window)
	{
		glfwTerminate();
		return -1;
	}

	// 4. 윈도우의 OpenGL 기능 활성화
	glfwMakeContextCurrent(window);

	// 5. GLEW 시작
	glewExperimental = GL_TRUE;

	if (glewInit() != GLEW_OK)
	{
		glfwDestroyWindow(window);
		glfwTerminate();
		return -1;
	}

	// 6. 셰이더 만들기
	make_vertexShaders();
	make_fragmentShaders();
	shaderProgramID = make_shaderProgram();

	// 7. VAO 생성
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	// 8. uColor의 위치 가져오기
	colorLocation = glGetUniformLocation(
		shaderProgramID,
		"uColor"
	);

	// 9. 화면을 계속 그리는 반복문
	while (!glfwWindowShouldClose(window))
	{
		drawScene();

		// 완성된 그림을 화면에 보여주기
		glfwSwapBuffers(window);

		// 키보드, 마우스 등의 이벤트 확인
		glfwPollEvents();
	}

	// 10. 사용한 OpenGL 자원 정리
	glDeleteVertexArrays(1, &VAO);
	glDeleteProgram(shaderProgramID);

	// 11. 윈도우와 GLFW 정리
	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}