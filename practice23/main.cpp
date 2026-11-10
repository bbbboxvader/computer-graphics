// PDF 기본코드: main.cpp + vertex.glsl + fragmet.glsl
// OpenGL 기초: 창/컨텍스트/콜백/main. GLSL 기초: shader/VAO/VBO/EBO.
// 좌표계 변환: model, view, projection을 CPU에서 만들고 uniform으로 전달한다.
// 시작 전 PDF_기준_새프로젝트_설정.md 참고. 창 안에는 안내 글자를 그리지 않는다.
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <GL/glew.h>
#include <GL/glfw3.h>
#include <gl/glm/glm.hpp>
#include <gl/glm/gtc/matrix_transform.hpp>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <stdexcept>
#include <vector>
#include <cstdlib>
#include <random>
#include <cmath>

// [기본코드 1: 전역 상태] 숫자는 이곳에서 바꾼다. 각도 단위는 도(degree).
GLuint shaderProgramID = 0, VAO = 0, VBO = 0, EBO = 0;
int width = 800, height = 600;
float angle = 0.0f, speed = 45.0f;
glm::vec3 position(0.0f), scaleValue(1.0f), objectColor(1.0f);
bool use2D = false, orthographic = false, wire = false, paused = true;
bool dragging = false;
bool doorOpen = false;
double previousX = 0.0;
// [TODO STATE] 문제의 물체 배열/각도/속도/키 상태를 여기에 추가한다.
float doorAmount = 0.0f;        // 문 열림 정도: 0~1
const float doorSpeed = 0.8f;   // 문 여닫기 속도: 1초에 0~1로 이동
                                // ===== 무대 크기: 이 숫자를 바꾸면 된다 =====
const float stageWidth = 8.0f;  // 가로: x 방향
const float stageHeight = 5.0f; // 높이: y 방향
const float stageDepth = 8.0f;  // 깊이: z 방향

// 처음 카메라 위치와 바라보는 지점.
// 시작 시선을 바꾸려면 이 두 줄의 숫자를 수정한다.
const glm::vec3 cameraStartPosition(0.0f, stageHeight * 0.5f, 15.0f);
const glm::vec3 cameraStartTarget(0.0f, stageHeight * 0.5f, 0.0f);

// 키보드로 변경할 현재 상태.
glm::vec3 cameraPosition = cameraStartPosition;
glm::vec3 cameraTarget = cameraStartTarget;

// 키를 한 번 누를 때 이동 거리와 공전 각도.
const float cameraMoveStep = 0.5f;
const float cameraOrbitStep = 15.0f;

// 로봇 위치는 몸통 중심이 아니라 발이 닿는 바닥 지점이다.
glm::vec3 robotPosition(0.0f, 0.0f, 2.5f);

float robotYaw = 0.0f;      // 로봇 전체가 바라보는 방향
float robotArmAngle = 0.0f; // 팔 흔들기 각도
float robotLegAngle = 0.0f; // 다리 흔들기 각도

float robotVerticalSpeed = 0.0f; // 위아래 속도
bool robotGrounded = true;       // 바닥이나 장애물 위에 서 있는가?
bool jumpMove = false;           // 공중에서 수평 이동할 것인가?

// -z 방향으로 걷기 시작.
// 방향 벡터: x는 좌우, z는 앞뒤.
glm::vec3 robotDirection(0.0f, 0.0f, -1.0f);

float robotSpeed = 1.3f; // 초당 이동 거리
float walkPhase = 0.0f;  // 팔다리 흔들기의 진행 정도

const float robotMinSpeed = 0.3f;
const float robotMaxSpeed = 3.0f;
const float robotSpeedStep = 0.2f;

// 로봇 중심에서 팔·다리까지 고려한 바닥 위 충돌 여유.
const float robotRadius = 0.85f;

const float jumpSpeed = 8.5f;    // 점프 시작 속도
const float gravity = 20.0f;     // 중력
const float robotHeight = 2.65f; // 앞에서 만든 로봇의 전체 높이

// 장애물 하나의 정보.
// 이 위치와 크기는 나중에 로봇과의 충돌 검사에도 사용한다.
struct Obstacle
{
	glm::vec3 center; // 상자 중심 위치
	glm::vec3 size;   // 가로, 높이, 깊이
	glm::vec3 color;  // RGB 색상
};

Obstacle obstacles[3]; // 장애물 3개

