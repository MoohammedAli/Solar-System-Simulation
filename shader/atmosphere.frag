#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec3 ViewDir;

uniform vec3 atmosphereColor;
uniform float atmosphereIntensity;

void main(){
    vec3 normal = normalize(Normal);
    vec3 viewDir = normalize(ViewDir);

    // Fresnel effect - glow at edges
    float fresnel = 1.0 - max(dot(viewDir, normal), 0.0);
    fresnel = pow(fresnel, 3.0);

    // Atmospheric glow
    vec3 glow = atmosphereColor * fresnel * atmosphereIntensity;

    FragColor = vec4(glow, fresnel * 0.6);
}
