#version 450 core
in vec3 vNormal;
in vec3 vWorldPos;
out vec4 FragColor;

uniform vec3 uLightPos;

void main() {
    vec3 N = normalize(vNormal);
    vec3 L = normalize(uLightPos - vWorldPos);
    float diff = max(dot(N, L), 0.0);
    vec3 base = vec3(0.25, 0.65, 0.95);
    vec3 color = base * (0.2 + diff * 0.8);
    FragColor = vec4(color, 1.0);
}
