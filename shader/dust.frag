#version 330 core
out vec4 FragColor;
in vec2 TexCoords;
in float alpha;

uniform sampler2D texture1;
uniform vec3 lightColor;

void main()
{
    vec4 texColor = texture(texture1, TexCoords);

    // Discard fragments with low alpha for better performance
    if(texColor.a < 0.1)
        discard;

    // Subtle glow effect
    float glow = 0.2 + 0.1 * sin(TexCoords.x * 20.0 + TexCoords.y * 20.0);
    vec3 finalColor = texColor.rgb * lightColor + vec3(glow);

    // Distance-based fading
    FragColor = vec4(finalColor, texColor.a * alpha * 0.8);
}
