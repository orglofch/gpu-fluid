uniform sampler2D velocity_sampler;
uniform sampler2D colour_sampler;

uniform vec2 grid_size;

void main()
{
	vec2 velocity = texture2D(velocity_sampler, gl_TexCoord[0].st).xy;

	vec2 step = velocity / grid_size * 2.0;

	// Pseudo motion blur.
	vec3 colour_accum = texture2D(colour_sampler, gl_TexCoord[0].st).xyz;
	for (int i = 0; i < 100; ++i) {
		colour_accum += texture2D(colour_sampler, gl_TexCoord[0].st - step * i).xyz;
	}
	colour_accum /= 100.0;

	gl_FragColor.xyz = colour_accum;
}