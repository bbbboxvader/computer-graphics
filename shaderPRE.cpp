// 실습 13
// 3차원 육면체와 사각뿔의 면을 키보드로 골라 그린다.
// PracticeCommon.h를 사용하지 않고 필요한 내용을 이 파일 안에 직접 작성했다.

#include <GL/glew.h>
#include <GL/glfw3.h>

#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iostream>
#include <string>

// ------------------------------------------------------------
// 1. 정점과 색을 저장할 간단한 자료형
// ------------------------------------------------------------

// VBO에 들어가는 정점 한 개의 모양이다.
// 위치 x, y, z 다음에 색상 r, g, b가 저장된다.
struct Vertex
{
	float x;
	float y;
	float z;
	float r;
	float g;
	float b;
};

struct Point3D
{
	float x;
	float y;
	float z;
};

struct Color3
{
	float r;
	float g;
	float b;
};

// 지금 무엇을 화면에 그릴지를 기억한다.
enum DrawMode
{
	SHOW_CUBE_ALL,
	SHOW_CUBE_ONE,
	SHOW_PYRAMID_ONE,
	SHOW_PYRAMID_ALL,
	SHOW_CUBE_RANDOM_TWO,
	SHOW_PYRAMID_BASE_AND_SIDE
};

// ------------------------------------------------------------
// 2. 함수 미리 알리기
// ------------------------------------------------------------

std::string filetobuf(const char* fileName);
GLuint compileShader(GLenum shaderType, const char* fileName);
GLuint make_shaderProgram();

void InitBuffer();
void DrawVertices(GLenum drawMode, const Vertex vertices[], int count);
void DrawScene();
void Reshape(GLFWwindow* window, int width, int height);
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

// ------------------------------------------------------------
// 3. OpenGL에서 사용할 번호와 uniform 위치
// ------------------------------------------------------------

GLuint shaderProgramID = 0;
GLuint VAO = 0;
GLuint VBO = 0;

GLint angleXLocation = -1;
GLint angleYLocation = -1;
GLint offsetLocation = -1;

// 키보드로 바뀌는 현재 상태
DrawMode drawMode = SHOW_CUBE_ALL;
int selectedCubeFace = 0;
int selectedPyramidSide = 0;
int randomCubeFaceA = 0;
int randomCubeFaceB = 1;
int randomPyramidSide = 0;

// ------------------------------------------------------------
// 4. 육면체의 점, 면, 색상
// ------------------------------------------------------------

// 육면체에는 서로 다른 꼭짓점이 8개 있다.
const Point3D cubePoints[8] =
{
	{ -0.42f, -0.42f, -0.42f }, // 0: 왼쪽 아래 뒤
	{  0.42f, -0.42f, -0.42f }, // 1: 오른쪽 아래 뒤
	{  0.42f,  0.42f, -0.42f }, // 2: 오른쪽 위 뒤
	{ -0.42f,  0.42f, -0.42f }, // 3: 왼쪽 위 뒤
	{ -0.42f, -0.42f,  0.42f }, // 4: 왼쪽 아래 앞
	{  0.42f, -0.42f,  0.42f }, // 5: 오른쪽 아래 앞
	{  0.42f,  0.42f,  0.42f }, // 6: 오른쪽 위 앞
	{ -0.42f,  0.42f,  0.42f }  // 7: 왼쪽 위 앞
};

// 사각형 한 면을 만드는 꼭짓점 번호 4개이다.
const int cubeFaces[6][4] =
{
	{ 4, 5, 6, 7 }, // 0: 앞면
	{ 0, 3, 2, 1 }, // 1: 뒷면
	{ 0, 4, 7, 3 }, // 2: 왼쪽 면
	{ 1, 2, 6, 5 }, // 3: 오른쪽 면
	{ 3, 7, 6, 2 }, // 4: 윗면
	{ 0, 1, 5, 4 }  // 5: 아랫면
};

const Color3 cubeFaceColors[6] =
{
	{ 0.10f, 0.25f, 1.00f }, // 파랑
	{ 1.00f, 0.90f, 0.05f }, // 노랑
	{ 1.00f, 0.10f, 0.10f }, // 빨강
	{ 0.05f, 0.90f, 0.20f }, // 초록
	{ 0.00f, 0.90f, 0.90f }, // 청록
	{ 1.00f, 0.00f, 0.85f }  // 자홍
};

// ------------------------------------------------------------
// 5. 사각뿔의 점, 면, 색상
// ------------------------------------------------------------

