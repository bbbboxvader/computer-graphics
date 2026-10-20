#include <GL/glew.h>
#include <GL/glfw3.h>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

// 실습 18: 중심 구, 행성 3개, 각 행성의 달과 모든 공전 궤도
const float PI = 3.1415926535f; // 도 단위 각도를 라디안으로 바꿀 때 사용하는 원주율
// Vertex는 정점 위치(x,y,z)와 색상(r,g,b)을 VBO에 함께 저장한다.
struct Vertex { float x, y, z, r, g, b; };
// Point3D는 중심 구, 행성, 달의 3차원 위치를 저장한다.
struct Point3D { float x, y, z; };
// Color3의 각 색상 값은 0.0~1.0 범위이다.
struct Color3 { float r, g, b; };

struct Planet
{
	float radius;       // 중심 구에서 행성까지의 거리
	float angle;        // 현재 공전 각도
	float speed;        // 행성 공전 속도
	float tilt;         // 공전 궤도의 기울기
	float size;         // 행성 구의 크기
	Color3 color;       // 행성마다 다른 색상
	float moonAngle;    // 달이 행성 주위를 공전 중인 현재 각도
	float moonSpeed;    // 달의 초당 공전 각도
	float moonRadius;   // 행성 중심에서 달 궤도까지의 거리
};

// shaderProgramID는 셰이더, VAO는 정점 설정, VBO는 정점 데이터의 OpenGL 번호이다.
GLuint shaderProgramID = 0, VAO = 0, VBO = 0;
// Location은 C++의 변환값을 GLSL uniform으로 보내기 위한 위치 번호이다.
GLint positionLocation = -1, scaleLocation = -1;
GLint systemOffsetLocation = -1, alphaLocation = -1, perspectiveLocation = -1;

Planet planets[3];                         // 서로 다른 궤도·속도·색상을 가진 행성 세 개
Point3D systemOffset = { 0,0,0 };          // w/a/s/d와 +/-로 전체 태양계를 이동한 양
bool perspectiveMode = false;              // false: 직각 투영(p), true: 원근 투영(P)
bool solidMode = true;                     // true: 솔리드(m), false: 와이어(M)
bool orbitRadiusAnimation = false;         // 1번 궤도 반지름 애니메이션 실행 여부
bool centerPulseAnimation = false;         // 2번 중심 구 크기·색 애니메이션 실행 여부
bool moonPlaneAnimation = false;           // 3번 달 궤도 회전 실행 여부
float totalTime = 0.0f;                    // sin을 이용한 반복 애니메이션의 누적 시간
float radiusScale = 1.0f;                  // 세 행성 궤도 반지름에 함께 곱할 배율
float centerScale = 1.0f;                  // 중심 구 크기에 곱할 배율
float moonPlaneAngle = 0.0f;               // 달 궤도가 행성 z축 둘레로 회전한 각도
Color3 centerColor = { 0.10f, 0.25f, 1.00f }; // 중심 구의 현재 색상(초기값은 파랑)

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
	GLuint vertex = CompileShader(GL_VERTEX_SHADER, "vertex-18.glsl");
	GLuint fragment = CompileShader(GL_FRAGMENT_SHADER, "fragment-18.glsl");
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
	// 솔리드 구의 많은 삼각형을 담을 수 있도록 정점 12000개분의 공간을 준비한다.
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * 12000, nullptr, GL_DYNAMIC_DRAW);
	// location 0은 위치 x,y,z, location 1은 색상 r,g,b를 읽는다.
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(0));
	glEnableVertexAttribArray(0);
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

void SendTransform(Point3D position, float scale, float alpha)
{
	// 현재 그릴 구/궤도의 개별 위치와 크기, 전체 이동량, 투영 방식을 셰이더로 보낸다.
	// alpha는 불투명도이며 현재 구는 1.0, 궤도선은 0.55를 사용한다.
	glUniform3f(positionLocation, position.x, position.y, position.z);
	glUniform1f(scaleLocation, scale);
	glUniform3f(systemOffsetLocation, systemOffset.x, systemOffset.y, systemOffset.z);
	glUniform1f(alphaLocation, alpha);
	glUniform1i(perspectiveLocation, perspectiveMode ? 1 : 0);
}

Point3D RotateX(Point3D point, float degree)
{
	// sin/cos는 라디안을 사용하므로 도 단위 값에 PI/180을 곱한다.
	float a = degree * PI / 180.0f;
	return {
		point.x,
		point.y * std::cos(a) - point.z * std::sin(a),
		point.y * std::sin(a) + point.z * std::cos(a)
	};
}

