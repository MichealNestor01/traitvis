#version 330 core
out vec4 FragColor;
  
uniform vec3 objectColor;
uniform float objectTransparency; 
uniform vec3 lightColor;
uniform vec3 lightPos;
uniform vec3 viewPos;
// global lighting
uniform vec3 lightDir; // should be normalised 
uniform vec3 lightDiffuse;
uniform vec3 sceneAmbient;
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
    /*
    float ambientStrength = 1.0;
    vec3 ambient = ambientStrength * lightColor;

    
    vec3 norm = normalize(Normal);
    vec3 pointLightDir = normalize(lightPos-FragPos);
    float diff = max(dot(norm, pointLightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    float specularStrength = 0.2;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-pointLightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32); // last val is shininess
    vec3 specular = specularStrength * spec * lightColor;

    vec3 resultPointLight = (ambient+diffuse+specular) * objectColor; 
    */
    vec3 ambient = ambientLightStrength * ambientLightColour;
    vec3 diffuse = diffuseLightStrength * diffuseLightColour;

    vec3 normal = normalize(Normal);
    float nDotL = max(0.0, dot(normal, diffuseLightDirection));
    vec3 globalDiffuse = nDotL * diffuse;
    vec3 result = (ambient + globalDiffuse) * objectColor;

    FragColor = vec4(result, objectTransparency);
}