#version 330 core

// C++의 VBO에서 정점 위치를 받는다.
// Vertex의 x, y, z가 들어온다.
layout(location = 0) in vec3 vPosition;

// C++의 VBO에서 정점 색상을 받는다.
// Vertex의 r, g, b가 들어온다.
layout(location = 1) in vec3 vColor;

// 프래그먼트 셰이더로 전달할 색상
out vec3 outColor;

// C++에서 전달받는 x축 회전각
uniform float uAngleX;

// C++에서 전달받는 y축 회전각
uniform float uAngleY;

// C++에서 전달받는 이동값
uniform vec3 uOffset;


// --------------------------------------------------
// x축 회전 행렬 만들기
// --------------------------------------------------
mat4 MakeRotateX(float degree)
{
    // C++에서는 각도를 도 단위로 보내므로
    // GLSL 계산을 위해 라디안으로 바꾼다.
    float angle = radians(degree);

    float c = cos(angle);
    float s = sin(angle);

    return mat4(
        1.0, 0.0, 0.0, 0.0,
        0.0,   c,   s, 0.0,
        0.0,  -s,   c, 0.0,
        0.0, 0.0, 0.0, 1.0
    );
}


// --------------------------------------------------
// y축 회전 행렬 만들기
// --------------------------------------------------
mat4 MakeRotateY(float degree)
{
    float angle = radians(degree);

    float c = cos(angle);
    float s = sin(angle);

    return mat4(
          c, 0.0,  -s, 0.0,
        0.0, 1.0, 0.0, 0.0,
          s, 0.0,   c, 0.0,
        0.0, 0.0, 0.0, 1.0
    );
}


// --------------------------------------------------
// 이동 행렬 만들기
// --------------------------------------------------
mat4 MakeMove(vec3 offset)
{
    return mat4(
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,

        offset.x,
        offset.y,
        offset.z,
        1.0
    );
}


void main()
{
    // ----------------------------------------------
    // 화면을 비스듬하게 바라보게 만드는 고정 회전
    // ----------------------------------------------
    // 이 회전이 없으면 z축과 앞뒤 면을 보기 어렵다.
    mat4 view =
        MakeRotateX(10.0) *
        MakeRotateY(-20.0);


    // ----------------------------------------------
    // 현재 물체의 변환 행렬
    // ----------------------------------------------
    mat4 model =
        MakeMove(uOffset) *
        MakeRotateY(uAngleY) *
        MakeRotateX(uAngleX);


    // ----------------------------------------------
    // 최종 정점 위치
    // ----------------------------------------------
    gl_Position =
        view *
        model *
        vec4(vPosition, 1.0);


    // 정점 색상을 프래그먼트 셰이더에 전달한다.
    outColor = vColor;
}