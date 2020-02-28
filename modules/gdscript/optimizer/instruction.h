#ifndef GDSCRIPT_INSTRUCTION_H
#define GDSCRIPT_INSTRUCTION_H

#include "core/variant.h"
#include "modules/gdscript/gdscript_function.h"
#include "fastvector.h"

#define INSTRUCTION_DEFUSE_TARGET 1
#define INSTRUCTION_DEFUSE_SOURCE0 2
#define INSTRUCTION_DEFUSE_SOURCE1 4
#define INSTRUCTION_DEFUSE_VARARGS 8
#define INSTRUCTION_DEFUSE_INDEX 16
#define INSTRUCTION_DEFUSE_SELF 32

class Instruction {
public:
	Instruction() : omit(false), defuse_mask(0) {}
    bool is_branch() const;
	String to_string() const;
    void sort_operands();
    bool may_have_side_effects() const;

public:
    GDScriptFunction::Opcode opcode;
    Variant::Operator variant_op;
    int target_address;
    int source_address0;
	union {
		int source_address1;
		int index_address;
	};
    int index_arg;
    int type_arg;
    int vararg_count;
    int branch_ip;
    int stride;
	bool omit;
	int defuse_mask;
    FastVector<int> varargs;

public:
    static Instruction parse(const int* buffer, const int index, const int buffer_size);
    static int normalize_address(int address);
private:
	String arguments_to_string() const;
};

#endif