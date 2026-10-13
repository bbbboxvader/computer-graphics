#include "PracticeCommon.h"
#include <ctime>

// 실습 15
// 육면체와 사각뿔을 그리고, 육면체의 한 면·두 면·네 면을 따로 움직인다.

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

struct FaceTransform
{
	float angleX;
	float angleY;
	float angleZ;
	Point3D pivot;
	Point3D offset;
	Point3D scale;
};

enum ObjectType
{
	CUBE_OBJECT,
	PYRAMID_OBJECT
};

enum AnimationMode
{
	NO_FACE_ANIMATION,
	TOP_FACE_SPIN,
	FRONT_FACE_DOOR,
	SIDE_FACE_SPIN,
	BACK_FACE_SCALE,
	TWO_FACE_EDGE_SPIN,
	TWO_FACE_SWAP,
	FOUR_SIDE_ORBIT,
	FOUR_MIXED_ORBIT
};

GLuint shaderProgramID = 0;
GLuint VAO = 0;
GLuint VBO = 0;

GLint objectAngleXLocation = -1;
GLint objectAngleYLocation = -1;
GLint partAngleXLocation = -1;
GLint partAngleYLocation = -1;
GLint partAngleZLocation = -1;
GLint pivotLocation = -1;
GLint offsetLocation = -1;
GLint scaleLocation = -1;

ObjectType objectType = CUBE_OBJECT;
AnimationMode animationMode = NO_FACE_ANIMATION;
bool hiddenSurface = true;
bool wholeObjectRotating = false;

float objectAngleX = 30.0f;
float objectAngleY = 30.0f;
float topSpinAngle = 0.0f;
float frontDoorAngle = 0.0f;
float sideSpinAngle = 0.0f;
float backScale = 1.0f;
float edgeSpinAngle = 0.0f;
float swapAngle = 0.0f;
float orbitAngle = 0.0f;

bool frontDoorOpen = false;
bool backFaceHidden = false;
int selectedPair = 0;

const Point3D cubePoints[8] = {
	{ -0.42f, -0.42f, -0.42f },
	{  0.42f, -0.42f, -0.42f },
	{  0.42f,  0.42f, -0.42f },
	{ -0.42f,  0.42f, -0.42f },
	{ -0.42f, -0.42f,  0.42f },
	{  0.42f, -0.42f,  0.42f },
	{  0.42f,  0.42f,  0.42f },
	{ -0.42f,  0.42f,  0.42f }
};

// 면 번호: 0 앞, 1 뒤, 2 왼쪽, 3 오른쪽, 4 위, 5 아래
const int cubeFaces[6][4] = {
	{ 4, 5, 6, 7 },
	{ 0, 3, 2, 1 },
	{ 0, 4, 7, 3 },
	{ 1, 2, 6, 5 },
	{ 3, 7, 6, 2 },
	{ 0, 1, 5, 4 }
};

const Color3 cubeFaceColors[6] = {
	{ 0.10f, 1.00f, 0.25f }, // 앞: 초록
	{ 1.00f, 0.90f, 0.05f }, // 뒤: 노랑
	{ 1.00f, 0.00f, 0.85f }, // 왼쪽: 자홍
	{ 0.10f, 0.30f, 1.00f }, // 오른쪽: 파랑
	{ 0.00f, 0.95f, 0.90f }, // 위: 청록
	{ 1.00f, 0.15f, 0.15f }  // 아래: 빨강
};

// 앞, 오른쪽, 뒤, 왼쪽 순서이다. 이웃한 두 면은 모서리 하나를 공유한다.
const int sideFaceOrder[4] = { 0, 3, 1, 2 };
const Point3D sharedEdges[4] = {
	{  0.42f, 0.0f,  0.42f },
	{  0.42f, 0.0f, -0.42f },
	{ -0.42f, 0.0f, -0.42f },
	{ -0.42f, 0.0f,  0.42f }
};

