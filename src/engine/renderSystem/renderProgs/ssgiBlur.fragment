uniform sampler2D u_DiffuseMap;

varying vec4	var_Local0; // scan_pixel_size_x, scan_pixel_size_y, scan_width, is_ssgi
varying vec2	var_Dimensions;
varying vec2	var_TexCoords;

void main(void)
{
	float NUM_VALUES = 1.0;
	vec2 PIXEL_OFFSET = vec2(1.0 / var_Dimensions);

	vec3 col0 = texture2D(u_DiffuseMap, var_TexCoords.xy).rgb;
	gl_FragColor.rgb = col0.rgb;

	for (float width = 1.0; width <= var_Local0.z; width += 1.0)
	{
		float dist_mult = clamp(1.0 - (width / var_Local0.z), 0.0, 1.0);
		vec3 col1 = texture2D(u_DiffuseMap, var_TexCoords.xy + ((var_Local0.xy * width) * PIXEL_OFFSET)).rgb;
		vec3 col2 = texture2D(u_DiffuseMap, var_TexCoords.xy - ((var_Local0.xy * width) * PIXEL_OFFSET)).rgb;
		vec3 add_color = ((col0 / 2) + ((col1 + col2) * (dist_mult * 2.0))) / 4;

		//add_color.rgb *= 2.0;
		float mult = clamp((3.0 - (add_color.r + add_color.g + add_color.b)) + 0.1, 0.0, 3.0);
		add_color.rgb *= mult;

		gl_FragColor.rgb += clamp(add_color, 0.0, 1.0);
		NUM_VALUES += 1.0;
	}

	gl_FragColor.rgb /= NUM_VALUES;
	gl_FragColor.a	= 1.0;
}