// 정점 하나 = 위치 x,y,z + 색 r,g,b
// 정점색을 흰색으로 두면 셰이더에서 곱하는 objectColor가 그대로 나타난다.
// 면마다 objectColor를 다르게 보내서 무대의 각 면을 다른 색으로 그린다.
const GLfloat cubeVertices[] = {
    -.5f, -.5f, -.5f, 1, 1, 1, .5f, -.5f, -.5f, 1, 1, 1, .5f, .5f, -.5f, 1, 1, 1, -.5f, .5f, -.5f, 1, 1, 1,
    -.5f, -.5f, .5f,  1, 1, 1, .5f, -.5f, .5f,  1, 1, 1, .5f, .5f, .5f,  1, 1, 1, -.5f, .5f, .5f,  1, 1, 1};
const GLfloat triangleVertices[] = {-.6f, -.5f, 0, 1, 0, 0, .6f, -.5f, 0, 0, 1, 0, 0, .6f, 0, 0, 0, 1};
const GLuint cubeIndices[] = {0, 2, 1, 0, 3, 2, 4, 5, 6, 4, 6, 7, 0, 4, 7, 0, 7, 3,
                              1, 2, 6, 1, 6, 5, 0, 1, 5, 0, 5, 4, 3, 7, 6, 3, 6, 2};

// [GLSL 기초 2, 9쪽] 강의처럼 현재 작업 폴더의 파일을 문자열로 읽는다.
std::string filetobuf(const char *file)
{
	std::ifstream shaderFile(file);
	if (!shaderFile.is_open())
		throw std::runtime_error(std::string("Cannot read shader: ") + file);
	std::string source((std::istreambuf_iterator<char>(shaderFile)), std::istreambuf_iterator<char>());
	return source;
}

// GLSL은 C++ 빌드 때가 아니라 실행 중 GPU 드라이버가 컴파일한다.
GLuint compileShader(GLenum type, const char *file)
{
	std::string source = filetobuf(file);
	const GLchar *text = source.c_str();
	GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, &text, nullptr);
	glCompileShader(shader);
	GLint success = GL_FALSE;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		char log[4096];
		glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
		glDeleteShader(shader);
		throw std::runtime_error(std::string(file) + ": " + log);
	}
	return shader;
}

void InitShader()
{
	GLuint vertexShader = compileShader(GL_VERTEX_SHADER, "vertex.glsl");
	GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, "fragment.glsl");
	shaderProgramID = glCreateProgram();
	glAttachShader(shaderProgramID, vertexShader);
	glAttachShader(shaderProgramID, fragmentShader);
	glLinkProgram(shaderProgramID);
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
	GLint success = GL_FALSE;
	glGetProgramiv(shaderProgramID, GL_LINK_STATUS, &success);
	if (!success)
	{
		char log[4096];
		glGetProgramInfoLog(shaderProgramID, sizeof(log), nullptr, log);
		throw std::runtime_error(log);
	}
}

void InitBuffer()
{
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_DYNAMIC_DRAW);
	glGenBuffers(1, &EBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cubeIndices), cubeIndices, GL_STATIC_DRAW);
	// layout(location=0): 좌표, location=1: 색. stride는 정점 하나의 전체 바이트 수.
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), nullptr);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void *)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);
	glBindVertexArray(0);
	// [TODO BUFFER] 다른 도형은 정점 배열/인덱스를 만들고 VAO/VBO를 추가한다.
}

// 기존 육면체 메시를 위치·크기·색만 바꿔서 다시 그리는 함수.
void DrawBox(const glm::vec3 &center, const glm::vec3 &size, const glm::vec3 &color)
{
	// T * S: 원본 육면체의 크기를 바꾼 다음 원하는 위치에 배치.
	glm::mat4 boxModel = glm::translate(glm::mat4(1.0f), center);
	boxModel = glm::scale(boxModel, size);

	glUniformMatrix4fv(glGetUniformLocation(shaderProgramID, "model"), 1, GL_FALSE, &boxModel[0][0]);

	glUniform3fv(glGetUniformLocation(shaderProgramID, "objectColor"), 1, &color[0]);

	// 무대와 같은 VAO/EBO를 사용한다.
	glBindVertexArray(VAO);

	// 상자는 여섯 면 전부 필요하므로 인덱스 36개를 그린다.
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, nullptr);
}

// parent: 로봇 전체 또는 관절의 기준 행렬
// center: 그 기준에서 부품 중심까지의 위치
// size: 부품의 가로·높이·깊이
void DrawRobotPart(const glm::mat4 &parent, const glm::vec3 &center, const glm::vec3 &size,
                   const glm::vec3 &color)
{
	// 부모 기준 → 부품 위치 → 부품 크기
	// 실제 정점에는 오른쪽부터 적용된다.
	glm::mat4 model = glm::translate(parent, center);
	model = glm::scale(model, size);

	glUniformMatrix4fv(glGetUniformLocation(shaderProgramID, "model"), 1, GL_FALSE, &model[0][0]);

	glUniform3fv(glGetUniformLocation(shaderProgramID, "objectColor"), 1, &color[0]);

	glBindVertexArray(VAO);
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, nullptr);
}

