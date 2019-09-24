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
	void GDAPI gd2c_variant_get_named(const godot_variant *p_self, const godot_string *p_name, godot_variant *p_dest, godot_bool *r_error) {
		Variant *self = (Variant *)p_self;
		String *name = (String *)p_name;
		Variant *dest = (Variant *)p_dest;

		*dest = self->get_named(*name, r_error);
	}

	void GDAPI gd2c_variant_set_named(godot_variant *p_self, const godot_string *p_name, const godot_variant *p_value, godot_bool *r_error) {
		Variant *self = (Variant *)p_self;
		String *name = (String *)p_name;
		Variant *value = (Variant *)p_value;

		self->set_named(*name, *value, r_error);
	}

	void GDAPI gd2c_variant_get(const godot_variant *p_self, const godot_variant *p_index, godot_variant *p_dest, godot_bool *r_error) {
		Variant *self = (Variant *)p_self;
		Variant *index = (Variant *)p_index;
		Variant *dest = (Variant *)p_dest;

		*dest = self->get(*index, r_error);
	}

	void GDAPI gd2c_variant_set(godot_variant *p_self, const godot_variant *p_index, const godot_variant *p_value, godot_bool *r_error) {
		Variant *self = (Variant *)p_self;
		Variant *index = (Variant *)p_index;
		Variant *value = (Variant *)p_value;

		self->set(*index, *value, r_error);
	}

	godot_error GDAPI gd2c_variant_decode(godot_variant *r_variant, const uint8_t *p_buffer, int p_len, int *r_len, godot_bool p_allow_objects) {
		Variant *ret = (Variant *)r_variant;
		return (godot_error) decode_variant(*ret, p_buffer, p_len, r_len, p_allow_objects);
	}

	void GDAPI gd2c_resource_load(godot_variant *r_result, const godot_string *p_path) {
		Variant *result = (Variant *)r_result;
		const String *path = (const String *)p_path;
		*result = ResourceLoader::load(*path);
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