#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D flareTexture;
uniform vec3 flareColor;
uniform float flareOpacity;

void main() {
    vec4 texColor = texture(flareTexture, TexCoords);

    // Apply color tint and opacity
    vec3 finalColor = texColor.rgb * flareColor;
    float finalAlpha = texColor.a * flareOpacity;

    FragColor = vec4(finalColor, finalAlpha);
}