void DrawRobot()
{
	// ===== 로봇 전체의 기준 =====
	// 이 행렬을 모든 부품에 전달하므로 전체가 함께 이동·회전한다.
	glm::mat4 root = glm::translate(glm::mat4(1.0f), robotPosition);

	root = glm::rotate(root, glm::radians(robotYaw), glm::vec3(0, 1, 0));

	// ===== 몸통 =====
	DrawRobotPart(root, {0, 1.5f, 0}, // 중심 위치
	              {.8f, 1.0f, .5f},   // 가로·높이·깊이
	              {.15f, .30f, .85f}  // 파랑
	);

	// ===== 머리 =====
	DrawRobotPart(root, {0, 2.35f, 0}, {.6f, .6f, .6f}, {.95f, .80f, .60f});

	// ===== 코: 로봇의 앞쪽은 +z 방향 =====
	DrawRobotPart(root, {0, 2.35f, .425f}, {.18f, .18f, .25f}, {1.0f, .55f, .15f});

	// ===== 양팔·양다리 =====
	// side=-1과 side=+1로 좌우 부품을 하나씩 만든다.
	for (int side = -1; side <= 1; side += 2)
	{
		// 어깨 위치를 먼저 잡고, 어깨에서 팔을 회전한다.
		glm::mat4 shoulder = glm::translate(root, glm::vec3(side * .65f, 1.85f, 0));

		shoulder = glm::rotate(shoulder, glm::radians(side * robotArmAngle), glm::vec3(1, 0, 0));

		DrawRobotPart(shoulder, {0, -.4f, 0}, // 어깨에서 아래로 내려간 팔 중심
		              {.28f, .8f, .28f},
		              side < 0 ? glm::vec3(.20f, .80f, .30f) // 초록 팔
		                       : glm::vec3(.95f, .20f, .25f) // 빨강 팔
		);

		// 다리 윗부분을 회전 기준으로 잡는다.
		glm::mat4 hip = glm::translate(root, glm::vec3(side * .23f, 1.0f, 0));

		// 같은 쪽 팔과 다리는 반대 방향으로 흔들리게 한다.
		hip = glm::rotate(hip, glm::radians(-side * robotLegAngle), glm::vec3(1, 0, 0));

		DrawRobotPart(hip, {0, -.5f, 0}, // 관절에서 아래로 내려간 다리 중심
		              {.32f, 1.0f, .38f},
		              side < 0 ? glm::vec3(.70f, .25f, .90f) // 보라 다리
		                       : glm::vec3(.15f, .65f, .95f) // 하늘색 다리
		);
	}
}

// 이동하려는 위치 p에 로봇을 놓아도 되는지 검사.
// 현재는 바닥 위를 걷는 로봇이므로 x/z 방향을 검사한다.
bool CanRobotStandAt(const glm::vec3 &p)
{
	if (glm::abs(p.x) + robotRadius > stageWidth / 2 || glm::abs(p.z) + robotRadius > stageDepth / 2)
		return false;

	for (int i = 0; i < 3; ++i)
	{
		float top = obstacles[i].center.y + obstacles[i].size.y / 2;

		float bottom = obstacles[i].center.y - obstacles[i].size.y / 2;

		bool overlapX = glm::abs(p.x - obstacles[i].center.x) < obstacles[i].size.x / 2 + robotRadius;

		bool overlapZ = glm::abs(p.z - obstacles[i].center.z) < obstacles[i].size.z / 2 + robotRadius;

		// p.y는 발 높이, p.y + robotHeight는 머리 높이.
		// 발이 장애물 위에 있으면 높이 범위가 겹치지 않는다.
		bool overlapY = p.y < top - 0.001f && p.y + robotHeight > bottom + 0.001f;

		if (overlapX && overlapZ && overlapY)
			return false;
	}

	return true;
}

