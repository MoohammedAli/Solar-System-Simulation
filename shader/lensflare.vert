#version 330 core
layout(location = 0) in vec2 aPos;      // Quad corners: -1 to 1
layout(location = 1) in vec2 aTexCoord; // Texture coords: 0 to 1

out vec2 TexCoords;

uniform vec2 flarePosition;  // Screen position of flare element
uniform float flareSize;     // Size of this flare element

void main() {
    TexCoords = aTexCoord;

    // Position quad at flare location with given size
    vec2 position = flarePosition + aPos * flareSize;

    gl_Position = vec4(position, 0.0, 1.0);
}
