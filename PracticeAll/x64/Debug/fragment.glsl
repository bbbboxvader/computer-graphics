#version 330 core
// [GLSL 기초 2: 30쪽] vertex shader의 out vec3 outColor와 연결된다.
in vec3 outColor;

// [GLSL 기초 1: 16~17쪽 / GLSL 기초 2: 35~37쪽]
// 물체 전체의 색은 uniform으로 보낸다. 같은 육면체 VBO를 여러 색으로 재사용할 수 있다.
uniform vec3 objectColor;
uniform int useVertexColor;

// 이 출력이 프레임버퍼의 색상 쓰기 단계로 전달된다.
// 프래그먼트가 깊이 테스트 등에서 탈락하면 최종 픽셀에는 기록되지 않는다.
out vec4 FragColor;

void main()
{
    // 두 전달 방식을 함께 비교한다.
    // 1: 정점 색(attribute -> out/in)에 물체 색(uniform)을 곱한다.
    // 0: uniform 색만 사용한다. 궤도·구·방의 평면을 또렷한 단색으로 표시한다.
    vec3 vertexColor = useVertexColor != 0 ? outColor : vec3(1.0);
    FragColor = vec4(vertexColor * objectColor, 1.0); // alpha=1: 불투명
}
