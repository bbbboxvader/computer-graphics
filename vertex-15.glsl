#version 330 core

layout(location = 0) in vec3 vPosition;
layout(location = 1) in vec3 vColor;

out vec3 outColor;

// 물체 전체의 기울기와 y축 자전
uniform float uObjectAngleX;
uniform float uObjectAngleY;

// 면 하나에만 적용할 변환값
uniform float uPartAngleX;
uniform float uPartAngleY;
uniform float uPartAngleZ;
uniform vec3 uPivot;
uniform vec3 uOffset;
uniform vec3 uScale;

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

mat4 MakeRotateZ(float degree)
{
    float angle = radians(degree);
    float c = cos(angle);
    float s = sin(angle);

    return mat4(
          c,   s, 0.0, 0.0,
         -s,   c, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
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

mat4 MakeScale(vec3 scaleValue)
{
    return mat4(
        scaleValue.x, 0.0, 0.0, 0.0,
        0.0, scaleValue.y, 0.0, 0.0,
        0.0, 0.0, scaleValue.z, 0.0,
        0.0, 0.0, 0.0, 1.0
    );
}

void main()
{
    // 축과 물체의 z방향을 보기 위한 고정된 시점이다.
    mat4 view = MakeRotateX(10.0) * MakeRotateY(-20.0);

    // 면의 회전 중심을 원점으로 옮긴 뒤 회전/크기 변환하고 다시 돌려놓는다.
    mat4 part =
        MakeMove(uOffset) *
        MakeMove(uPivot) *
        MakeRotateZ(uPartAngleZ) *
        MakeRotateY(uPartAngleY) *
        MakeRotateX(uPartAngleX) *
        MakeScale(uScale) *
        MakeMove(-uPivot);

    // 면 애니메이션 뒤에 물체 전체의 기울기와 자전을 적용한다.
    mat4 object =
        MakeRotateY(uObjectAngleY) *
        MakeRotateX(uObjectAngleX);

    // +z로 만든 앞면이 깊이 검사에서도 실제 앞면으로 보이도록
    // 화면에 보내기 전에 z방향을 뒤집는다.
    mat4 projection = mat4(
        1.0, 0.0,  0.0, 0.0,
        0.0, 1.0,  0.0, 0.0,
        0.0, 0.0, -1.0, 0.0,
        0.0, 0.0,  0.0, 1.0
    );

    gl_Position = projection * view * object * part * vec4(vPosition, 1.0);
    outColor = vColor;
}
