#version 330

layout(location = 0) in vec2 vPos; 

uniform sampler2D vector_field;

void main()
{
	vec2 sampler_pos = vPos / 2 + vec2(0.5, 0.5);
	vec2 vector = texture2D(vector_field, sampler_pos).xy;
	float length = vector.length();
	vector = vector / vector.length() / 40.0 * log(1.0f * vector.length());

	vec2 parallel = vec2(-vector.y, vector.x);
	if (gl_VertexID % 3 == 0) {
	    gl_Position.xy = vPos + vector;
	} else if (gl_VertexID % 3 == 1) {
	    gl_Position.xy = vPos + -parallel / 4;
	} else {
	    gl_Position.xy = vPos + parallel / 4;
	}
}