// 꼭대기 한 점과 바닥의 네 점이다.
const Point3D pyramidPoints[5] =
{
	{  0.00f,  0.55f,  0.00f }, // 0: 꼭대기
	{ -0.48f, -0.42f,  0.48f }, // 1: 왼쪽 앞
	{  0.48f, -0.42f,  0.48f }, // 2: 오른쪽 앞
	{  0.48f, -0.42f, -0.48f }, // 3: 오른쪽 뒤
	{ -0.48f, -0.42f, -0.48f }  // 4: 왼쪽 뒤
};

// 사각뿔의 옆면은 삼각형 4개이다.
const int pyramidSides[4][3] =
{
	{ 0, 1, 2 }, // 0: 앞쪽 면
	{ 0, 2, 3 }, // 1: 오른쪽 면
	{ 0, 3, 4 }, // 2: 뒤쪽 면
	{ 0, 4, 1 }  // 3: 왼쪽 면
};

// 바닥은 사각형 한 면이다.
const int pyramidBase[4] = { 1, 4, 3, 2 };

const Color3 pyramidColors[5] =
{
	{ 0.10f, 0.25f, 1.00f }, // 앞쪽 면: 파랑
	{ 1.00f, 0.10f, 0.10f }, // 오른쪽 면: 빨강
	{ 0.00f, 0.90f, 0.20f }, // 뒤쪽 면: 초록
	{ 1.00f, 0.00f, 0.85f }, // 왼쪽 면: 자홍
	{ 1.00f, 0.95f, 0.05f }  // 바닥면: 노랑
};

// ------------------------------------------------------------
// 6. GLSL 파일 읽기와 셰이더 프로그램 만들기
// ------------------------------------------------------------

std::string filetobuf(const char* fileName)
{
	std::ifstream shaderFile(fileName);

	if (!shaderFile.is_open())
	{
		std::cerr << "파일을 열 수 없습니다: " << fileName << std::endl;
		return "";
	}

	std::string source(
		(std::istreambuf_iterator<char>(shaderFile)),
		std::istreambuf_iterator<char>()
	);

	return source;
}

GLuint compileShader(GLenum shaderType, const char* fileName)
{
	std::string shaderSource = filetobuf(fileName);
	if (shaderSource.empty())
		return 0;

	const char* source = shaderSource.c_str();
	GLuint shader = glCreateShader(shaderType);
	glShaderSource(shader, 1, &source, nullptr);
	glCompileShader(shader);

	GLint result = GL_FALSE;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &result);

	if (result == GL_FALSE)
	{
		GLchar errorLog[512];
		glGetShaderInfoLog(shader, 512, nullptr, errorLog);
		std::cerr << "셰이더 컴파일 실패: " << fileName << '\n';
		std::cerr << errorLog << std::endl;
		glDeleteShader(shader);
		return 0;
	}

	return shader;
}

GLuint make_shaderProgram()
{
	// 실습 13용 GLSL 파일을 각각 컴파일한다.
	GLuint vertexShader =
		compileShader(GL_VERTEX_SHADER, "vertex-3d.glsl");

	GLuint fragmentShader =
		compileShader(GL_FRAGMENT_SHADER, "fragment-basic.glsl");

	if (vertexShader == 0 || fragmentShader == 0)
	{
		if (vertexShader != 0)
			glDeleteShader(vertexShader);
		if (fragmentShader != 0)
			glDeleteShader(fragmentShader);
		return 0;
	}

	GLuint shaderID = glCreateProgram();
	glAttachShader(shaderID, vertexShader);
	glAttachShader(shaderID, fragmentShader);
	glLinkProgram(shaderID);

	GLint result = GL_FALSE;
	glGetProgramiv(shaderID, GL_LINK_STATUS, &result);

	if (result == GL_FALSE)
	{
		GLchar errorLog[512];
		glGetProgramInfoLog(shaderID, 512, nullptr, errorLog);
		std::cerr << "셰이더 프로그램 연결 실패\n";
		std::cerr << errorLog << std::endl;
		glDeleteProgram(shaderID);
		shaderID = 0;
	}

	// 링크가 끝나면 개별 셰이더 객체는 지워도 된다.
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	return shaderID;
}

// ------------------------------------------------------------
// 7. VAO와 VBO 준비하기
// ------------------------------------------------------------

void InitBuffer()
{
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);

	// 한 번에 가장 많이 보내는 정점은 사각형을 만드는 정점 6개이다.
	glBufferData(
		GL_ARRAY_BUFFER,
		sizeof(Vertex) * 6,
		nullptr,
		GL_DYNAMIC_DRAW
	);

	// location 0: Vertex 안의 x, y, z를 읽는다.
	glVertexAttribPointer(
		0,
		3,
		GL_FLOAT,
		GL_FALSE,
		sizeof(Vertex),
		reinterpret_cast<void*>(0)
	);
	glEnableVertexAttribArray(0);

	// location 1: x, y, z 다음에 있는 r, g, b를 읽는다.
	glVertexAttribPointer(
		1,
		3,
		GL_FLOAT,
		GL_FALSE,
		sizeof(Vertex),
		reinterpret_cast<void*>(sizeof(float) * 3)
	);
	glEnableVertexAttribArray(1);
}