const Point3D pyramidPoints[5] = {
	{  0.00f,  0.55f,  0.00f },
	{ -0.48f, -0.42f,  0.48f },
	{  0.48f, -0.42f,  0.48f },
	{  0.48f, -0.42f, -0.48f },
	{ -0.48f, -0.42f, -0.48f }
};

const int pyramidSides[4][3] = {
	{ 0, 1, 2 },
	{ 0, 2, 3 },
	{ 0, 3, 4 },
	{ 0, 4, 1 }
};

const int pyramidBase[4] = { 1, 4, 3, 2 };

const Color3 pyramidColors[5] = {
	{ 0.10f, 0.30f, 1.00f }, // 앞쪽 옆면: 파랑
	{ 1.00f, 0.10f, 0.10f }, // 오른쪽 옆면: 빨강
	{ 0.10f, 1.00f, 0.20f },
	{ 1.00f, 0.00f, 0.85f }, // 왼쪽 옆면: 자홍
	{ 1.00f, 1.00f, 0.10f }  // 바닥면: 노랑
};

Vertex MakeVertex3D(Point3D point, Color3 color)
{
	return { point.x, point.y, point.z, color.r, color.g, color.b };
}

FaceTransform MakeNormalTransform()
{
	FaceTransform transform;
	transform.angleX = 0.0f;
	transform.angleY = 0.0f;
	transform.angleZ = 0.0f;
	transform.pivot = { 0.0f, 0.0f, 0.0f };
	transform.offset = { 0.0f, 0.0f, 0.0f };
	transform.scale = { 1.0f, 1.0f, 1.0f };
	return transform;
}

void SendTransform(float wholeX, float wholeY, const FaceTransform& transform)
{
	glUniform1f(objectAngleXLocation, wholeX);
	glUniform1f(objectAngleYLocation, wholeY);
	glUniform1f(partAngleXLocation, transform.angleX);
	glUniform1f(partAngleYLocation, transform.angleY);
	glUniform1f(partAngleZLocation, transform.angleZ);
	glUniform3f(pivotLocation, transform.pivot.x, transform.pivot.y, transform.pivot.z);
	glUniform3f(offsetLocation, transform.offset.x, transform.offset.y, transform.offset.z);
	glUniform3f(scaleLocation, transform.scale.x, transform.scale.y, transform.scale.z);
}

void DrawAxes()
{
	Vertex axes[6] = {
		{ -0.95f, 0.0f, 0.0f, 1.0f, 0.15f, 0.15f },
		{  0.95f, 0.0f, 0.0f, 1.0f, 0.15f, 0.15f },
		{ 0.0f, -0.95f, 0.0f, 0.15f, 1.0f, 0.15f },
		{ 0.0f,  0.95f, 0.0f, 0.15f, 1.0f, 0.15f },
		{ 0.0f, 0.0f, -0.95f, 0.20f, 0.35f, 1.0f },
		{ 0.0f, 0.0f,  0.95f, 0.20f, 0.35f, 1.0f }
	};
	DrawVertices(VBO, GL_LINES, axes, 6);
}

void DrawQuadFace(const Point3D points[], const int face[4], Color3 color)
{
	Vertex vertices[6] = {
		MakeVertex3D(points[face[0]], color),
		MakeVertex3D(points[face[1]], color),
		MakeVertex3D(points[face[2]], color),
		MakeVertex3D(points[face[0]], color),
		MakeVertex3D(points[face[2]], color),
		MakeVertex3D(points[face[3]], color)
	};
	DrawVertices(VBO, GL_TRIANGLES, vertices, 6);
}

void DrawTriangleFace(const Point3D points[], const int face[3], Color3 color)
{
	Vertex vertices[3] = {
		MakeVertex3D(points[face[0]], color),
		MakeVertex3D(points[face[1]], color),
		MakeVertex3D(points[face[2]], color)
	};
	DrawVertices(VBO, GL_TRIANGLES, vertices, 3);
}

