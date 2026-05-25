#version 450 core

in VS_OUT {
    vec3 fragPos;
    vec3 normal;
    vec2 texCoord;
    vec4 fragPosLightSpace;
} fs_in;

// Textures
uniform sampler2D   texture_diffuse1;
uniform sampler2D   texture_normal1;
uniform sampler2DShadow shadowMap;     // hardware shadow comparison sampler

// Lighting
struct Light {
    vec3  position;
    vec3  direction;
    vec3  ambient;
    vec3  diffuse;
    vec3  specular;
    float constant;
    float linear;
    float quadratic;
    int   type;   // 0=directional, 1=point, 2=spot
};
uniform Light lights[8];
uniform int   numLights;

// Material
uniform vec3  viewPos;
uniform float shininess;

out vec4 FragColor;

// PCF — 3x3 kernel averages 9 depth samples for soft edges
float shadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir)
{
    // Perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    // Outside shadow map = fully lit
    if (projCoords.z > 1.0) return 0.0;

    // Slope-scaled depth bias prevents shadow acne
    float bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.001);
    projCoords.z -= bias;

    // PCF 3x3
    float shadow = 0.0;
    vec2  texelSize = 1.0 / textureSize(shadowMap, 0);
    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            vec2 offset = vec2(x, y) * texelSize;
            // texture() on sampler2DShadow returns 0.0 or 1.0 via GL_COMPARE_REF_TO_TEXTURE
            shadow += texture(shadowMap, vec3(projCoords.xy + offset, projCoords.z));
        }
    }
    shadow /= 9.0;
    return 1.0 - shadow;  // 0 = lit, 1 = fully in shadow
}

vec3 calcLight(Light light, vec3 normal, vec3 viewDir, vec3 diffuseColor)
{
    vec3 lightDir;
    float attenuation = 1.0;

    if (light.type == 0) {
        lightDir = normalize(-light.direction);
    } else {
        lightDir = normalize(light.position - fs_in.fragPos);
        float d   = length(light.position - fs_in.fragPos);
        attenuation = 1.0 / (light.constant + light.linear * d + light.quadratic * d * d);
    }

    float diff = max(dot(normal, lightDir), 0.0);
    vec3  halfway = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfway), 0.0), shininess);

    vec3 ambient  = light.ambient  * diffuseColor;
    vec3 diffuse  = light.diffuse  * diff * diffuseColor;
    vec3 specular = light.specular * spec;

    return (ambient + diffuse + specular) * attenuation;
}

void main()
{
    vec3 diffuseColor = texture(texture_diffuse1, fs_in.texCoord).rgb;
    vec3 normal       = normalize(fs_in.normal);
    vec3 viewDir      = normalize(viewPos - fs_in.fragPos);

    // Sun shadow (from light[0] which should be directional)
    vec3  sunDir    = normalize(-lights[0].direction);
    float shadowFac = (numLights > 0) ? shadowCalculation(fs_in.fragPosLightSpace, normal, sunDir) : 0.0;

    vec3 result = vec3(0.0);
    for (int i = 0; i < numLights && i < 8; i++) {
        vec3 contrib = calcLight(lights[i], normal, viewDir, diffuseColor);
        // Apply shadow only to the main directional light (index 0)
        if (i == 0) contrib *= (1.0 - shadowFac * 0.7);
        result += contrib;
    }

    FragColor = vec4(result, 1.0);
}
