#version 330 core

const vec2 positions[6] = vec2[](
    // 0, 1, 2번: 왼쪽 삼각형
    vec2(-0.2, -0.6),
    vec2(-0.8, -0.6),
    vec2(-0.5,  0.2),

    // 3, 4, 5번: 오른쪽 삼각형
    vec2(0.2, -0.6),
    vec2(0.8, -0.6),
    vec2(0.5,  0.2)
);

void main()
{
    vec2 position = positions[gl_VertexID];
    gl_Position = vec4(position, 0.0, 1.0);
}