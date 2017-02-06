uniform sampler2D velocity;
uniform sampler2D colour;

void main()
{
	float2 prev = gl_TexCoord[0].xy - texture2D(velocity, gl_TexCoord[0].xy);
	gl_FragData[0] = texture2D(colour, prev);
}