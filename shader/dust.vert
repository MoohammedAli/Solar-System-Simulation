#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTex;
out vec2 TexCoords;
out float alpha;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 cameraRight;
uniform vec3 cameraUp;
uniform float size;

void main()
{
    vec3 pos = aPos;
    vec3 worldPos = (model * vec4(pos, 1.0)).xyz;

    // Billboard effect
    vec3 posWorld = worldPos +
        cameraRight * aPos.x * size +
        cameraUp * aPos.y * size;

    gl_Position = projection * view * vec4(posWorld, 1.0);
    TexCoords = aTex;

    // Distance-based alpha (fade with distance)
    float dist = distance(worldPos, vec3(0.0));
    alpha = 1.0 - smoothstep(50.0, 300.0, dist);
}
