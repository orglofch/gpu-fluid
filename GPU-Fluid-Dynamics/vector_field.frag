uniform vec4 colour;

void main()
{
	gl_FragColor.xyz = vec4(colour.xyz, 0.5f);
}