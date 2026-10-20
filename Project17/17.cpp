#include <GL/glew.h>
#include <GL/glfw3.h>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

// 실습 17: 화면 좌우의 두 3차원 모델을 변환한다.
const float PI = 3.1415926535f; // 도(degree)를 라디안으로 바꿀 때 사용하는 원주율
// Vertex는 정점의 3차원 위치와 색상을 VBO에 함께 저장하기 위한 구조체이다.
struct Vertex { float x, y, z, r, g, b; };
// Point3D는 물체의 위치나 육면체의 꼭짓점을 저장한다.
struct Point3D { float x, y, z; };
// Color3는 0.0~1.0 범위의 빨강, 초록, 파랑 값이다.
struct Color3 { float r, g, b; };
enum ShapeType { CUBE, SPHERE };
enum SelectedObject { LEFT_ONLY = 1, RIGHT_ONLY = 2, BOTH_OBJECTS = 3 };
enum AnimationMode { STOPPED, ROTATE_X, ROTATE_Y, ORBIT_Y, PASS_ORIGIN, CURVED_SWAP, SCALE_AND_ORBIT };

struct ObjectState
{
	ShapeType shape;          // 현재 도형이 육면체인지 구인지 구분
	Point3D position;         // 현재 도형 중심의 x, y, z 위치
	Point3D startPosition;    // t/u 자리 교환을 시작한 순간의 위치
	float scale;              // 도형의 현재 크기 배율
	float angleX, angleY, angleZ; // 제자리에서 자전한 각도
	Color3 color;             // 도형의 선 색상
};

// shaderProgramID는 셰이더, VAO는 정점 설정, VBO는 정점 데이터의 OpenGL 번호이다.
GLuint shaderProgramID = 0, VAO = 0, VBO = 0;
// Location은 C++의 위치/크기/각도를 GLSL uniform에 보내기 위한 위치 번호이다.
GLint positionLocation = -1, scaleLocation = -1;
GLint angleXLocation = -1, angleYLocation = -1, angleZLocation = -1;
ObjectState objects[2];                        // objects[0]은 왼쪽, objects[1]은 오른쪽
SelectedObject selectedObject = BOTH_OBJECTS; // 1/2/3 중 현재 변환할 대상을 저장
AnimationMode animationMode = STOPPED;        // 현재 실행 중인 자전·공전·자리교환 종류
float animationDirection = 1.0f;              // 1.0은 양의 방향, -1.0은 음의 방향
float animationTime = 0.0f;                   // t/u/v 애니메이션의 진행 시간을 저장

std::string ReadTextFile(const char* fileName)
{
	std::ifstream file(fileName);
	if (!file.is_open()) { std::cerr << "파일을 열 수 없습니다: " << fileName << '\n'; return ""; }
	std::stringstream text; text << file.rdbuf(); return text.str();
}

GLuint CompileShader(GLenum type, const char* fileName)
{
	std::string text = ReadTextFile(fileName);
	if (text.empty()) return 0;
	const char* source = text.c_str();
	GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, &source, nullptr);
	glCompileShader(shader);
	GLint success = GL_FALSE; glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (success == GL_FALSE) {
		char message[1024]; glGetShaderInfoLog(shader, 1024, nullptr, message);
		std::cerr << "셰이더 컴파일 실패: " << fileName << '\n' << message << '\n';
		glDeleteShader(shader); return 0;
	}
	return shader;
}

GLuint MakeShaderProgram()
{
	GLuint vertex = CompileShader(GL_VERTEX_SHADER, "vertex-17.glsl");
	GLuint fragment = CompileShader(GL_FRAGMENT_SHADER, "fragment-17.glsl");
	if (vertex == 0 || fragment == 0) return 0;
	GLuint program = glCreateProgram();
	glAttachShader(program, vertex); glAttachShader(program, fragment); glLinkProgram(program);
	GLint success = GL_FALSE; glGetProgramiv(program, GL_LINK_STATUS, &success);
	if (success == GL_FALSE) {
		char message[1024]; glGetProgramInfoLog(program, 1024, nullptr, message);
		std::cerr << "셰이더 연결 실패\n" << message << '\n';
		glDeleteProgram(program); program = 0;
	}
	glDeleteShader(vertex); glDeleteShader(fragment); return program;
}

