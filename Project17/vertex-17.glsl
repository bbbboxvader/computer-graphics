#version 330 core
layout(location = 0) in vec3 vPosition;
layout(location = 1) in vec3 vColor;
out vec3 outColor;
// C++에서 선택된 도형의 위치, 크기, x/y/z 자전각을 받는다.
uniform vec3 uPosition;
uniform float uScale, uAngleX, uAngleY, uAngleZ;
mat4 RX(float d){float a=radians(d),c=cos(a),s=sin(a);return mat4(1,0,0,0,0,c,s,0,0,-s,c,0,0,0,0,1);}
mat4 RY(float d){float a=radians(d),c=cos(a),s=sin(a);return mat4(c,0,-s,0,0,1,0,0,s,0,c,0,0,0,0,1);}
mat4 RZ(float d){float a=radians(d),c=cos(a),s=sin(a);return mat4(c,s,0,0,-s,c,0,0,0,0,1,0,0,0,0,1);}
mat4 Move(vec3 v){return mat4(1,0,0,0,0,1,0,0,0,0,1,0,v.x,v.y,v.z,1);}
mat4 Scale(float s){return mat4(s,0,0,0,0,s,0,0,0,0,s,0,0,0,0,1);}
void main(){
    // 실습 조건대로 좌표계와 두 객체 전체를 x축, y축에 대해 각각 30도 기울인다.
    mat4 view=RY(30.0)*RX(30.0);
    // 도형 자체 크기·자전을 먼저 적용하고 마지막에 화면상의 위치로 이동한다.
    mat4 model=Move(uPosition)*RZ(uAngleZ)*RY(uAngleY)*RX(uAngleX)*Scale(uScale);
    gl_Position=view*model*vec4(vPosition,1.0);
    outColor=vColor;
}
