#version 330 core

uniform sampler2D uTexture;

in vec2 fTexCoords;

out vec4 color;

void main() {
    vec3 texColor = texture(uTexture, fTexCoords).rgb;

    color = vec4(1.0 - texColor, texture(uTexture, fTexCoords).a);
}