void InitBuffer()
{
	glGenVertexArrays(1, &VAO); glBindVertexArray(VAO);
	glGenBuffers(1, &VBO); glBindBuffer(GL_ARRAY_BUFFER, VBO);
	// 구의 위도선과 경도선을 한 번에 담을 수 있도록 정점 4000개 공간을 준비한다.
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * 4000, nullptr, GL_DYNAMIC_DRAW);
	// location 0은 위치 x,y,z 세 값, location 1은 색상 r,g,b 세 값을 읽는다.
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(0));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(sizeof(float) * 3));
	glEnableVertexAttribArray(1);
}

void DrawVertices(GLenum mode, const Vertex vertices[], int count)
{
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vertex) * count, vertices);
	glDrawArrays(mode, 0, count);
}

Vertex MakeVertex(float x, float y, float z, Color3 color)
{
	return { x, y, z, color.r, color.g, color.b };
}

void SendTransform(Point3D position, float scale, float x, float y, float z)
{
	// 현재 그릴 도형 하나의 위치, 크기, 자전각을 정점 셰이더로 보낸다.
	glUniform3f(positionLocation, position.x, position.y, position.z);
	glUniform1f(scaleLocation, scale);
	glUniform1f(angleXLocation, x);
	glUniform1f(angleYLocation, y);
	glUniform1f(angleZLocation, z);
}

void DrawAxes()
{
	// 축은 이동·확대·자전시키지 않으므로 위치 0, 크기 1, 각도 0을 보낸다.
	SendTransform({ 0,0,0 }, 1.0f, 0, 0, 0);
	Vertex axes[6] =
	{
		{ -0.95f,0,0, 1,0.15f,0.15f }, { 0.95f,0,0, 1,0.15f,0.15f },
		{ 0,-0.95f,0, 0.15f,0.8f,0.15f }, { 0,0.95f,0, 0.15f,0.8f,0.15f },
		{ 0,0,-0.95f, 0.15f,0.35f,1 }, { 0,0,0.95f, 0.15f,0.35f,1 }
	};
	DrawVertices(GL_LINES, axes, 6);
}

void DrawCube(Color3 color)
{
	// 각 좌표의 0.5는 한 변의 절반이므로 기본 육면체의 한 변 길이는 1.0이다.
	const Point3D p[8] =
	{
		{-0.5f,-0.5f,-0.5f},{0.5f,-0.5f,-0.5f},{0.5f,0.5f,-0.5f},{-0.5f,0.5f,-0.5f},
		{-0.5f,-0.5f,0.5f},{0.5f,-0.5f,0.5f},{0.5f,0.5f,0.5f},{-0.5f,0.5f,0.5f}
	};
	const int edges[12][2] =
	{
		{0,1},{1,2},{2,3},{3,0},{4,5},{5,6},{6,7},{7,4},{0,4},{1,5},{2,6},{3,7}
	};
	Vertex vertices[24];
	// 모서리 12개마다 시작점과 끝점 두 개가 필요하므로 정점은 24개이다.
	for (int i = 0; i < 12; ++i) {
		vertices[i * 2] = MakeVertex(p[edges[i][0]].x, p[edges[i][0]].y, p[edges[i][0]].z, color);
		vertices[i * 2 + 1] = MakeVertex(p[edges[i][1]].x, p[edges[i][1]].y, p[edges[i][1]].z, color);
	}
	DrawVertices(GL_LINES, vertices, 24);
}

