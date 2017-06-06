uniform sampler2D velocity_sampler;

uniform vec2 grid_size;

uniform float timestep;

void main()
{
	vec2 e_x = vec2(1.0 / grid_size.x, 0.0);
	vec2 e_y = vec2(0.0, 1.0 / grid_size.y);
	vec2 uv = gl_FragCoord.xy / grid_size;

	float uL = texture2D(velocity_sampler, uv - e_x).x;
	float uR = texture2D(velocity_sampler, uv + e_x).x;
	float uB = texture2D(velocity_sampler, uv - e_y).y;
	float uT = texture2D(velocity_sampler, uv + e_y).y;
	
	gl_FragData[0].x = -2 * (e_x.x * (uR - uL) + e_y.y * (uT - uB)) / timestep;

	// Add a y component so it can be rendered.
	gl_FragData[0].y = gl_FragData[0].x * 20;
}