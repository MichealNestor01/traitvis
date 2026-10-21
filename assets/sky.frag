uniform vec3 zenithColour;
uniform vec3 groundColour;
uniform vec3 cameraPosition;
uniform mat4 invViewProj;
uniform vec2 viewportSize;

out vec4 FragColor;

// Mix the two colours by how far the view ray points up, so turning the camera moves the gradient.
void main()
{
    vec2 ndc = (gl_FragCoord.xy / viewportSize) * 2.0 - 1.0;
    vec4 world = invViewProj * vec4(ndc, 1.0, 1.0);
    vec3 direction = normalize(world.xyz / world.w - cameraPosition);
    float height = direction.y * 0.5 + 0.5;
    FragColor = vec4(mix(groundColour, zenithColour, height), 1.0);
}
