#version 330 core

#define MAX_POINT_LIGHTS 16
#define MAX_SPOT_LIGHTS 8
#define MAX_DIRECTIONAL_LIGHTS 2

out vec4 FragColor;

in vec3 fNormal;
in vec2 fTexCoord;
in vec3 fFragPos;

struct DirectionalLight {
    vec3 direction;
    float intensity;
    vec3 color;
    float padding;
};

struct PointLight {
    vec3 position;
    float range;
    vec3 color;
    float intensity;
};

struct SpotLight {
    vec3 position;
    float range;
    vec3 direction;
    float intensity;
    vec3 color;
    float innerConeCos;
    float outerConeCos;
};

vec3 GetSpotLightColor(vec3 norm, vec3 viewDir, SpotLight light,
    vec3 albedo, vec3 specularColor);
vec3 GetPointLightColor(vec3 norm, vec3 viewDir, PointLight light,
    vec3 albedo, vec3 specularColor);
vec3 GetDirectionalLightColor(vec3 norm, vec3 viewDir, DirectionalLight light,
    vec3 albedo, vec3 specularColor);

uniform DirectionalLight uDirectionalLights[MAX_DIRECTIONAL_LIGHTS];
uniform SpotLight uSpotLights[MAX_SPOT_LIGHTS];
uniform PointLight uPointLights[MAX_POINT_LIGHTS];
uniform int uDirectionalLightCount;
uniform int uPointLightCount;
uniform int uSpotLightCount;
uniform vec3 uAmbientColor;
uniform float uAmbientIntensity;

uniform vec3 uViewPos;

uniform sampler2D uDiffuseMap;
uniform sampler2D uSpecularMap;
uniform sampler2D uOpacityMap;
uniform vec3 uDiffuse = vec3(1.0);
uniform vec3 uSpecular = vec3(1.0);
uniform float uShininess = 32.0;
uniform float uOpacity = 1.0;
uniform int uHasOpacityMap = 0;
uniform int uHasSpecularMap = 0;
uniform int uAlphaMasked = 0;
uniform float uAlphaCutoff = 0.5;

void main() {
    vec4 texColor = texture(uDiffuseMap, fTexCoord);
    float alpha = texColor.a * uOpacity;
    if (uHasOpacityMap != 0) {
        alpha *= texture(uOpacityMap, fTexCoord).r;
    }
    if (uAlphaMasked != 0 && alpha < uAlphaCutoff) {
        discard;
    }

    vec3 normal = normalize(fNormal);
    vec3 viewDir = normalize(uViewPos - fFragPos);
    vec3 albedo = texColor.rgb * uDiffuse;
    vec3 specularColor = uSpecular;
    if (uHasSpecularMap != 0) {
        specularColor *= texture(uSpecularMap, fTexCoord).rgb;
    }

    vec3 color = uAmbientColor * uAmbientIntensity * albedo * 0.0;
    for (int i = 0; i < uDirectionalLightCount; ++i) {
        color += GetDirectionalLightColor(normal, 
            viewDir, uDirectionalLights[i], albedo, specularColor);
    }
    for (int i = 0; i < uPointLightCount; ++i) {
        color += GetPointLightColor(normal, 
            viewDir, uPointLights[i], albedo, specularColor);
    }
    for (int i = 0; i < uSpotLightCount; ++i) {
        color += GetSpotLightColor(normal, 
            viewDir, uSpotLights[i], albedo, specularColor);
    }

    FragColor = vec4(color, alpha);
}

vec3 GetDirectionalLightColor(vec3 norm, vec3 viewDir, DirectionalLight light,
    vec3 albedo, vec3 specularColor) {
    vec3 lightDir = normalize(-light.direction);
    float diff = max(0.0, dot(lightDir, norm));
    vec3 diffuse = diff * light.color * light.intensity 
        * albedo;

    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(0.0, dot(halfwayDir, norm)), uShininess);
    vec3 specular = spec * light.color * light.intensity 
        * specularColor;

    return diffuse + specular;
}

vec3 GetPointLightColor(vec3 norm, vec3 viewDir, PointLight light,
    vec3 albedo, vec3 specularColor) {
    vec3 toLight = light.position - fFragPos;
    float distance = length(toLight);
    float d = distance / max(light.range, 0.0001);
    float rangeFactor = max(1.0 - d, 0.0);
    float attenuation = rangeFactor * rangeFactor / (1.0 + 25.0 * d * d);

    vec3 lightDir = toLight / max(distance, 0.0001);
    float diff = max(0.0, dot(lightDir, norm));
    vec3 diffuse = diff * light.color * light.intensity 
        * albedo;

    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(0.0, dot(halfwayDir, norm)), uShininess);
    vec3 specular = spec * light.color * light.intensity 
        * specularColor;

    return attenuation * (diffuse + specular);
}

vec3 GetSpotLightColor(vec3 norm, vec3 viewDir, SpotLight light,
    vec3 albedo, vec3 specularColor) {
    vec3 toLight = light.position - fFragPos;
    float distance = length(toLight);
    float d = distance / max(light.range, 0.0001);
    float rangeFactor = max(1.0 - d, 0.0);
    float attenuation = rangeFactor * rangeFactor / (1.0 + 25.0 * d * d);

    vec3 lightDir = toLight / max(distance, 0.0001);
    float diff = max(0.0, dot(lightDir, norm));
    vec3 diffuse = diff * light.color * light.intensity 
        * albedo;

    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(0.0, dot(halfwayDir, norm)), uShininess);
    vec3 specular = spec * light.color * light.intensity 
        * specularColor;

    float denom = max(light.innerConeCos - light.outerConeCos, 0.0001);
    float cosTheta = dot(-lightDir, normalize(light.direction));
    float intensity = clamp((cosTheta - light.outerConeCos) / denom, 0.0, 1.0);

    return attenuation * intensity * (diffuse + specular);
}

