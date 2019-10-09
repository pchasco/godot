#include "register_types.h"
#include "core/class_db.h"

#include "gd2c.h"
#include "core/variant.h"
#include "core/io/marshalls.h"
#include "core/variant_parser.h"
#include "core/io/resource_loader.h"
#include "core/ustring.h"
#include "modules/gdscript/gdscript.h"
#include "modules/gdscript/gdscript_functions.h"

#include "gd2c_api_struct.h"

extern "C" {
	void GDAPI gd2c_variant_get_named(const godot_variant *p_self, const godot_string_name *p_name, godot_variant *p_dest, godot_bool *r_error) {
		Variant *self = (Variant *)p_self;
		StringName *name = (StringName *)p_name;
		Variant *dest = (Variant *)p_dest;

		*dest = self->get_named(*name, r_error);
	}

	void GDAPI gd2c_variant_set_named(godot_variant *p_self, const godot_string_name *p_name, const godot_variant *p_value, godot_bool *r_error) {
		Variant *self = (Variant *)p_self;
		StringName *name = (StringName *)p_name;
		Variant *value = (Variant *)p_value;

		self->set_named(*name, *value, r_error);
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

	void GDAPI gd2c_variant_convert(godot_variant *r_result, godot_int variant_type, const godot_variant **p_args, int p_arg_count, godot_variant_call_error *r_error) {
		Variant *result = (Variant *)r_result;
		Variant::CallError err;
		*result = Variant::construct((Variant::Type)variant_type, (const Variant **)p_args, p_arg_count, err);
		if (r_error != NULL && err.error != Variant::CallError::Error::CALL_OK) {
			r_error->error = (godot_variant_call_error_error)err.error;
			r_error->argument = err.argument;
			r_error->expected = (godot_variant_type)err.expected;
		}		
	}

	void GDAPI gd2c_object_get_property(godot_variant *r_result, godot_object *p_instance, godot_string_name *p_index) {
		Variant *result = (Variant *)r_result;
		Object *instance = (Object *)p_instance;
		StringName *index = (StringName *)p_index;
		ClassDB::get_property(instance, *index, *result);
	}

	void GDAPI gd2c_object_set_property(godot_object *p_instance, godot_string_name *p_index, godot_variant *p_value) {
		Variant *value = (Variant *)p_value;
		Object *instance = (Object *)p_instance;
		StringName *index = (StringName *)p_index;
		bool valid;
		ClassDB::set_property(instance, *index, *value, &valid);
	}

	void GDAPI gd2c_call_gdscript_builtin(int p_func, const godot_variant ** p_args, godot_int p_arg_count, godot_variant *r_result, godot_variant_call_error *r_error) {
		Variant *result = (Variant *)r_result;
		Variant::CallError err;
		GDScriptFunctions::call((GDScriptFunctions::Function)p_func, (const Variant **)p_args, p_arg_count, *result, err);
		if (r_error != NULL && err.error != Variant::CallError::Error::CALL_OK) {
			r_error->error = (godot_variant_call_error_error)err.error;
			r_error->argument = err.argument;
			r_error->expected = (godot_variant_type)err.expected;
		}	
	}

	godot_bool GDAPI gd2c_variant_iter_init(godot_variant *p_self, godot_variant *r_iter, godot_bool *r_valid) {
		Variant *self = (Variant *)p_self;
		Variant *iter = (Variant *)r_iter;
		bool *valid = (bool *)r_valid;
		return (godot_bool) self->iter_init(*iter, *valid);
	}

	godot_bool GDAPI gd2c_variant_iter_next(godot_variant *p_self, godot_variant *r_iter, godot_bool *r_valid) {
		Variant *self = (Variant *)p_self;
		Variant *iter = (Variant *)r_iter;
		bool *valid = (bool *)r_valid;
		return (godot_bool) self->iter_next(*iter, *valid);
	}

	void GDAPI gd2c_variant_iter_get(godot_variant *p_self, godot_variant *r_iter, godot_variant *r_result, godot_bool *r_valid) {
		Variant *self = (Variant *)p_self;
		Variant *iter = (Variant *)r_iter;
		Variant *result = (Variant *)r_result;
		bool *valid = (bool *)r_valid;
		*result = self->iter_get(*iter, *valid);
	}

	void GDAPI gd2c_get_gdscript_nativeclass(godot_variant *r_dest, godot_string_name *p_name) {
		Variant *dest = (Variant *)r_dest;
		StringName *name = (StringName *)p_name;
		Ref<GDScriptNativeClass> ref = memnew(GDScriptNativeClass(*name));
		if (!ref.is_null()) {
			memnew_placement_custom(dest, Variant, Variant(ref.get_ref_ptr()));
		} else {
			// WTF
		}
	}

	void GDAPI gd2c_variant_call(godot_variant *p_self, godot_string_name *p_method_name, godot_int p_argc, godot_variant **p_args, godot_variant *r_result, godot_variant_call_error *r_error) {
		Variant *self = (Variant *)p_self;
		StringName *name = (StringName *)p_method_name;
		Variant *result = (Variant *)r_result;
		Variant::CallError err;

		self->call_ptr(*name, (const Variant **)p_args, p_argc, result, err);
		if (r_error != NULL && err.error) {
			r_error->error = (godot_variant_call_error_error)err.error;
			r_error->argument = err.argument;
			r_error->expected = (godot_variant_type)err.expected;
		}			
	}
}

extern const struct gd2c_api_1_0 __api10 = {
	1,
	0,
	gd2c_variant_get_named,
	gd2c_variant_set_named,
	gd2c_variant_decode,
	gd2c_resource_load,
	gd2c_variant_convert,
	gd2c_object_get_property,
	gd2c_object_set_property,
	gd2c_call_gdscript_builtin,
	gd2c_variant_iter_init,
	gd2c_variant_iter_next,
	gd2c_variant_iter_get,
	gd2c_get_gdscript_nativeclass,
	gd2c_variant_call
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