Point3D OrbitPoint(float radius, float angle, float tilt)
{
	// 먼저 xz 평면의 원 위 좌표를 만들고, 그 원 전체를 x축으로 tilt만큼 기울인다.
	float a = angle * PI / 180.0f;
	Point3D point = { radius * std::cos(a), 0.0f, radius * std::sin(a) };
	return RotateX(point, tilt);
}

Point3D MoonOffset(float radius, float angle, float planeAngle)
{
	// 화면과 평행한 타원형 궤도를 z축 둘레로 돌린다.
	// 0.45는 원을 화면에서 보기 쉬운 타원 높이로 만드는 비율이다.
	float a = angle * PI / 180.0f;
	float p = planeAngle * PI / 180.0f;
	float x = radius * std::cos(a);
	float y = radius * 0.45f * std::sin(a);
	return { x * std::cos(p) - y * std::sin(p), x * std::sin(p) + y * std::cos(p), 0.0f };
}

void DrawSphereWire(Color3 color)
{
	Vertex vertices[3000];
	int count = 0;
	// 24개의 세로 분할과 12개의 가로 분할을 선으로 연결하여 와이어 구를 만든다.
	const int slices = 24, stacks = 12;
	for (int stack = 1; stack < stacks; ++stack) {
		float latitude = -PI * 0.5f + PI * stack / stacks;
		float y = std::sin(latitude);
		float radius = std::cos(latitude);
		for (int slice = 0; slice < slices; ++slice) {
			float a = 2.0f * PI * slice / slices;
			float b = 2.0f * PI * (slice + 1) / slices;
			vertices[count++] = MakeVertex({ radius * std::cos(a), y, radius * std::sin(a) }, color);
			vertices[count++] = MakeVertex({ radius * std::cos(b), y, radius * std::sin(b) }, color);
		}
	}
	for (int slice = 0; slice < slices; ++slice) {
		float longitude = 2.0f * PI * slice / slices;
		for (int stack = 0; stack < stacks; ++stack) {
			float a = -PI * 0.5f + PI * stack / stacks;
			float b = -PI * 0.5f + PI * (stack + 1) / stacks;
			vertices[count++] = MakeVertex({ std::cos(a) * std::cos(longitude), std::sin(a), std::cos(a) * std::sin(longitude) }, color);
			vertices[count++] = MakeVertex({ std::cos(b) * std::cos(longitude), std::sin(b), std::cos(b) * std::sin(longitude) }, color);
		}
	}
	DrawVertices(GL_LINES, vertices, count);
}

void DrawSphereSolid(Color3 color)
{
	Vertex vertices[4000];
	int count = 0;
	// 같은 24×12 분할을 사용하지만 각 칸을 삼각형 두 개로 채운다.
	const int slices = 24, stacks = 12;
	for (int stack = 0; stack < stacks; ++stack) {
		float latA = -PI * 0.5f + PI * stack / stacks;
		float latB = -PI * 0.5f + PI * (stack + 1) / stacks;
		for (int slice = 0; slice < slices; ++slice) {
			float lonA = 2.0f * PI * slice / slices;
			float lonB = 2.0f * PI * (slice + 1) / slices;
			Point3D p1 = { std::cos(latA)*std::cos(lonA), std::sin(latA), std::cos(latA)*std::sin(lonA) };
			Point3D p2 = { std::cos(latA)*std::cos(lonB), std::sin(latA), std::cos(latA)*std::sin(lonB) };
			Point3D p3 = { std::cos(latB)*std::cos(lonB), std::sin(latB), std::cos(latB)*std::sin(lonB) };
			Point3D p4 = { std::cos(latB)*std::cos(lonA), std::sin(latB), std::cos(latB)*std::sin(lonA) };
			vertices[count++] = MakeVertex(p1, color); vertices[count++] = MakeVertex(p2, color); vertices[count++] = MakeVertex(p3, color);
			vertices[count++] = MakeVertex(p1, color); vertices[count++] = MakeVertex(p3, color); vertices[count++] = MakeVertex(p4, color);
		}
	}
	DrawVertices(GL_TRIANGLES, vertices, count);
}

void DrawSphere(Point3D position, float size, Color3 color)
{
	// m/M으로 선택한 상태에 따라 같은 구를 면 또는 선으로 그린다.
	SendTransform(position, size, 1.0f);
	if (solidMode) DrawSphereSolid(color); else DrawSphereWire(color);
}

void DrawPlanetOrbit(float radius, float tilt, Color3 color)
{
	Vertex vertices[120];
	// 한 궤도를 96개의 점으로 나누면 선이 충분히 둥글게 보인다.
	const int count = 96;
	for (int i = 0; i < count; ++i)
		vertices[i] = MakeVertex(OrbitPoint(radius, 360.0f * i / count, tilt), color);
	SendTransform({ 0,0,0 }, 1.0f, 0.55f);
	DrawVertices(GL_LINE_LOOP, vertices, count);
}