void DrawSphere(Color3 color)
{
	Vertex vertices[2500];
	int count = 0;
	// slices 24는 세로 방향 분할 수, stacks 12는 가로 방향 분할 수이다.
	// 숫자가 클수록 구가 부드럽지만 그릴 선도 많아진다.
	const int slices = 24, stacks = 12;
	for (int stack = 1; stack < stacks; ++stack) {
		float latitude = -PI * 0.5f + PI * stack / stacks;
		float y = std::sin(latitude) * 0.5f;
		float radius = std::cos(latitude) * 0.5f;
		for (int slice = 0; slice < slices; ++slice) {
			float a = 2.0f * PI * slice / slices;
			float b = 2.0f * PI * (slice + 1) / slices;
			vertices[count++] = MakeVertex(radius * std::cos(a), y, radius * std::sin(a), color);
			vertices[count++] = MakeVertex(radius * std::cos(b), y, radius * std::sin(b), color);
		}
	}
	for (int slice = 0; slice < slices; ++slice) {
		float longitude = 2.0f * PI * slice / slices;
		for (int stack = 0; stack < stacks; ++stack) {
			float a = -PI * 0.5f + PI * stack / stacks;
			float b = -PI * 0.5f + PI * (stack + 1) / stacks;
			vertices[count++] = MakeVertex(0.5f * std::cos(a) * std::cos(longitude), 0.5f * std::sin(a), 0.5f * std::cos(a) * std::sin(longitude), color);
			vertices[count++] = MakeVertex(0.5f * std::cos(b) * std::cos(longitude), 0.5f * std::sin(b), 0.5f * std::cos(b) * std::sin(longitude), color);
		}
	}
	DrawVertices(GL_LINES, vertices, count);
}

// index는 0부터 시작하지만 키 번호는 1부터 시작하므로 index + 1과 비교한다.
bool IsSelected(int index) { return selectedObject == BOTH_OBJECTS || selectedObject == index + 1; }

void ResetScene()
{
	// 실습 예시처럼 왼쪽에는 구, 오른쪽에는 육면체를 둔다.
	// -0.55와 +0.55는 두 도형이 화면 중앙을 기준으로 좌우에 놓이게 하는 x좌표이다.
	// 0.48과 0.42는 두 도형이 비슷한 화면 크기로 보이게 정한 초기 크기이다.
	objects[0] = { SPHERE, { -0.55f,0,0 }, { -0.55f,0,0 }, 0.48f, 0,0,0, {0.95f,0.75f,0.15f} };
	objects[1] = { CUBE, { 0.55f,0,0 }, { 0.55f,0,0 }, 0.42f, 0,0,0, {0.1f,0.8f,1.0f} };
	selectedObject = BOTH_OBJECTS;
	animationMode = STOPPED;
	animationDirection = 1.0f;
	animationTime = 0.0f;
}

void BeginSwap(AnimationMode mode)
{
	// t 또는 u를 누른 순간의 위치를 저장해야 도착 지점까지 보간할 수 있다.
	animationMode = mode; animationTime = 0.0f;
	for (int i = 0; i < 2; ++i) objects[i].startPosition = objects[i].position;
}

void RotatePointAroundY(Point3D& point, float degree)
{
	// sin/cos는 라디안을 사용하므로 입력받은 도 단위 각도에 PI/180을 곱한다.
	float angle = degree * PI / 180.0f;
	// 위치의 x,z 값을 회전시키면 물체 자체가 아닌 원점의 y축 주위를 공전한다.
	float x = point.x * std::cos(angle) + point.z * std::sin(angle);
	float z = -point.x * std::sin(angle) + point.z * std::cos(angle);
	point.x = x; point.z = z;
}

