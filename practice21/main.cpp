// 실습 21: 이 main.cpp와 같은 폴더의 vertex.glsl / fragmet.glsl만 사용한다.
// 필요한 자료형·셰이더 로딩·도형 그리기·상태 갱신·콜백·main을 이 파일 하나에 모았다.
// 외부 include는 C++ 표준 라이브러리와 OpenGL/GLFW/GLEW/GLM 라이브러리뿐이다.

// 1. 라이브러리, 정점 자료형, 변환 함수, 함수 선언
// 공통 개념: 강의의 열 벡터 규약을 사용한다. clip = Projection * View * Model * position.
// GLM의 함수는 항상 glm:: 접두어를 사용하고 단위행렬은 glm::mat4(1.0f)로 명시한다.
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <cctype>
#include <stdexcept>
#include <string>
#include <vector>
#include <iostream>

namespace cg
{
constexpr float pi = 3.14159265358979323846f;

// 실습 15의 GLSL MakeMove/MakeScale과 같은 역할을 CPU의 GLM 함수로 만든다.
inline glm::mat4 MakeMove(glm::vec3 offset)
{
	glm::mat4 matrix(1.0f);
	return glm::translate(matrix, offset);
}

inline glm::mat4 MakeScale(glm::vec3 factors)
{
	glm::mat4 matrix(1.0f);
	return glm::scale(matrix, factors);
}

// 외부 인터페이스의 각도는 도(degree); GLM 호출 시 라디안으로 변환한다.
inline glm::mat4 MakeRotate(float degrees, glm::vec3 axis)
{
	return glm::rotate(glm::mat4(1.0f), glm::radians(degrees), axis);
}

inline glm::vec3 TransformPoint(const glm::mat4 &m, glm::vec3 p)
{
	return glm::vec3(m * glm::vec4(p, 1));
}

inline bool IsShiftPressed(int mods)
{
	return (mods & GLFW_MOD_SHIFT) != 0;
}

inline bool IsPlusKey(int key)
{
	return key == GLFW_KEY_EQUAL || key == GLFW_KEY_KP_ADD;
}

inline bool IsMinusKey(int key)
{
	return key == GLFW_KEY_MINUS || key == GLFW_KEY_KP_SUBTRACT;
}

inline void require(bool condition, const char *message)
{
	if (!condition)
		throw std::runtime_error(message);
}

// 실습 9~17의 Vertex와 같은 순서다. 위치 x/y/z 다음에 색 r/g/b를 저장한다.
struct Vertex
{
	float x, y, z;
	float r, g, b;
};

static_assert(sizeof(Vertex) == 6 * sizeof(float), "Vertex must contain six packed floats");

// [GLSL 기초 2, 17~25/44~46쪽]
// VBO: 정점의 위치·색상 데이터, VAO: 그 데이터를 읽는 연결 설정,
// EBO: VBO의 정점을 어떤 순서로 재사용할지 정하는 인덱스 배열이다.
struct Mesh
{
	GLuint vao = 0, vbo = 0, ebo = 0;
	GLsizei vertexCount = 0, indexCount = 0;
	GLsizeiptr capacityBytes = 0;
	GLenum mode = GL_TRIANGLES;
};

class Renderer
{
	GLuint program = 0;
	GLint modelLoc = -1, viewLoc = -1, projectionLoc = -1, colorLoc = -1, vertexColorLoc = -1;
	std::vector<Mesh> owned;
	void InitShader(); // 외부 GLSL 읽기 -> 컴파일 -> 링크 -> uniform 위치 확인
	void InitBuffer(); // 정점 배열 -> VAO/VBO/EBO -> 속성 0/1 연결
  public:
	Mesh cube, sphere, quad, circle, line;
	glm::mat4 view{1.0f}, projection{1.0f};
	Renderer();
	~Renderer();
	Renderer(const Renderer &) = delete;
	Renderer &operator=(const Renderer &) = delete;
	Mesh mesh(const std::vector<Vertex> &vertices, GLenum mode = GL_TRIANGLES,
	          const std::vector<unsigned int> &indices = {});
	void SendTransform(const glm::mat4 &model, glm::vec3 color, bool useVertexColor = true);
	void draw(const Mesh &, const glm::mat4 &model, glm::vec3 color, bool useVertexColor = true);
	void selfTest(); // GPU의 attribute/EBO 연결과 실제 출력 색을 검증한다.
	void box(const glm::mat4 &parent, glm::vec3 center, glm::vec3 size, glm::vec3 color);
	void ball(const glm::mat4 &parent, glm::vec3 center, float radius, glm::vec3 color);
	void segment(glm::vec3 a, glm::vec3 b, glm::vec3 color);
	void grid(float extent = 8);

	void viewport(int x, int y, int w, int h, glm::vec3 background);
};

// 실습 9~17처럼 각 실습 cpp에서 정의하는 일반 함수들이다.
// 프로젝트마다 실습 cpp가 하나만 연결되므로 함수명이 같아도 충돌하지 않는다.
const char *GetWindowTitle();
void PrintControls();
void ResetScene();
void UpdateScene(float deltaTime);
void HandleKey(int key, int mods);
void DragScene(float deltaX);
void DrawScene(Renderer &renderer, int width, int height);
void SelfTest();
void SmokeStep(int frame);

// 카메라와 투영은 모델 변환과 독립적이다. aspect는 실제 뷰포트 크기로 계산한다.
inline glm::mat4 MakePerspective(float aspect)
{
	return glm::perspective(glm::radians(45.0f), aspect, 0.1f, 150.0f);
}

inline glm::mat4 MakeOrthographic(float halfHeight, float aspect)
{
	return glm::ortho(-halfHeight * aspect, halfHeight * aspect, -halfHeight, halfHeight, 0.1f, 150.0f);
}

// 카메라가 원점 가까이 이동해도 lookAt의 eye==target 특이점에 들어가지 않게 한다.
inline glm::mat4 MakeViewMatrix(glm::vec3 eye, glm::vec3 target = {0, 0, 0}, glm::vec3 up = {0, 1, 0})
{
	if (glm::length(eye - target) < 0.1f)
		eye.z += 0.1f;
	glm::vec3 f = glm::normalize(target - eye);
	if (glm::length(glm::cross(f, up)) < 0.001f)
		up = {0, 0, -1};
	return glm::lookAt(eye, target, up);
}
} // namespace cg

