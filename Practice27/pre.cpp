// 실습 27: PracticeAll의 창·셰이더·위치/법선·퐁 조명 코드를 확장한 독립 프로젝트.
#include <GL/glew.h> // GLEW가 OpenGL 함수들을 사용할 수 있게 선언한다.
#include <GL/glfw3.h> // GLFW의 창, 키보드, OpenGL 컨텍스트 함수를 선언한다.
#include <gl/glm/glm.hpp> // GLM의 vec3, mat4 같은 벡터와 행렬을 사용한다.
#include <gl/glm/gtc/matrix_transform.hpp> // translate, rotate, scale, lookAt, perspective를 사용한다.
#include <fstream> // 셰이더 파일을 읽는다.
#include <iostream> // 오류 메시지를 콘솔에 출력한다.
#include <iterator> // 파일 전체를 문자열로 읽을 때 사용한다.
#include <stdexcept> // 오류를 예외로 전달한다.
#include <string> // GLSL 소스와 오류 메시지를 문자열로 보관한다.

#include <vector> // 사각뿔·구·원 궤도의 정점 배열을 만든다.
#include <cmath> // sin, cos, atan2, fmod로 회전과 구면 좌표를 계산한다.


GLuint shaderProgramID = 0; // PracticeAll과 같은 셰이더 프로그램 번호.
const float PI = 3.14159265358979323846f; // 도↔라디안과 원·구 생성에 쓰는 원주율.
const float ambientStrength = 0.2f; // 조명을 켰을 때의 주변 조명 계수. 끄면 0을 전달한다.
const float diffuseStrength = 0.7f; // 빛을 향하는 면의 산란반사 계수.
const float specularStrength = 0.5f; // 표면 하이라이트의 거울반사 계수.
const float shininess = 32.0f; // 높일수록 반짝이는 면적이 좁아지는 광택 계수.
bool lightingEnabled = true; // M키에서 주변광·산란반사·거울반사를 함께 켜고 끈다.
float lightAngle = 0.0f; // 장면 중심 y축을 기준으로 한 광원의 공전각.
glm::vec3 lightPosition(0); // 공전각·반지름으로 계산해 GLSL에 보내는 월드 위치.
glm::vec3 lightColor(1); // C키에서 바꿀 수 있는 현재 광원의 RGB 색.

// M: 조명 전체 켜기/끄기. C: 광원 색 순환. R/Shift+R: 광원 양/음 방향 공전 시작, 같은 방향을 다시 누르면 정지. Q/Esc: 종료.
GLuint sphereVAO = 0, sphereVBO = 0; // 위치와 매끈한 구면 법선을 저장한다.
GLsizei sphereVertexCount = 0; // 구를 그릴 때 사용할 정점 개수.
const int stacks = 24, slices = 36; // 위도·경도 분할 수.
const float lightRadius = 4.0f, lightHeight = 1.6f; // 화면 앞쪽 광원의 거리와 높이.
const float lightOrbitSpeed = 30.0f; // 초당 공전각(도). 숫자를 키우면 더 빨리 돈다.
int lightOrbitDirection = 0; // 0=정지, +1=R 방향, -1=Shift+R 방향.
int lightColorIndex = 0; // C키로 순환할 광원 색 번호.
const glm::vec3 lightColors[3] = {{1, 1, 1}, {1, .3f, .2f}, {.25f, .5f, 1}}; // 다른 세 가지 빛.
const glm::vec3 spherePositions[3] = {{0, 0, 0}, {-2.4f, -.1f, .2f}, {2.8f, 0, -.2f}}; // 중앙 1개와 양쪽 2개.
const float sphereRadii[3] = {1.0f, .65f, 1.1f}; // 그림처럼 서로 다른 구 크기.
const glm::vec3 sphereColors[3] = {{.12f, .9f, .2f}, {.12f, .25f, 1}, {1, .12f, .15f}}; // 녹색·파랑·빨강 재질.
const glm::vec3 cameraPosition(0, 2.3f, 10.5f), cameraTarget(0); // 구 세 개가 함께 보이는 시점.

