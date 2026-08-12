#version 330 core
layout (location = 0) in vec2 vPos;
layout (location = 1) in vec3 vColor;

out vec3 fColor;

uniform mat4 uModel;

void main() {
    gl_Position = uModel * vec4(vPos, 0.0f, 1.0f);
    fColor = vColor;
}
