#version 330
uniform sampler2D gColorMap;

in vec2 TexCoord;
in vec3 fColor;
in float fAlpha;

out vec4 FragColor;

void main()
{
    float a = 1 - texture2D(gColorMap, TexCoord).g;

    if (a < .1) {
        discard;
    }

	FragColor = vec4(fColor,a*fAlpha);

}
