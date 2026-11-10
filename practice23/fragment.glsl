#version 330 core
// vertex shader의 out과 이름/타입을 맞춘다. 면 내부에서는 색이 보간된다.
in vec3 vertexColor;
uniform vec3 objectColor;
out vec4 FragColor;
void main(){
    FragColor=vec4(vertexColor*objectColor,1.0);
}
