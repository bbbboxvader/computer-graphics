#include <GL/glew.h>
#include <GL/glfw3.h>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

// 실습 16: 사진처럼 닫힌 사각뿔에서 시작하여 네 옆면을 움직인다.
// Vertex: 정점 하나의 3차원 위치(x,y,z)와 색상(r,g,b)을 셰이더에 전달한다.
struct Vertex { float x, y, z, r, g, b; };
// Point3D: 사각뿔 꼭짓점이나 회전 기준점의 3차원 좌표이다.
struct Point3D { float x, y, z; };
// Color3: 빨강, 초록, 파랑 값이며 각 값의 범위는 0.0~1.0이다.
struct Color3 { float r, g, b; };
// FaceTransform: 옆면 하나의 x/y/z 회전각과 회전 중심점(pivot)을 저장한다.
struct FaceTransform { float angleX, angleY, angleZ; Point3D pivot; };

enum AnimationMode
{
	NO_ANIMATION,
	OPEN_ALL_FROM_BOTTOM, // 6: 아래 모서리 기준으로 함께 열기
	OPEN_ONE_BY_ONE,      // 7: 한 면씩 열기
	SWAP_OPPOSITE_FACES,  // 8: 네 옆면의 마주 보는 두 쌍 자리 바꾸기
	OPEN_ALL_FROM_APEX    // 9: 꼭짓점 기준으로 열기
};

// shaderProgramID: 완성된 셰이더 프로그램 번호
// VAO: 정점 설정을 기억하는 번호, VBO: 실제 정점 데이터를 저장하는 번호
GLuint shaderProgramID = 0, VAO = 0, VBO = 0;
// Location 변수는 C++ 값을 GLSL의 uniform 변수로 보내기 위한 위치 번호이다.
GLint objectAngleXLocation = -1, objectAngleYLocation = -1;
GLint partAngleXLocation = -1, partAngleYLocation = -1, partAngleZLocation = -1;
GLint pivotLocation = -1;

AnimationMode animationMode = NO_ANIMATION; // 현재 6~9 중 어떤 동작을 실행 중인지 저장
bool hiddenSurface = true;                  // true이면 깊이 검사로 뒤쪽 면을 가린다.
float allBottomAngle = 0.0f;                // 6번: 네 면이 함께 열린 현재 각도
float oneByOneAngles[4] =                   // 7번: 앞/오른쪽/뒤/왼쪽 면의 현재 각도
{ 0.0f, 0.0f, 0.0f, 0.0f };
float swapAngle = 0.0f;                     // 8번: 네 면의 현재 자리 교환 각도
float apexAngle = 0.0f;                     // 9번: 꼭짓점 기준으로 열린 현재 각도
bool allBottomOpened = false;               // 6번의 목표가 열린 상태인지
bool oneByOneOpened = false;                // 7번의 목표가 열린 상태인지
bool swapped = false;                       // 8번의 목표가 180도 자리 교환인지
bool apexOpened = false;                    // 9번의 목표가 열린 상태인지

const Point3D pyramidPoints[5] =
{
	{  0.00f,  0.55f,  0.00f }, // 0: 꼭짓점
	{ -0.48f, -0.42f,  0.48f }, // 1: 왼쪽 앞
	{  0.48f, -0.42f,  0.48f }, // 2: 오른쪽 앞
	{  0.48f, -0.42f, -0.48f }, // 3: 오른쪽 뒤
	{ -0.48f, -0.42f, -0.48f }  // 4: 왼쪽 뒤
};

const int pyramidSides[4][3] =
{
	{ 0, 1, 2 }, // 0: 앞면을 만드는 점 번호
	{ 0, 2, 3 }, // 1: 오른쪽 면
	{ 0, 3, 4 }, // 2: 뒷면
	{ 0, 4, 1 }  // 3: 왼쪽 면
};
// 바닥 사각형을 만드는 네 점 번호이다.
const int pyramidBase[4] = { 1, 4, 3, 2 };
const Color3 pyramidColors[5] =
{
	{ 0.10f, 0.25f, 1.00f }, // 앞면: 파랑
	{ 1.00f, 0.10f, 0.10f }, // 오른쪽 면: 빨강
	{ 0.05f, 0.90f, 0.20f }, // 뒷면: 초록
	{ 1.00f, 0.00f, 0.85f }, // 왼쪽 면: 자홍
	{ 1.00f, 0.95f, 0.05f }  // 바닥면: 노랑
};

std::string ReadTextFile(const char* fileName)
{
	std::ifstream file(fileName);
	if (!file.is_open()) {
		std::cerr << "파일을 열 수 없습니다: " << fileName << '\n';
		return "";
	}
	std::stringstream text;
	text << file.rdbuf();
	return text.str();
}

