#version 330 core

in vec2 fTexCoord;

out vec4 FragColor;

uniform vec4 uColor;
uniform vec2 uLowerLeftUV;
uniform vec2 uUpperRightUV;

uniform sampler2D uTexture;

void main() {

    vec2 coord = mix(uLowerLeftUV, uUpperRightUV, fTexCoord);


    FragColor = texture(uTexture, coord) * uColor;
    // FragColor = vec4(1.0, 0.0, 0.0, 1.0);
}