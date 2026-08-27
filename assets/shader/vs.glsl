#version 330 core
layout (location = 0) in vec3 vPos;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec2 vTexCoord;

out vec3 fNormal;
out vec2 fTexCoord;
out vec3 fFragPos;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform mat3 uNormalMatrix;

void main() {
    gl_Position = uProjection * uView * uModel * vec4(vPos, 1.0f);
    fNormal = uNormalMatrix * vNormal;
    fTexCoord = vTexCoord;
    fFragPos = vec3(uModel * vec4(vPos, 1.0f));
}
