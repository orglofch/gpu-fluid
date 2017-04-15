uniform sampler2D divergence_sampler;
uniform sampler2D pressure_sampler;

uniform vec2 grid_size;

uniform float timestep;

void main()
{
	vec2 e_x = vec2(1.0 / grid_size.x, 0.0);
	vec2 e_y = vec2(0.0, 1.0 / grid_size.y);
	vec2 uv = gl_FragCoord.xy / grid_size;

	float divergence = texture2D(divergence_sampler, uv).x;

	float pL = texture2D(pressure_sampler, uv - 2 * e_x).x;
	float pR = texture2D(pressure_sampler, uv + 2 * e_x).x;
	float pB = texture2D(pressure_sampler, uv - 2 * e_y).x;
	float pT = texture2D(pressure_sampler, uv + 2 * e_y).x;

	gl_FragData[0].x = (divergence + pL + pR + pB + pT) / 4;

	// Add a y component so it can be rendered.
	gl_FragData[0].y = gl_FragData[0].x * 20;
}