// 정점 배열을 VBO에 복사하고 화면에 그린다.
void DrawVertices(GLenum drawMode, const Vertex vertices[], int count)
{
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferSubData(
		GL_ARRAY_BUFFER,
		0,
		sizeof(Vertex) * count,
		vertices
	);
	glDrawArrays(drawMode, 0, count);
}

// ------------------------------------------------------------
// 8. 점과 색을 합쳐 Vertex 하나 만들기
// ------------------------------------------------------------

Vertex MakeVertex3D(Point3D point, Color3 color)
{
	Vertex result;
	result.x = point.x;
	result.y = point.y;
	result.z = point.z;
	result.r = color.r;
	result.g = color.g;
	result.b = color.b;
	return result;
}

// C++의 회전각과 이동량을 GLSL의 uniform 변수로 보낸다.
void SendTransform(float angleX, float angleY, float moveX, float moveY)
{
	glUniform1f(angleXLocation, angleX);
	glUniform1f(angleYLocation, angleY);
	glUniform3f(offsetLocation, moveX, moveY, 0.0f);
}

// ------------------------------------------------------------
// 9. 좌표축 그리기
// ------------------------------------------------------------

void DrawAxes()
{
	Vertex xAxis[2] =
	{
		{ -0.95f, 0.0f, 0.0f, 1.0f, 0.20f, 0.20f },
		{  0.95f, 0.0f, 0.0f, 1.0f, 0.20f, 0.20f }
	};

	Vertex yAxis[2] =
	{
		{ 0.0f, -0.95f, 0.0f, 0.20f, 1.0f, 0.20f },
		{ 0.0f,  0.95f, 0.0f, 0.20f, 1.0f, 0.20f }
	};

	Vertex zAxis[2] =
	{
		{ 0.0f, 0.0f, -0.95f, 0.20f, 0.40f, 1.0f },
		{ 0.0f, 0.0f,  0.95f, 0.20f, 0.40f, 1.0f }
	};

	DrawVertices(GL_LINES, xAxis, 2);
	DrawVertices(GL_LINES, yAxis, 2);
	DrawVertices(GL_LINES, zAxis, 2);
}

// ------------------------------------------------------------
// 10. 사각형 면과 삼각형 면 그리기
// ------------------------------------------------------------

// 사각형은 삼각형 두 개, 즉 정점 6개로 바꾸어 그린다.
void DrawQuadFace(const Point3D points[], const int face[4], Color3 color)
{
	Vertex vertices[6] =
	{
		MakeVertex3D(points[face[0]], color),
		MakeVertex3D(points[face[1]], color),
		MakeVertex3D(points[face[2]], color),

		MakeVertex3D(points[face[0]], color),
		MakeVertex3D(points[face[2]], color),
		MakeVertex3D(points[face[3]], color)
	};

	DrawVertices(GL_TRIANGLES, vertices, 6);
}

void DrawTriangleFace(const Point3D points[], const int face[3], Color3 color)
{
	Vertex vertices[3] =
	{
		MakeVertex3D(points[face[0]], color),
		MakeVertex3D(points[face[1]], color),
		MakeVertex3D(points[face[2]], color)
	};

	DrawVertices(GL_TRIANGLES, vertices, 3);
}

void DrawCubeFace(int face)
{
	DrawQuadFace(
		cubePoints,
		cubeFaces[face],
		cubeFaceColors[face]
	);
}

void DrawPyramidSide(int side)
{
	DrawTriangleFace(
		pyramidPoints,
		pyramidSides[side],
		pyramidColors[side]
	);
}

void DrawPyramidBase()
{
	DrawQuadFace(
		pyramidPoints,
		pyramidBase,
		pyramidColors[4]
	);
}

void DrawPyramidAll()
{
	for (int side = 0; side < 4; ++side)
		DrawPyramidSide(side);

	DrawPyramidBase();
}

// ------------------------------------------------------------
// 11. 매 프레임 화면 그리기
// ------------------------------------------------------------

