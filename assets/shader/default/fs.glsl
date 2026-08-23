#version 330 core

out vec4 FragColor;

in vec3 fNormal;
in vec2 fTexCoord;

uniform vec3 uDiffuse;
uniform sampler2D uDiffuseMap;
uniform sampler2D uOpacityMap;
uniform float uOpacity = 1.0;
uniform int uHasOpacityMap = 0;
uniform int uAlphaMasked = 0;
uniform float uAlphaCutoff = 0.5;

void main() {
    FragColor = vec4(1.0f, 0.0f, 0.0f, 1.0f);
    vec3 N = normalize(fNormal);
    vec3 L = normalize(vec3(0.5, 1.0, 0.6));
    float diff = max(dot(N, L), 0.0);
    vec4 texColor = texture(uDiffuseMap, fTexCoord);
    float alpha = texColor.a * uOpacity;
    if (uHasOpacityMap != 0) {
        alpha *= texture(uOpacityMap, fTexCoord).r;
    }
    if (uAlphaMasked != 0 && alpha < uAlphaCutoff) {
        discard;
    }
    FragColor = vec4(
        texColor.rgb * (0.25 + 0.75 * diff),
        1.0f);
}
