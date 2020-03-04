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

// todo: maybe this should go in controlflow
class AvailableExpression {
public:
    AvailableExpression() : removed(false) {}

    OpExpression expression;
    int target_address;
    bool removed;

    bool uses_any(int address0, int address1) {
        return expression.uses(address0)
            || expression.uses(address1);
    }

public:
    static AvailableExpression* find_available_expression(const FastVector<AvailableExpression>& availables, const OpExpression& expression);
    static AvailableExpression* find_available_expression_by_target(const FastVector<AvailableExpression>& availables, int target_address);
};


#endif