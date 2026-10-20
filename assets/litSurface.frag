#version 330 core
out vec4 FragColor;

uniform vec3 objectColor;
uniform float objectTransparency;
uniform vec3 ambientLightColour;
uniform float ambientLightStrength;
uniform vec3 diffuseLightColour;
uniform float diffuseLightStrength;
uniform vec3 diffuseLightDirection;
uniform bool pointLight;
uniform vec3 lightPosition;
uniform float lightRange;
uniform float shadowFloor;
uniform bool shadowsEnabled;
uniform mat4 view;
uniform mat4 lightViewProjection;
uniform bool shadowPass;
uniform bool emissive;
uniform sampler2D peelDepth;
uniform bool hasPeelDepth;
// Depth layer 0 is the opaque surface. Later layers are transparent peels, nearest first.
uniform sampler2DArray lightDepth;
uniform sampler2DArray lightAlpha;
uniform int lightLayerCount;
// A plane through grid space. Fragments on the positive side are dropped.
// discard is the GLSL ES 3.00 form of this clip; gl_ClipDistance is desktop-only.
uniform bool clipEnabled;
uniform vec3 clipNormal;
uniform float clipDistance;

in vec3 Normal;
in vec3 FragPos;

// How far, in grid units, a flat marching-cubes facet may sit off the smooth
// surface. Coverage ramps across that distance instead of cutting on the facet.
const float kShadowSmoothDistance = 2.0;

float windowDepth(vec3 worldPos) {
    vec4 clip = lightViewProjection * vec4(worldPos, 1.0);
    return clip.z / clip.w * 0.5 + 0.5;
}

// 0 on the surface itself, 1 once the occluder is a full smooth-step in front.
float coverage(float occluderDepth, float fragmentDepth, float bias, float feather) {
    return smoothstep(bias, bias + feather, fragmentDepth - occluderDepth);
}

// Same product as transmittance() in pointLight.hpp, softened so a facet does
// not stamp its outline. The bias widens where the surface is edge-on to the light.
float keyTransmittance(vec3 normal, vec3 lightDirection, vec2 uv, float fragmentDepth, float feather) {
    float bias = max(0.0001, 0.001 * (1.0 - abs(dot(normal, lightDirection))));
    float opaqueDepth = texture(lightDepth, vec3(uv, 0.0)).r;
    float passing = 1.0 - coverage(opaqueDepth, fragmentDepth, bias, feather);
    for (int layer = 0; layer < lightLayerCount; ++layer) {
        float layerDepth = texture(lightDepth, vec3(uv, float(layer + 1))).r;
        float layerAlpha = texture(lightAlpha, vec3(uv, float(layer))).r;
        float cover = coverage(layerDepth, fragmentDepth, bias, feather);
        if (cover <= 0.0) break;
        passing *= mix(1.0, 1.0 - layerAlpha, cover);
    }
    return passing;
}

void main()
{
    if (clipEnabled && dot(clipNormal, FragPos) > clipDistance) discard;

    // A nearer peel already owns this fragment. The bias keeps the same surface
    // from being captured again when the stored depth is not bit-identical.
    if (hasPeelDepth) {
        float earlier = texture(peelDepth, gl_FragCoord.xy / vec2(textureSize(peelDepth, 0))).r;
        if (gl_FragCoord.z <= earlier + 0.000001) discard;
    }

    // The light peel stores transparency in the red channel of an R8 texture.
    if (shadowPass) {
        FragColor = vec4(objectTransparency, 0.0, 0.0, 1.0);
        return;
    }
    if (emissive) {
        // Side faces darken as they turn away, so a white light still reads as a cube.
        vec3 toCamera = vec3(inverse(view)[3]) - FragPos;
        float facing = abs(dot(normalize(Normal), normalize(toCamera)));
        float rim = 1.0 - smoothstep(0.25, 0.9, facing);
        FragColor = vec4(mix(objectColor, vec3(0.05), rim), objectTransparency);
        return;
    }

    vec3 ambient = ambientLightStrength * ambientLightColour;
    vec3 diffuse = diffuseLightStrength * diffuseLightColour;
    vec3 backlightDiffuse = 0.1 * diffuseLightColour;
    vec3 normal = normalize(Normal);

    if (!pointLight) {
        float nDotL = max(0.0, dot(normal, diffuseLightDirection));
        float nDotB = max(0.0, dot(normal, -diffuseLightDirection));
        vec3 globalDiffuse = nDotL * diffuse + nDotB * backlightDiffuse;
        FragColor = vec4((ambient + globalDiffuse) * objectColor, objectTransparency);
        return;
    }

    vec3 toLight = lightPosition - FragPos;
    float distanceToLight = length(toLight);
    vec3 lightDirection = distanceToLight > 0.0001 ? toLight / distanceToLight : vec3(0.0, 1.0, 0.0);

    float key = 0.0;
    vec4 lightClip = lightViewProjection * vec4(FragPos, 1.0);
    if (lightClip.w > 0.0) {
        vec3 ndc = lightClip.xyz / lightClip.w;
        vec2 uv = ndc.xy * 0.5 + 0.5;
        float fragmentDepth = ndc.z * 0.5 + 0.5;
        bool inLight = uv.x >= 0.0 && uv.y >= 0.0 && uv.x <= 1.0 && uv.y <= 1.0
            && fragmentDepth >= 0.0 && fragmentDepth <= 1.0;
        if (inLight) {
            float attenuation = lightRange > 0.0 ? clamp(1.0 - distanceToLight / lightRange, 0.0, 1.0) : 0.0;
            float shade = 1.0;
            if (shadowsEnabled) {
                float feather = max(abs(fragmentDepth - windowDepth(FragPos + lightDirection * kShadowSmoothDistance)), 0.0001);
                vec2 texel = 1.0 / vec2(textureSize(lightDepth, 0).xy);
                float passing = 0.0;
                for (int y = -1; y <= 1; ++y) {
                    for (int x = -1; x <= 1; ++x) {
                        vec2 sampleUv = uv + vec2(x, y) * texel;
                        passing += keyTransmittance(normal, lightDirection, sampleUv, fragmentDepth, feather);
                    }
                }
                shade = max(shadowFloor, passing / 9.0);
            }
            key = max(0.0, dot(normal, lightDirection)) * attenuation * shade;
        }
    }

    float nDotB = max(0.0, dot(normal, -lightDirection));
    vec3 globalDiffuse = key * diffuse + nDotB * backlightDiffuse;
    vec3 result = (ambient + globalDiffuse) * objectColor;
    FragColor = vec4(result, objectTransparency);
}
