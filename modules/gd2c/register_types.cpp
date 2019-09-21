#include "register_types.h"

#include "core/class_db.h"
#include "bytecode_exporter.h"
#include "gd2c.h"

void register_gd2c_types() {
    ClassDB::register_class<GDScriptBytecodeExporter>();
    ClassDB::register_class<GD2CApi>();
}

void unregister_gd2c_types() {
   //nothing to do here
}