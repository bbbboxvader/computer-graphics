#version 330 core
in vec3 outColor;
out vec4 FragColor;
// 1.0은 완전히 불투명하다는 뜻이다.
void main() { FragColor = vec4(outColor, 1.0); }