GLuint CompileShader(GLenum shaderType, const char* fileName)
{
	std::string shaderText = ReadTextFile(fileName);
	if (shaderText.empty()) return 0;
	const char* source = shaderText.c_str();
	GLuint shader = glCreateShader(shaderType);
	glShaderSource(shader, 1, &source, nullptr);
	glCompileShader(shader);
	GLint success = GL_FALSE;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (success == GL_FALSE) {
		char message[1024];
		glGetShaderInfoLog(shader, 1024, nullptr, message);
		std::cerr << "셰이더 컴파일 실패: " << fileName << '\n' << message << '\n';
		glDeleteShader(shader);
		return 0;
	}
	return shader;
}

GLuint MakeShaderProgram()
{
	GLuint vertexShader = CompileShader(GL_VERTEX_SHADER, "vertex-16.glsl");
	GLuint fragmentShader = CompileShader(GL_FRAGMENT_SHADER, "fragment-16.glsl");
	if (vertexShader == 0 || fragmentShader == 0) return 0;
	GLuint program = glCreateProgram();
	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);
	glLinkProgram(program);
	GLint success = GL_FALSE;
	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if (success == GL_FALSE) {
		char message[1024];
		glGetProgramInfoLog(program, 1024, nullptr, message);
		std::cerr << "셰이더 연결 실패\n" << message << '\n';
		glDeleteProgram(program);
		program = 0;
	}
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
	return program;
}

void InitBuffer()
{
	// 한 번에 보내는 정점 수보다 넉넉하게 Vertex 16개분의 공간을 만든다.
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * 16, nullptr, GL_DYNAMIC_DRAW);
	// location 0은 Vertex의 처음 세 값인 위치 x, y, z를 읽는다.
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(0));
	glEnableVertexAttribArray(0);
	// location 1은 위치 다음에 저장된 색상 r, g, b를 읽는다.
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(sizeof(float) * 3));
	glEnableVertexAttribArray(1);
}

Vertex MakeVertex(Point3D point, Color3 color)
{
	return { point.x, point.y, point.z, color.r, color.g, color.b };
}

void DrawVertices(GLenum mode, const Vertex vertices[], int count)
{
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vertex) * count, vertices);
	glDrawArrays(mode, 0, count);
}

FaceTransform NormalTransform()
{
	// 회전하지 않은 기본 상태: 모든 각도 0도, 회전 중심은 원점이다.
	return { 0.0f, 0.0f, 0.0f, { 0.0f, 0.0f, 0.0f } };
}

void SendTransform(const FaceTransform& transform)
{
	// 사진처럼 빨강·파랑 면이 함께 보이도록 사각뿔 전체를 기울인다.
	// 25.0은 x축 기울기, -30.0은 y축 기울기(도 단위)이다.
	glUniform1f(objectAngleXLocation, 25.0f);
	glUniform1f(objectAngleYLocation, -30.0f);
	// 아래 값은 사각뿔 전체가 아니라 현재 그릴 옆면 하나에만 적용한다.
	glUniform1f(partAngleXLocation, transform.angleX);
	glUniform1f(partAngleYLocation, transform.angleY);
	glUniform1f(partAngleZLocation, transform.angleZ);
	glUniform3f(pivotLocation, transform.pivot.x, transform.pivot.y, transform.pivot.z);
}

// side 번호에 맞는 아래 모서리를 회전축으로 만든다.
FaceTransform BottomEdgeTransform(int side, float angle)
{
	FaceTransform result = NormalTransform();
	if (side == 0) {
		// 앞면: 앞쪽 아래 모서리의 중점 (0, -0.42, 0.48)을 기준으로 x축 회전
		result.pivot = { 0.0f, -0.42f, 0.48f };
		result.angleX = angle;
	}
	else if (side == 1) {
		// 오른쪽 면: 오른쪽 아래 모서리 중점을 기준으로 z축 음의 방향 회전
		result.pivot = { 0.48f, -0.42f, 0.0f };
		result.angleZ = -angle;
	}
	else if (side == 2) {
		// 뒷면: 뒤쪽 아래 모서리 중점을 기준으로 x축 음의 방향 회전
		result.pivot = { 0.0f, -0.42f, -0.48f };
		result.angleX = -angle;
	}
	else {
		// 왼쪽 면: 왼쪽 아래 모서리 중점을 기준으로 z축 양의 방향 회전
		result.pivot = { -0.48f, -0.42f, 0.0f };
		result.angleZ = angle;
	}
	return result;
}

