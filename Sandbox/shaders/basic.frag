#version 450 core
in vec3 vNormal;
in vec3 vWorldPos;
out vec4 FragColor;

uniform vec3 uBaseColor;
uniform vec3 uLightPos;
uniform vec3 uLightColor;
uniform vec3 uViewPos;
uniform float uAmbientStrength;
uniform float uRoughness;
uniform float uShadowStrength;

void main() {
    vec3 N = normalize(vNormal);
    vec3 L = normalize(uLightPos - vWorldPos);
    vec3 V = normalize(uViewPos - vWorldPos);
    vec3 H = normalize(L + V);

    float diff = max(dot(N, L), 0.0);
    float specPower = mix(96.0, 16.0, clamp(uRoughness, 0.0, 1.0));
    float spec = pow(max(dot(N, H), 0.0), specPower) * (1.0 - clamp(uRoughness, 0.0, 1.0));

    float distanceToLight = length(uLightPos - vWorldPos);
    float attenuation = 1.0 / (1.0 + 0.08 * distanceToLight + 0.025 * distanceToLight * distanceToLight);
    float contactShadow = 1.0 - uShadowStrength * smoothstep(0.0, 1.25, max(vWorldPos.y + 2.0, 0.0));

    vec3 ambient = uBaseColor * uAmbientStrength;
    vec3 diffuse = uBaseColor * diff * attenuation * contactShadow;
    vec3 specular = uLightColor * spec * attenuation * contactShadow;
    vec3 color = ambient + diffuse * uLightColor + specular;
    color = pow(color, vec3(1.0 / 2.2));
    FragColor = vec4(color, 1.0);
}