GLuint CompileShader(GLenum type, const char* fileName) // GLSL 파일 하나를 읽고 컴파일한다.
{
    std::ifstream file(fileName); // 현재 작업 폴더에서 셰이더 파일을 연다.
    if (!file) // 파일 열기에 실패했는지 검사한다.
        throw std::runtime_error(std::string("Cannot open shader: ") + fileName); // 오류를 전달한다.
    std::string source((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>()); // 파일 전체를 읽는다.
    const char* text = source.c_str(); // OpenGL에 전달할 문자열의 시작 주소를 구한다.
    GLuint shader = glCreateShader(type); // 버텍스 또는 프래그먼트 셰이더 객체를 만든다.
    glShaderSource(shader, 1, &text, nullptr); // 셰이더 객체에 소스 문자열 한 개를 지정한다.
    glCompileShader(shader); // GPU 드라이버가 GLSL 코드를 컴파일한다.
    GLint success = GL_FALSE; // 컴파일 성공 여부를 받을 변수.
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success); // 컴파일 결과를 가져온다.
    if (!success) // 컴파일 오류가 발생하면 원인을 읽는다.
    {
        char log[4096] = {}; // 오류 설명을 받을 문자 배열.
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log); // 드라이버의 오류 설명을 읽는다.
        glDeleteShader(shader); // 실패한 셰이더 객체를 정리한다.
        throw std::runtime_error(std::string(fileName) + "\n" + log); // 파일명과 오류를 전달한다.
    }
    return shader; // 컴파일된 셰이더 객체 번호를 돌려준다.
}

void InitShader() // 두 셰이더를 연결해 그리기에 사용할 프로그램을 만든다.
{
    GLuint vertex = CompileShader(GL_VERTEX_SHADER, "vertex.glsl"); // 정점 셰이더를 컴파일한다.
    GLuint fragment = CompileShader(GL_FRAGMENT_SHADER, "fragment.glsl"); // 색 계산 셰이더를 컴파일한다.
    shaderProgramID = glCreateProgram(); // 셰이더 프로그램 객체를 만든다.
    glAttachShader(shaderProgramID, vertex); // 정점 셰이더를 프로그램에 붙인다.
    glAttachShader(shaderProgramID, fragment); // 프래그먼트 셰이더를 프로그램에 붙인다.
    glLinkProgram(shaderProgramID); // 두 셰이더의 입력·출력을 연결한다.
    glDeleteShader(vertex); // 연결 후 개별 셰이더 객체의 삭제를 요청한다.
    glDeleteShader(fragment); // 프로그램이 사용하는 동안 연결된 코드는 유지된다.
    GLint success = GL_FALSE; // 링크 성공 여부를 받을 변수.
    glGetProgramiv(shaderProgramID, GL_LINK_STATUS, &success); // 링크 결과를 읽는다.
    if (!success) // 연결 실패 시 원인을 읽는다.
    {
        char log[4096] = {}; // 링크 오류 설명을 받을 배열.
        glGetProgramInfoLog(shaderProgramID, sizeof(log), nullptr, log); // 오류 설명을 읽는다.
        throw std::runtime_error(log); // 오류를 main 함수로 전달한다.
    }
}


void CreateBuffer(GLuint& vao, GLuint& vbo, const GLfloat* data, size_t byteCount)
{
    glGenVertexArrays(1, &vao); // 정점 속성 설정을 보관할 객체 하나를 만든다.
    glBindVertexArray(vao); // 아래의 위치·법선 읽기 규칙을 이 VAO에 기록한다.
    glGenBuffers(1, &vbo); // 정점 데이터를 저장할 GPU 버퍼를 만든다.
    glBindBuffer(GL_ARRAY_BUFFER, vbo); // 정점 버퍼를 선택한다.
    glBufferData(GL_ARRAY_BUFFER, byteCount, data, GL_STATIC_DRAW); // CPU 배열을 GPU에 복사한다.
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), nullptr); // 위치 xyz는 처음 3개.
    glEnableVertexAttribArray(0); // vertex.glsl의 location 0 입력을 켠다.
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat))); // 법선은 뒤의 3개.
    glEnableVertexAttribArray(1); // vertex.glsl의 location 1 입력을 켠다.
    glBindVertexArray(0); // 버퍼 준비를 끝낸다.
}

