#version 330 core
// [GLSL 기초 1: 13~18쪽 / GLSL 기초 2: 28~31쪽]
// layout location 번호는 CPU의 glVertexAttribPointer 첫 인자와 같아야 한다.
// attribute는 '정점마다 다른 값'이다. VBO의 [x y z r g b]에서 각각 읽어 온다.
layout(location = 0) in vec3 vPosition;
layout(location = 1) in vec3 vColor;

// uniform은 CPU가 draw 호출 전에 설정하는 공통 값이다.
// 하나의 물체를 그리는 동안 모든 정점에 같은 모델·뷰·투영 행렬을 사용한다.
uniform mat4 modelTransform;
uniform mat4 viewTransform;
uniform mat4 projectionTransform;

// out과 다음 셰이더의 in은 이름과 타입을 일치시킨다.
// 정점에서 정한 색은 래스터화 과정에서 보간되어 프래그먼트로 전달된다.
out vec3 outColor;

void main()
{
    // [좌표계 변환 1·2] 열 벡터 오른쪽의 M부터 적용: 모델 -> 뷰 -> 투영.
    // w=1은 위치(점)여서 이동의 영향을 받는다. gl_Position은 아직 클립 좌표다.
    // 클리핑 후 w 나누기와 viewport 매핑은 OpenGL이 수행한다.
    gl_Position = projectionTransform * viewTransform * modelTransform * vec4(vPosition, 1.0);
    outColor = vColor;
}
