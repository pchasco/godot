#ifndef GD2C_RUNTIME_SERVICES_H
#define GD2C_RUNTIME_SERVICES_H

#include "core/reference.h"
#include "modules/gdnative/include/gdnative/gdnative.h"

#ifndef memnew_placement_custom
#define memnew_placement_custom(m_placement, m_class, m_constr) _post_initialize(new (m_placement, sizeof(m_class), "") m_constr)
#endif

/*
	void variant_get_named(const void *p_self, const void *p_name, void *p_dest, bool *r_error);
	void variant_set_named(void *p_self, const void *p_name, const void *p_value, bool *r_error);
	void variant_get(const void *p_instance, const void *p_index, void *p_dest, bool *r_error);
	void variant_set(void *p_instance, const void *p_index, const void *p_value, bool *r_error);
	godot_error variant_decode(void *r_variant, const uint8_t *p_buffer, int p_len, int *r_len, bool p_allow_objects);
	void resource_load(void *r_result, const void *p_path);
*/

extern "C" {
	void GDAPI gd2c_test(godot_variant *r_dest, godot_variant *p_in);
	//godot_error GDAPI gd2c_variant_decode(godot_variant *r_variant, const uint8_t *p_buffer, int p_len, int *r_len, godot_bool p_allow_objects);
}

struct gd2c_api_1_0 {
	int major;
	int minor;

	void GDAPI (*test)(godot_variant *r_dest, godot_variant *p_in);
};

class GD2CApi : public Reference {
    GDCLASS(GD2CApi, Reference)
public:
    GD2CApi() {};

	String get_api(int major, int minor);

protected:
    static void _bind_methods();
};

#endif
