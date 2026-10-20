#version 330 core
in vec3 outColor;
out vec4 FragColor;
uniform float uAlpha;
// C++에서 받은 RGB 색상과 불투명도 uAlpha를 최종 픽셀 색상으로 사용한다.
void main(){FragColor=vec4(outColor,uAlpha);}
