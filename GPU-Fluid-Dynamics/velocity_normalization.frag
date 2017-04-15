uniform sampler2D velocity_sampler;
uniform sampler2D pressure_sampler;

uniform vec2 grid_size;

uniform float timestep;

void main()
{
	vec2 e_x = vec2(1.0 / grid_size.x, 0.0);
	vec2 e_y = vec2(0.0, 1.0 / grid_size.y);
	vec2 uv = gl_FragCoord.xy / grid_size;

	vec2 velocity = texture2D(velocity_sampler, uv).xy;

	float pL = texture2D(pressure_sampler, uv - e_x).x;
	float pR = texture2D(pressure_sampler, uv + e_x).x;
	float pB = texture2D(pressure_sampler, uv - e_y).x;
	float pT = texture2D(pressure_sampler, uv + e_y).x;

	// Subtract the gradient of the pressure.
	gl_FragData[0].x = velocity.x - timestep / (2 * e_x.x) * (pR - pL);
	gl_FragData[0].y = velocity.y - timestep / (2 * e_y.y) * (pT - pB);
}