// 2. 셰이더 생성과 VAO/VBO/EBO, 도형 그리기
#include <filesystem>
#include <fstream>
#include <sstream>

namespace cg
{
namespace
{
std::string shaderFile(const wchar_t *name)
{
	wchar_t exe[32768];
	GetModuleFileNameW(nullptr, exe, 32768);
	auto path = std::filesystem::path(exe).parent_path() / name;
	std::ifstream input(path);
	if (!input)
		throw std::runtime_error("Cannot read shaders beside executable. Rebuild the solution.");
	return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

GLuint compileShader(GLenum type, const std::string &source)
{
	// [GLSL 기초 1, 25~34쪽] C++ 코드는 CPU에서 실행되고, 이 문자열의 GLSL은 GPU용으로
	// 실행 시점에 컴파일된다. C++ 빌드 성공과 GLSL 컴파일 성공은 서로 다른 검사다.
	GLuint shader = glCreateShader(type); // 1. 단계별 셰이더 객체 생성
	const char *data = source.c_str();
	glShaderSource(shader, 1, &data, nullptr); // 2. 소스 문자열 복사
	glCompileShader(shader);                   // 3. GPU 드라이버에서 컴파일
	GLint success = GL_FALSE;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (success != GL_TRUE)
	{
		char log[4096];
		glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
		glDeleteShader(shader);
		std::string stage = type == GL_VERTEX_SHADER ? "vertex.glsl" : "fragmet.glsl";
		throw std::runtime_error(stage + " compile failed:\n" + log);
	}
	return shader;
}

void vertex(std::vector<Vertex> &data, glm::vec3 position, glm::vec3 color)
{
	// [GLSL 기초 2, 19/28/31쪽] 정점 하나 = 위치 3개 + 색상 3개, 총 float 6개.
	data.push_back({position.x, position.y, position.z, color.r, color.g, color.b});
}

// 외부 글꼴 파일 없이 실행되도록 사용하는 ASCII 글자만 5x7 비트맵으로 정의한다.

} // namespace

Renderer::Renderer()
{
	// 강의 최종 예제처럼 셰이더 준비와 정점 버퍼 준비를 분리한다.
	InitShader();
	InitBuffer();
}

void Renderer::InitShader()
{
	GLuint vertexShader = compileShader(GL_VERTEX_SHADER, shaderFile(L"vertex.glsl"));
	GLuint fragmentShader = 0;
	try
	{
		fragmentShader = compileShader(GL_FRAGMENT_SHADER, shaderFile(L"fragmet.glsl"));
	}
	catch (...)
	{
		glDeleteShader(vertexShader);
		throw;
	}

	program = glCreateProgram();             // 4. 링크할 프로그램 생성
	glAttachShader(program, vertexShader);   // 5. vertex 단계 연결
	glAttachShader(program, fragmentShader); // 6. fragment 단계 연결
	glLinkProgram(program);                  // 7. 단계 사이 in/out까지 연결
	glDeleteShader(vertexShader);            // 링크 후에는 개별 셰이더 삭제 가능
	glDeleteShader(fragmentShader);

	GLint success = GL_FALSE;
	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if (success != GL_TRUE)
	{
		char log[4096];
		glGetProgramInfoLog(program, sizeof(log), nullptr, log);
		glDeleteProgram(program);
		program = 0;
		throw std::runtime_error(std::string("Shader link failed:\n") + log);
	}
	glUseProgram(program); // 8. 현재 사용할 프로그램 활성화

	// uniform 위치는 '링크가 끝난 프로그램'에서 조회한다. 위치는 음수가 될 수 있어 GLint이다.
	// 같은 프로그램을 다시 링크하지 않는 한 위치를 저장해 두고 draw마다 값만 바꿀 수 있다.
	modelLoc = glGetUniformLocation(program, "modelTransform");
	viewLoc = glGetUniformLocation(program, "viewTransform");
	projectionLoc = glGetUniformLocation(program, "projectionTransform");
	colorLoc = glGetUniformLocation(program, "objectColor");
	vertexColorLoc = glGetUniformLocation(program, "useVertexColor");
	require(modelLoc >= 0 && viewLoc >= 0 && projectionLoc >= 0 && colorLoc >= 0 && vertexColorLoc >= 0,
	        "Required shader uniform missing");
}

void Renderer::InitBuffer()
{
	std::vector<Vertex> data;
	std::vector<unsigned int> indices;
	// 육면체의 각 면은 바깥쪽에서 볼 때 반시계(CCW) 순서이다. 컬링의 기준이 된다.
	const glm::vec3 normals[] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
	const glm::vec3 tangents[] = {{0, 0, -1}, {0, 0, 1}, {1, 0, 0}, {1, 0, 0}, {1, 0, 0}, {-1, 0, 0}};
	const float faceTones[] = {.50f, .60f, 1.f, .30f, .65f, .50f};
	for (int face = 0; face < 6; ++face)
	{
		auto n = normals[face], u = tangents[face], v = glm::cross(n, u);
		glm::vec3 p[] = {0.5f * (n - u - v), 0.5f * (n + u - v), 0.5f * (n + u + v), 0.5f * (n - u + v)};
		unsigned int base = static_cast<unsigned int>(data.size());
		// 조명 계산 대신 면마다 미리 지정한 명도를 정점 색에 저장한다.
		// 같은 좌표라도 면의 색이 다르면 다른 정점이다: 8개가 아닌 6면*4=24개.
		for (auto position : p)
			vertex(data, position, glm::vec3(faceTones[face]));
		for (unsigned int i : {0u, 1u, 2u, 0u, 2u, 3u})
			indices.push_back(base + i);
	}
	cube = mesh(data, GL_TRIANGLES, indices);
	data.clear();
	indices.clear();

	// 구의 원본 정점은 위도/경도 격자에 한 번 저장하고, 옆 칸과 공유하는 점은 EBO로 재사용한다.
	constexpr int stacks = 24, slices = 36;
	for (int i = 0; i <= stacks; ++i)
		for (int j = 0; j <= slices; ++j)
		{
			float latitude = pi * i / stacks, longitude = 2 * pi * j / slices;
			glm::vec3 position{std::sin(latitude) * std::cos(longitude), std::cos(latitude),
			                   std::sin(latitude) * std::sin(longitude)};
			float tone = .72f + .28f * (position.y + 1) * .5f;
			vertex(data, position, glm::vec3(tone));
		}
	for (int i = 0; i < stacks; ++i)
		for (int j = 0; j < slices; ++j)
		{
			unsigned int a = i * (slices + 1) + j, b = a + 1, c = a + slices + 1, d = c + 1;
			indices.insert(indices.end(), {a, b, c, b, d, c});
		}
	sphere = mesh(data, GL_TRIANGLES, indices);
	data.clear();
	indices.clear();

	// [GLSL 기초 2, 44~45쪽] 사각형: VBO 정점 4개 + EBO 인덱스 6개로 삼각형 두 개.
	for (auto p :
	     {glm::vec3(-.5f, -.5f, 0), glm::vec3(.5f, -.5f, 0), glm::vec3(.5f, .5f, 0), glm::vec3(-.5f, .5f, 0)})
		vertex(data, p, {1, 1, 1});
	quad = mesh(data, GL_TRIANGLES, {0, 1, 2, 0, 2, 3});
	data.clear();
	// 원 궤도/직선은 정점을 순서대로 사용하므로 EBO 없이 glDrawArrays로 충분하다.
	for (int i = 0; i < 160; ++i)
	{
		float a = 2 * pi * i / 160;
		vertex(data, {std::cos(a), std::sin(a), 0}, {1, 1, 1});
	}
	circle = mesh(data, GL_LINE_LOOP);
	data.clear();
	vertex(data, {0, 0, 0}, {1, 1, 1});
	vertex(data, {1, 0, 0}, {1, 1, 1});
	line = mesh(data, GL_LINES);
}

Renderer::~Renderer()
{
	// GPU 객체의 이름(id)을 보유한 쪽에서 한 번만 해제한다. context 파괴 전에 실행된다.
	for (auto &m : owned)
	{
		if (m.ebo)
			glDeleteBuffers(1, &m.ebo);
		glDeleteBuffers(1, &m.vbo);
		glDeleteVertexArrays(1, &m.vao);
	}
	glDeleteProgram(program);
}

Mesh Renderer::mesh(const std::vector<Vertex> &data, GLenum mode, const std::vector<unsigned int> &indices)
{
	for (auto index : indices)
		require(index < data.size(), "EBO index exceeds vertex count");
	Mesh m;
	m.vertexCount = static_cast<GLsizei>(data.size());
	m.indexCount = static_cast<GLsizei>(indices.size());
	m.capacityBytes = static_cast<GLsizeiptr>(data.size() * sizeof(Vertex));
	m.mode = mode;

	// 1. VAO를 '현재 설정할 대상'으로 바인드한다. VAO 자체에 정점이 복사되는 것은 아니다.
	glGenVertexArrays(1, &m.vao);
	glBindVertexArray(m.vao);
	// 2. VBO에 CPU 배열을 복사한다. glGenBuffers는 이름을 만들고 glBufferData가 저장 공간을 만든다.
	glGenBuffers(1, &m.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
	glBufferData(GL_ARRAY_BUFFER, m.capacityBytes, data.data(), GL_STATIC_DRAW);
	// 3. 강의 예제의 [x y z | r g b]. 한 정점의 stride=24바이트, 색상 offset=12바이트.
	// location=0/1은 vertex.glsl과 맞춘다. GL_FLOAT 값은 그대로 사용하므로 normalized=GL_FALSE.
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
	                      reinterpret_cast<void *>(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	// 4. 인덱스가 있으면 EBO를 이 VAO에 연결한다. 이후 VAO를 바인드하면 EBO도 복원된다.
	if (!indices.empty())
	{
		glGenBuffers(1, &m.ebo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m.ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(),
		             GL_STATIC_DRAW);
	}
	// VAO를 먼저 해제한다. 활성 VAO 상태에서 EBO를 0으로 바꾸면 그 VAO의 연결까지 지워진다.
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	owned.push_back(m);
	return m;
}

void Renderer::SendTransform(const glm::mat4 &model, glm::vec3 color, bool useVertexColor)
{
	// 실습 13~17의 SendTransform처럼 '현재 그릴 물체 하나'의 변환을 보내는 함수다.
	// 이전 실습의 개별 위치/각도 uniform을 이제 완성된 mat4 model/view/projection으로 보낸다.
	// [GLSL 기초 2, 36/41/45쪽] 프로그램 활성화 -> uniform 설정 -> VAO 선택 -> 그리기.
	glUseProgram(program);
	// 개념: uniform으로 CPU의 행렬을 GPU 정점 셰이더에 전달한다.
	// GLM과 OpenGL은 모두 열 우선 저장이므로 transpose=GL_FALSE로 보낸다.
	// 매 물체마다 model을 새로 보내야 앞 물체의 변환이 다음 물체에 누적되지 않는다.
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
	// '3fv': float 3성분 벡터의 배열 주소, count=1은 vec3 한 개를 보낸다는 의미이다.
	glUniform3fv(colorLoc, 1, glm::value_ptr(color));
	glUniform1i(vertexColorLoc, useVertexColor ? 1 : 0);
}

void Renderer::draw(const Mesh &m, const glm::mat4 &model, glm::vec3 color, bool useVertexColor)
{
	SendTransform(model, color, useVertexColor);
	glBindVertexArray(m.vao);
	if (m.ebo != 0)
	{
		// count는 고유 정점 수가 아니라 인덱스 수. nullptr는 EBO 시작 위치의 바이트 offset=0.
		glDrawElements(m.mode, m.indexCount, GL_UNSIGNED_INT, nullptr);
	}
	else
	{
		// 배열의 0번 정점부터 vertexCount개를 순서대로 사용한다.
		glDrawArrays(m.mode, 0, m.vertexCount);
	}
	glBindVertexArray(0);
}

void Renderer::selfTest()
{
	// 새 정점 형식을 실제 GPU 상태와 출력 픽셀로 검사한다.
	// 아래 검사는 smoke-test에서만 실행되며 일반 실행의 장면에는 추가 도형을 그리지 않는다.
	for (const Mesh *m : {&cube, &sphere, &quad})
	{
		glBindVertexArray(m->vao);
		GLint eboBinding = 0, bufferSize = 0;
		glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &eboBinding);
		require(static_cast<GLuint>(eboBinding) == m->ebo, "VAO did not restore its EBO");
		glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &bufferSize);
		require(bufferSize == m->indexCount * static_cast<GLint>(sizeof(unsigned int)),
		        "EBO byte size mismatch");
		for (GLuint attribute : {0u, 1u})
		{
			GLint enabled = 0, stride = 0, binding = 0;
			glGetVertexAttribiv(attribute, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &enabled);
			glGetVertexAttribiv(attribute, GL_VERTEX_ATTRIB_ARRAY_STRIDE, &stride);
			glGetVertexAttribiv(attribute, GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING, &binding);
			require(enabled == GL_TRUE && stride == sizeof(Vertex) && static_cast<GLuint>(binding) == m->vbo,
			        "Position/color attribute is not connected to the VBO");
		}
	}
	require(cube.vertexCount == 24 && cube.indexCount == 36,
	        "Cube must use 24 colored vertices / 36 indices");
	require(quad.vertexCount == 4 && quad.indexCount == 6, "Quad EBO did not reuse shared vertices");
	require(circle.ebo == 0 && circle.vertexCount == 160, "Orbit must use non-indexed line loop");
	glBindVertexArray(0);

	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);
	auto oldView = view, oldProjection = projection;
	GLboolean depthEnabled = glIsEnabled(GL_DEPTH_TEST), cullEnabled = glIsEnabled(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	view = glm::mat4(1);
	projection = glm::mat4(1);
	auto expectColor = [&](int red, int green, int blue) {
		unsigned char rgba[4] = {};
		glReadPixels(viewport[0] + viewport[2] / 2, viewport[1] + viewport[3] / 2, 1, 1, GL_RGBA,
		             GL_UNSIGNED_BYTE, rgba);
		require(std::abs(int(rgba[0]) - red) <= 3 && std::abs(int(rgba[1]) - green) <= 3 &&
		            std::abs(int(rgba[2]) - blue) <= 3,
		        "Framebuffer color differs from attribute/uniform shader output");
	};
	glClear(GL_COLOR_BUFFER_BIT);
	draw(quad, glm::mat4(1), {.2f, .4f, .6f}); // EBO 경로와 흰 정점 색 * uniform.
	expectColor(51, 102, 153);

	Mesh triangle = mesh({{-.8f, -.8f, 0, 1, 0, 0}, {.8f, -.8f, 0, 0, 1, 0}, {0, .8f, 0, 0, 0, 1}});
	glClear(GL_COLOR_BUFFER_BIT);
	draw(triangle, glm::mat4(1), {1, 1, 1}); // DrawArrays + RGB 정점 색의 out/in 보간.
	expectColor(64, 64, 128);
	glClear(GL_COLOR_BUFFER_BIT);
	draw(triangle, glm::mat4(1), {.6f, .2f, .4f}, false); // uniform만 사용하는 경로.
	expectColor(153, 51, 102);

	// 화면 문자를 그리지 않고, 시험용 삼각형 VBO의 내용만 갱신해 확인한다.
	Vertex updated[3] = {
	    {-.8f, -.8f, 0, .1f, .7f, .2f}, {.8f, -.8f, 0, .1f, .7f, .2f}, {0, .8f, 0, .1f, .7f, .2f}};
	glBindBuffer(GL_ARRAY_BUFFER, triangle.vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(updated), nullptr, GL_DYNAMIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(updated), updated);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glClear(GL_COLOR_BUFFER_BIT);
	draw(triangle, glm::mat4(1), {1, 1, 1});
	expectColor(26, 179, 51);
	view = oldView;
	projection = oldProjection;
	if (depthEnabled)
		glEnable(GL_DEPTH_TEST);
	if (cullEnabled)
		glEnable(GL_CULL_FACE);
	require(glGetError() == GL_NO_ERROR, "OpenGL error in renderer self-test");
}

void Renderer::box(const glm::mat4 &parent, glm::vec3 center, glm::vec3 size, glm::vec3 color)
{
	draw(cube, parent * MakeMove(center) * MakeScale(size), color);
}

void Renderer::ball(const glm::mat4 &parent, glm::vec3 center, float radius, glm::vec3 color)
{
	draw(sphere, parent * MakeMove(center) * MakeScale(glm::vec3(radius)), color, false);
}

void Renderer::segment(glm::vec3 a, glm::vec3 b, glm::vec3 color)
{
	glm::vec3 delta = b - a;
	if (glm::length(delta) < .00001f)
		return;
	glm::vec3 axis = glm::normalize(delta);
	glm::vec3 reference = std::abs(axis.y) > .99f ? glm::vec3(0, 0, 1) : glm::vec3(0, 1, 0);
	glm::vec3 z = glm::normalize(glm::cross(axis, reference)), y = glm::cross(z, axis);
	glm::mat4 m(1);
	m[0] = glm::vec4(delta, 0);
	m[1] = glm::vec4(y, 0);
	m[2] = glm::vec4(z, 0);
	m[3] = glm::vec4(a, 1);
	draw(line, m, color, false);
}

void Renderer::grid(float extent)
{
	// 실습 그림처럼 장식 격자 없이 xz 평면의 바닥만 그린다.
	draw(quad, MakeMove({0, -.005f, 0}) * MakeRotate(-90, {1, 0, 0}) * MakeScale({extent * 2, extent * 2, 1}),
	     {.65f, .75f, .84f}, false);
}

void Renderer::viewport(int x, int y, int w, int h, glm::vec3 background)
{
	// 뷰포트는 NDC를 지정한 픽셀 사각형에 매핑한다. scissor는 그 화면의 clear 범위를 제한한다.
	glViewport(x, y, w, h);
	glEnable(GL_SCISSOR_TEST);
	glScissor(x, y, w, h);
	glClearColor(background.r, background.g, background.b, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_SCISSOR_TEST);
}
} // namespace cg

// 4. 이 실습의 상태, 초기화, 애니메이션, 키 입력, 장면 그리기
#include <random>

namespace cg
{
// [기초 개념 적용] 평면/육면체/구의 VAO를 바인드하면 그 VAO의 속성 설정과 EBO가 복원된다.
// 벽마다 다른 objectColor uniform을 보내고, 벽은 useVertexColor=0으로 단색을 사용한다.
// 구는 그림의 파란 단색을 따르므로 objectColor uniform만 적용한다. 정점/색상 입력 형식은 유지한다.
// 마우스는 버튼/이동 콜백을 함께 사용한다. 현재 x와 이전 x의 차이가 drag(dx)에 전달된다.
// glCullFace(GL_FRONT)는 '앞쪽 벽 전체 삭제'가 아니라 삼각형의 앞면 제거 설정이다.
// 도형 그리기 순서와 무관하게 깊이 버퍼가 은면을 처리하며 매 프레임 깊이도 초기화한다.
struct BounceBall
{
	glm::vec3 position, velocity, color;
	float radius = .35f;
};

struct FloorBox
{
	glm::vec3 position, size, color;
	float vx = 0, vy = 0;
};

// 실습 9~17과 같은 함수 중심 구성이다.
// 아래 전역 변수/구조체가 상태를 저장하고, ResetScene -> UpdateScene -> DrawScene 순서로 처리한다.
// HandleKey는 공통 main.cpp의 KeyCallback에서 호출한다. 클래스 상속이나 가상 함수는 사용하지 않는다.

const int MAX_BALL_COUNT = 5;
const int BOX_COUNT = 3;
const float ROOM_HEIGHT = 10.f, ROOM_HALF_DEPTH = 7.f;
BounceBall balls[MAX_BALL_COUNT];
int ballCount = 0; // 실습 10~12의 pieceCount/stackedCount처럼 사용 중인 칸만 센다.
FloorBox boxes[BOX_COUNT];
float tilt = 0, sceneYaw = 0, sceneZ = 0, door = 0;
bool open = false;
std::mt19937 rng{21};

void AddBall()
{
	if (ballCount >= MAX_BALL_COUNT)
		return;
	std::uniform_real_distribution<float> d(-1, 1);
	const float radii[] = {.35f, .28f, .32f, .27f, .30f};
	const float initialX[] = {1.1f, .5f, -1.f, 2.5f, -1.8f};
	const float initialY[] = {7.8f, 6.8f, 4.f, 3.2f, 5.2f};
	balls[ballCount] = {{initialX[ballCount] + d(rng) * .3f, initialY[ballCount] + d(rng) * .3f, d(rng) * 2},
	                    {2.5f + d(rng), 2 + d(rng), 2.3f + d(rng)},
	                    {.18f, .12f, 1.f},
	                    radii[ballCount]};
	++ballCount;
}

static void ReflectWall(float &p, float &v, float lo, float hi)
{
	if (p < lo)
	{
		p = lo;
		v = std::abs(v);
	}
	if (p > hi)
	{
		p = hi;
		v = -std::abs(v);
	}
}

static void CollideBox(BounceBall &ball, const FloorBox &box)
{
	// 구-AABB 충돌: 상자에서 구 중심에 가장 가까운 점을 clamp로 찾는다.
	// 거리 < 구 반지름이면 충돌이며, 접촉 법선으로 v'=v-2(v.n)n을 적용한다.
	glm::vec3 half = box.size * .5f, relative = ball.position - box.position;
	glm::vec3 nearest = glm::clamp(relative, -half, half), delta = relative - nearest;
	float length = glm::length(delta);
	if (length >= ball.radius)
		return;
	glm::vec3 normal;
	float penetration;
	if (length > .0001f)
	{
		normal = delta / length;
		penetration = ball.radius - length;
	}
	else
	{
		glm::vec3 depth = half - glm::abs(relative);
		int axis = depth.x < depth.y ? 0 : 1;
		if (depth.z < depth[axis])
			axis = 2;
		normal = glm::vec3(0);
		normal[axis] = relative[axis] >= 0 ? 1.f : -1.f;
		penetration = ball.radius + depth[axis];
	}
	ball.position += normal * (penetration + .001f);
	float approach = glm::dot(ball.velocity, normal);
	if (approach < 0)
		ball.velocity -= 2 * approach * normal;
}

void UpdatePhysics(float deltaTime)
{
	// 방의 로컬 좌표에서 충돌을 처리하면 기울어진 벽도 단순한 min/max로 검사할 수 있다.
	// 중력 방향만 방 회전의 역변환으로 로컬 공간에 옮긴다(w=0: 방향은 이동의 영향 없음).
	glm::vec3 gravity = glm::vec3(MakeRotate(-tilt, {0, 0, 1}) * glm::vec4(0, -9.8f, 0, 0));
	bool floorGone = door > 35;
	for (auto &b : boxes)
	{
		b.vx += gravity.x * deltaTime;
		b.vx *= std::exp(-1.2f * deltaTime);
		b.position.x += b.vx * deltaTime;
		ReflectWall(b.position.x, b.vx, -5 + b.size.x * .5f, 5 - b.size.x * .5f);
		if (floorGone)
		{
			b.vy += gravity.y * deltaTime;
			b.position.y += b.vy * deltaTime;
		}
		else if (b.position.y >= b.size.y * .5f - .05f)
		{
			b.position.y = b.size.y * .5f;
			b.vy = 0;
		}
	}
	for (int ballIndex = 0; ballIndex < ballCount; ++ballIndex)
	{
		BounceBall &b = balls[ballIndex];
		if (floorGone)
			b.velocity += gravity * deltaTime;
		b.position += b.velocity * deltaTime;
		ReflectWall(b.position.x, b.velocity.x, -5 + b.radius, 5 - b.radius);
		// 앞면은 보이도록 그리지 않지만, 공을 공간 안에 유지하는 가상 충돌 경계는 둔다.
		ReflectWall(b.position.z, b.velocity.z, -ROOM_HALF_DEPTH + b.radius, ROOM_HALF_DEPTH - b.radius);
		if (!floorGone && b.position.y >= -.1f)
			ReflectWall(b.position.y, b.velocity.y, b.radius, ROOM_HEIGHT - b.radius);
		else if (b.position.y > ROOM_HEIGHT - b.radius)
		{
			b.position.y = ROOM_HEIGHT - b.radius;
			b.velocity.y = -std::abs(b.velocity.y);
		}
		for (auto &box : boxes)
			if (box.position.y > -4)
				CollideBox(b, box);
	}
}

const char *GetWindowTitle()
{
	return "Practice 21 - 3D bouncing balls";
}

void ResetScene()
{
	tilt = sceneYaw = sceneZ = door = 0;
	open = false;
	ballCount = 0;
	rng.seed(21);
	AddBall();
	AddBall();
	AddBall();
	boxes[0] = {{0, .4f, -3.4f}, {.8f, .8f, .8f}, {1, .10f, .08f}};
	boxes[1] = {{0, .5f, 0}, {1, 1, 1}, {1, .10f, .08f}};
	boxes[2] = {{0, .6f, 3.4f}, {1.2f, 1.2f, 1.2f}, {1, .10f, .08f}};
}

void UpdateScene(float deltaTime)
{
	door += std::clamp((open ? 100.f : 0.f) - door, -90 * deltaTime, 90 * deltaTime);
	// 물리 소단계를 사용해 빠른 공이 얇은 충돌 영역을 한 프레임에 통과하지 않도록 한다.
	int count = std::max(1, static_cast<int>(std::ceil(deltaTime * 240)));
	for (int i = 0; i < count; ++i)
		UpdatePhysics(deltaTime / count);
}

void HandleKey(int key, int mods)
{
	if (key == GLFW_KEY_B)
		AddBall(); // B는 Shift 여부와 무관하게 허용한다.
	if (key == GLFW_KEY_Z)
		sceneZ += IsShiftPressed(mods) ? -.35f : .35f;
	if (key == GLFW_KEY_Y)
		sceneYaw += IsShiftPressed(mods) ? 8.f : -8.f;
	if (key == GLFW_KEY_O)
		open = !open;
	if (key == GLFW_KEY_C)
		ResetScene();
}

void DragScene(float dx)
{
	tilt = std::clamp(tilt - dx * .25f, -65.f, 65.f);
}

void DrawScene(Renderer &renderer, int w, int h)
{
	renderer.view = MakeViewMatrix({0, 5, 24}, {0, 5, 0});
	renderer.projection = MakePerspective(float(w) / h);
	glm::mat4 room = MakeMove({0, 0, sceneZ}) * MakeRotate(sceneYaw, {0, 1, 0}) * MakeRotate(tilt, {0, 0, 1});
	// 다섯 면은 서로 다른 회색 RGB값이다. 바깥쪽 앞면을 컬링하여 내부만 보인다.
	glCullFace(GL_FRONT);
	renderer.draw(renderer.quad,
	              room * MakeMove({-5, 5, 0}) * MakeRotate(-90, {0, 1, 0}) * MakeScale({14, 10, 1}),
	              {.30f, .30f, .31f}, false);
	renderer.draw(renderer.quad,
	              room * MakeMove({5, 5, 0}) * MakeRotate(90, {0, 1, 0}) * MakeScale({14, 10, 1}),
	              {.40f, .40f, .42f}, false);
	renderer.draw(renderer.quad,
	              room * MakeMove({0, 10, 0}) * MakeRotate(-90, {1, 0, 0}) * MakeScale({10, 14, 1}),
	              {.36f, .36f, .38f}, false);
	renderer.draw(renderer.quad,
	              room * MakeMove({0, 5, -7}) * MakeRotate(180, {0, 1, 0}) * MakeScale({10, 10, 1}),
	              {.70f, .70f, .72f}, false);
	for (int side : {-1, 1})
	{
		glm::mat4 panel = room * MakeMove({side * 5.f, 0, 0}) * MakeRotate(side * door, {0, 0, 1}) *
		                  MakeMove({-side * 2.5f, 0, 0});
		renderer.draw(renderer.quad, panel * MakeRotate(90, {1, 0, 0}) * MakeScale({5, 14, 1}),
		              {.50f, .50f, .52f}, false);
	}
	glCullFace(GL_BACK);
	for (auto &box : boxes)
		renderer.box(room, box.position, box.size, box.color);
	for (int i = 0; i < ballCount; ++i)
		renderer.ball(room, balls[i].position, balls[i].radius, balls[i].color);
}

void SelfTest()
{
	ResetScene();
	for (int i = 0; i < 9; ++i)
		AddBall();
	require(ballCount == MAX_BALL_COUNT, "Ball limit failed");
	for (int i = 0; i < 1800; ++i)
		UpdateScene(1.f / 120);
	for (int ballIndex = 0; ballIndex < ballCount; ++ballIndex)
	{
		BounceBall &b = balls[ballIndex];
		require(std::abs(b.position.x) <= 5 - b.radius + .001f && b.position.y >= b.radius - .001f &&
		            b.position.y <= ROOM_HEIGHT - b.radius + .001f &&
		            std::abs(b.position.z) <= ROOM_HALF_DEPTH - b.radius + .001f,
		        "Ball escaped closed room");
	}
	ResetScene();
	DragScene(120);
	for (int i = 0; i < 240; ++i)
		UpdateScene(1.f / 120);
	require(std::abs(boxes[0].position.x) > .1f, "Boxes did not slide with room tilt");
	ResetScene();
	HandleKey(GLFW_KEY_O, 0);
	for (int i = 0; i < 480; ++i)
		UpdateScene(1.f / 120);
	require(boxes[0].position.y < -3 && balls[0].position.y < 0, "Open floor did not release objects");
	ResetScene();
}

void SmokeStep(int f)
{
	if (f == 0)
	{
		HandleKey(GLFW_KEY_B, GLFW_MOD_SHIFT);
		HandleKey(GLFW_KEY_B, GLFW_MOD_SHIFT);
		DragScene(65);
	}
	if (f == 90)
		HandleKey(GLFW_KEY_Y, 0);
}

void PrintControls()
{
	std::cout << "[실습 21: 3차원 공 튕기기]\n";
	std::cout << "B: 새 공 추가(최대 5개), z/Z: 장면 이동, y/Y: 회전\n";
	std::cout << "왼쪽 마우스 드래그: 방 z축 기울이기, o: 바닥 열기/닫기\n";
	std::cout << "c: 초기화, q 또는 Esc: 종료\n";
	std::cout << "대문자는 Shift와 함께 누릅니다.\n";
}
} // namespace cg

// 5. GLFW 콜백과 프로그램 시작점 main
#include <filesystem>
#include <fstream>
#include <iostream>

namespace
{
bool dragging = false;
double previousX = 0;

void KeyCallback(GLFWwindow *w, int key, int, int action, int mods)
{
	// [OpenGL 기초, 35~37쪽] GLFW가 PollEvents에서 호출한다. 직접 매 프레임 호출하지 않는다.
	// PRESS=눌림, REPEAT=누른 상태의 반복, RELEASE=떼기. 토글은 PRESS 한 번으로 처리한다.
	if (action != GLFW_PRESS && action != GLFW_REPEAT)
		return;
	if (key == GLFW_KEY_Q)
		glfwSetWindowShouldClose(w, GL_TRUE);
	// 토글 명령은 길게 눌러도 한 번만 바뀌게 하고, 방향키 이동은 반복 입력을 허용한다.
	if (action == GLFW_REPEAT && key != GLFW_KEY_LEFT && key != GLFW_KEY_RIGHT && key != GLFW_KEY_UP &&
	    key != GLFW_KEY_DOWN && !cg::IsPlusKey(key) && !cg::IsMinusKey(key))
		return;
	cg::HandleKey(key, mods);
}

void ProcessInput(GLFWwindow *window)
{
	// [OpenGL 기초, 23/26/28쪽] 폴링: 프로그램이 현재 키 상태를 매 프레임 조회한다.
	// Esc는 폴링, 실습의 명령키와 드래그는 콜백으로 처리해 두 방식을 비교한다.
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
}

void Reshape(GLFWwindow *, int width, int height)
{
	// [OpenGL 기초, 42~43/51쪽] 창의 논리 크기가 아닌 실제 픽셀 크기를 사용한다.
	// 투영 aspect는 render가 다시 계산한다. 0 높이일 때는 메인 루프에서 그리기를 건너뛴다.
	glViewport(0, 0, width, height);
}

void MouseButtonCallback(GLFWwindow *window, int button, int action, int)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT)
	{
		dragging = action == GLFW_PRESS;
		double y;
		glfwGetCursorPos(window, &previousX, &y); // 누른 순간의 좌표를 저장해 첫 드래그가 튀지 않게 한다.
	}
}

