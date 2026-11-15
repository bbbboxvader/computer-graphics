// 지엘2 GLSL 기초 + 지엘3 좌표계 변환 + 지엘4 조명에 맞춘 육면체 조명 예제.
#include <GL/glew.h> // GLEW가 OpenGL 함수들을 사용할 수 있게 선언한다.
#include <GL/glfw3.h> // GLFW의 창, 키보드, OpenGL 컨텍스트 함수를 선언한다.
#include <gl/glm/glm.hpp> // GLM의 vec3, mat4 같은 벡터와 행렬을 사용한다.
#include <gl/glm/gtc/matrix_transform.hpp> // translate, rotate, scale, lookAt, perspective를 사용한다.
#include <fstream> // 셰이더 파일을 읽는다.
#include <iostream> // 오류 메시지를 콘솔에 출력한다.
#include <iterator> // 파일 전체를 문자열로 읽을 때 사용한다.
#include <stdexcept> // 오류를 예외로 전달한다.
#include <string> // GLSL 소스와 오류 메시지를 문자열로 보관한다.

GLuint shaderProgramID = 0; // 연결된 버텍스·프래그먼트 셰이더 프로그램 번호.
GLuint VAO = 0; // GPU가 정점 데이터를 읽는 방법을 저장하는 객체 번호.
GLuint VBO = 0; // GPU에 저장한 정점 데이터 버퍼 번호.

// 숫자를 바꾸는 곳. vec3의 순서는 위치·크기이면 x,y,z이고 색이면 r,g,b이다.
const glm::vec3 objectPosition(0.0f, 0.0f, 0.0f); // 육면체 중심의 월드 위치.
const glm::vec3 objectSize(2.0f, 2.0f, 2.0f); // 원본 크기 1을 가로·높이·깊이 2로 확대.
const float objectAngle = 0.0f; // 육면체를 y축으로 회전할 각도. 단위는 도.
const glm::vec3 objectColor(1.0f, 0.5f, 0.3f); // 육면체 재질의 RGB 색.
const glm::vec3 lightPosition(1.2f, 1.0f, 2.0f); // 점광원의 월드 위치.
const glm::vec3 lightColor(1.0f, 1.0f, 1.0f); // 점광원의 색. 흰색 빛.
const glm::vec3 cameraPosition(3.0f, 2.0f, 5.0f); // 관찰자인 카메라의 월드 위치.
const float ambientStrength = 0.2f; // 주변 조명 계수. 기본 밝기를 정한다.
const float diffuseStrength = 0.7f; // 산란반사 계수. 빛을 향하는 면의 밝기를 정한다.
const float specularStrength = 0.5f; // 거울반사 계수. 반짝임의 세기를 정한다.
const float shininess = 32.0f; // 광택 계수. 클수록 하이라이트가 좁아진다.

