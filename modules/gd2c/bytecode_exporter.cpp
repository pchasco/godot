#include "core/global_constants.h"
#include "core/io/marshalls.h"
#include "core/variant_parser.h"
#include "core/engine.h"
#include "modules/gdscript/gdscript.h"
#include "bytecode_exporter.h"
#include "core/string_builder.h"
#include "core/os/os.h"

String q(String value) {
    return "\"" + value.json_escape() + "\"";
}

String kv(char *key, String value) {
    return q(String(key)) + ":" + q(value);
}

String kv(char *key, const char *value) {
    return q(key) + ":" + q(String(value));
}

String kv(char *key, int value) {
    return q(key) + ":" + q(String(itos(value)));
}

String kv(char *key, double value) {
    return q(key) + ":" + String(rtos(value));
}

int put_var(const Variant &p_packet, Vector<uint8_t> *data) {
	int len;
	Error err = encode_variant(p_packet, NULL, len, false); // compute len first
	if (err) return err;
	if (len == 0) return OK;

	uint8_t *buf = (uint8_t *)alloca(len);
	ERR_FAIL_COND_V(!buf, ERR_OUT_OF_MEMORY);
	err = encode_variant(p_packet, buf, len, false);
	ERR_FAIL_COND_V(err, err);

	for (int i = 0; i < len; ++i) {
		data->push_back(buf[i]);
	}

	return len;
}

int get_member_index(const GDScript &script, const StringName &member) {
	return script.debug_get_member_indices()[member].index;
}

void trace(int line, const char *text) {
	char trace_buffer[4096];
	sprintf(trace_buffer, "%i: %s\n", line, text);
	OS::get_singleton()->print(String(trace_buffer).utf8().get_data());
}

#define TRACE(text) trace(__LINE__, text)

