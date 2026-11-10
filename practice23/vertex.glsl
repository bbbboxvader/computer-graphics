#version 330 core
// VAO의 location 0/1과 타입을 맞춘다. 정점 하나당 한 번 실행한다.
layout(location=0) in vec3 inPosition;
layout(location=1) in vec3 inColor;
out vec3 vertexColor;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
void main(){
    vertexColor=inColor;
    gl_Position=projection*view*model*vec4(inPosition,1.0);
}
