#version 450 core

// Input from vertex shader
in VS_OUT {
    vec3 fragPos;
    vec3 normal;
    vec2 texCoord;
    vec3 tangent;
    vec3 bitangent;
    vec4 prevPos;
} fs_in;

// Uniforms
uniform sampler2D texture_diffuse1;
uniform sampler2D texture_normal1;
uniform sampler2D texture_specular1;

// Light structure
struct Light {
    vec3 position;
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    int type; // 0: directional, 1: point, 2: spot
};

#define MAX_LIGHTS 8
uniform Light lights[MAX_LIGHTS];
uniform int numLights;

// Output
layout(location = 0) out vec4 FragColor;
layout(location = 1) out vec4 Normal;
layout(location = 2) out vec4 AlbedoSpec;
layout(location = 3) out vec4 Velocity;

vec3 calculateAmbient(Light light, vec3 color)
{
    return light.ambient * color;
}

vec3 calculateDiffuse(Light light, vec3 normal, vec3 lightDir, vec3 color)
{
    float diff = max(dot(normal, lightDir), 0.0);
    return light.diffuse * diff * color;
}

vec3 calculateSpecular(Light light, vec3 normal, vec3 lightDir, vec3 viewDir, float shininess)
{
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    return light.specular * spec;
}

void main()
{
    // Get material properties from textures
    vec3 color = texture(texture_diffuse1, fs_in.texCoord).rgb;
    vec3 normal = normalize(fs_in.normal);
    float specular = texture(texture_specular1, fs_in.texCoord).r;
    
    vec3 viewDir = normalize(-fs_in.fragPos); // Simple view direction
    vec3 result = vec3(0.0);
    
    // Process lights
    for(int i = 0; i < numLights && i < MAX_LIGHTS; i++)
    {
        Light light = lights[i];
        vec3 lightDir;
        float attenuation = 1.0;
        
        if(light.type == 0) // Directional
        {
            lightDir = normalize(-light.direction);
        }
        else if(light.type == 1) // Point
        {
            vec3 lightVec = light.position - fs_in.fragPos;
            float distance = length(lightVec);
            lightDir = normalize(lightVec);
            attenuation = 1.0 / (distance * distance + 0.1);
        }
        
        // Calculate components
        vec3 ambient = calculateAmbient(light, color);
        vec3 diffuse = calculateDiffuse(light, normal, lightDir, color);
        vec3 spec = calculateSpecular(light, normal, lightDir, viewDir, 32.0) * specular;
        
        result += (ambient + diffuse + spec) * attenuation;
    }
    
    FragColor = vec4(result, 1.0);
    Normal = vec4(normalize(fs_in.normal) * 0.5 + 0.5, 1.0);
    AlbedoSpec = vec4(color, specular);
    
    // Velocity for motion blur (will be updated with animation)
    Velocity = vec4(0.0, 0.0, 0.0, 1.0);
}
