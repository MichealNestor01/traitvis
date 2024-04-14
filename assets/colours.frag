#version 330 core
out vec4 FragColor;
  
uniform vec3 objectColor;
uniform float objectTransparency; 
// New lighting variabels 
uniform vec3 ambientLightColour;
uniform float ambientLightStrength;
uniform vec3 diffuseLightColour;
uniform float diffuseLightStrength;
uniform vec3 diffuseLightDirection; 

in vec3 Normal;
in vec3 FragPos;

void main()
{
    vec3 ambient = ambientLightStrength * ambientLightColour;
    vec3 diffuse = diffuseLightStrength * diffuseLightColour;

    vec3 normal = normalize(Normal);
    float nDotL = max(0.0, dot(normal, diffuseLightDirection));
    vec3 globalDiffuse = nDotL * diffuse;
    vec3 result = (ambient + globalDiffuse) * objectColor;

    FragColor = vec4(result, objectTransparency);
}