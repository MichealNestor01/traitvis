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
uniform sampler2D peelDepth;
uniform bool hasPeelDepth;

in vec3 Normal;
in vec3 FragPos;

void main()
{
    // A nearer peel already owns this fragment. The bias keeps the same surface
    // from being captured again when the stored depth is not bit-identical.
    if (hasPeelDepth) {
        float earlier = texture(peelDepth, gl_FragCoord.xy / vec2(textureSize(peelDepth, 0))).r;
        if (gl_FragCoord.z <= earlier + 0.000001) discard;
    }

    vec3 ambient = ambientLightStrength * ambientLightColour;
    vec3 diffuse = diffuseLightStrength * diffuseLightColour;
    vec3 backlightDiffuse = 0.1 * diffuseLightColour;

    vec3 normal = normalize(Normal);
    float nDotL = max(0.0, dot(normal, diffuseLightDirection));
    float nDotB = max(0.0, dot(normal, -diffuseLightDirection));
    vec3 globalDiffuse = nDotL * diffuse + nDotB * backlightDiffuse;
    vec3 result = (ambient + globalDiffuse) * objectColor;

    FragColor = vec4(result, objectTransparency);
}