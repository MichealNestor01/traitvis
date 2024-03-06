#version 330 core
out vec4 FragColor;
  
uniform vec3 objectColor;
uniform vec3 lightColor;
uniform vec3 lightPos;
uniform vec3 viewPos;
// global lighting
uniform vec3 lightDir; // should be normalised 
uniform vec3 lightDiffuse;
uniform vec3 sceneAmbient;


in vec3 Normal;
in vec3 FragPos;

void main()
{
    float ambientStrength = 0.3;
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

    vec3 normal = normalize(Normal);
    float nDotL = max(0.0, dot(normal, lightDir));
    vec3 globalDiffuse = nDotL * lightDiffuse;
    vec3 result = (sceneAmbient + globalDiffuse + (resultPointLight * vec3(0.2))) * objectColor;

    FragColor = vec4(result, 1.0);
}