void UpdateRobotJump(float dt)
{
	// 현재 로봇 아래에서 착지할 수 있는 가장 높은 면.
	// 기본은 바닥 y=0.
	float supportHeight = 0.0f;

	for (int i = 0; i < 3; ++i)
	{
		bool overlapX =
		    glm::abs(robotPosition.x - obstacles[i].center.x) < obstacles[i].size.x / 2 + robotRadius;

		bool overlapZ =
		    glm::abs(robotPosition.z - obstacles[i].center.z) < obstacles[i].size.z / 2 + robotRadius;

		float top = obstacles[i].center.y + obstacles[i].size.y / 2;

		// 발 아래에 있는 장애물 윗면만 착지 후보로 사용.
		// 아직 발보다 높은 장애물 위로 순간이동하지 않도록 한다.
		if (overlapX && overlapZ && top <= robotPosition.y + 0.001f)
		{
			supportHeight = glm::max(supportHeight, top);
		}
	}

	if (robotGrounded)
	{
		// 발 바로 아래에 받쳐주는 면이 있으면 그대로 서 있다.
		if (glm::abs(robotPosition.y - supportHeight) < 0.001f)
		{
			robotPosition.y = supportHeight;
			return;
		}

		// 장애물 끝에서 걸어 나왔다면 떨어지기 시작.
		robotGrounded = false;
		jumpMove = true; // 걸어 나가던 수평 이동은 유지
	}

	// 중력 때문에 상승 속도가 줄고, 결국 하강 속도가 된다.
	robotVerticalSpeed -= gravity * dt;

	float nextY = robotPosition.y + robotVerticalSpeed * dt;

	// 머리가 천장을 뚫지 않도록 제한.
	if (nextY + robotHeight > stageHeight)
	{
		nextY = stageHeight - robotHeight;
		robotVerticalSpeed = 0.0f;
	}

	// 내려오는 중 착지면을 지나가면 그 높이에 정확히 맞춘다.
	if (robotVerticalSpeed <= 0.0f && nextY <= supportHeight)
	{
		robotPosition.y = supportHeight;
		robotVerticalSpeed = 0.0f;
		robotGrounded = true;
		jumpMove = false;
	}
	else
	{
		robotPosition.y = nextY;
	}
}

void UpdateRobotWalk(float dt)
{
	// 긴 멈춤 뒤 한 번에 멀리 이동하지 않도록 제한한다.
	float remaining = glm::clamp(dt, 0.0f, 0.1f);

	// 시간을 작게 나눠 이동·충돌을 검사한다.
	// 속도가 빨라도 장애물을 건너뛰는 현상을 줄인다.
	while (remaining > 0.0f)
	{
		float stepDt = glm::min(remaining, 1.0f / 120.0f);
		remaining -= stepDt;

		// 지상에서는 자동 걷기.
		// 공중에서는 WASD를 눌러 jumpMove가 켜졌을 때만 이동.
		if (robotGrounded || jumpMove)
		{
			float stepDistance = robotSpeed * stepDt;

			glm::vec3 nextPosition = robotPosition + robotDirection * stepDistance;

			if (CanRobotStandAt(nextPosition))
			{
				robotPosition = nextPosition;

				// 발을 딛고 걸을 때 걷기 동작을 진행.
				if (robotGrounded)
					walkPhase += stepDistance * 4.0f;
			}
			else
			{
				robotDirection = -robotDirection;
			}
		}

		// 수평 이동 후 발 아래의 바닥/장애물을 확인하고 높이를 갱신.
		UpdateRobotJump(stepDt);
	}

	// DrawRobot의 코는 로컬 +z 방향을 향하고 있다.
	// 이동 방향을 바라보도록 로봇 전체의 y축 회전각을 계산.
	robotYaw = glm::degrees(std::atan2(robotDirection.x, robotDirection.z));

	// 속도를 0~1 범위로 환산.
	float speedRatio = (robotSpeed - robotMinSpeed) / (robotMaxSpeed - robotMinSpeed);

	speedRatio = glm::clamp(speedRatio, 0.0f, 1.0f);

	// 느릴 때 최대 10도, 빠를 때 최대 45도로 흔들린다.
	float swingAmplitude = glm::mix(10.0f, 45.0f, speedRatio);

	// sin은 -1~+1을 반복한다.
	// 따라서 각도가 음수와 양수를 반복하며 앞뒤로 흔들린다.
	robotLegAngle = std::sin(walkPhase) * swingAmplitude;
	robotArmAngle = robotLegAngle * 0.8f;

	// 오랫동안 실행해도 위상 값이 계속 커지지 않도록 정리.
	walkPhase = std::fmod(walkPhase, 6.2831853f);
}

