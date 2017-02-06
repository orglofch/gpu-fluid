uniform sampler2D colour;

void main()
{
	gl_FragColor = texture2D(colour, gl_TexCoord[0].xy);
}