// 각 줄은 위치 x,y,z와 법선 nx,ny,nz이다. 같은 면의 법선은 모두 같다.
const GLfloat vertices[] = {
    -.5f,-.5f,-.5f, 0,0,-1, // 뒷면, 첫 번째 삼각형의 첫 점.
     .5f, .5f,-.5f, 0,0,-1, // 뒷면, 첫 번째 삼각형의 둘째 점.
     .5f,-.5f,-.5f, 0,0,-1, // 뒷면, 첫 번째 삼각형의 셋째 점.
    -.5f,-.5f,-.5f, 0,0,-1, // 뒷면, 두 번째 삼각형의 첫 점.
    -.5f, .5f,-.5f, 0,0,-1, // 뒷면, 두 번째 삼각형의 둘째 점.
     .5f, .5f,-.5f, 0,0,-1, // 뒷면, 두 번째 삼각형의 셋째 점.
    -.5f,-.5f, .5f, 0,0,1, // 앞면, 첫 번째 삼각형의 첫 점.
     .5f,-.5f, .5f, 0,0,1, // 앞면, 첫 번째 삼각형의 둘째 점.
     .5f, .5f, .5f, 0,0,1, // 앞면, 첫 번째 삼각형의 셋째 점.
    -.5f,-.5f, .5f, 0,0,1, // 앞면, 두 번째 삼각형의 첫 점.
     .5f, .5f, .5f, 0,0,1, // 앞면, 두 번째 삼각형의 둘째 점.
    -.5f, .5f, .5f, 0,0,1, // 앞면, 두 번째 삼각형의 셋째 점.
    -.5f,-.5f,-.5f, -1,0,0, // 왼쪽 면, 첫 번째 삼각형의 첫 점.
    -.5f,-.5f, .5f, -1,0,0, // 왼쪽 면, 첫 번째 삼각형의 둘째 점.
    -.5f, .5f, .5f, -1,0,0, // 왼쪽 면, 첫 번째 삼각형의 셋째 점.
    -.5f,-.5f,-.5f, -1,0,0, // 왼쪽 면, 두 번째 삼각형의 첫 점.
    -.5f, .5f, .5f, -1,0,0, // 왼쪽 면, 두 번째 삼각형의 둘째 점.
    -.5f, .5f,-.5f, -1,0,0, // 왼쪽 면, 두 번째 삼각형의 셋째 점.
     .5f,-.5f,-.5f, 1,0,0, // 오른쪽 면, 첫 번째 삼각형의 첫 점.
     .5f, .5f, .5f, 1,0,0, // 오른쪽 면, 첫 번째 삼각형의 둘째 점.
     .5f,-.5f, .5f, 1,0,0, // 오른쪽 면, 첫 번째 삼각형의 셋째 점.
     .5f,-.5f,-.5f, 1,0,0, // 오른쪽 면, 두 번째 삼각형의 첫 점.
     .5f, .5f,-.5f, 1,0,0, // 오른쪽 면, 두 번째 삼각형의 둘째 점.
     .5f, .5f, .5f, 1,0,0, // 오른쪽 면, 두 번째 삼각형의 셋째 점.
    -.5f,-.5f,-.5f, 0,-1,0, // 바닥면, 첫 번째 삼각형의 첫 점.
     .5f,-.5f,-.5f, 0,-1,0, // 바닥면, 첫 번째 삼각형의 둘째 점.
     .5f,-.5f, .5f, 0,-1,0, // 바닥면, 첫 번째 삼각형의 셋째 점.
    -.5f,-.5f,-.5f, 0,-1,0, // 바닥면, 두 번째 삼각형의 첫 점.
     .5f,-.5f, .5f, 0,-1,0, // 바닥면, 두 번째 삼각형의 둘째 점.
    -.5f,-.5f, .5f, 0,-1,0, // 바닥면, 두 번째 삼각형의 셋째 점.
    -.5f, .5f,-.5f, 0,1,0, // 윗면, 첫 번째 삼각형의 첫 점.
     .5f, .5f, .5f, 0,1,0, // 윗면, 첫 번째 삼각형의 둘째 점.
     .5f, .5f,-.5f, 0,1,0, // 윗면, 첫 번째 삼각형의 셋째 점.
    -.5f, .5f,-.5f, 0,1,0, // 윗면, 두 번째 삼각형의 첫 점.
    -.5f, .5f, .5f, 0,1,0, // 윗면, 두 번째 삼각형의 둘째 점.
     .5f, .5f, .5f, 0,1,0  // 윗면, 두 번째 삼각형의 셋째 점.
};

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

void InitBuffer() // 정점 데이터를 GPU에 보내고 읽는 규칙을 지정한다.
{
    glGenVertexArrays(1, &VAO); // 정점 속성 설정을 보관할 VAO 하나를 만든다.
    glBindVertexArray(VAO); // 지금부터의 정점 설정을 이 VAO에 기록한다.
    glGenBuffers(1, &VBO); // 정점 데이터를 저장할 VBO 하나를 만든다.
    glBindBuffer(GL_ARRAY_BUFFER, VBO); // 이 VBO를 정점 버퍼로 선택한다.
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW); // CPU 배열을 GPU로 복사한다.
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), nullptr); // location 0은 정점마다 처음 3개인 위치를 읽는다.
    glEnableVertexAttribArray(0); // location 0의 배열 입력을 켠다.
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat))); // location 1은 위치 뒤의 법선 3개를 읽는다.
    glEnableVertexAttribArray(1); // location 1의 배열 입력을 켠다.
    glBindVertexArray(0); // 초기화가 끝난 VAO의 선택을 해제한다.
}

