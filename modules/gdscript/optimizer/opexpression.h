#ifndef GDSCRIPT_EXPRESSION_H
#define GDSCRIPT_EXPRESSION_H

#include "core/variant.h"
#include "modules/gdscript/gdscript_function.h"
#include "instruction.h"

class OpExpression {
public:
    OpExpression()
    : opcode(GDScriptFunction::Opcode::OPCODE_END)
    , defuse_mask(0)
    , expression_type(Variant::Type::VARIANT_MAX) { }

    int defuse_mask;
    int source_address0;
    int source_address1;
    GDScriptFunction::Opcode opcode;
    Variant::Operator variant_op;
    Variant::Type expression_type;

public:
	bool uses(int address) const;

public:
    friend bool operator< (const OpExpression& p_a, const OpExpression& p_b); 
    friend bool operator== (const OpExpression& p_a, const OpExpression& p_b);
    static OpExpression from_instruction(const Instruction& instruction);
    static bool is_instruction_expression(const Instruction& instruction);
};

#endif