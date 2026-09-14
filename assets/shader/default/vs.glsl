#version 330 core
layout (location = 0) in vec3 vPos;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec2 vTexCoord;
layout (location = 3) in vec4 vBoneIds;
layout (location = 4) in vec4 vBoneWeights;

#define MAX_BONES 150

out vec3 fNormal;
out vec2 fTexCoord;
out vec3 fFragPos;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform mat3 uNormalMatrix;
uniform int uSkinned;
uniform mat4 uBones[MAX_BONES];

void main() {
    vec4 localPosition = vec4(vPos, 1.0);
    vec3 localNormal = vNormal;
    if (uSkinned != 0 && dot(vBoneWeights, vec4(1.0)) > 0.0001) {
        ivec4 boneIds = ivec4(vBoneIds);
        mat4 skin = vBoneWeights.x * uBones[boneIds.x]
                  + vBoneWeights.y * uBones[boneIds.y]
                  + vBoneWeights.z * uBones[boneIds.z]
                  + vBoneWeights.w * uBones[boneIds.w];
        localPosition = skin * localPosition;
        localNormal = mat3(skin) * localNormal;
    }
    gl_Position = uProjection * uView * uModel * localPosition;
    fNormal = uNormalMatrix * localNormal;
    fTexCoord = vTexCoord;
    fFragPos = vec3(uModel * localPosition);
}
