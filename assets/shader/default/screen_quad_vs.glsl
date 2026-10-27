#version 330 core
layout(location = 0) in vec3 vPos;
layout (location = 2) in vec2 vTexCoord;

out vec2 fTexCoord;

uniform mat4 uModel;
uniform mat4 uProjection;

void main() {
    vec4 pos = uProjection * uModel * vec4(vPos, 1.0f);
    pos.z = 0.0f;
    gl_Position = pos;

    fTexCoord = vTexCoord;
}