void DrawScene()
{
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(shaderProgramID);
	glBindVertexArray(VAO);

	// 좌표축은 물체와 함께 회전시키지 않는다.
	SendTransform(0.0f, 0.0f, 0.0f, 0.0f);
	glLineWidth(1.5f);
	DrawAxes();

	// 실습 조건: 물체를 x축과 y축으로 각각 30도 회전한다.
	SendTransform(30.0f, 30.0f, 0.0f, 0.0f);

	if (drawMode == SHOW_CUBE_ALL)
	{
		for (int face = 0; face < 6; ++face)
			DrawCubeFace(face);
	}
	else if (drawMode == SHOW_CUBE_ONE)
	{
		DrawCubeFace(selectedCubeFace);
	}
	else if (drawMode == SHOW_PYRAMID_ONE)
	{
		DrawPyramidSide(selectedPyramidSide);
	}
	else if (drawMode == SHOW_PYRAMID_ALL)
	{
		DrawPyramidAll();
	}
	else if (drawMode == SHOW_CUBE_RANDOM_TWO)
	{
		DrawCubeFace(randomCubeFaceA);
		DrawCubeFace(randomCubeFaceB);
	}
	else if (drawMode == SHOW_PYRAMID_BASE_AND_SIDE)
	{
		DrawPyramidBase();
		DrawPyramidSide(randomPyramidSide);
	}
}

// ------------------------------------------------------------
// 12. 키보드 입력
// ------------------------------------------------------------

void KeyCallback(GLFWwindow* window, int key, int, int action, int)
{
	// 키를 처음 눌렀을 때 한 번만 처리한다.
	if (action != GLFW_PRESS)
		return;

	// 1~6: 육면체의 각 면
	if (key >= GLFW_KEY_1 && key <= GLFW_KEY_6)
	{
		drawMode = SHOW_CUBE_ONE;
		selectedCubeFace = key - GLFW_KEY_1;
	}
	// 7~9: 사각뿔의 옆면 0~2
	else if (key >= GLFW_KEY_7 && key <= GLFW_KEY_9)
	{
		drawMode = SHOW_PYRAMID_ONE;
		selectedPyramidSide = key - GLFW_KEY_7;
	}
	// 0: 사각뿔의 네 번째 옆면
	else if (key == GLFW_KEY_0)
	{
		drawMode = SHOW_PYRAMID_ONE;
		selectedPyramidSide = 3;
	}
	// P: 사진처럼 사각뿔의 모든 면을 합친 완성형
	else if (key == GLFW_KEY_P)
	{
		drawMode = SHOW_PYRAMID_ALL;
	}
	// C: 육면체에서 서로 다른 면 두 개
	else if (key == GLFW_KEY_C)
	{
		drawMode = SHOW_CUBE_RANDOM_TWO;
		randomCubeFaceA = std::rand() % 6;

		do
		{
			randomCubeFaceB = std::rand() % 6;
		} while (randomCubeFaceA == randomCubeFaceB);
	}
	// T: 사각뿔의 바닥면과 임의의 옆면 하나
	else if (key == GLFW_KEY_T)
	{
		drawMode = SHOW_PYRAMID_BASE_AND_SIDE;
		randomPyramidSide = std::rand() % 4;
	}
	else if (key == GLFW_KEY_Q || key == GLFW_KEY_ESCAPE)
	{
		glfwSetWindowShouldClose(window, GLFW_TRUE);
	}
}

void Reshape(GLFWwindow*, int width, int height)
{
	glViewport(0, 0, width, height);
}

// ------------------------------------------------------------
// 13. 프로그램 시작
// ------------------------------------------------------------

int main()
{
	std::srand(static_cast<unsigned int>(std::time(nullptr)));

	if (glfwInit() == GLFW_FALSE)
		return -1;

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(
		800,
		800,
		"Practice 13",
		nullptr,
		nullptr
	);

	if (window == nullptr)
	{
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);

	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK)
	{
		glfwDestroyWindow(window);
		glfwTerminate();
		return -1;
	}

	// 입력 함수와 화면 크기 변경 함수를 GLFW에 등록한다.
	glfwSetKeyCallback(window, KeyCallback);
	glfwSetFramebufferSizeCallback(window, Reshape);

	// 정점 셰이더와 프래그먼트 셰이더를 하나의 프로그램으로 만든다.
	shaderProgramID = make_shaderProgram();
	if (shaderProgramID == 0)
	{
		glfwDestroyWindow(window);
		glfwTerminate();
		return -1;
	}

	InitBuffer();

	// GLSL의 uniform 변수 위치를 찾는다.
	angleXLocation = glGetUniformLocation(shaderProgramID, "uAngleX");
	angleYLocation = glGetUniformLocation(shaderProgramID, "uAngleY");
	offsetLocation = glGetUniformLocation(shaderProgramID, "uOffset");

	// 앞에 있는 면이 뒤에 있는 면을 가리도록 깊이 검사를 켠다.
	glEnable(GL_DEPTH_TEST);

	while (glfwWindowShouldClose(window) == GLFW_FALSE)
	{
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
