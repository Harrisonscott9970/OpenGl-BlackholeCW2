#version 330 core
// blur.frag — separable 13-tap Gaussian blur for bloom

in vec2 vUV;

uniform sampler2D image;
uniform int       horizontal;
uniform int       firstPass;
uniform vec2      uResolution;

out vec4 FragColor;

// 13-tap Gaussian weights (sigma ≈ 3)
const float weight[7] = float[](
    0.1953125,
    0.1640625,
    0.1171875,
    0.0703125,
    0.0351563,
    0.0136719,
    0.0039063
);

void main()
{
    vec2 texel = 1.0 / uResolution;
    vec3 result = texture(image, vUV).rgb * weight[0];

    if (horizontal == 1) {
        for (int i = 1; i < 7; i++) {
            result += texture(image, vUV + vec2(texel.x * float(i), 0.0)).rgb * weight[i];
            result += texture(image, vUV - vec2(texel.x * float(i), 0.0)).rgb * weight[i];
        }
    } else {
        for (int i = 1; i < 7; i++) {
            result += texture(image, vUV + vec2(0.0, texel.y * float(i))).rgb * weight[i];
            result += texture(image, vUV - vec2(0.0, texel.y * float(i))).rgb * weight[i];
        }
    }

    // First bloom pass: threshold — only bright pixels contribute
    if (firstPass == 1) {
        float brightness = dot(result, vec3(0.2126, 0.7152, 0.0722));
        if (brightness < 0.85) result = vec3(0.0);
    }

    FragColor = vec4(result, 1.0);
}
