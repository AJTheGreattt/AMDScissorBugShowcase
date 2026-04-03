#version 330 core

uniform sampler2D uTexture;

in vec2 fTexCoords;

out vec4 color;

void main() {
    vec3 textureRGB = 1. - texture(uTexture, fTexCoords).rgb;

    color = vec4(textureRGB, texture(uTexture, fTexCoords).a);
}