void ResetScene()
{
	angle = 0;
	position = glm::vec3(0);
	scaleValue = glm::vec3(1);
	objectColor = glm::vec3(1);
	use2D = orthographic = wire = false;
	paused = true;
	dragging = false;
	previousX = 0.0;
	speed = 45.0f;
	doorOpen = false;
	doorAmount = 0.0f;

	robotPosition = glm::vec3(0.0f, 0.0f, 2.5f);
	robotDirection = glm::vec3(0.0f, 0.0f, -1.0f);
	robotYaw = 180.0f; // 코가 이동 방향인 -z를 바라보게 시작

	robotSpeed = 1.3f;
	walkPhase = 0.0f;

	robotArmAngle = 0.0f;
	robotLegAngle = 0.0f;

	robotVerticalSpeed = 0.0f;
	robotGrounded = true;
	jumpMove = false;

	cameraPosition = cameraStartPosition;
	cameraTarget = cameraStartTarget;

	// [TODO RESET] 문제의 초기 상태로 복구한다.
	// 난수 발생기는 한 번 준비하고, 초기화할 때마다 다음 난수를 사용한다.
	static std::mt19937 randomEngine(std::random_device{}());

	// 순서: 가로(x), 높이(y), 깊이(z)
	const glm::vec3 sizes[3] = {
	    {1.5f, 0.8f, 1.0f}, // 첫 번째 상자
	    {0.8f, 1.2f, 0.8f}, // 두 번째 상자
	    {1.2f, 0.6f, 1.3f}  // 세 번째 상자
	};

	// 색도 상자별 고정값
	const glm::vec3 colors[3] = {
	    {1.0f, 0.55f, 0.15f},  // 주황
	    {0.60f, 0.60f, 0.60f}, // 회색
	    {0.60f, 0.30f, 0.80f}  // 보라
	};

	for (int i = 0; i < 3; ++i)
	{
		obstacles[i].size = sizes[i];
		obstacles[i].color = colors[i];

		// 상자 중심만 무대 안에 두면 상자 일부가 벽을 뚫을 수 있다.
		// 상자 반크기와 벽 여유 0.2를 제외한 범위에서 중심을 뽑는다.
		float limitX = stageWidth / 2 - obstacles[i].size.x / 2 - 0.2f;
		float limitZ = stageDepth / 2 - obstacles[i].size.z / 2 - 0.2f;

		std::uniform_real_distribution<float> randomX(-limitX, limitX);
		std::uniform_real_distribution<float> randomZ(-limitZ, limitZ);

		bool placed = false;

		// 이미 배치한 장애물과 겹치면 위치를 다시 뽑는다.
		// 반복 횟수를 제한해서 무한 반복을 막는다.
		for (int attempt = 0; attempt < 200; ++attempt)
		{
			glm::vec3 candidate(randomX(randomEngine),
			                    obstacles[i].size.y / 2, // 바닥이 y=0에 닿도록 배치
			                    randomZ(randomEngine));

			// 로봇 시작 위치 주변도 비워 둔다.
			// 0.85는 정지한 로봇의 팔까지 고려한 x/z 방향 여유다.
			bool overlap = glm::abs(candidate.x - robotPosition.x) < obstacles[i].size.x / 2 + 0.85f &&
			               glm::abs(candidate.z - robotPosition.z) < obstacles[i].size.z / 2 + 0.85f;

			for (int j = 0; j < i; ++j)
			{
				float distanceX = glm::abs(candidate.x - obstacles[j].center.x);
				float distanceZ = glm::abs(candidate.z - obstacles[j].center.z);

				// 두 상자의 반크기를 더하고 간격 0.25를 확보한다.
				float requiredX = (obstacles[i].size.x + obstacles[j].size.x) / 2 + 0.25f;
				float requiredZ = (obstacles[i].size.z + obstacles[j].size.z) / 2 + 0.25f;

				if (distanceX < requiredX && distanceZ < requiredZ)
				{
					overlap = true;
					break;
				}
			}

			if (!overlap)
			{
				obstacles[i].center = candidate;
				placed = true;
				break;
			}
		}

		if (!placed)
			throw std::runtime_error("Cannot place obstacles: enlarge stage or reduce boxes.");
	}
}

void UpdateScene(float dt)
{
	if (!paused)
		angle += speed * dt;

	// dt는 지난 프레임부터 흐른 초.
	// 속도에 dt를 곱하면 컴퓨터의 프레임 수와 관계없이 움직인다.
	if (doorOpen)
		doorAmount += doorSpeed * dt;
	else
		doorAmount -= doorSpeed * dt;

	// 열림 정도가 0~1을 넘어가지 않도록 제한한다.
	doorAmount = glm::clamp(doorAmount, 0.0f, 1.0f);

	UpdateRobotWalk(dt);
}

