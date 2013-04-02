#version 330 core
in vec2 uv;

uniform vec2 scale;
uniform sampler2D tex;
uniform sampler2D stretch;

out vec4 outColor;


//from http://developer.download.nvidia.com/presentations/2007/gdc/Advanced_Skin.pdf slide 98
vec4 gauss[] = vec4[]
(
	vec4(.042,.22,.437,.635)
	,vec4(.22,.101,.355,.365)
	,vec4(.433,.119,.208,0)
	,vec4(.753,.114,0,0)
	,vec4(1.412,.364,0,0)
	,vec4(2.722,.080,0,0)
);

void main()
{
	vec4 stretch = texture(stretch, uv);
	//outColor = stretch;
	//return;
	//vec2 width = scale * stretch * GaussWidth;//not sure what to use for gaussWidth
	vec3 color = vec3(0);
	for( int i = -5; i <= 5; i++ ){
		vec4 fil = gauss[abs(i)];
		vec3 irr = texture( tex, vec2( uv.x + i * scale.x, uv.y + i * scale.y ) ).rgb;
		float w = (i==0 ? 1 : .5);//since all but i==0 get added twice
		color += irr * fil.gba * w;
	}

	outColor = vec4(color,1);
}