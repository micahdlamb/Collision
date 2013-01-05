#version 330 core
in vec2 coordXY;

uniform vec2 coordScale;
uniform sampler2D tex;

out vec4 outColor;

vec2 gaussFilter[] = vec2[]
(
	vec2(-3.0,	0.015625),
	vec2(-2.0,	0.09375),
	vec2(-1.0,	0.234375),
	vec2(0.0,	0.3125),
	vec2(1.0,	0.234375),
	vec2(2.0,	0.09375),
	vec2(3.0,	0.015625)
);

void main()
{
	vec4 color = vec4(0.0);
	for( int i = 0; i < 7; i++ )
	{
		color += texture2D( tex, vec2( coordXY.x+gaussFilter[i].x*coordScale.x, coordXY.y+gaussFilter[i].x*coordScale.y ) )*gaussFilter[i].y;
	}

	outColor = color;
}