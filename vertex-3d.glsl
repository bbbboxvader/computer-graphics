#version 330 core

layout(location = 0) in vec3 vPosition;
layout(location = 1) in vec3 vColor;

out vec3 outColor;

// C++에서 보내 주는 물체의 상태이다.
uniform float uAngleX;
uniform float uAngleY;
uniform vec3 uOffset;

mat4 MakeRotateX(float degree)
{
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

mat4 MakeMove(vec3 offset)
{
    return mat4(
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        offset.x, offset.y, offset.z, 1.0
    );
}

void main()
{
    // 카메라를 약간 비스듬히 둔 것처럼 모든 좌표를 같은 각도로 바라본다.
    // 축에도 똑같이 적용하므로 x, y, z 세 축을 모두 볼 수 있다.
    mat4 view = MakeRotateX(10.0) * MakeRotateY(-20.0);

    // 실제 적용 순서: x축 회전 -> y축 회전 -> 이동
    mat4 model =
        MakeMove(uOffset) *
        MakeRotateY(uAngleY) *
        MakeRotateX(uAngleX);

    // OpenGL 화면에서는 카메라 앞쪽이 -z 방향이다.
    // 기존 코드에서는 우리가 '앞'이라고 정한 +z 면이 뒤쪽으로 판정되었다.
    // z 부호를 뒤집는 간단한 직교 투영을 적용해 +z 면이 앞에 보이게 한다.
    mat4 projection = mat4(
        1.0, 0.0,  0.0, 0.0,
        0.0, 1.0,  0.0, 0.0,
        0.0, 0.0, -1.0, 0.0,
        0.0, 0.0,  0.0, 1.0
    );

    gl_Position = projection * view * model * vec4(vPosition, 1.0);
    outColor = vColor;
}
