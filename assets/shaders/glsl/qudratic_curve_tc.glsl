#extension GL_ARB_tessellation_shader: enable

layout( vertices = 1 ) out; // same size as input,

in vec2 transformed_base_point_1[ ];
in vec2 transformed_tangent_0[ ];
in vec2 transformed_tangent_1[ ];
in vec2 transformed_cp_0[ ];
in vec2 transformed_cp_1[ ];
in float tc_distance_index[ ];

patch out vec2 p0;
patch out vec2 p1;
patch out vec2 cp;

patch out float pos_fold;
patch out float fold_mid;

patch out vec2 p0_B;
patch out vec2 p1_B;
patch out vec2 cp_B;

patch out float pos_fold_B;
patch out float fold_mid_B;

patch out float te_distance_index;

uniform float aspect_ratio;
uniform float zoom;


void main( ) {
	te_distance_index = tc_distance_index[0];
	
	//float normalized_width = zoom * 0.001f;
	float normalized_width = 0.1f;

	gl_out[ gl_InvocationID ].gl_Position =  gl_in[ gl_InvocationID ].gl_Position;
	
	vec2 b0 = gl_in[0].gl_Position.xy;
	vec2 b1 = transformed_cp_0[0];
	vec2 b2 = transformed_cp_1[0];
	vec2 b3 = transformed_base_point_1[0];
	
	// test region
	
	float tx_min = min(min(b0.x, b1.x), min(b2.x, b3.x)) - normalized_width / aspect_ratio;
	float tx_max = max(max(b0.x, b1.x), max(b2.x, b3.x)) + normalized_width / aspect_ratio;
	float ty_min = min(min(b0.y, b1.y), min(b2.y, b3.y)) - normalized_width;
	float ty_max = max(max(b0.y, b1.y), max(b2.y, b3.y)) + normalized_width;
	
	bool triangle_in_vp = (-1 <= tx_min && tx_min <= 1 && -1 <= ty_min && ty_min <= 1)
		|| (-1 <= tx_min && tx_min <= 1 && -1 <= ty_max && ty_max <= 1)
		|| (-1 <= tx_max && tx_max <= 1 && -1 <= ty_min && ty_min <= 1)
		|| (-1 <= tx_max && tx_max <= 1 && -1 <= ty_max && ty_max <= 1);
	bool vp_in_triangle = (tx_min <= -1 && -1 <= tx_max && ty_min <= -1 && -1 <= ty_max)
		|| (tx_min <= -1 && -1 <= tx_max && ty_min <= 1 && 1 <= ty_max)
		|| (tx_min <= 1 && 1 <= tx_max && ty_min <= -1 && -1 <= ty_max)
		|| (tx_min <= 1 && 1 <= tx_max && ty_min <= 1 && 1 <= ty_max);
	
	int resolution = 4;
	
	// decide on resolution
	//if(triangle_in_vp || vp_in_triangle) {
		float max_diff = float(max(tx_max - tx_min, ty_max - ty_min));
		float scale = max_diff * 35.0f;
		resolution = int(max(scale, 4.0));
	//}
	
	gl_TessLevelOuter[0] = gl_TessLevelOuter[2] = 1;
	gl_TessLevelOuter[1] = gl_TessLevelOuter[3] = resolution; // divisions are along u direction
	gl_TessLevelInner[0] = resolution;
	gl_TessLevelInner[1] = 1;
	
	b0 *= vec2(aspect_ratio, 1.0);
	b1 *= vec2(aspect_ratio, 1.0);
	b2 *= vec2(aspect_ratio, 1.0);
	b3 *= vec2(aspect_ratio, 1.0);
	
	vec2 c0 = b0 + 0.75f * (b1-b0); // control point for first quadratic
	vec2 c1 = b3 + 0.75f * (b2-b3); // control point for second quadratic
	vec2 d = (c0 + c1) / 2.0f;  // d = shared endpoint of quadratics
	vec2 d_tangent = normalize(c1 - d);

	
	// single-sided approximation
	
	p0 = b0 * vec2(1.0 / aspect_ratio, 1.0);
	p1 = d * vec2(1.0 / aspect_ratio, 1.0);
	cp = c0 * vec2(1.0 / aspect_ratio, 1.0);

	
	p0_B = d * vec2(1.0 / aspect_ratio, 1.0);
	p1_B = b3 * vec2(1.0 / aspect_ratio, 1.0);
	cp_B = c1 * vec2(1.0 / aspect_ratio, 1.0);
	
	// first quadratic
	
	{
		float curvature_numerator = 4.0f * (b0.x * (c0.y - d.y) + c0.x * (d.y - b0.y) + d.x * (b0.y - c0.y)); // = 8 times the area of the triangle formed by the control points
		float value_solve = pow(curvature_numerator * normalized_width, 0.66666666666);
		
		float gamma_prime_x_constant_term = 2.0f * (c0.x - b0.x);
		float gamma_prime_y_constant_term = 2.0f * (c0.y - b0.y);
		float gamma_prime_x_linear_term = 2.0f * (b0.x + d.x - 2.0f * c0.x);
		float gamma_prime_y_linear_term = 2.0f * (b0.y + d.y - 2.0f * c0.y);
		
		float gsquared_quadratic_term = gamma_prime_x_linear_term * gamma_prime_x_linear_term + gamma_prime_y_linear_term * gamma_prime_y_linear_term;
		float gsquared_linear_term = 2.0f * gamma_prime_x_constant_term * gamma_prime_x_linear_term + 2.0f * gamma_prime_y_constant_term * gamma_prime_y_linear_term;
		float gsquared_constant_term = gamma_prime_x_constant_term * gamma_prime_x_constant_term + gamma_prime_y_constant_term * gamma_prime_y_constant_term;
		
		float adj_constant_term = gsquared_constant_term - value_solve;
		
		float determinant = gsquared_linear_term * gsquared_linear_term - 4.0f * gsquared_quadratic_term * adj_constant_term;
		
		vec2 line_across = d - b0;
		pos_fold = -dot(vec2(-line_across.y, line_across.x), c0 - b0);
			
		if(determinant <= 0.0f || abs(gsquared_quadratic_term) < 0.0001f) {
			fold_mid = 2.0f;
		} else {
			//float pos_term = (-gsquared_linear_term + sqrt(determinant)) / (2.0f * gsquared_quadratic_term);
			//float neg_term = (-gsquared_linear_term - sqrt(determinant)) / (2.0f * gsquared_quadratic_term);
			//fold_mid =  (pos_term + neg_term) / 2.0f;
			
			fold_mid =  gsquared_linear_term / (2.0f * gsquared_quadratic_term);
		}
	}
	
	// second quadratic
	
	{
		float curvature_numerator = 4.0f * (d.x * (c1.y - b3.y) + c1.x * (b3.y - d.y) + b3.x * (d.y - c1.y)); // = 8 times the area of the triangle formed by the control points
		float value_solve = pow(curvature_numerator * normalized_width, 0.66666666666);
		
		float gamma_prime_x_constant_term = 2.0f * (c1.x - d.x);
		float gamma_prime_y_constant_term = 2.0f * (c1.y - d.y);
		float gamma_prime_x_linear_term = 2.0f * (d.x + b3.x - 2.0f * c1.x);
		float gamma_prime_y_linear_term = 2.0f * (d.y + b3.y - 2.0f * c1.y);
		
		float gsquared_quadratic_term = gamma_prime_x_linear_term * gamma_prime_x_linear_term + gamma_prime_y_linear_term * gamma_prime_y_linear_term;
		float gsquared_linear_term = 2.0f * gamma_prime_x_constant_term * gamma_prime_x_linear_term + 2.0f * gamma_prime_y_constant_term * gamma_prime_y_linear_term;
		float gsquared_constant_term = gamma_prime_x_constant_term * gamma_prime_x_constant_term + gamma_prime_y_constant_term * gamma_prime_y_constant_term;
		
		float adj_constant_term = gsquared_constant_term - value_solve;
		
		float determinant = gsquared_linear_term * gsquared_linear_term - 4.0f * gsquared_quadratic_term * adj_constant_term;
		float pos_term = (-gsquared_linear_term + sqrt(determinant)) / (2.0f * gsquared_quadratic_term);
		float neg_term = (-gsquared_linear_term - sqrt(determinant)) / (2.0f * gsquared_quadratic_term);
		
		if(determinant <= 0.0f || (pos_term >= 1.0f && neg_term >= 1.0f) || (pos_term <= 0.0f && neg_term <= 0.0f)) {
			// no fold case
			pos_fold_B = 0.0f;
			fold_mid_B = 2.0f;
		} else {
			fold_mid_B = (pos_term + neg_term) / 2.0f;
			vec2 line_across = d - b0;
			pos_fold_B = -dot(vec2(-line_across.y, line_across.x), c0 - b0);
		}
	}
}