void DrawMesh(GLuint vao, GLsizei count, GLenum mode, const glm::mat4& model,
              const glm::vec3& color)
{
    glUniformMatrix4fv(glGetUniformLocation(shaderProgramID, "model"), 1, GL_FALSE, &model[0][0]); // 물체 변환을 보낸다.
    glUniform3fv(glGetUniformLocation(shaderProgramID, "objectColor"), 1, &color[0]); // 물체 재질색을 보낸다.
    glBindVertexArray(vao); // 그릴 도형의 정점과 속성 설정을 선택한다.
    glDrawArrays(mode, 0, count); // 삼각형 또는 원 궤도의 선을 그린다.
}

void UpdateLightPosition()
{
    float angle = glm::radians(lightAngle); // 삼각함수에는 도 대신 라디안을 사용한다.
    lightPosition = glm::vec3(lightRadius * std::sin(angle), lightHeight,
                              lightRadius * std::cos(angle)); // y축 주위 원 궤도의 월드 위치.
}

glm::vec3 SpherePoint(float latitude, float longitude)
{
    return glm::vec3(std::sin(latitude) * std::cos(longitude), std::cos(latitude),
                     std::sin(latitude) * std::sin(longitude)); // 반지름 1인 구의 표면 위치.
}

void AppendSphereTriangle(std::vector<GLfloat>& data, const glm::vec3& a,
                          const glm::vec3& b, const glm::vec3& c)
{
    for (const glm::vec3& point : {a, b, c})
        data.insert(data.end(), {point.x, point.y, point.z, point.x, point.y, point.z}); // 단위 구는 위치 방향이 법선이다.
}

void InitBuffer()
{
    std::vector<GLfloat> sphere; // 위도·경도로 구면을 작은 삼각형으로 나눈다.
    for (int i = 0; i < stacks; ++i)
    {
        for (int j = 0; j < slices; ++j)
        {
            float lat0 = PI * i / stacks, lat1 = PI * (i + 1) / stacks; // 이웃 위도.
            float lon0 = 2 * PI * j / slices, lon1 = 2 * PI * (j + 1) / slices; // 이웃 경도.
            glm::vec3 a = SpherePoint(lat0, lon0), b = SpherePoint(lat1, lon0); // 격자의 두 점.
            glm::vec3 c = SpherePoint(lat1, lon1), d = SpherePoint(lat0, lon1); // 나머지 두 점.
            AppendSphereTriangle(sphere, a, d, c); // 바깥쪽이 반시계 방향인 첫 삼각형.
            AppendSphereTriangle(sphere, a, c, b); // 두 번째 삼각형.
        }
    }
    sphereVertexCount = static_cast<GLsizei>(sphere.size() / 6); // 정점마다 위치3+법선3.
    CreateBuffer(sphereVAO, sphereVBO, sphere.data(), sphere.size() * sizeof(GLfloat)); // GPU로 전송한다.
}

void ResetScene()
{
    lightAngle = 0.0f; // 광원은 세 구의 앞쪽에서 시작.
    lightOrbitDirection = 0; // R/Shift+R을 누르기 전에는 정지한다.
    lightColorIndex = 0; // 흰색 광원부터 시작.
    lightColor = lightColors[lightColorIndex]; // 색 번호를 RGB로 바꾼다.
    lightingEnabled = true; // 구에 퐁 조명을 적용한다.
    UpdateLightPosition(); // 공전각으로 광원 위치를 계산한다.
}

void UpdateScene(float dt)
{
    // 초당 각도 × 지난 시간 × 방향으로 프레임마다 공전각을 갱신한다.
    lightAngle = std::fmod(lightAngle + lightOrbitDirection * lightOrbitSpeed * dt, 360.0f);
    UpdateLightPosition(); // 바뀐 각도를 실제 광원 위치로 바꾼다.
}

