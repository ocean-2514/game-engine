#version 330 core

in vec3 fNormal;
in vec3 fFragPos;
in vec2 fTexCoords;

out vec4 FragColor;

uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 clothColor;
uniform vec3 cameraPos;
uniform float ambientStrength;
uniform float specularStrength;
uniform float shininess;

void main() {
    vec3 ambient = ambientStrength * lightColor;

    vec3 norm = normalize(fNormal);
    // 背面翻转法线，使光照正确区分正反面
    if (!gl_FrontFacing) {
        norm = -norm;
    }
    vec3 lightDir = normalize(lightPos - fFragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    vec3 viewDir = normalize(cameraPos - fFragPos);
    vec3 halfWay = normalize(lightDir + viewDir);
    float spec = pow(max(dot(norm, halfWay), 0.0), shininess);
    vec3 specular = specularStrength * spec * lightColor;

    vec3 result = (ambient + diffuse + specular) * clothColor;
    FragColor = vec4(result, 1.0);
}
