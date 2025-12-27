#extension GL_ARB_tessellation_shader: enable

layout( quads, fractional_odd_spacing, ccw ) in;

patch in vec2 p0;
patch in vec2 p1;
patch in vec2 cp;

patch in float pos_fold;
patch in float fold_mid;

patch in vec2 p0_B;
patch in vec2 p1_B;
patch in vec2 cp_B;

patch in float pos_fold_B;
patch in float fold_mid_B;

patch in float te_distance_index;

out vec2 texture_coord;

uniform samplerBuffer curves;
uniform float aspect_ratio;
uniform float zoom;
uniform float distance_adjust;

void main( ) {
	//float normalized_width = zoom * 0.001f;
	float normalized_width = 0.1f;

	float u = gl_TessCoord.x;
	float v = gl_TessCoord.y;
	
	// single sided approximation
	if(u <= 0.5f) { // first quadratic
		u *= 2.0f;
		
		// push u towards max curvature point
		if(0 < fold_mid && fold_mid < 1.0f) {
			if(u < fold_mid) {
				u = sqrt(u / fold_mid) * fold_mid;
			} else {
				u = 1.0 - u;
				u = sqrt(u / (1.0f - fold_mid)) * (1.0f - fold_mid);
				u = 1.0 - u;
			}
		}
		
		vec2 textel_data = texelFetch(curves, u / 16.0f + te_distance_index - 0.5f);
		texture_coord.x = textel_data.x;
		
		vec2 b0 = p0 * vec2(aspect_ratio, 1.0);
		vec2 d = p1 * vec2(aspect_ratio, 1.0);
		vec2 c0 = cp * vec2(aspect_ratio, 1.0);
		
		float curvature_numerator = 4.0f * (b0.x * (c0.y - d.y) + c0.x * (d.y - b0.y) + d.x * (b0.y - c0.y)); // = 8 times the area of the triangle formed by the control points
		
			
		float gamma_prime_x_constant_term = 2.0f * (c0.x - b0.x);
		float gamma_prime_y_constant_term = 2.0f * (c0.y - b0.y);
		float gamma_prime_x_linear_term = 2.0f * (b0.x + d.x - 2.0f * c0.x);
		float gamma_prime_y_linear_term = 2.0f * (b0.y + d.y - 2.0f * c0.y);
			
		float gsquared_quadratic_term = gamma_prime_x_linear_term * gamma_prime_x_linear_term + gamma_prime_y_linear_term * gamma_prime_y_linear_term;
		float gsquared_linear_term = 2.0f * gamma_prime_x_constant_term * gamma_prime_x_linear_term + 2.0f * gamma_prime_y_constant_term * gamma_prime_y_linear_term;
		float gsquared_constant_term = gamma_prime_x_constant_term * gamma_prime_x_constant_term + gamma_prime_y_constant_term * gamma_prime_y_constant_term;
	
		float cdenom = pow(gsquared_quadratic_term * u * u + gsquared_linear_term * u + gsquared_constant_term , 1.5f);
		float adj_width = normalized_width;
		if(cdenom > 0 && curvature_numerator > 0) {
			adj_width = min(adj_width, 0.88f * cdenom / curvature_numerator);
		}
		
		vec2 point = (1.0f - u) * (1.0f - u) * p0 + 2.0f * (1.0f - u) * u * cp + u * u * p1;
		vec2 tangent = normalize((-2.0f * (1.0f - u) * (p0 - cp) + 2.0f * u * (p1 - cp)) * vec2(aspect_ratio, 1.0));
		vec2 norm = vec2(-tangent.y, tangent.x);
		
		gl_Position = vec4((v - 0.5f) * 2.0f * adj_width * vec2(1.0f / aspect_ratio, 1.0) * norm + point, 0.0f, 1.0f);
		
		if(pos_fold >= 0.0f) {
			vec2 left_hand = -normalized_width * vec2(1.0f / aspect_ratio, 1.0) * norm + point;
			vec2 right_hand = adj_width * vec2(1.0f / aspect_ratio, 1.0) * norm + point;
			gl_Position = vec4((1.0f - v) * left_hand + v * right_hand, 0.0f, 1.0f);
			texture_coord.y = (1.0f - v) * 0.0f + v * (0.5f + 0.5f * adj_width / normalized_width);
		} else {
			vec2 left_hand = -adj_width * vec2(1.0f / aspect_ratio, 1.0) * norm + point;
			vec2 right_hand = normalized_width * vec2(1.0f / aspect_ratio, 1.0) * norm + point;
			gl_Position = vec4((1.0f - v) * left_hand + v * right_hand, 0.0f, 1.0f);
			texture_coord.y = (1.0f - v) * (0.5f - 0.5f * adj_width / normalized_width) + v * 1.0f;
		}
	} else {
		u -= 0.5f;
		u *= 2.0f;


		// push u towards max curvature point
		if(0 < fold_mid_B && fold_mid_B < 1.0f) {
			if(u < fold_mid_B) {
				u = sqrt(u / fold_mid_B) * fold_mid_B;
			} else {
				u = 1.0 - u;
				u = sqrt(u / (1.0f - fold_mid_B)) * (1.0f - fold_mid_B);
				u = 1.0 - u;
			}
		}
		
		vec2 textel_data = texelFetch(curves, (u + 8.0f) / 16.0f + te_distance_index - 0.5f);
		texture_coord.x = textel_data.x;
		
		vec2 b0 = p0_B * vec2(aspect_ratio, 1.0);
		vec2 d = p1_B * vec2(aspect_ratio, 1.0);
		vec2 c0 = cp_B * vec2(aspect_ratio, 1.0);
		
		float curvature_numerator = 4.0f * (b0.x * (c0.y - d.y) + c0.x * (d.y - b0.y) + d.x * (b0.y - c0.y)); // = 8 times the area of the triangle formed by the control points
		
			
		float gamma_prime_x_constant_term = 2.0f * (c0.x - b0.x);
		float gamma_prime_y_constant_term = 2.0f * (c0.y - b0.y);
		float gamma_prime_x_linear_term = 2.0f * (b0.x + d.x - 2.0f * c0.x);
		float gamma_prime_y_linear_term = 2.0f * (b0.y + d.y - 2.0f * c0.y);
			
		float gsquared_quadratic_term = gamma_prime_x_linear_term * gamma_prime_x_linear_term + gamma_prime_y_linear_term * gamma_prime_y_linear_term;
		float gsquared_linear_term = 2.0f * gamma_prime_x_constant_term * gamma_prime_x_linear_term + 2.0f * gamma_prime_y_constant_term * gamma_prime_y_linear_term;
		float gsquared_constant_term = gamma_prime_x_constant_term * gamma_prime_x_constant_term + gamma_prime_y_constant_term * gamma_prime_y_constant_term;
	
		float cdenom = pow(gsquared_quadratic_term * u * u + gsquared_linear_term * u + gsquared_constant_term , 1.5f);
		float adj_width = normalized_width;
		if(cdenom > 0 && curvature_numerator > 0) {
			adj_width = min(adj_width, 0.88f * cdenom / curvature_numerator);
		}
		
		vec2 point = (1.0f - u) * (1.0f - u) * p0_B + 2.0f * (1.0f - u) * u * cp_B + u * u * p1_B;
		vec2 tangent = normalize((-2.0f * (1.0f - u) * (p0_B - cp_B) + 2.0f * u * (p1_B - cp_B)) * vec2(aspect_ratio, 1.0));
		vec2 norm = vec2(-tangent.y, tangent.x);
		
		gl_Position = vec4((v - 0.5f) * 2.0f * adj_width * vec2(1.0f / aspect_ratio, 1.0) * norm + point, 0.0f, 1.0f);
		
		if(pos_fold >= 0.0f) {
			vec2 left_hand = -normalized_width * vec2(1.0f / aspect_ratio, 1.0) * norm + point;
			vec2 right_hand = adj_width * vec2(1.0f / aspect_ratio, 1.0) * norm + point;
			gl_Position = vec4((1.0f - v) * left_hand + v * right_hand, 0.0f, 1.0f);
			texture_coord.y = (1.0f - v) * 0.0f + v * (0.5f + 0.5f * adj_width / normalized_width);
		} else {
			vec2 left_hand = -adj_width * vec2(1.0f / aspect_ratio, 1.0) * norm + point;
			vec2 right_hand = normalized_width * vec2(1.0f / aspect_ratio, 1.0) * norm + point;
			gl_Position = vec4((1.0f - v) * left_hand + v * right_hand, 0.0f, 1.0f);
			texture_coord.y = (1.0f - v) * (0.5f - 0.5f * adj_width / normalized_width) + v * 1.0f;
		}
	}
	
}