void UpdateScene(float deltaTime)
{
	// 75.0은 자전·공전 속도(초당 75도)이다.
	// 대문자이면 animationDirection이 -1이 되어 반대 방향으로 움직인다.
	float degree = 75.0f * animationDirection * deltaTime;
	if (animationMode == ROTATE_X || animationMode == ROTATE_Y || animationMode == ORBIT_Y) {
		for (int i = 0; i < 2; ++i) if (IsSelected(i)) {
			if (animationMode == ROTATE_X) objects[i].angleX += degree;
			if (animationMode == ROTATE_Y) objects[i].angleY += degree;
			if (animationMode == ORBIT_Y) RotatePointAroundY(objects[i].position, degree);
		}
	}
	else if (animationMode == PASS_ORIGIN || animationMode == CURVED_SWAP) {
		// 0.55는 자리 교환 진행 속도이다. animationTime은 0.0에서 1.0까지만 증가한다.
		animationTime = std::min(animationTime + deltaTime * 0.55f, 1.0f);
		for (int i = 0; i < 2; ++i) {
			Point3D start = objects[i].startPosition;
			Point3D end = objects[1 - i].startPosition;
			// 시작점 + (도착점-시작점)*진행률: 두 위치 사이를 부드럽게 이동시키는 식이다.
			objects[i].position.x = start.x + (end.x - start.x) * animationTime;
			objects[i].position.y = start.y + (end.y - start.y) * animationTime;
			objects[i].position.z = start.z + (end.z - start.z) * animationTime;
			if (animationMode == CURVED_SWAP)
				// 0.35는 u 동작의 위/아래 높이이다. sin은 시작과 끝에서 다시 0이 된다.
				objects[i].position.y += (i == 0 ? 0.35f : -0.35f) * std::sin(PI * animationTime);
		}
		if (animationTime >= 1.0f) animationMode = STOPPED;
	}
	else if (animationMode == SCALE_AND_ORBIT) {
		// v 동작은 왼쪽/오른쪽 도형을 서로 반대 방향으로 공전·자전시킨다.
		animationTime += deltaTime;
		for (int i = 0; i < 2; ++i) {
			// 45는 공전 속도, 100은 제자리 y축 자전 속도(초당 각도)이다.
			RotatePointAroundY(objects[i].position, (i == 0 ? 45.0f : -45.0f) * deltaTime);
			objects[i].angleY += (i == 0 ? 100.0f : -100.0f) * deltaTime;
		}
		// 0.35는 기본 크기에서 최대 35%까지 커지거나 작아지는 변화량이다.
		objects[0].scale = 0.48f * (1.0f + 0.35f * std::sin(animationTime * 2.0f));
		objects[1].scale = 0.42f * (1.0f - 0.35f * std::sin(animationTime * 2.0f));
	}
}

