#include "register_types.h"
#include "core/class_db.h"

#include "gd2c.h"
#include "core/variant.h"
#include "core/io/marshalls.h"
#include "core/variant_parser.h"
#include "core/io/resource_loader.h"
#include "core/ustring.h"

extern "C" {
	void GDAPI gd2c_test(godot_variant *r_dest, godot_variant *p_in) {
		Variant *dest = (Variant*)r_dest;
		Variant *a = (Variant*)p_in;
		*a = "Clobberd it yo";
		memnew_placement_custom(dest, Variant, Variant(*a));
	}
}

extern const struct gd2c_api_1_0 __api10 = {
	1,
	0,
	gd2c_test
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