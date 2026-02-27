#version 450 core
in vec3 vNormal;
in vec3 vWorldPos;
out vec4 FragColor;

uniform vec3 uLightPos;
uniform vec3 uCameraPos;
uniform vec3 uAlbedo;
uniform float uShininess;

void main() {
    vec3 N = normalize(vNormal);
    vec3 L = normalize(uLightPos - vWorldPos);
    vec3 V = normalize(uCameraPos - vWorldPos);
    vec3 H = normalize(L + V);

    float diff = max(dot(N, L), 0.0);
    float spec = pow(max(dot(N, H), 0.0), uShininess);

    vec3 ambient = 0.08 * uAlbedo;
    vec3 diffuse = diff * uAlbedo;
    vec3 specular = vec3(0.25) * spec;
    FragColor = vec4(ambient + diffuse + specular, 1.0);
}