bool FaceBelongsToSelectedPair(int face)
{
	int first = sideFaceOrder[selectedPair];
	int second = sideFaceOrder[(selectedPair + 1) % 4];
	return face == first || face == second;
}

FaceTransform TransformForCubeFace(int face)
{
	FaceTransform transform = MakeNormalTransform();

	if (animationMode == TOP_FACE_SPIN && face == 4) {
		transform.pivot = { 0.0f, 0.42f, 0.0f };
		transform.angleZ = topSpinAngle;
	}
	else if (animationMode == FRONT_FACE_DOOR && face == 0) {
		transform.pivot = { 0.0f, 0.42f, 0.42f };
		transform.angleX = -frontDoorAngle;
	}
	else if (animationMode == SIDE_FACE_SPIN && face == 3) {
		transform.pivot = { 0.42f, 0.0f, 0.0f };
		transform.angleY = sideSpinAngle;
	}
	else if (animationMode == BACK_FACE_SCALE && face == 1) {
		transform.pivot = { 0.0f, 0.0f, -0.42f };
		transform.scale = { backScale, backScale, backScale };
	}
	else if (animationMode == TWO_FACE_EDGE_SPIN && FaceBelongsToSelectedPair(face)) {
		transform.pivot = sharedEdges[selectedPair];
		transform.angleY = edgeSpinAngle;
	}
	else if (animationMode == TWO_FACE_SWAP && FaceBelongsToSelectedPair(face)) {
		int first = sideFaceOrder[selectedPair];
		transform.angleY = (face == first) ? swapAngle : -swapAngle;
	}
	else if (animationMode == FOUR_SIDE_ORBIT && face >= 0 && face <= 3) {
		// 4번: 네 옆면이 육면체 중심의 y축을 기준으로 공전한다.
		transform.angleY = orbitAngle;
	}
	else if (animationMode == FOUR_MIXED_ORBIT &&
		(face == 0 || face == 1 || face == 4 || face == 5)) {
		// 5번: 앞·뒤·위·아래 네 면이 중심의 x축을 기준으로 공전한다.
		// 4번과 다른 방향이 확실하게 보이도록 angleX를 사용한다.
		transform.angleX = orbitAngle;
	}

	return transform;
}

void DrawCube()
{
	for (int face = 0; face < 6; ++face) {
		FaceTransform transform = TransformForCubeFace(face);
		SendTransform(objectAngleX, objectAngleY, transform);
		DrawQuadFace(cubePoints, cubeFaces[face], cubeFaceColors[face]);
	}
}

void DrawPyramid()
{
	FaceTransform normal = MakeNormalTransform();
	SendTransform(objectAngleX, objectAngleY, normal);

	for (int side = 0; side < 4; ++side)
		DrawTriangleFace(pyramidPoints, pyramidSides[side], pyramidColors[side]);
	DrawQuadFace(pyramidPoints, pyramidBase, pyramidColors[4]);
}

float MoveToward(float current, float target, float amount)
{
	if (current < target)
		return std::min(current + amount, target);
	if (current > target)
		return std::max(current - amount, target);
	return current;
}

void WrapAngle(float& angle)
{
	if (angle >= 360.0f)
		angle -= 360.0f;
}

