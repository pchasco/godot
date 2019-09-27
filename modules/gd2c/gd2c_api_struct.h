#ifndef __GD2C_API_STRUCT__
#define __GD2C_API_STRUCT__

#include "modules/gdnative/include/gdnative/gdnative.h"

// This struct must be identical to the client
struct gd2c_api_1_0 {
	int major;
	int minor;

	void GDAPI (*variant_get_named)(const godot_variant *p_self, const godot_string_name *p_name, godot_variant *p_dest, godot_bool *r_error);
	void GDAPI (*variant_set_named)(godot_variant *p_self, const godot_string_name *p_name, const godot_variant *p_value, godot_bool *r_error);
	godot_error GDAPI (*variant_decode)(godot_variant *r_variant, const uint8_t *p_buffer, int p_len, int *r_len, godot_bool p_allow_objects);
	void GDAPI (*resource_load)(godot_variant *r_result, const godot_string *p_path);
	void GDAPI (*variant_convert)(godot_variant *r_result, godot_int variant_type, const godot_variant **p_args, int p_len, godot_variant_call_error *r_error);
	void GDAPI (*object_get_property)(godot_variant *r_result, godot_object *p_instance, godot_string_name *p_index);
	void GDAPI (*object_set_property)(godot_object *p_instance, godot_string_name *p_index, godot_variant *p_value);
};

#endif