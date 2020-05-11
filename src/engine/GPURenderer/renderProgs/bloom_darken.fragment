uniform sampler2D u_DiffuseMap;
varying vec4	var_Local0;
varying vec2	var_TexCoords;
void main(void)
{
  vec3 col = texture2D(u_DiffuseMap, var_TexCoords.xy).rgb;
  gl_FragColor.rgb = pow( col, var_Local0.xxx );
  gl_FragColor.a	= 1.0;
}
