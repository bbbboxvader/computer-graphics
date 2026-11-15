#version 330 core // GLSL 3.30 Core 문법을 사용한다.
in vec3 FragPos; // 버텍스 셰이더에서 받은 표면 위치. 삼각형 안에서는 보간된다.
in vec3 Normal; // 버텍스 셰이더에서 받은 표면의 법선.
out vec4 FragColor; // 최종 RGB와 불투명도를 기록할 출력 변수.
uniform vec3 objectColor; // 물체 재질의 RGB 색.
uniform vec3 lightPos; // 광원의 월드 위치.
uniform vec3 lightColor; // 광원의 RGB 색.
uniform vec3 viewPos; // 관찰자인 카메라의 월드 위치.
uniform float ambientStrength; // 주변 조명 계수.
uniform float diffuseStrength; // 산란반사 계수.
uniform float specularStrength; // 거울반사 계수.
uniform float shininess; // 하이라이트 집중도를 정하는 광택 계수.
void main() // 현재 프래그먼트의 색을 계산한다.
{
    vec3 N = normalize(Normal); // 법선의 길이를 1로 만들어 방향만 사용한다.
    vec3 L = normalize(lightPos - FragPos); // 표면에서 광원으로 향하는 단위벡터.
    vec3 V = normalize(viewPos - FragPos); // 표면에서 카메라로 향하는 단위벡터.
    vec3 R = reflect(-L, N); // 들어오는 빛 -L이 법선 N에서 반사되는 방향.
    vec3 ambient = ambientStrength * lightColor; // 주변 조명값을 구한다.
    float diffuseFactor = max(dot(N, L), 0.0); // 빛을 정면으로 받을수록 1에 가까워진다.
    vec3 diffuse = diffuseStrength * diffuseFactor * lightColor; // 산란반사 조명값을 구한다.
    float specularFactor = 0.0; // 빛을 받지 않는 면의 하이라이트는 0으로 시작한다.
    if (diffuseFactor > 0.0) // 빛이 면의 앞쪽에서 오는 경우에 거울반사를 계산한다.
    {
        specularFactor = pow(max(dot(V, R), 0.0), shininess); // 시선과 반사 방향이 일치하는 부분에 반짝임을 모은다.
    }
    vec3 specular = specularStrength * specularFactor * lightColor; // 거울반사 조명값을 구한다.
    vec3 result = (ambient + diffuse + specular) * objectColor; // PDF의 퐁 조명식으로 재질의 최종색을 구한다.
    FragColor = vec4(result, 1.0); // 최종 RGB와 완전 불투명 1을 출력한다.
}