void DrawMoonOrbit(Point3D center, float radius, float planeAngle)
{
	Vertex vertices[80];
	// 달 궤도는 크기가 작으므로 64개의 점으로 나눈다.
	const int count = 64;
	Color3 color = { 0.45f, 0.45f, 0.45f };
	for (int i = 0; i < count; ++i) {
		Point3D offset = MoonOffset(radius, 360.0f * i / count, planeAngle);
		vertices[i] = MakeVertex({ center.x + offset.x, center.y + offset.y, center.z + offset.z }, color);
	}
	SendTransform({ 0,0,0 }, 1.0f, 0.55f);
	DrawVertices(GL_LINE_LOOP, vertices, count);
}

void ResetScene()
{
	// 순서: 궤도 반지름, 시작각, 행성 속도, 궤도 기울기, 크기, 색,
	//       달 시작각, 달 속도, 달 궤도 반지름
	// 첫 궤도는 xz 평면(0도), 두 번째는 +45도, 세 번째는 -45도이다.
	// 행성 속도 34/-23/16과 달 속도 100/-82/68은 방향과 속도를 서로 다르게 만든다.
	planets[0] = { 0.42f,   0.0f,  34.0f,   0.0f, 0.070f, {0.05f,0.85f,0.35f},   0.0f, 100.0f, 0.105f };
	planets[1] = { 0.58f, 120.0f, -23.0f,  45.0f, 0.080f, {0.85f,0.65f,0.10f}, 120.0f, -82.0f, 0.120f };
	planets[2] = { 0.74f, 240.0f,  16.0f, -45.0f, 0.090f, {0.10f,0.75f,0.85f}, 240.0f,  68.0f, 0.135f };
	systemOffset = { 0,0,0 };
	perspectiveMode = false;
	solidMode = true;
	orbitRadiusAnimation = centerPulseAnimation = moonPlaneAnimation = false;
	totalTime = 0.0f; radiusScale = centerScale = 1.0f; moonPlaneAngle = 0.0f;
	centerColor = { 0.10f,0.25f,1.00f };
}

void UpdateScene(float deltaTime)
{
	// 프로그램 실행 후의 시간을 누적하여 반복되는 sin 애니메이션에 사용한다.
	totalTime += deltaTime;
	for (int i = 0; i < 3; ++i) {
		// 각도 += 초당 속도 × 지난 시간: 컴퓨터 성능과 무관한 일정 속도가 된다.
		planets[i].angle += planets[i].speed * deltaTime;
		planets[i].moonAngle += planets[i].moonSpeed * deltaTime;
	}
	if (orbitRadiusAnimation)
		// 0.22는 기본 반지름의 ±22%, 1.8은 커졌다 작아지는 반복 속도이다.
		radiusScale = 1.0f + 0.22f * std::sin(totalTime * 1.8f);
	if (centerPulseAnimation) {
		// sin의 -1~1을 0~1로 바꾼 wave를 크기와 RGB 색상에 함께 사용한다.
		float wave = (std::sin(totalTime * 2.4f) + 1.0f) * 0.5f;
		// 중심 구는 기본 크기의 0.75배에서 1.30배 사이로 반복 변화한다.
		centerScale = 0.75f + wave * 0.55f;
		centerColor = { 0.10f + 0.85f * wave, 0.20f + 0.45f * (1.0f - wave), 1.00f - 0.70f * wave };
	}
	if (moonPlaneAnimation)
		// 달 궤도 전체를 행성의 z축 기준으로 초당 40도씩 회전시킨다.
		moonPlaneAngle += 40.0f * deltaTime;
}