void UpdateScene(float deltaTime)
{
	if (wholeObjectRotating) {
		objectAngleY += 35.0f * deltaTime;
		WrapAngle(objectAngleY);
	}

	if (animationMode == TOP_FACE_SPIN) {
		topSpinAngle += 90.0f * deltaTime;
		WrapAngle(topSpinAngle);
	}
	else if (animationMode == FRONT_FACE_DOOR) {
		float target = frontDoorOpen ? 100.0f : 0.0f;
		frontDoorAngle = MoveToward(frontDoorAngle, target, 70.0f * deltaTime);
	}
	else if (animationMode == SIDE_FACE_SPIN) {
		sideSpinAngle += 100.0f * deltaTime;
		WrapAngle(sideSpinAngle);
	}
	else if (animationMode == BACK_FACE_SCALE) {
		float target = backFaceHidden ? 0.0f : 1.0f;
		backScale = MoveToward(backScale, target, 0.8f * deltaTime);
	}
	else if (animationMode == TWO_FACE_EDGE_SPIN) {
		edgeSpinAngle += 80.0f * deltaTime;
		WrapAngle(edgeSpinAngle);
	}
	else if (animationMode == TWO_FACE_SWAP) {
		swapAngle = MoveToward(swapAngle, 90.0f, 65.0f * deltaTime);
	}
	else if (animationMode == FOUR_SIDE_ORBIT || animationMode == FOUR_MIXED_ORBIT) {
		orbitAngle += 55.0f * deltaTime;
		WrapAngle(orbitAngle);
	}
}