void KeyCallback(GLFWwindow* window, int key, int, int action, int mods)
{
    if (action != GLFW_PRESS) return; // 한 번 누를 때 한 번 바꾼다.
    if (key == GLFW_KEY_Q || key == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(window, GL_TRUE); // 종료.
    if (key == GLFW_KEY_M) lightingEnabled = !lightingEnabled; // 모든 조명 성분을 함께 켜고 끈다.
    if (key == GLFW_KEY_C) // 다른 세 가지 색을 순환한다.
    {
        lightColorIndex = (lightColorIndex + 1) % 3; // 0→1→2→0 순서.
        lightColor = lightColors[lightColorIndex]; // 현재 빛의 RGB를 바꾼다.
    }
    if (key == GLFW_KEY_R) // 키를 누른 순간에 시작·정지·방향을 정한다.
    {
        int direction = (mods & GLFW_MOD_SHIFT) ? -1 : 1; // Shift+R은 반대 방향.
        lightOrbitDirection = (lightOrbitDirection == direction) ? 0 : direction; // 같은 방향을 다시 누르면 정지.
    }
}

void DrawScene(int width, int height)
{

    if (width <= 0 || height <= 0) return; // 최소화 상태의 화면비 나눗셈을 피한다.
    glViewport(0, 0, width, height); // 실제 프레임버퍼 전체를 사용한다.
    glClearColor(0.08f, 0.10f, 0.15f, 1.0f); // 배경색은 조명의 켜짐/꺼짐과 무관하다.
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // 이전 색·깊이를 지운다.
    glUseProgram(shaderProgramID); // PracticeAll의 퐁 조명 프로그램을 선택한다.
    glm::mat4 view = glm::lookAt(cameraPosition, cameraTarget, glm::vec3(0, 1, 0)); // 시선 행렬.
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), float(width) / height, 0.1f, 120.0f); // 원근 투영.
    glUniformMatrix4fv(glGetUniformLocation(shaderProgramID, "view"), 1, GL_FALSE, &view[0][0]); // 시점을 보낸다.
    glUniformMatrix4fv(glGetUniformLocation(shaderProgramID, "projection"), 1, GL_FALSE, &projection[0][0]); // 투영을 보낸다.
    glUniform3fv(glGetUniformLocation(shaderProgramID, "lightPos"), 1, &lightPosition[0]); // 광원 월드 위치.
    glUniform3fv(glGetUniformLocation(shaderProgramID, "lightColor"), 1, &lightColor[0]); // 광원 RGB.
    glUniform3fv(glGetUniformLocation(shaderProgramID, "viewPos"), 1, &cameraPosition[0]); // 거울반사에 쓰는 관찰자 위치.
    glUniform1f(glGetUniformLocation(shaderProgramID, "ambientStrength"), lightingEnabled ? ambientStrength : 0.0f); // 껐을 때 주변광도 완전히 제거한다.
    glUniform1f(glGetUniformLocation(shaderProgramID, "diffuseStrength"), lightingEnabled ? diffuseStrength : 0.0f); // M으로 산란반사 전환.
    glUniform1f(glGetUniformLocation(shaderProgramID, "specularStrength"), lightingEnabled ? specularStrength : 0.0f); // M으로 거울반사 전환.
    glUniform1f(glGetUniformLocation(shaderProgramID, "shininess"), shininess); // 광택 계수.

    for (int i = 0; i < 3; ++i) // 같은 구 메시를 위치·크기·색을 바꿔 세 번 사용한다.
    {
        glm::mat4 model = glm::translate(glm::mat4(1), spherePositions[i]); // 각 구의 위치.
        model = glm::scale(model, glm::vec3(sphereRadii[i])); // 각 구의 반지름.
        DrawMesh(sphereVAO, sphereVertexCount, GL_TRIANGLES, model, sphereColors[i]); // 구 표면 조명.
    }

    glBindVertexArray(0); // 이번 프레임의 도형 선택을 끝낸다.
}

