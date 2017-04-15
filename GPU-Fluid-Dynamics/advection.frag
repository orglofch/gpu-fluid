uniform sampler2D velocity_sampler;
uniform sampler2D colour_sampler;

uniform vec2 grid_size;

uniform vec2 mouse_position;
uniform vec2 mouse_impulse;
uniform float impulse_radius;

uniform float timestep;

void main()
{
	vec2 uv = gl_FragCoord.xy / grid_size;

	vec2 velocity = texture2D(velocity_sampler, uv).xy;
	vec2 prev_pos = (gl_FragCoord.xy - velocity * timestep) / grid_size;

	vec4 prev_colour = texture2D(colour_sampler, prev_pos);
	vec4 prev_velocity = texture2D(velocity_sampler, prev_pos);

	// Apply mouse force.
	float dist = distance(gl_FragCoord.xy, mouse_position);
	float r = min(dist / impulse_radius, 1.0);
	float mag = 1.0 - r;
	prev_velocity.xy += mouse_impulse * mag*mag;
	
	// Add some additional ink within the mouse radius.
	if (r < 1.0) {
		prev_colour.x = mod(gl_FragCoord.x + gl_FragCoord.y, 100) < 50;
		prev_colour.y = mod(gl_FragCoord.x, 100) < 50;
		prev_colour.z = mod(gl_FragCoord.y, 100) < 50;
	}

	gl_FragData[0] = prev_colour;
	gl_FragData[1] = prev_velocity;
}