void DrawScene()
{
	// 흰 배경으로 색상 버퍼와 깊이 버퍼를 지운다.
	glClearColor(1, 1, 1, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram(shaderProgramID); glBindVertexArray(VAO); glEnable(GL_DEPTH_TEST);
	glLineWidth(1.4f); DrawAxes();
	for (int i = 0; i < 2; ++i) {
		// 배열에 저장된 위치·크기·각도를 보낸 후 shape에 맞는 도형을 그린다.
		ObjectState& object = objects[i];
		SendTransform(object.position, object.scale, object.angleX, object.angleY, object.angleZ);
		if (object.shape == CUBE) DrawCube(object.color); else DrawSphere(object.color);
	}
}

void KeyCallback(GLFWwindow* window, int key, int, int action, int mods)
{
	if (action != GLFW_PRESS) return;
	// GLFW는 x와 X를 같은 키로 전달하므로 Shift가 눌렸는지 따로 검사한다.
	bool upper = (mods & GLFW_MOD_SHIFT) != 0;
	float direction = upper ? -1.0f : 1.0f;
	// 1/2/3은 이후 실행할 변환의 대상만 선택한다.
	if (key == GLFW_KEY_1) selectedObject = LEFT_ONLY;
	else if (key == GLFW_KEY_2) selectedObject = RIGHT_ONLY;
	else if (key == GLFW_KEY_3) selectedObject = BOTH_OBJECTS;
	else if (key == GLFW_KEY_X) { animationMode = ROTATE_X; animationDirection = direction; }
	else if (key == GLFW_KEY_Y) { animationMode = ROTATE_Y; animationDirection = direction; }
	// r/R은 각도가 아닌 position을 회전시켜 중앙 y축 기준 공전이 된다.
	else if (key == GLFW_KEY_R) { animationMode = ORBIT_Y; animationDirection = direction; }
	else if (key == GLFW_KEY_A) {
		// a는 +0.05, A는 -0.05만큼 제자리 크기만 변경한다.
		// 0.08~1.2 제한은 너무 작아 사라지거나 화면보다 커지는 것을 막는다.
		for (int i = 0; i < 2; ++i) if (IsSelected(i))
			objects[i].scale = std::clamp(objects[i].scale + (upper ? -0.05f : 0.05f), 0.08f, 1.2f);
	}
	else if (key == GLFW_KEY_B) {
		// b는 1.1배, B는 0.9배이다. 크기와 위치를 함께 곱해 원점 기준 변환을 만든다.
		float factor = upper ? 0.9f : 1.1f;
		for (int i = 0; i < 2; ++i) if (IsSelected(i)) {
			objects[i].scale = std::clamp(objects[i].scale * factor, 0.08f, 1.2f);
			objects[i].position.x *= factor; objects[i].position.y *= factor; objects[i].position.z *= factor;
		}
	}
	else if (key == GLFW_KEY_D) {
		// d는 왼쪽(-0.08), D는 오른쪽(+0.08)으로 이동한다.
		for (int i = 0; i < 2; ++i) if (IsSelected(i)) objects[i].position.x += upper ? 0.08f : -0.08f;
	}
	else if (key == GLFW_KEY_E) {
		// e는 위(+0.08), E는 아래(-0.08)로 이동한다.
		for (int i = 0; i < 2; ++i) if (IsSelected(i)) objects[i].position.y += upper ? -0.08f : 0.08f;
	}
	else if (key == GLFW_KEY_T) BeginSwap(PASS_ORIGIN); // 두 도형이 직선으로 원점을 지나 자리 교환
	else if (key == GLFW_KEY_U) BeginSwap(CURVED_SWAP); // 하나는 위, 하나는 아래로 휘어 자리 교환
	else if (key == GLFW_KEY_V) animationMode = (animationMode == SCALE_AND_ORBIT) ? STOPPED : SCALE_AND_ORBIT;
	else if (key == GLFW_KEY_C) {
		// CUBE이면 SPHERE로, SPHERE이면 CUBE로 바꾼다.
		for (int i = 0; i < 2; ++i) objects[i].shape = (objects[i].shape == CUBE) ? SPHERE : CUBE;
	}
	else if (key == GLFW_KEY_S) ResetScene();
	else if (key == GLFW_KEY_Q || key == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(window, GLFW_TRUE);
}

void Reshape(GLFWwindow*, int width, int height) { glViewport(0, 0, width, height); }

int main()
{
	if (glfwInit() == GLFW_FALSE) return -1;
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	GLFWwindow* window = glfwCreateWindow(900, 800, "Practice 17", nullptr, nullptr);
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
	positionLocation = glGetUniformLocation(shaderProgramID, "uPosition");
	scaleLocation = glGetUniformLocation(shaderProgramID, "uScale");
	angleXLocation = glGetUniformLocation(shaderProgramID, "uAngleX");
	angleYLocation = glGetUniformLocation(shaderProgramID, "uAngleY");
	angleZLocation = glGetUniformLocation(shaderProgramID, "uAngleZ");
	ResetScene();

	std::cout << "[실습 17]\n1/2/3: 왼쪽/오른쪽/모두 선택\n";
	std::cout << "x/X, y/Y: 자전  r/R: 공전  a/A: 제자리 확대/축소\n";
	std::cout << "b/B: 원점 기준 확대/축소  d/D: 좌/우  e/E: 위/아래\n";
	std::cout << "t: 원점 통과 자리교환  u: 곡선 자리교환  v: 복합 애니메이션\n";
	std::cout << "c: 모델 변경  s: 초기화  q: 종료\n";

	float previousTime = static_cast<float>(glfwGetTime());
	while (glfwWindowShouldClose(window) == GLFW_FALSE) {
		// deltaTime은 이전 프레임부터 현재 프레임까지 지나간 시간(초)이다.
		float currentTime = static_cast<float>(glfwGetTime());
		float deltaTime = currentTime - previousTime;
		previousTime = currentTime;
		UpdateScene(deltaTime);
		DrawScene();
		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	glDeleteBuffers(1, &VBO); glDeleteVertexArrays(1, &VAO); glDeleteProgram(shaderProgramID);
	glfwDestroyWindow(window); glfwTerminate(); return 0;
}
