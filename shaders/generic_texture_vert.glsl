#version 330 core

layout(location = 0) in vec2 ndc;
layout(location = 1) in vec2 uvs;

out vec2 fTexCoords;

void main() {
    fTexCoords = uvs;

    gl_Position = vec4(ndc, 0., 1.);
}