void drawScene()
{
	if (width <= 0 || height <= 0)
		return;

	glViewport(0, 0, width, height);
	glClearColor(.08f, .10f, .15f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram(shaderProgramID);

	// [은면 제거] 가까운 면이 먼 면을 가리도록 깊이를 비교한다.
	glEnable(GL_DEPTH_TEST);

	// 원래 육면체는 각 축에서 -0.5 ~ +0.5 크기이다.
	// S로 크기를 바꾸고 T로 높이의 절반만큼 올리면 바닥이 y=0이 된다.
	// model = T * S → 실제 적용 순서는 크기 변경 후 이동.
	glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(0, stageHeight / 2, 0));
	model = glm::scale(model, glm::vec3(stageWidth, stageHeight, stageDepth));

	glm::mat4 view = glm::lookAt(cameraPosition, cameraTarget, glm::vec3(0, 1, 0));

	float aspect = float(width) / height;

	// F3으로 직교/원근 투영 전환.
	// 직교 범위도 무대 전체가 보이도록 넓혀 준다.
	glm::mat4 projection = orthographic ? glm::ortho(-6.0f * aspect, 6.0f * aspect, -6.0f, 6.0f, .1f, 100.0f)
	                                    : glm::perspective(glm::radians(45.0f), aspect, .1f, 100.0f);

	// CPU에서 만든 변환 행렬을 vertex shader에 전달한다.
	glUniformMatrix4fv(glGetUniformLocation(shaderProgramID, "model"), 1, GL_FALSE, &model[0][0]);

	glUniformMatrix4fv(glGetUniformLocation(shaderProgramID, "view"), 1, GL_FALSE, &view[0][0]);

	glUniformMatrix4fv(glGetUniformLocation(shaderProgramID, "projection"), 1, GL_FALSE, &projection[0][0]);

	// ===== 면 색상: RGB 각각 0~1 =====
	// 기존 cubeIndices에 저장된 면 순서와 일치시킨다.
	const glm::vec3 faceColors[6] = {
	    {.60f, .20f, .20f}, // 0: 뒷벽
	    {.65f, .45f, .20f}, // 1: 앞벽 → 나중에 문으로 사용할 면
	    {.20f, .50f, .25f}, // 2: 왼쪽 벽
	    {.20f, .30f, .60f}, // 3: 오른쪽 벽
	    {.45f, .65f, .42f}, // 4: 바닥
	    {.55f, .45f, .65f}  // 5: 천장
	};

	// [컬링] 육면체의 바깥쪽을 향하는 면을 제거한다.
	// 카메라 쪽 외벽이 내부를 가리지 않아 방 안이 보인다.

	// F4로 선/면 표시 전환.
	glPolygonMode(GL_FRONT_AND_BACK, wire ? GL_LINE : GL_FILL);
	glBindVertexArray(VAO);

	// 문이 완전히 열린 뒤에 카메라 쪽 외벽을 제거한다.
	const bool doorFullyOpen = doorAmount >= 1.0f;
	glFrontFace(GL_CCW);
	if (doorFullyOpen)
	{
		glEnable(GL_CULL_FACE);
		glCullFace(GL_FRONT);
	}
	else
	{
		glDisable(GL_CULL_FACE);
	}

	// 육면체 한 면 = 삼각형 2개 = 인덱스 6개.
	// 면 하나씩 색을 보내고, EBO에서 해당 면의 인덱스만 읽어 그린다.
	for (int face = 0; face < 6; ++face)
	{
		// 기본적으로는 기존 무대 변환을 사용한다.
		glm::mat4 faceModel = model;

		// 기존 cubeIndices에서 1번 면은 앞벽이다.
		if (face == 1)
		{
			// 완전히 열리면 앞벽을 그리지 않는다.
			if (doorFullyOpen)
				continue;

			// 열릴수록 앞벽 높이가 줄어든다.
			float doorHeight = stageHeight * (1.0f - doorAmount);

			// 위쪽 끝을 고정하려면 중심도 위로 올려야 한다.
			// 중심 높이 = 무대 높이 - 줄어든 문 높이의 절반
			float doorCenterY = stageHeight - doorHeight / 2.0f;

			faceModel = glm::translate(glm::mat4(1.0f), glm::vec3(0, doorCenterY, 0));

			faceModel = glm::scale(faceModel, glm::vec3(stageWidth, doorHeight, stageDepth));
		}

		// 면마다 변환을 다시 전달한다.
		// 앞벽만 별도 변환, 나머지 면은 기존 무대 변환.
		glUniformMatrix4fv(glGetUniformLocation(shaderProgramID, "model"), 1, GL_FALSE, &faceModel[0][0]);

		glUniform3fv(glGetUniformLocation(shaderProgramID, "objectColor"), 1, &faceColors[face][0]);

		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT,
		               reinterpret_cast<const void *>(face * 6 * sizeof(GLuint)));
	}

	// 상자와 로봇은 바깥에서 보이는 면을 그린다.
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	// 저장된 위치·크기·색으로 장애물 3개를 그린다.
	for (int i = 0; i < 3; ++i)
	{
		DrawBox(obstacles[i].center, obstacles[i].size, obstacles[i].color);
	}

	DrawRobot();

	// 이후 그릴 로봇과 장애물에 무대용 컬링이 적용되지 않도록 복구한다.
	glDisable(GL_CULL_FACE);
	glBindVertexArray(0);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

}