void GDScriptBytecodeExporter::export_bytecode_to_file(String input_script_path, String output_json_path) {
	Ref<Script> scr = ResourceLoader::load(input_script_path);
    ERR_EXPLAIN("Can't load script: " + input_script_path);
    ERR_FAIL_COND(scr.is_null());

    ERR_EXPLAIN("Cannot instance script: " + input_script_path);
    ERR_FAIL_COND(!scr->can_instance());

    Ref<GDScript> script = scr;

    StringName instance_type = script->get_instance_base_type();
	Set<StringName> members;
	Vector<uint8_t> variantBuffer;
	Map<String, int> objectJsonIndex;
	Vector<String> objectJson;

	StringBuilder json;
	bool comma1 = false;
	bool comma2 = false;

	json.append("{");

	// We need the a way to identify global constants by index when compiling the script.

	// Integer constants
	TRACE("global_constants");
	json.append("\"global_constants\":[");
	int gcc = GlobalConstants::get_global_constant_count();
	int global_index = 0;
	comma1 = false;
	for (int i = 0; i < gcc; ++i) {
		int value = GlobalConstants::get_global_constant_value(i);
		if (comma1) json.append(",");
		json.append("{");
		json.append("\"source\":\"GlobalConstants\",");
		json.append("\"original_name\":" + q(String(GlobalConstants::get_global_constant_name(i))) + ",");
		json.append("\"name\":" + q(String(GlobalConstants::get_global_constant_name(i))) + ",");
		json.append("\"value\":" + itos(value) + ",");
		json.append("\"type_code\":" + itos(Variant::Type::INT) + ",");
		json.append("\"kind_code\":-1,");
		json.append("\"index\":" + itos(global_index++));
		json.append("}");
		comma1 = true;
	}

	// GDSCriptLanguage-defined constants
	TRACE("hard coded constants");
	if (comma1) json.append(",");
	json.append("{\"source\":\"hard-coded\",\"name\":\"PI\",\"original_name\":\"PI\",\"value\":\"" + rtos(Math_PI) + "\",\"type_code\":" + itos(Variant::Type::REAL) + ",\"index\":" + itos(global_index++) + ",\"kind_code\":-1},");
	json.append("{\"source\":\"hard-coded\",\"name\":\"TAU\",\"original_name\":\"TAU\",\"value\":\"" + rtos(Math_TAU) + "\",\"type_code\":" + itos(Variant::Type::REAL) + ",\"index\":" + itos(global_index++) + ",\"kind_code\":-1},");
	json.append("{\"source\":\"hard-coded\",\"name\":\"INF\",\"original_name\":\"INF\",\"value\":\"" + rtos(Math_INF) + "\",\"type_code\":" + itos(Variant::Type::REAL) + ",\"index\":" + itos(global_index++) + ",\"kind_code\":-1},");
	json.append("{\"source\":\"hard-coded\",\"name\":\"NAN\",\"original_name\":\"NAN\",\"value\":\"" + rtos(Math_NAN) + "\",\"type_code\":" + itos(Variant::Type::REAL) + ",\"index\":" + itos(global_index++) + ",\"kind_code\":-1}");
	comma1 = true;

	// Godot ClassDB-defined constants
	
	//populate native classes
	TRACE("native classes");
	List<StringName> class_list;
	ClassDB::get_class_list(&class_list);
	Map<StringName, int> added_globals;
	for (List<StringName>::Element *E = class_list.front(); E; E = E->next()) {
		StringName n = E->get();
		String s = String(n);
		if (s.begins_with("_"))
			n = s.substr(1, s.length());

		if (added_globals.has(n))
			continue;

		added_globals[s] = added_globals.size();

		String js;
		js += "{";
		js += "\"source\":\"ClassDB\",";
		js += "\"original_name\":\"" + s + "\",";
		js += "\"name\":\"" + n + "\",";
		js += "\"value\":null,";
		js += "\"type_code\":null,";
		js += "\"index\":" + itos(global_index++) + ",";
		js += "\"kind_code\":" + itos(GDScriptDataType::NATIVE);
		js += "}";

		objectJsonIndex[s] = objectJson.size();
		objectJson.push_back(js);
	}

	//populate singletons

	TRACE("singletons");
	List<Engine::Singleton> singletons;
	Engine::get_singleton()->get_singletons(&singletons);
	for (List<Engine::Singleton>::Element *E = singletons.front(); E; E = E->next()) {
		bool replace = objectJsonIndex.has(E->get().name);
		int gi;
		if (replace) {
			gi = added_globals[E->get().name];
		} else {
			gi = global_index;
		}

		String js;
		js += "{";
		js += "\"source\":\"Singleton\",";
		js += "\"original_name\":\"" + E->get().name + "\",";
		js += "\"name\":\"" + E->get().name + "\",";
		js += "\"value\":null,";
		js += "\"type_code\":null,";
		js += "\"index\":" + itos(gi) + ",";
		js += "\"kind_code\":" + itos(GDScriptDataType::BUILTIN);
		js += "}";

		if (replace) {
			// Singletons replace constants that have the same name
			int object_index = objectJsonIndex[E->get().name];
			objectJson.set(object_index, js);
		} else {
			// No constant with this name so push it back to the end
			objectJsonIndex[E->get().name] = objectJson.size();
			objectJson.push_back(js);
			global_index += 1;
		}
	}

	json.append(",");

	comma1 = false;
	for (int i = 0; i < objectJson.size(); ++i) {
		if (comma1) json.append(",");
		json.append(objectJson[i]);
		comma1 = true;
	}

	json.append("],");

	json.append("\"type\":\"" + instance_type + "\",");

	if (!script->get_base().is_null()) {
		json.append("\"base_type\":\"" + script->get_base()->get_path() + "\",");
	} else {
		json.append("\"base_type\": null,");
	}

	TRACE("members");
	script->get_members(&members);
	comma1 = false;
	json.append("\"members\":[");
	for (Set<StringName>::Element *member = members.front(); member; member = member->next()) {
		GDScriptDataType member_type = script->get_member_type(member->get());
		int member_index = get_member_index(*script.ptr(), member->get());

		if (comma1) json.append(",");
		json.append("{");
		json.append("\"index\":" + itos(member_index) + ",");
		json.append("\"name\":\"" + member->get() + "\",");
		json.append("\"kind\":" + itos(member_type.kind) + ",");
		json.append("\"type\":" + itos(member_type.builtin_type) + ",");
		json.append("\"native_type\":\"" + member_type.native_type + "\",");
		json.append("\"has_type\":");
		if (member_type.has_type) {
			json.append("true");
		} else {
			json.append("false");
		}
		json.append("}");
		comma1 = true;
	}
	json.append("],");

	TRACE("class constants");
	Map<StringName, Variant> constants;
	script->get_constants(&constants);
	comma1 = false;
	json.append("\"constants\":[");
	for (Map<StringName, Variant>::Element *constant = constants.front(); constant; constant = constant->next()) {
		if (comma1) json.append(",");
		String vars;
		VariantWriter::write_to_string(constant->value(), vars);

		json.append("{");
		json.append("\"name\":\"" + constant->key() + "\",");
		json.append("\"declaration\":\"" + vars.replace("\"", "\\\"").replace("\n", "\\n") + "\",");
		json.append("\"type\":" + itos(constant->value().get_type()) + ",");
		json.append("\"type_name\":\"" + constant->value().get_type_name(constant->value().get_type()) + "\",");
		json.append("\"data\":[");

		variantBuffer.clear();
		int len = put_var(constant->value(), &variantBuffer);
		bool comma3 = false;
		for (int j = 0; j < len; ++j) {
			if (comma3) json.append(",");
			json.append(itos(variantBuffer[j]));
			comma3 = true;
		}
		json.append("]");
		json.append("}");
		comma1 = true;
	}
	json.append("],");

	TRACE("class signals");
	List<MethodInfo> signals;
	script->get_script_signal_list(&signals);
	comma1 = false;
	json.append("\"signals\":[");
	for (List<MethodInfo>::Element *signal = signals.front(); signal; signal = signal->next()) {
		if (comma1) json.append(",");
		json.append("\"" + signal->get().name + "\"");
		comma1 = true;
	}
	json.append("],");

	TRACE("class methods");
	List<MethodInfo> methods;
	script->get_script_method_list(&methods);
	json.append("\"methods\":[");
	comma1 = false;
	for (List<MethodInfo>::Element *method = methods.front(); method; method = method->next()) {
		TRACE("class method");
		TRACE(method->get().name.utf8().get_data());

		if (script.ptr()->get_member_functions().has(method->get().name)) {
			GDScriptFunction *f = ((GDScript *)script.ptr())->get_member_functions()[method->get().name];
			TRACE("  found method...");

			if (comma1) json.append(",");
			json.append("{");
			json.append("\"name\": \"" + method->get().name + "\",");
			String s = itos(method->get().id);
			json.append("\"id\": \"" + s + "\",");
			json.append("\"return_type\": {");
			json.append("\"type\":" + itos(f->get_return_type().builtin_type) + ",");
			json.append("\"type_name\":\"" + f->get_return_type().native_type + "\",");
			json.append("\"kind\":" + itos(f->get_return_type().kind));
			json.append("},");
			json.append("\"stack_size\":" + itos(f->get_max_stack_size()) + ",");
			json.append("\"parameters\":[");
			comma2 = false;
			TRACE("  arguments:");
			for (int i = 0; i < f->get_argument_count(); ++i) {
				TRACE("    argument");
				if (comma2) json.append(",");
				json.append("{");
				json.append("\"name\":\"" + f->get_argument_name(i) + "\",");
				json.append("\"type\":" + itos(f->get_argument_type(i).builtin_type) + ",");
				json.append("\"type_name\":\"" + f->get_argument_type(i).native_type + "\",");
				json.append("\"kind\":" + itos(f->get_argument_type(i).kind));
				json.append("}");
				comma2 = true;
			}
			json.append("],");

			json.append("\"default_arguments\": [");
			TRACE("  default_arguments");
			comma2 = false;
			for (int i = 0; i < f->get_default_argument_count(); ++i) {
				if (comma2) json.append(",");
				json.append(itos(f->get_default_argument_addr(i)));
				comma2 = true;
			}
			json.append("],");

			json.append("\"global_names\": [");
			comma2 = false;
			for (int i = 0; i < f->get_global_name_count(); ++i) {
				if (comma2) json.append(",");
				json.append("\"" + f->get_global_name(i) + "\"");
				comma2 = true;
			}
			json.append("],");

			TRACE("  constants");
			json.append("\"constants\":[");
			comma2 = false;
			for (int i = 0; i < f->get_constant_count(); ++i) {
				TRACE("    constant");
				if (comma2) json.append(",");
				variantBuffer.clear();
				int len = put_var(f->get_constant(i), &variantBuffer);

				String declaration;
				VariantWriter::write_to_string(f->get_constant(i), declaration);

				json.append("{");
				json.append("\"index\":" + itos(i) + ",");
				json.append("\"type\":" + itos(f->get_constant(i).get_type()) + ",");
				json.append("\"type_name\":\"" + f->get_constant(i).get_type_name(f->get_constant(i).get_type()) + "\",");
				json.append("\"declaration\":\"" + declaration.replace("\"", "\\\"").replace("\n", "\\\n") + "\",");
				json.append("\"data\":[");
				bool comma3 = false;
				for (int j = 0; j < len; ++j) {
					if (comma3) json.append(",");
					json.append(itos(variantBuffer[j]));
					comma3 = true;
				}
				json.append("]");
				json.append("}");
				comma2 = true;
			}
			json.append("],");

			TRACE("bytecode");
			json.append("\"bytecode\":[");
			comma2 = false;
			for (int i = 0; i < f->get_code_size(); ++i) {
				if (comma2) json.append(",");
				json.append(itos(f->get_code()[i]));
				comma2 = true;
			}
			json.append("]");
			json.append("}");
		} else {
			TRACE("  Method not found. must be derived");
		}
		comma1 = true;
	}
	json.append("]");

	json.append("}");

	String jsonFilename = input_script_path + ".json";
	FileAccess *f = NULL;
	f = FileAccess::open(output_json_path.utf8().get_data(), FileAccess::WRITE);
	ERR_FAIL_COND(!f);
	f->store_string(json);
	f->close();
	memdelete(f);
}

void GDScriptBytecodeExporter::_bind_methods() {
    ClassDB::bind_method(D_METHOD("export_bytecode_to_file", "input_script_path", "output_json_path"), &GDScriptBytecodeExporter::export_bytecode_to_file);
}