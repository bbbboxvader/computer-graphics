#version 330 core
layout(location = 0) in vec3 vPosition;
layout(location = 1) in vec3 vColor;
out vec3 outColor;
// uObjectAngle은 사각뿔 전체 기울기, uPartAngle은 현재 옆면 하나의 회전각이다.
uniform float uObjectAngleX, uObjectAngleY;
uniform float uPartAngleX, uPartAngleY, uPartAngleZ;
// uPivot은 옆면을 회전시킬 아래 모서리 또는 꼭짓점 좌표이다.
uniform vec3 uPivot;

mat4 RotateX(float d) { float a=radians(d),c=cos(a),s=sin(a); return mat4(1,0,0,0, 0,c,s,0, 0,-s,c,0, 0,0,0,1); }
mat4 RotateY(float d) { float a=radians(d),c=cos(a),s=sin(a); return mat4(c,0,-s,0, 0,1,0,0, s,0,c,0, 0,0,0,1); }
mat4 RotateZ(float d) { float a=radians(d),c=cos(a),s=sin(a); return mat4(c,s,0,0, -s,c,0,0, 0,0,1,0, 0,0,0,1); }
mat4 Move(vec3 v) { return mat4(1,0,0,0, 0,1,0,0, 0,0,1,0, v.x,v.y,v.z,1); }

void main()
{
    // 10도와 -20도는 z방향 깊이가 화면에서 잘 보이게 하는 고정 시점 각도이다.
    mat4 view = RotateX(10.0) * RotateY(-20.0);
    // 기준점을 원점으로 옮기고 회전한 뒤 다시 원래 기준점 위치로 돌려놓는다.
    mat4 part = Move(uPivot) * RotateZ(uPartAngleZ) * RotateY(uPartAngleY)
              * RotateX(uPartAngleX) * Move(-uPivot);
    // 면별 변환을 마친 뒤 사각뿔 전체 기울기를 적용한다.
    mat4 object = RotateY(uObjectAngleY) * RotateX(uObjectAngleX);
    // z의 부호를 뒤집어 현재 좌표계에서 앞뒤 깊이 검사가 올바르게 보이게 한다.
    mat4 projection = mat4(1,0,0,0, 0,1,0,0, 0,0,-1,0, 0,0,0,1);
    gl_Position = projection * view * object * part * vec4(vPosition, 1.0);
    outColor = vColor;
}