int main() // C++ 프로그램이 처음 실행되는 함수.
{
    GLFWwindow* window = nullptr; // 나중에 만들 창을 가리킬 변수를 준비한다.
    try // 초기화·셰이더 오류가 생기면 아래 catch에서 처리한다.
    {
        if (!glfwInit()) // GLFW를 초기화하고 성공 여부를 검사한다.
            throw std::runtime_error("GLFW initialization failed"); // 실패 원인을 전달한다.
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // OpenGL 주 버전을 3으로 요청한다.
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3); // OpenGL 부 버전을 3으로 요청해 3.3을 사용한다.
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // Core Profile을 요청한다.
        window = glfwCreateWindow(800, 600, "Practice27 - Lighting", nullptr, nullptr); // 800×600 창과 컨텍스트를 만든다.
        if (!window) // 창 생성이 실패했는지 검사한다.
            throw std::runtime_error("Cannot create OpenGL window"); // 오류를 전달한다.
        glfwMakeContextCurrent(window); // OpenGL 명령이 이 창의 컨텍스트에 적용되도록 한다.
        glewExperimental = GL_TRUE; // Core Profile에서 사용할 OpenGL 함수 검색을 활성화한다.
        if (glewInit() != GLEW_OK) // GLEW로 함수들을 준비하고 성공 여부를 검사한다.
            throw std::runtime_error("GLEW initialization failed"); // 오류를 전달한다.
        while (glGetError() != GL_NO_ERROR) {} // GLEW 초기 함수 검색에서 남을 수 있는 오류 상태를 비운다.
        InitShader(); // GLSL 파일을 읽어 컴파일·링크한다.
        InitBuffer(); // 세 구가 공유할 구 정점과 법선을 GPU에 보낸다.
        ResetScene(); // 도형·광원·카메라의 초기 상태를 정한다.
        glfwSetKeyCallback(window, KeyCallback); // PDF 키 명령을 연결한다.
        glfwSwapInterval(1); // 화면 갱신을 모니터 주기에 맞춘다.
        glEnable(GL_CULL_FACE); // 보이지 않는 뒤쪽 면을 제거한다.
        glCullFace(GL_BACK); // 바깥을 향한 CCW 면을 그린다.
        glEnable(GL_DEPTH_TEST); // 가까운 면이 먼 면을 가리도록 깊이 테스트를 켠다.
        double previousTime = glfwGetTime(); // 이전 프레임의 시각(초).
        while (!glfwWindowShouldClose(window)) // 창 닫기 요청이 올 때까지 화면을 갱신한다.
        {
            glfwPollEvents(); // 창 이동·닫기·키보드 같은 이벤트를 처리한다.
            double now = glfwGetTime(); // 현재 시각(초).
            float dt = glm::clamp(static_cast<float>(now - previousTime), 0.0f, 0.1f); // 긴 멈춤 뒤 이동량을 제한한다.
            previousTime = now; // 다음 프레임의 기준 시각.
            UpdateScene(dt); // 실제 지난 시간에 맞춰 광원을 계속 공전시킨다.
            if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) // Esc 또는 Q를 검사한다.
                glfwSetWindowShouldClose(window, GL_TRUE); // 다음 반복 전에 종료하도록 요청한다.
            int width, height; // 실제 그리기 영역의 가로·세로 픽셀 수를 받을 변수.
            glfwGetFramebufferSize(window, &width, &height); // 현재 프레임버퍼 크기를 읽는다.
            if (width == 0 || height == 0) // 창 최소화 등으로 그릴 영역이 없는지 검사한다.
            {
                glfwWaitEvents(); // 다음 창 이벤트가 올 때까지 기다린다.
                continue; // 크기가 생기면 다음 반복에서 다시 그린다.
            }
            DrawScene(width, height); // 세 구와 현재 위치의 조명을 그린다.
            glfwSwapBuffers(window); // 뒤쪽 버퍼에 완성한 화면을 사용자에게 보여준다.
        }
        glDeleteBuffers(1, &sphereVBO); // 추가 도형 버퍼 정리.
        glDeleteVertexArrays(1, &sphereVAO); // 추가 도형 VAO 정리.
        glDeleteProgram(shaderProgramID); // 셰이더 프로그램을 정리한다.
        glfwDestroyWindow(window); // 창과 OpenGL 컨텍스트를 정리한다.
        glfwTerminate(); // GLFW가 사용한 자원을 정리한다.
        return 0; // 정상 종료를 운영체제에 알린다.
    }
    catch (const std::exception& error) // 전달된 오류를 받는다.
    {
        std::cerr << error.what() << '\n'; // 오류 원인을 콘솔에 출력한다.
        if (window) glfwDestroyWindow(window); // 만들어진 창이 있으면 정리한다.
        glfwTerminate(); // GLFW 자원을 정리한다.
        return 1; // 오류로 종료했음을 운영체제에 알린다.
    }
}