void DrawScene()
{
	glClearColor(0.02f, 0.02f, 0.02f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram(shaderProgramID);
	glBindVertexArray(VAO);

	if (hiddenSurface)
		glEnable(GL_DEPTH_TEST);
	else
		glDisable(GL_DEPTH_TEST);

	FaceTransform normal = MakeNormalTransform();
	SendTransform(0.0f, 0.0f, normal);
	glLineWidth(1.4f);
	DrawAxes();

	if (objectType == CUBE_OBJECT)
		DrawCube();
	else
		DrawPyramid();
}

void ResetScene()
{
	objectType = CUBE_OBJECT;
	animationMode = NO_FACE_ANIMATION;
	hiddenSurface = true;
	wholeObjectRotating = false;
	objectAngleX = 30.0f;
	objectAngleY = 30.0f;
	topSpinAngle = 0.0f;
	frontDoorAngle = 0.0f;
	sideSpinAngle = 0.0f;
	backScale = 1.0f;
	edgeSpinAngle = 0.0f;
	swapAngle = 0.0f;
	orbitAngle = 0.0f;
	frontDoorOpen = false;
	backFaceHidden = false;
	selectedPair = 0;
}

void ToggleContinuousAnimation(AnimationMode mode)
{
	if (animationMode == mode)
		animationMode = NO_FACE_ANIMATION;
	else {
		objectType = CUBE_OBJECT;
		animationMode = mode;
	}
}

void KeyCallback(GLFWwindow* window, int key, int, int action, int)
{
	if (action != GLFW_PRESS)
		return;

	if (key == GLFW_KEY_H)
		hiddenSurface = !hiddenSurface;
	else if (key == GLFW_KEY_P) {
		if (objectType == CUBE_OBJECT) {
			objectType = PYRAMID_OBJECT;

			// 사진처럼 꼭짓점은 위, 파란 앞면과 자홍 옆면이 함께 보이게 한다.
			objectAngleX = 30.0f;
			objectAngleY = 30.0f;
			wholeObjectRotating = false;
		}
		else {
			objectType = CUBE_OBJECT;
			objectAngleX = 30.0f;
			objectAngleY = 30.0f;
		}
		animationMode = NO_FACE_ANIMATION;
	}
	else if (key == GLFW_KEY_Y)
		wholeObjectRotating = !wholeObjectRotating;
	else if (key == GLFW_KEY_R) {
		ResetScene();
		std::cout << "리셋: 육면체를 출력합니다.\n";
	}
	else if (key == GLFW_KEY_T) {
		if (animationMode != TOP_FACE_SPIN)
			topSpinAngle = 0.0f;
		ToggleContinuousAnimation(TOP_FACE_SPIN);
	}
	else if (key == GLFW_KEY_F) {
		objectType = CUBE_OBJECT;
		animationMode = FRONT_FACE_DOOR;
		frontDoorOpen = !frontDoorOpen;
	}
	else if (key == GLFW_KEY_S) {
		if (animationMode != SIDE_FACE_SPIN)
			sideSpinAngle = 0.0f;
		ToggleContinuousAnimation(SIDE_FACE_SPIN);
	}
	else if (key == GLFW_KEY_B) {
		objectType = CUBE_OBJECT;
		animationMode = BACK_FACE_SCALE;
		backFaceHidden = !backFaceHidden;
	}
	else if (key == GLFW_KEY_1) {
		objectType = CUBE_OBJECT;
		animationMode = TWO_FACE_EDGE_SPIN;
		selectedPair = std::rand() % 4;
		edgeSpinAngle = 0.0f;
	}
	else if (key == GLFW_KEY_2) {
		objectType = CUBE_OBJECT;
		animationMode = TWO_FACE_SWAP;
		selectedPair = std::rand() % 4;
		swapAngle = 0.0f;
	}
	else if (key == GLFW_KEY_4) {
		if (animationMode != FOUR_SIDE_ORBIT)
			orbitAngle = 0.0f;
		ToggleContinuousAnimation(FOUR_SIDE_ORBIT);
	}
	else if (key == GLFW_KEY_5) {
		if (animationMode != FOUR_MIXED_ORBIT)
			orbitAngle = 0.0f;
		ToggleContinuousAnimation(FOUR_MIXED_ORBIT);
	}
	else if (key == GLFW_KEY_Q || key == GLFW_KEY_ESCAPE)
		glfwSetWindowShouldClose(window, GLFW_TRUE);
}

void PrintControls()
{
	std::cout << "[실습 15 키보드]\n";
	std::cout << "H: 은면 제거 켜기/끄기\n";
	std::cout << "P: 육면체/사각뿔 바꾸기\n";
	std::cout << "Y: 물체 전체 y축 자전/정지\n";
	std::cout << "T/F/S/B: 윗면/앞면/옆면/뒷면 애니메이션\n";
	std::cout << "1/2: 두 면 애니메이션\n";
	std::cout << "4: 네 옆면을 y축으로 공전\n";
	std::cout << "5: 앞/뒤/위/아래 면을 x축으로 공전\n";
	std::cout << "R: 리셋, Q 또는 Esc: 종료\n";
}

int main()
{
	std::srand(static_cast<unsigned int>(std::time(nullptr)));

	if (glfwInit() == GLFW_FALSE)
		return -1;

	GLFWwindow* window = glfwCreateWindow(800, 800, "Practice 15", nullptr, nullptr);
	if (window == nullptr) {
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);
	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK) {
		glfwDestroyWindow(window);
		glfwTerminate();
		return -1;
	}

	glfwSetKeyCallback(window, KeyCallback);
	glfwSwapInterval(1);

	shaderProgramID = MakeShaderProgram("vertex-15.glsl", "fragment-basic.glsl");
	if (shaderProgramID == 0) {
		glfwDestroyWindow(window);
		glfwTerminate();
		return -1;
	}

	InitBuffer(VAO, VBO, 100);
	objectAngleXLocation = glGetUniformLocation(shaderProgramID, "uObjectAngleX");
	objectAngleYLocation = glGetUniformLocation(shaderProgramID, "uObjectAngleY");
	partAngleXLocation = glGetUniformLocation(shaderProgramID, "uPartAngleX");
	partAngleYLocation = glGetUniformLocation(shaderProgramID, "uPartAngleY");
	partAngleZLocation = glGetUniformLocation(shaderProgramID, "uPartAngleZ");
	pivotLocation = glGetUniformLocation(shaderProgramID, "uPivot");
	offsetLocation = glGetUniformLocation(shaderProgramID, "uOffset");
	scaleLocation = glGetUniformLocation(shaderProgramID, "uScale");

	ResetScene();
	PrintControls();

	float previousTime = static_cast<float>(glfwGetTime());
	while (glfwWindowShouldClose(window) == GLFW_FALSE) {
		float currentTime = static_cast<float>(glfwGetTime());
		float deltaTime = currentTime - previousTime;
		previousTime = currentTime;
		if (deltaTime > 0.05f)
			deltaTime = 0.05f;

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
