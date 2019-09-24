#ifndef __GD2C_API_STRUCT__
#define __GD2C_API_STRUCT__

#include "modules/gdnative/include/gdnative/gdnative.h"

struct gd2c_api_1_0 {
	int major;
	int minor;

	void GDAPI (*variant_get_named)(const godot_variant *p_self, const godot_string *p_name, godot_variant *p_dest, godot_bool *r_error);
	void GDAPI (*variant_set_named)(godot_variant *p_self, const godot_string *p_name, const godot_variant *p_value, godot_bool *r_error);
	void GDAPI (*variant_get)(const godot_variant *p_self, const godot_variant *p_index, godot_variant *p_dest, godot_bool *r_error);
	void GDAPI (*variant_set)(godot_variant *p_self, const godot_variant *p_index, const godot_variant *p_value, godot_bool *r_error);
	godot_error GDAPI (*variant_decode)(godot_variant *r_variant, const uint8_t *p_buffer, int p_len, int *r_len, godot_bool p_allow_objects);
	void GDAPI (*resource_load)(godot_variant *r_result, const godot_string *p_path);
};

#endif