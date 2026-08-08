#version 330 core
layout (location = 0) in vec3 vPos;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec2 vTexCoords;

out vec3 fNormal;
out vec3 fFragPos;
out vec2 fTexCoords;

uniform mat4 model, view, projection;
uniform mat3 normalMatrix;

void main() {
    gl_Position = projection * view * model * vec4(vPos, 1.0);
    fNormal    = normalMatrix * vNormal;
    fFragPos   = vec3(model * vec4(vPos, 1.0));
    fTexCoords = vTexCoords;
}
