layout (location = 0) in vec2 base_point_0;
layout (location = 1) in vec2 base_point_1;
layout (location = 2) in vec2 initial_cp_0;
layout (location = 3) in vec2 initial_cp_1;
layout (location = 4) in float v_distance_index;

out float tc_distance_index;
out vec2 transformed_base_point_1;
out vec2 transformed_tangent_0;
out vec2 transformed_tangent_1;
out vec2 transformed_cp_0;
out vec2 transformed_cp_1;

uniform vec2 offset;
uniform float aspect_ratio;
uniform float zoom;
uniform vec2 map_size;
uniform mat3 rotation;
uniform uint subroutines_index;

vec4 globe_coords(vec2 world_pos) {

	vec3 new_world_pos;
	float section_x = 200;
	float angle_x1 = 2 * PI * floor(world_pos.x * section_x) / section_x;
	float angle_x2 = 2 * PI * floor(world_pos.x * section_x + 1) / section_x;
	float angle_x = mix(angle_x1, angle_x2, mod(world_pos.x * section_x, 1));
	new_world_pos.x = mix(cos(angle_x1), cos(angle_x2), mod(world_pos.x * section_x, 1));
	new_world_pos.y = mix(sin(angle_x1), sin(angle_x2), mod(world_pos.x * section_x, 1));
	float section_y = 200;
	float angle_y1 = PI * floor(world_pos.y * section_y) / section_y;
	float angle_y2 = PI * floor(world_pos.y * section_y + 1) / section_y;
	new_world_pos.x *= mix(sin(angle_y1), sin(angle_y2), mod(world_pos.y * section_y, 1));
	new_world_pos.y *= mix(sin(angle_y1), sin(angle_y2), mod(world_pos.y * section_y, 1));
	new_world_pos.z = mix(cos(angle_y1), cos(angle_y2), mod(world_pos.y * section_y, 1));
	new_world_pos = rotation * new_world_pos;
	new_world_pos /= PI; 		// Will make the zoom be the same for the globe and flat map
	new_world_pos.y *= 0.02; 	// Sqeeze the z coords. Needs to be between -1 and 1
	new_world_pos.xz *= -1; 	// Invert the globe
	new_world_pos.xyz += 0.5; 	// Move the globe to the center

	return vec4(
		(2. * new_world_pos.x - 1.f) / aspect_ratio  * zoom,
		(2. * new_world_pos.z - 1.f) * zoom,
		(2. * new_world_pos.y - 1.f), 1.0);
}

vec4 flat_coords(vec2 world_pos) {
	world_pos += vec2(-offset.x, offset.y);
	world_pos.x = mod(world_pos.x, 1.0f);
	return vec4(
		(2. * world_pos.x - 1.f) * zoom / aspect_ratio * map_size.x / map_size.y,
		(2. * world_pos.y - 1.f) * zoom,
		abs(world_pos.x - 0.5) * 2.1f,
		1.0);
}

vec4 perspective_coords(vec2 world_pos) {
	vec3 new_world_pos;
	float angle_x = 2 * world_pos.x * PI;
	new_world_pos.x = cos(angle_x);
	new_world_pos.y = sin(angle_x);
	float angle_y = world_pos.y * PI;
	new_world_pos.x *= sin(angle_y);
	new_world_pos.y *= sin(angle_y);
	new_world_pos.z = cos(angle_y);

	new_world_pos = rotation * new_world_pos;
	new_world_pos /= PI; // Will make the zoom be the same for the globe and flat map
	new_world_pos.xz *= -1;
	new_world_pos.zy = new_world_pos.yz;

	new_world_pos.x /= aspect_ratio;
	new_world_pos.z -= 1.2;
	float near = 0.1;
	float tangent_length_square = 1.2f * 1.2f - 1 / PI / PI;
	float far = tangent_length_square / 1.2f;
	float right = near * tan(PI / 6) / zoom;
	float top = near * tan(PI / 6) / zoom;
	new_world_pos.x *= near / right;
	new_world_pos.y *= near / top;
	float w = -new_world_pos.z;
	new_world_pos.z = -(far + near) / (far - near) * new_world_pos.z - 2 * far * near / (far - near);
	return vec4(new_world_pos, w);
}

vec4 calc_gl_position(vec2 world_pos) {
	switch(int(subroutines_index)) {
case 0: return globe_coords(world_pos);
case 1: return perspective_coords(world_pos);
case 2: return flat_coords(world_pos);
default: break;
	}
	return vec4(0.f);
}


void main() {
	// TODO: fix cp distance
	vec4 base_point_0_res = calc_gl_position(base_point_0.xy);
	vec4 base_cp_0_res = calc_gl_position(initial_cp_0.xy);
	
	vec2 initial_tangent_0 = normalize(initial_cp_0.xy - base_point_0.xy);
	
	vec4 tangent_pos_0 = calc_gl_position(base_point_0.xy + initial_tangent_0 * 0.0001f) - base_point_0_res;
	vec4 tangent_neg_0 = -(calc_gl_position(base_point_0.xy + initial_tangent_0 * -0.0001f) - base_point_0_res);
	gl_Position = base_point_0_res;
	transformed_tangent_0 = normalize(tangent_pos_0.xy + tangent_neg_0.xy);
	transformed_cp_0 = base_point_0_res.xy  + transformed_tangent_0 * distance(base_point_0_res, base_cp_0_res);
	
	vec4 base_point_1_res = calc_gl_position(base_point_1.xy);
	vec4 base_cp_1_res = calc_gl_position(initial_cp_1.xy);
	
	vec2 initial_tangent_1 = normalize(base_point_1.xy - initial_cp_1.xy);
	
	vec4 tangent_pos_1 = calc_gl_position(base_point_1.xy + initial_tangent_1 * 0.0001f) - base_point_1_res;
	vec4 tangent_neg_1 = -(calc_gl_position(base_point_1.xy + initial_tangent_1 * -0.0001f) - base_point_1_res);
	transformed_base_point_1 = base_point_1_res.xy;
	
	transformed_tangent_1 = normalize(tangent_pos_1.xy + tangent_neg_1.xy);
	transformed_cp_1 = base_point_1_res.xy  - transformed_tangent_1 * distance(base_point_1_res, base_cp_1_res);
	
	tc_distance_index = v_distance_index;
}