FaceTransform TransformForSide(int side)
{
	if (animationMode == OPEN_ALL_FROM_BOTTOM)
		return BottomEdgeTransform(side, allBottomAngle);
	if (animationMode == OPEN_ONE_BY_ONE)
		return BottomEdgeTransform(side, oneByOneAngles[side]);
	// 8번: 네 면 모두 y축으로 180도 회전한다.
	// 앞↔뒤, 오른쪽↔왼쪽 면이 동시에 서로의 위치로 이동한다.
	if (animationMode == SWAP_OPPOSITE_FACES) {
		FaceTransform result = NormalTransform();
		result.angleY = swapAngle;
		return result;
	}
	if (animationMode == OPEN_ALL_FROM_APEX) {
		FaceTransform result = NormalTransform();
		// pyramidPoints[0]은 네 옆면이 공유하는 꼭짓점이므로 9번의 회전 중심이다.
		result.pivot = pyramidPoints[0];
		if (side == 0) result.angleX = -apexAngle;
		if (side == 1) result.angleZ = apexAngle;
		if (side == 2) result.angleX = apexAngle;
		if (side == 3) result.angleZ = -apexAngle;
		return result;
	}
	return NormalTransform();
}

void DrawAxes()
{
	// 축 길이 0.95는 화면 좌표 경계(-1~1)를 넘지 않도록 정한 값이다.
	// x축은 빨강, y축은 초록, z축은 파랑으로 두 점씩 연결한다.
	Vertex axes[6] =
	{
		{ -0.95f,0,0, 1,0.1f,0.1f }, { 0.95f,0,0, 1,0.1f,0.1f },
		{ 0,-0.95f,0, 0.1f,0.8f,0.1f }, { 0,0.95f,0, 0.1f,0.8f,0.1f },
		{ 0,0,-0.95f, 0.1f,0.3f,1 }, { 0,0,0.95f, 0.1f,0.3f,1 }
	};
	DrawVertices(GL_LINES, axes, 6);
}

void DrawTriangleFace(const int face[3], Color3 color)
{
	Vertex vertices[3] =
	{
		MakeVertex(pyramidPoints[face[0]], color),
		MakeVertex(pyramidPoints[face[1]], color),
		MakeVertex(pyramidPoints[face[2]], color)
	};
	DrawVertices(GL_TRIANGLES, vertices, 3);
}

void DrawBase()
{
	// 사각형 바닥을 OpenGL 삼각형 두 개, 즉 정점 6개로 나누어 그린다.
	Vertex vertices[6] =
	{
		MakeVertex(pyramidPoints[pyramidBase[0]], pyramidColors[4]),
		MakeVertex(pyramidPoints[pyramidBase[1]], pyramidColors[4]),
		MakeVertex(pyramidPoints[pyramidBase[2]], pyramidColors[4]),
		MakeVertex(pyramidPoints[pyramidBase[0]], pyramidColors[4]),
		MakeVertex(pyramidPoints[pyramidBase[2]], pyramidColors[4]),
		MakeVertex(pyramidPoints[pyramidBase[3]], pyramidColors[4])
	};
	DrawVertices(GL_TRIANGLES, vertices, 6);
}

float MoveToward(float current, float target, float amount)
{
	// current를 target 방향으로 amount만큼 이동시키되 목표값을 넘어가지 않게 한다.
	if (current < target) return std::min(current + amount, target);
	if (current > target) return std::max(current - amount, target);
	return current;
}

void UpdateScene(float deltaTime)
{
	// 초당 85도씩 움직인다. deltaTime을 곱하면 컴퓨터 속도와 상관없이 같은 속도로 보인다.
	float speed = 85.0f * deltaTime;
	if (animationMode == OPEN_ALL_FROM_BOTTOM)
		// 6번은 열 때 180도, 닫을 때 0도를 목표로 네 면이 함께 움직인다.
		allBottomAngle = MoveToward(allBottomAngle, allBottomOpened ? 180.0f : 0.0f, speed);
	else if (animationMode == OPEN_ONE_BY_ONE) {
		if (oneByOneOpened) {
			// 7번은 한 면이 정확히 90도가 된 다음에만 다음 면을 연다.
			for (int side = 0; side < 4; ++side) {
				if (oneByOneAngles[side] < 90.0f) {
					oneByOneAngles[side] = MoveToward(oneByOneAngles[side], 90.0f, speed);
					break;
				}
			}
		}
		else {
			// 닫을 때에는 마지막 면부터 반대 순서로 0도까지 닫는다.
			for (int side = 3; side >= 0; --side) {
				if (oneByOneAngles[side] > 0.0f) {
					oneByOneAngles[side] = MoveToward(oneByOneAngles[side], 0.0f, speed);
					break;
				}
			}
		}
	}
	else if (animationMode == SWAP_OPPOSITE_FACES)
		// 8번의 네 면 모두 180도 회전하면 마주 보는 면끼리 위치가 바뀐다.
		swapAngle = MoveToward(swapAngle, swapped ? 180.0f : 0.0f, speed);
	else if (animationMode == OPEN_ALL_FROM_APEX)
		// 9번은 꼭짓점을 중심으로 네 면을 바깥쪽 90도까지 연다.
		apexAngle = MoveToward(apexAngle, apexOpened ? 90.0f : 0.0f, speed);
}