void KeyCallback(GLFWwindow *window, int key, int, int action, int mods)
{
	if (action != GLFW_PRESS)
		return; // 토글은 REPEAT를 무시한다.
	if (key == GLFW_KEY_ESCAPE || key == GLFW_KEY_Q)
		glfwSetWindowShouldClose(window, GL_TRUE);
	// I/F1: 모든 상태를 초기화하고 자동 걷기부터 다시 시작.
	if (key == GLFW_KEY_I || key == GLFW_KEY_F1)
	{
		ResetScene();
		return;
	}
	if (key == GLFW_KEY_F2)
		use2D = !use2D;
	if (key == GLFW_KEY_F3)
		orthographic = !orthographic;
	if (key == GLFW_KEY_F4)
		wire = !wire;
	if (key == GLFW_KEY_O)
		doorOpen = !doorOpen; // 문 열기/닫기 토글
	if (key == GLFW_KEY_SPACE)
		paused = !paused;
	// [TODO KEY] 문제의 키를 추가한다. 연속 이동은 main의 폴링 또는 상태 변수로 처리한다.
	// 방향만 바꾼다. 실제 이동은 UpdateRobotWalk에서 계속 처리한다.
	bool directionKey = false;

	if (key == GLFW_KEY_W)
	{
		robotDirection = glm::vec3(0, 0, -1);
		directionKey = true;
	}
	else if (key == GLFW_KEY_S)
	{
		robotDirection = glm::vec3(0, 0, 1);
		directionKey = true;
	}
	else if (key == GLFW_KEY_A)
	{
		robotDirection = glm::vec3(-1, 0, 0);
		directionKey = true;
	}
	else if (key == GLFW_KEY_D)
	{
		robotDirection = glm::vec3(1, 0, 0);
		directionKey = true;
	}
	// 공중에서 방향키를 누르면 그 방향으로 수평 이동 시작.
	if (directionKey && !robotGrounded)
		jumpMove = true;

	// 서 있을 때만 점프 가능. 공중 연속 점프는 막는다.
	if (key == GLFW_KEY_J && robotGrounded)
	{
		robotGrounded = false;
		robotVerticalSpeed = jumpSpeed;
		jumpMove = true; // J만 누르면 제자리 점프
	}
	// TODO 카메라 Z키: if(key==GLFW_KEY_Z) 뒤에 동작을 작성한 후 사용.
	// Shift를 누르면 반대 방향으로 동작.
	float sign = (mods & GLFW_MOD_SHIFT) ? -1.0f : 1.0f;

	// Z: 바라보는 지점으로 전진.
	// Shift+Z: 뒤로 이동.
	if (key == GLFW_KEY_Z)
	{
		glm::vec3 forward = glm::normalize(cameraTarget - cameraPosition);

		float distance = glm::length(cameraTarget - cameraPosition);

		// 바라보는 지점을 지나치거나 너무 멀어지지 않도록 제한.
		float newDistance = glm::clamp(distance - sign * cameraMoveStep, 1.0f, 60.0f);

		cameraPosition = cameraTarget - forward * newDistance;
	}

	// X: 화면 오른쪽으로 평행 이동.
	// Shift+X: 화면 왼쪽으로 평행 이동.
	if (key == GLFW_KEY_X)
	{
		glm::vec3 forward = glm::normalize(cameraTarget - cameraPosition);

		// 시선 방향과 위쪽 방향으로 화면의 오른쪽 방향을 구한다.
		glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0, 1, 0)));

		glm::vec3 movement = right * sign * cameraMoveStep;

		// 두 점을 함께 옮겨서 시선 방향을 유지한다.
		cameraPosition += movement;
		cameraTarget += movement;
	}

	// Y: 화면 중심을 지나는 수직 y축 주위로 공전.
	// Shift+Y: 반대 방향으로 공전.
	if (key == GLFW_KEY_Y)
	{
		// 공전 중심에서 카메라까지의 상대 위치.
		glm::vec3 offset = cameraPosition - cameraTarget;

		glm::mat4 rotation =
		    glm::rotate(glm::mat4(1.0f), glm::radians(sign * cameraOrbitStep), glm::vec3(0, 1, 0));

		// 상대 위치를 회전한 뒤 공전 중심을 다시 더한다.
		cameraPosition = cameraTarget + glm::vec3(rotation * glm::vec4(offset, 0.0f));
	}
}

