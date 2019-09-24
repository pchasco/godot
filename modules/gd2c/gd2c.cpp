#include "register_types.h"
#include "core/class_db.h"

#include "gd2c.h"
#include "core/variant.h"
#include "core/io/marshalls.h"
#include "core/variant_parser.h"
#include "core/io/resource_loader.h"
#include "core/ustring.h"

#include "gd2c_api_struct.h"

extern "C" {
	void GDAPI gd2c_test(godot_variant *r_dest, godot_variant *p_in) {
		Variant *dest = (Variant*)r_dest;
		Variant *a = (Variant*)p_in;
		*a = "Clobberd it yo";
		memnew_placement_custom(dest, Variant, Variant(*a));
	}

	void GDAPI gd2c_variant_get_named(const void *p_self, const void *p_name, void *p_dest, bool *r_error) {

	}
	void GDAPI gd2c_variant_set_named(void *p_self, const void *p_name, const void *p_value, bool *r_error) {
		
	}
	void GDAPI gd2c_variant_get(const void *p_instance, const void *p_index, void *p_dest, bool *r_error) {
		
	}
	void GDAPI gd2c_variant_set(void *p_instance, const void *p_index, const void *p_value, bool *r_error) {
		
	}
	godot_error GDAPI gd2c_variant_decode(void *r_variant, const uint8_t *p_buffer, int p_len, int *r_len, bool p_allow_objects){
		
	}
	void GDAPI gd2c_resource_load(void *r_result, const void *p_path){
		
	}
}

extern const struct gd2c_api_1_0 __api10 = {
	1,
	0,
	gd2c_variant_get_named,
	gd2c_variant_set_named,
	gd2c_variant_get,
	gd2c_variant_set,
	gd2c_variant_decode,
	gd2c_resource_load
};


String GD2CApi::get_api(int major, int minor) {
	// So... We get a ptr to the API struct then return it as a string...
	int ptr_size = sizeof(void *);
	void *api = nullptr;
	String result;

	switch (major) {
		case 1:
			switch (minor) {
				case 0:
					api = (void *)&__api10;
					break;
			}
			break;
	}

	if (api != nullptr) {
		uint64_t addr = reinterpret_cast<uint64_t>(api);
		result += itos(ptr_size) + "," + itos(addr);
	}

	return result;
}

void GD2CApi::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_api", "major", "minor"), &GD2CApi::get_api);
}