void CursorPositionCallback(GLFWwindow *, double x, double)
{
	// [OpenGL 기초, 32/38~39쪽] 화면 좌표 그 자체 대신 이전 좌표와의 차이를 회전량에 쓴다.
	if (dragging)
		cg::DragScene(float(x - previousX));
	previousX = x;
}

void SaveFrame(const std::filesystem::path &path, int width, int height)
{
	std::vector<unsigned char> rgb(static_cast<size_t>(width) * height * 3);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glReadBuffer(GL_BACK);
	glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, rgb.data());
	std::ofstream output(path, std::ios::binary);
	if (!output)
		throw std::runtime_error("Cannot write capture");
	output << "P6\n" << width << ' ' << height << "\n255\n";
	// OpenGL 원점은 왼쪽 아래, 일반 이미지 원점은 왼쪽 위이므로 행을 뒤집어 저장한다.
	for (int y = height - 1; y >= 0; --y)
		output.write(reinterpret_cast<char *>(rgb.data() + static_cast<size_t>(y) * width * 3), width * 3);
}
} // namespace

int main(int argc, char **argv)
{
	SetConsoleOutputCP(CP_UTF8);
	GLFWwindow *window = nullptr;
	try
	{
		cg::ResetScene();
		bool self = false, smoke = false;
		int frames = 0;
		std::filesystem::path capture;
		for (int i = 1; i < argc; ++i)
		{
			std::string arg = argv[i];
			if (arg == "--self-test")
				self = true;
			else if (arg == "--smoke-test")
				smoke = true;
			else if (arg == "--frames" && i + 1 < argc)
				frames = std::stoi(argv[++i]);
			else if (arg == "--capture" && i + 1 < argc)
				capture = std::filesystem::u8path(argv[++i]);
		}
		if (self)
		{
			cg::SelfTest();
			std::cout << cg::GetWindowTitle() << ": self-test PASS\n";
			return 0;
		}
		glfwSetErrorCallback(
		    [](int code, const char *message) { std::cerr << "GLFW " << code << ": " << message << '\n'; });
		// [1] GLFW: 창/입력/컨텍스트를 담당한다. OpenGL은 그 컨텍스트 안에서 그리기를 담당한다.
		if (!glfwInit())
			throw std::runtime_error("GLFW initialization failed");
		// [2] 강의의 OpenGL 3.3 Core 설정. 생성 전 hint를 지정해야 새 창에 적용된다.
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
		glfwWindowHint(GLFW_DEPTH_BITS, 24);
		if (frames > 0)
			glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
		// [3] 창 생성 -> 현재 스레드에 컨텍스트 활성화 -> GLEW 초기화 순서를 지킨다.
		// GLEW는 현재 드라이버에서 OpenGL 함수 주소를 얻는다. 컨텍스트보다 먼저 호출할 수 없다.
		window = glfwCreateWindow(960, 720, cg::GetWindowTitle(), nullptr, nullptr);
		if (!window)
			throw std::runtime_error("OpenGL 3.3 context creation failed");
		glfwMakeContextCurrent(window);
		glewExperimental = GL_TRUE;
		if (glewInit() != GLEW_OK)
			throw std::runtime_error("GLEW initialization failed");
		while (glGetError() != GL_NO_ERROR)
		{
		} // GLEW의 core-context 탐색 시 남는 오류를 비운다.
		// [OpenGL 기초, 46쪽] 보통 VSync=1로 버퍼 교환을 화면 갱신에 맞춘다.
		glfwSwapInterval(frames > 0 ? 0 : 1);
		std::cout << "OpenGL " << glGetString(GL_VERSION) << "\n" << cg::GetWindowTitle() << "\n";
		cg::PrintControls();
		// [4] 콜백 함수 '등록': 실행은 이벤트가 들어왔을 때 GLFW가 한다.
		glfwSetKeyCallback(window, KeyCallback);
		glfwSetFramebufferSizeCallback(window, Reshape);
		glfwSetMouseButtonCallback(window, MouseButtonCallback);
		glfwSetCursorPosCallback(window, CursorPositionCallback);
		{
			// [5] InitShader -> InitBuffer: GLSL을 연결하고 VBO/VAO/EBO의 연결 정보를 준비한다.
			cg::Renderer renderer;
			// 은면 제거: z-buffer에 더 가까운 조각만 남긴다. 매 프레임 깊이 버퍼도 지워야 한다.
			glEnable(GL_DEPTH_TEST);
			glDepthFunc(GL_LESS);
			// 컬링: 기본은 바깥에서 보이지 않는 뒷면 제거. 방 내부는 각 실습에서 반대로 설정한다.
			glEnable(GL_CULL_FACE);
			glFrontFace(GL_CCW);
			glCullFace(GL_BACK);
			int initialWidth, initialHeight;
			glfwGetFramebufferSize(window, &initialWidth, &initialHeight);
			glViewport(0, 0, initialWidth, initialHeight);
			if (smoke)
			{
				renderer.selfTest();
				std::cout << "Renderer test PASS (VBO/VAO/EBO, shader color, buffer update)\n";
			}
			double previous = glfwGetTime();
			int frame = 0;
			// [6] 렌더링 루프: 이벤트 -> 입력 -> 상태 갱신 -> 그리기 -> 버퍼 교환.
			while (!glfwWindowShouldClose(window))
			{
				glfwPollEvents(); // OS 이벤트를 GLFW 상태에 반영하고 등록 콜백을 호출한다.
				ProcessInput(window);
				if (glfwWindowShouldClose(window))
					break;
				double now = glfwGetTime();
				// 초 단위 delta time을 사용하면 모니터 FPS가 달라도 속도가 일정하다.
				float dt = frames > 0 ? 1.f / 60.f : std::min(float(now - previous), .05f);
				previous = now;
				if (smoke)
					cg::SmokeStep(frame);
				cg::UpdateScene(dt);
				int w, h;
				glfwGetFramebufferSize(window, &w, &h);
				if (w == 0 || h == 0)
				{
					glfwWaitEventsTimeout(.05);
					continue;
				}
				glEnable(GL_DEPTH_TEST);
				glEnable(GL_CULL_FACE);
				glCullFace(GL_BACK);
				// viewport 안에서 glClearColor와 color/depth glClear를 수행한다.
				// 물체의 색은 별도 uniform으로 보낸다. glClearColor는 배경색 설정이다.
				renderer.viewport(0, 0, w, h, {0, 0, 0});
				cg::DrawScene(renderer, w, h);
				GLenum error = glGetError();
				if (error != GL_NO_ERROR)
					throw std::runtime_error("OpenGL error: " + std::to_string(error));
				++frame;
				if (frames > 0 && frame >= frames)
				{
					if (!capture.empty())
						SaveFrame(capture, w, h);
					glfwSetWindowShouldClose(window, GL_TRUE);
				}
				// 지금까지 Back buffer에 그린 결과를 Front buffer와 교환해 한 프레임으로 표시한다.
				// SwapBuffers는 이벤트 처리 함수가 아니므로 PollEvents도 매 프레임 필요하다.
				glfwSwapBuffers(window);
			}
		} // GL 리소스를 context가 살아 있을 때 해제한다.
		// [7] GPU 리소스 해제 -> 창/컨텍스트 해제 -> GLFW 종료 순서를 지킨다.
		glfwDestroyWindow(window);
		window = nullptr;
		glfwTerminate();
		if (frames > 0)
			std::cout << "Render test PASS (" << frames << " frames)\n";
		return 0;
	}
	catch (const std::exception &e)
	{
		std::cerr << "ERROR: " << e.what() << '\n';
		if (window)
			glfwDestroyWindow(window);
		glfwTerminate();
		return 1;
	}
}