void DrawScene()
{
	// 연한 녹색 배경으로 색상/깊이 버퍼를 지우고 은면 제거를 켠다.
	glClearColor(0.88f, 0.94f, 0.84f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram(shaderProgramID); glBindVertexArray(VAO);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask(GL_TRUE);

	for (int i = 0; i < 3; ++i)
		// 세 궤도 모두 1번 애니메이션의 공통 radiusScale을 적용한다.
		DrawPlanetOrbit(planets[i].radius * radiusScale, planets[i].tilt, {0.42f,0.42f,0.42f});

	Point3D positions[3];
	const Color3 moonColors[3] =
	{
		// 모든 구가 다른 색이 되도록 달 세 개도 빨강/자홍/주황으로 구분한다.
		{ 1.00f, 0.12f, 0.12f },
		{ 0.85f, 0.15f, 0.95f },
		{ 1.00f, 0.50f, 0.08f }
	};
	for (int i = 0; i < 3; ++i) {
		// 행성의 현재 위치를 먼저 구해야 그 위치를 중심으로 달 궤도를 그릴 수 있다.
		positions[i] = OrbitPoint(planets[i].radius * radiusScale, planets[i].angle, planets[i].tilt);
		DrawMoonOrbit(positions[i], planets[i].moonRadius, moonPlaneAngle);
	}

	// 중심 구의 기본 반지름은 0.14이고 2번 동작의 centerScale을 곱한다.
	DrawSphere({0,0,0}, 0.14f * centerScale, centerColor);
	for (int i = 0; i < 3; ++i) {
		DrawSphere(positions[i], planets[i].size, planets[i].color);
		Point3D moon = MoonOffset(planets[i].moonRadius, planets[i].moonAngle, moonPlaneAngle);
		// MoonOffset은 행성 기준 상대 위치이므로 행성의 실제 위치를 더한다.
		moon.x += positions[i].x; moon.y += positions[i].y; moon.z += positions[i].z;
		DrawSphere(moon, 0.026f, moonColors[i]); // 달의 반지름은 0.026
	}
}

void KeyCallback(GLFWwindow* window, int key, int, int action, int mods)
{
	if (action != GLFW_PRESS && action != GLFW_REPEAT) return;
	// 이동키는 누르고 있을 때 반복 처리하고, 모드 전환키는 처음 누른 한 번만 처리한다.
	bool firstPress = action == GLFW_PRESS;
	// p/P와 m/M의 대문자 여부는 Shift 상태로 구분한다.
	bool upper = (mods & GLFW_MOD_SHIFT) != 0;
	// 0.05는 키를 한 번 처리할 때 전체 태양계가 이동하는 거리이다.
	if (key == GLFW_KEY_W) systemOffset.y += 0.05f;
	else if (key == GLFW_KEY_S) systemOffset.y -= 0.05f;
	else if (key == GLFW_KEY_A) systemOffset.x -= 0.05f;
	else if (key == GLFW_KEY_D) systemOffset.x += 0.05f;
	else if (key == GLFW_KEY_EQUAL || key == GLFW_KEY_KP_ADD) systemOffset.z += 0.05f;
	else if (key == GLFW_KEY_MINUS || key == GLFW_KEY_KP_SUBTRACT) systemOffset.z -= 0.05f;
	else if (firstPress && key == GLFW_KEY_P) perspectiveMode = upper; // p: 직각, P: 원근
	else if (firstPress && key == GLFW_KEY_M) solidMode = !upper;      // m: 솔리드, M: 와이어
	else if (firstPress && key == GLFW_KEY_1) orbitRadiusAnimation = !orbitRadiusAnimation; // 궤도 크기 반복/정지
	else if (firstPress && key == GLFW_KEY_2) centerPulseAnimation = !centerPulseAnimation; // 중심 구 변화/정지
	else if (firstPress && key == GLFW_KEY_3) moonPlaneAnimation = !moonPlaneAnimation;     // 달 궤도 회전/정지
	else if (firstPress && key == GLFW_KEY_C) ResetScene();
	else if (firstPress && (key == GLFW_KEY_Q || key == GLFW_KEY_ESCAPE))
		glfwSetWindowShouldClose(window, GLFW_TRUE);
}

void Reshape(GLFWwindow*, int width, int height) { glViewport(0, 0, width, height); }

int main()
{
	if (glfwInit() == GLFW_FALSE) return -1;
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	GLFWwindow* window = glfwCreateWindow(900, 800, "Practice 18", nullptr, nullptr);
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
	systemOffsetLocation = glGetUniformLocation(shaderProgramID, "uSystemOffset");
	alphaLocation = glGetUniformLocation(shaderProgramID, "uAlpha");
	perspectiveLocation = glGetUniformLocation(shaderProgramID, "uPerspective");
	ResetScene();

	std::cout << "[실습 18]\np/P: 직각 투영/원근 투영 전환  m/M: 솔리드/와이어\n";
	std::cout << "w/a/s/d: 전체 이동  +/-: 앞/뒤 이동\n";
	std::cout << "1: 궤도 반지름 변화  2: 중심 구 크기/색 변화  3: 달 궤도 회전\n";
	std::cout << "c: 초기화  q: 종료\n";

	float previousTime = static_cast<float>(glfwGetTime());
	while (glfwWindowShouldClose(window) == GLFW_FALSE) {
		// deltaTime은 이전 프레임부터 현재 프레임까지 지난 시간(초)이다.
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
