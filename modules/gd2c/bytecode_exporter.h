#ifndef BYTECODE_EXPORTER_H
#define BYTECODE_EXPORTER_H

#include "core/reference.h"
#include "core/ustring.h"

class GDScriptBytecodeExporter : public Reference {
    GDCLASS(GDScriptBytecodeExporter, Reference)
public:
    GDScriptBytecodeExporter() {};
    
    void export_bytecode_to_file(String input_script_path, String output_json_path);

protected:
    static void _bind_methods();
};

#endif