void DrawScene()
{
	// 사진과 비슷한 거의 검은 남색 배경이다. 네 번째 값 1.0은 불투명도이다.
	glClearColor(0.04f, 0.04f, 0.07f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram(shaderProgramID);
	glBindVertexArray(VAO);
	if (hiddenSurface) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);

	SendTransform(NormalTransform());
	glLineWidth(1.5f);
	DrawAxes();
	for (int side = 0; side < 4; ++side) {
		// 면마다 회전축과 각도가 다르므로 변환을 먼저 보내고 삼각형을 그린다.
		SendTransform(TransformForSide(side));
		DrawTriangleFace(pyramidSides[side], pyramidColors[side]);
	}
	SendTransform(NormalTransform());
	DrawBase();
}

void ResetScene()
{
	animationMode = NO_ANIMATION;
	allBottomAngle = swapAngle = apexAngle = 0.0f;
	for (int i = 0; i < 4; ++i) oneByOneAngles[i] = 0.0f;
	allBottomOpened = oneByOneOpened = swapped = apexOpened = false;
}

void KeyCallback(GLFWwindow* window, int key, int, int action, int)
{
	if (action != GLFW_PRESS) return;
	if (key == GLFW_KEY_6) {
		// bool 값을 반대로 바꾸므로 같은 키를 다시 누르면 닫히기 시작한다.
		animationMode = OPEN_ALL_FROM_BOTTOM;
		allBottomOpened = !allBottomOpened;
	}
	else if (key == GLFW_KEY_7) {
		// 네 면을 한 면씩 정확히 90도까지 여는 동작을 선택한다.
		animationMode = OPEN_ONE_BY_ONE;
		oneByOneOpened = !oneByOneOpened;
	}
	else if (key == GLFW_KEY_8) {
		// 네 면 모두를 180도 돌려 두 쌍의 위치를 동시에 바꾼다.
		animationMode = SWAP_OPPOSITE_FACES;
		swapped = !swapped;
	}
	else if (key == GLFW_KEY_9) {
		// 꼭짓점을 회전 중심으로 사용하는 동작을 선택한다.
		animationMode = OPEN_ALL_FROM_APEX;
		apexOpened = !apexOpened;
	}
	else if (key == GLFW_KEY_H) hiddenSurface = !hiddenSurface;
	else if (key == GLFW_KEY_R) ResetScene();
	else if (key == GLFW_KEY_Q || key == GLFW_KEY_ESCAPE)
		glfwSetWindowShouldClose(window, GLFW_TRUE);
}

void Reshape(GLFWwindow*, int width, int height) { glViewport(0, 0, width, height); }

int main()
{
	if (glfwInit() == GLFW_FALSE) return -1;
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	GLFWwindow* window = glfwCreateWindow(800, 800, "Practice 16", nullptr, nullptr);
	if (window == nullptr) { glfwTerminate(); return -1; }
	glfwMakeContextCurrent(window);
	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK) { glfwDestroyWindow(window); glfwTerminate(); return -1; }

	glfwSetKeyCallback(window, KeyCallback);
	glfwSetFramebufferSizeCallback(window, Reshape);
	glfwSwapInterval(1);
	shaderProgramID = MakeShaderProgram();
	if (shaderProgramID == 0) { glfwDestroyWindow(window); glfwTerminate(); return -1; }
	InitBuffer();
	objectAngleXLocation = glGetUniformLocation(shaderProgramID, "uObjectAngleX");
	objectAngleYLocation = glGetUniformLocation(shaderProgramID, "uObjectAngleY");
	partAngleXLocation = glGetUniformLocation(shaderProgramID, "uPartAngleX");
	partAngleYLocation = glGetUniformLocation(shaderProgramID, "uPartAngleY");
	partAngleZLocation = glGetUniformLocation(shaderProgramID, "uPartAngleZ");
	pivotLocation = glGetUniformLocation(shaderProgramID, "uPivot");

	std::cout << "[실습 16]\n6: 네 면 함께 열기/닫기\n7: 한 면씩 열기/닫기\n";
	std::cout << "8: 네 면 모두 자리 바꾸기\n9: 꼭짓점 기준 열기/닫기\n";
	std::cout << "H: 은면 제거, R: 초기화, Q: 종료\n";

	float previousTime = static_cast<float>(glfwGetTime());
	while (glfwWindowShouldClose(window) == GLFW_FALSE) {
		// deltaTime은 직전 화면을 그린 뒤 현재까지 지나간 시간(초)이다.
		float currentTime = static_cast<float>(glfwGetTime());
		float deltaTime = currentTime - previousTime;
		previousTime = currentTime;
		UpdateScene(deltaTime);
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
