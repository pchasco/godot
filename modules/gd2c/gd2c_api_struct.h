#ifndef __GD2C_API_STRUCT__
#define __GD2C_API_STRUCT__

#include "modules/gdnative/include/gdnative/gdnative.h"

struct gd2c_api_1_0 {
	int major;
	int minor;

	void GDAPI (*variant_get_named)(const void *p_self, const void *p_name, void *p_dest, bool *r_error);
	void GDAPI (*variant_set_named)(void *p_self, const void *p_name, const void *p_value, bool *r_error);
	void GDAPI (*variant_get)(const void *p_instance, const void *p_index, void *p_dest, bool *r_error);
	void GDAPI (*variant_set)(void *p_instance, const void *p_index, const void *p_value, bool *r_error);
	godot_error GDAPI (*variant_decode)(void *r_variant, const uint8_t *p_buffer, int p_len, int *r_len, bool p_allow_objects);
	void GDAPI (*resource_load)(void *r_result, const void *p_path);
};

#endif