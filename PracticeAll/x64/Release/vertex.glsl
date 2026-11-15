#version 330 core // GLSL 3.30 Core 문법을 사용한다.
layout(location=0) in vec3 inPosition; // VBO의 위치 x,y,z를 받는다.
layout(location=1) in vec3 inNormal; // VBO의 법선 nx,ny,nz를 받는다.
out vec3 FragPos; // 월드 공간의 표면 위치를 프래그먼트 셰이더에 전달한다.
out vec3 Normal; // 월드 공간의 법선을 프래그먼트 셰이더에 전달한다.
uniform mat4 model; // C++에서 물체의 이동·회전·크기 행렬을 받는다.
uniform mat4 view; // C++에서 카메라 기준 변환 행렬을 받는다.
uniform mat4 projection; // C++에서 원근 투영 행렬을 받는다.
void main() // 정점 하나를 처리하는 시작점.
{
    vec4 worldPosition = model * vec4(inPosition, 1.0); // 원본 위치를 월드 위치로 변환한다.
    FragPos = worldPosition.xyz; // 조명 방향 계산에 쓸 월드 위치 xyz를 보낸다.
    Normal = mat3(transpose(inverse(model))) * inNormal; // 회전·축별 크기 변경에 맞게 법선을 변환한다.
    gl_Position = projection * view * worldPosition; // 화면에 투영할 클립 공간 위치를 정한다.
}