void Reshape(GLFWwindow *, int w, int h)
{
	width = w;
	height = h;
	glViewport(0, 0, w, h);
}

void MouseButtonCallback(GLFWwindow *window, int button, int action, int)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT)
	{
		dragging = action == GLFW_PRESS;
		double y;
		glfwGetCursorPos(window, &previousX, &y);
	}
	// [TODO MOUSE] 눌림/놓기에서 선택 상태를 갱신한다.
}

void CursorPositionCallback(GLFWwindow *, double x, double)
{
	if (dragging)
		angle += float(x - previousX) * .35f;
	previousX = x;
	// [TODO CURSOR] 화면 좌표를 과제의 좌표계로 변환해서 사용한다.
}

// 자동 실행 검사용 선택 기능. 일반 Ctrl+F5 실행에는 프레임 제한이 없다.
void SaveFrame(const char *file)
{
	std::vector<unsigned char> rgb(size_t(width) * height * 3);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glReadBuffer(GL_BACK);
	glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, rgb.data());
	std::ofstream out(file, std::ios::binary);
	out << "P6\n" << width << ' ' << height << "\n255\n";
	for (int y = height - 1; y >= 0; --y)
		out.write(reinterpret_cast<const char *>(rgb.data() + size_t(y) * width * 3), width * 3);
}

int main(int argc, char **argv)
{
	GLFWwindow *window = nullptr;
	try
	{
		int frames = 0;
		const char *capture = nullptr;
		bool test2D = false;
		for (int i = 1; i < argc; ++i)
		{
			std::string arg = argv[i];
			if (arg == "--frames" && i + 1 < argc)
				frames = std::atoi(argv[++i]);
			else if (arg == "--capture" && i + 1 < argc)
				capture = argv[++i];
			else if (arg == "--2d")
				test2D = true;
		}
		// [OpenGL 기초] GLFW → 창/컨텍스트 → GLEW → shader/buffer 순서가 중요하다.
		if (!glfwInit())
			throw std::runtime_error("GLFW initialization failed");
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		if (frames > 0)
			glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
		window = glfwCreateWindow(width, height, "PDF Basic Code", nullptr, nullptr);
		if (!window)
			throw std::runtime_error("Cannot create window");
		glfwMakeContextCurrent(window);
		glewExperimental = GL_TRUE;
		if (glewInit() != GLEW_OK)
			throw std::runtime_error("GLEW initialization failed");
		while (glGetError() != GL_NO_ERROR)
		{
		} // GLEW 초기화의 탐색 오류를 비운다.
		glfwSetKeyCallback(window, KeyCallback);
		glfwSetFramebufferSizeCallback(window, Reshape);
		glfwSetMouseButtonCallback(window, MouseButtonCallback);
		glfwSetCursorPosCallback(window, CursorPositionCallback);
		glfwGetFramebufferSize(window, &width, &height);
		InitShader();
		InitBuffer();
		ResetScene();
		use2D = test2D;
		glEnable(GL_DEPTH_TEST); // 뒤의 면이 앞의 면을 덮지 않도록 깊이를 비교한다.
		double previous = glfwGetTime();
		int frame = 0;
		while (!glfwWindowShouldClose(window))
		{
			glfwPollEvents();
			double now = glfwGetTime();
			float dt = float(now - previous);
			previous = now;
			UpdateScene(dt);

			// [TODO POLL] if(glfwGetKey(window,GLFW_KEY_A)==GLFW_PRESS)position.x-=dt;
			if (width <= 0 || height <= 0)
			{
				glfwWaitEventsTimeout(.05);
				continue;
			}
			drawScene();
			if (glGetError() != GL_NO_ERROR)
				throw std::runtime_error("OpenGL draw error");
			if (frames > 0 && ++frame >= frames)
			{
				if (capture)
					SaveFrame(capture);
				glfwSetWindowShouldClose(window, GL_TRUE);
			}
			glfwSwapBuffers(window);
		}
		// 컨텍스트가 살아 있을 때 GPU 자원을 해제한 뒤 창을 닫는다.
		glDeleteBuffers(1, &EBO);
		glDeleteBuffers(1, &VBO);
		glDeleteVertexArrays(1, &VAO);
		glDeleteProgram(shaderProgramID);
		glfwDestroyWindow(window);
		glfwTerminate();
		return 0;
	}
	catch (const std::exception &error)
	{
		std::cerr << error.what() << '\n';
		if (window)
			glfwDestroyWindow(window);
		glfwTerminate();
		return 1;
	}
}