void DrawScene(int width, int height) // 육면체 한 개가 보이는 화면을 만든다.
{
    glViewport(0, 0, width, height); // 창의 실제 픽셀 크기를 그리기 영역으로 지정한다.
    glClearColor(0.08f, 0.10f, 0.15f, 1.0f); // 배경의 RGB와 불투명도를 정한다.
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // 이전 화면과 깊이 정보를 지운다.
    glUseProgram(shaderProgramID); // 연결해 둔 셰이더 프로그램을 선택한다.
    glm::mat4 model = glm::translate(glm::mat4(1.0f), objectPosition); // 단위행렬에서 시작해 물체 위치를 정한다.
    model = glm::rotate(model, glm::radians(objectAngle), glm::vec3(0, 1, 0)); // y축 회전을 더한다.
    model = glm::scale(model, objectSize); // 가로·높이·깊이 크기를 정한다. 정점에는 S→R→T 순서로 적용된다.
    glm::mat4 view = glm::lookAt(cameraPosition, objectPosition, glm::vec3(0, 1, 0)); // 카메라가 물체 중심을 바라보게 한다.
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), float(width) / height, 0.1f, 100.0f); // 원근 투영의 화각·화면비·near·far를 정한다.
    glUniformMatrix4fv(glGetUniformLocation(shaderProgramID, "model"), 1, GL_FALSE, &model[0][0]); // 모델 행렬 하나를 GLSL model로 보낸다.
    glUniformMatrix4fv(glGetUniformLocation(shaderProgramID, "view"), 1, GL_FALSE, &view[0][0]); // 뷰 행렬 하나를 GLSL view로 보낸다.
    glUniformMatrix4fv(glGetUniformLocation(shaderProgramID, "projection"), 1, GL_FALSE, &projection[0][0]); // 투영 행렬 하나를 GLSL projection으로 보낸다.
    glUniform3fv(glGetUniformLocation(shaderProgramID, "objectColor"), 1, &objectColor[0]); // 물체의 RGB 색 한 개를 보낸다.
    glUniform3fv(glGetUniformLocation(shaderProgramID, "lightPos"), 1, &lightPosition[0]); // 광원의 월드 위치를 보낸다.
    glUniform3fv(glGetUniformLocation(shaderProgramID, "lightColor"), 1, &lightColor[0]); // 광원의 RGB 색을 보낸다.
    glUniform3fv(glGetUniformLocation(shaderProgramID, "viewPos"), 1, &cameraPosition[0]); // 거울반사 계산에 필요한 카메라 위치를 보낸다.
    glUniform1f(glGetUniformLocation(shaderProgramID, "ambientStrength"), ambientStrength); // 주변 조명 계수를 보낸다.
    glUniform1f(glGetUniformLocation(shaderProgramID, "diffuseStrength"), diffuseStrength); // 산란반사 계수를 보낸다.
    glUniform1f(glGetUniformLocation(shaderProgramID, "specularStrength"), specularStrength); // 거울반사 계수를 보낸다.
    glUniform1f(glGetUniformLocation(shaderProgramID, "shininess"), shininess); // 광택 계수를 보낸다.
    glBindVertexArray(VAO); // 육면체의 정점 데이터와 속성 설정을 선택한다.
    glDrawArrays(GL_TRIANGLES, 0, 36); // 정점 3개씩을 삼각형으로 연결해 12개 삼각형, 육면체 한 개를 그린다.
    glBindVertexArray(0); // 그리기에 사용한 VAO의 선택을 해제한다.
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
        window = glfwCreateWindow(800, 600, "PracticeAll - Cube Lighting", nullptr, nullptr); // 800×600 창과 컨텍스트를 만든다.
        if (!window) // 창 생성이 실패했는지 검사한다.
            throw std::runtime_error("Cannot create OpenGL window"); // 오류를 전달한다.
        glfwMakeContextCurrent(window); // OpenGL 명령이 이 창의 컨텍스트에 적용되도록 한다.
        glewExperimental = GL_TRUE; // Core Profile에서 사용할 OpenGL 함수 검색을 활성화한다.
        if (glewInit() != GLEW_OK) // GLEW로 함수들을 준비하고 성공 여부를 검사한다.
            throw std::runtime_error("GLEW initialization failed"); // 오류를 전달한다.
        while (glGetError() != GL_NO_ERROR) {} // GLEW 초기 함수 검색에서 남을 수 있는 오류 상태를 비운다.
        InitShader(); // GLSL 파일을 읽어 컴파일·링크한다.
        InitBuffer(); // 육면체 정점을 GPU에 보내고 위치·법선 속성을 설정한다.
        glEnable(GL_DEPTH_TEST); // 가까운 면이 먼 면을 가리도록 깊이 테스트를 켠다.
        while (!glfwWindowShouldClose(window)) // 창 닫기 요청이 올 때까지 화면을 갱신한다.
        {
            glfwPollEvents(); // 창 이동·닫기·키보드 같은 이벤트를 처리한다.
            if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) // Esc 또는 Q를 검사한다.
                glfwSetWindowShouldClose(window, GL_TRUE); // 다음 반복 전에 종료하도록 요청한다.
            int width, height; // 실제 그리기 영역의 가로·세로 픽셀 수를 받을 변수.
            glfwGetFramebufferSize(window, &width, &height); // 현재 프레임버퍼 크기를 읽는다.
            if (width == 0 || height == 0) // 창 최소화 등으로 그릴 영역이 없는지 검사한다.
            {
                glfwWaitEvents(); // 다음 창 이벤트가 올 때까지 기다린다.
                continue; // 크기가 생기면 다음 반복에서 다시 그린다.
            }
            DrawScene(width, height); // 이번 화면의 육면체와 조명을 계산한다.
            glfwSwapBuffers(window); // 뒤쪽 버퍼에 완성한 화면을 사용자에게 보여준다.
        }
        glDeleteBuffers(1, &VBO); // GPU 정점 버퍼를 정리한다.
        glDeleteVertexArrays(1, &VAO); // 정점 속성 설정 객체를 정리한다.
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
