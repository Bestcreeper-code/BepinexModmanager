#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;

uniform float warpStrength;

void main()
{
    vec2 uv = fragTexCoord;
    vec2 centered = uv * 2.0 - 1.0;

    float r2 = dot(centered, centered);

    centered *= 1.0 + warpStrength * 0.1 * r2;

    uv = centered * 0.5 + 0.5;

    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0)
        discard;

    vec3 col = texture(texture0, uv).rgb;

    finalColor = vec4(col, 1.0);
}