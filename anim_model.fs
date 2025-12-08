#version 330 core
out vec4 FragColor;

in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;

uniform sampler2D texture_diffuse1;

// Directional light (sun)
uniform vec3 sunDirection;   // normalized direction *toward* the sun
uniform vec3 sunColor;       // usually white
uniform float sunIntensity;  // boost brightness (e.g., 5.0)

// Camera
uniform vec3 viewPos;

// Material
uniform float shininess;

void main()
{
    vec3 norm = normalize(Normal);

    // Invert direction: light travels *from* the sun
    vec3 lightDir = normalize(-sunDirection);

    // Ambient (boosted for sunlight)
    vec3 ambient = 0.2 * sunColor * sunIntensity;

    // Diffuse
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * sunColor * sunIntensity;

    // Specular (sharp, bright)
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(norm, halfwayDir), 0.0), shininess);
    vec3 specular = spec * sunColor * sunIntensity * 0.5; // lower to avoid washout

    vec4 texColor = texture(texture_diffuse1, TexCoords);

    vec3 result = (ambient + diffuse + specular) * texColor.rgb;

    FragColor = vec4(result, texColor.a);
}
