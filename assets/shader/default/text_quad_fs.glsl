#version 330 core

in vec2 fTexCoord;
out vec4 FragColor;

uniform vec4 uColor;
uniform vec2 uLowerLeftUV;
uniform vec2 uUpperRightUV;
uniform sampler2D uTexture;

void main() {
    vec2 coord = mix(uLowerLeftUV, uUpperRightUV, fTexCoord);
    float coverage = texture(uTexture, coord).r;
    // Screen quads use straight-alpha blending; coverage only scales alpha.
    FragColor = vec4(uColor.rgb, uColor.a * coverage);
}
