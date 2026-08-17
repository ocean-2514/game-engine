#version 330 core

out vec4 FragColor;

in vec3 fNormal;
in vec2 fTexCoord;

// uniform vec3 uColor = vec3(0.8, 0.8, 0.8);
uniform sampler2D objTexture;

void main() {

    vec3 N = normalize(fNormal);
    vec3 L = normalize(vec3(0.5, 1.0, 0.6));
    float diff = max(dot(N, L), 0.0);
    vec4 color = texture(objTexture, fTexCoord) * (0.25 + 0.75 * diff);
    FragColor = color;
}
