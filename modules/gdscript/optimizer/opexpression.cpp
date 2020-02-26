#include "core/variant.h"
#include "modules/gdscript/gdscript_function.h"
#include "instruction.h"
#include "opexpression.h"

bool OpExpression::uses(int address) const {
    if (((defuse_mask & INSTRUCTION_DEFUSE_SOURCE0) != 0)
    || ((defuse_mask & INSTRUCTION_DEFUSE_SOURCE1) != 0)) {
        return source_address0 == address
            || source_address1 == address;
    }

    return false;
}

static bool operator< (const OpExpression& p_a, const OpExpression& p_b) {
    if (p_a.opcode < p_b.opcode) {
        return true;
    }
    if (p_a.expression_type < p_b.expression_type) {
        return true;
    }
    if (p_a.variant_op < p_b.variant_op) {
        return true;
    }
    if (p_a.defuse_mask < p_b.defuse_mask) {
        return true;
    }
    if (p_a.defuse_mask & p_b.defuse_mask & INSTRUCTION_DEFUSE_SOURCE0) {
        if (p_a.source_address0 < p_a.source_address0) {
            return true;
        } else {
            return false;
        }
    }
    if (p_a.defuse_mask & p_b.defuse_mask & INSTRUCTION_DEFUSE_SOURCE1) {
        if (p_a.source_address1 < p_a.source_address1) {
            return true;
        } else {
            return false;
        }
    }        

    return false;
}

static bool operator== (const OpExpression& p_a, const OpExpression& p_b) {
    if (p_a.opcode != p_b.opcode) {
        return false;
    }
    if (p_a.expression_type != p_b.expression_type) {
        return false;
    }
    if (p_a.variant_op != p_b.variant_op) {
        return false;
    }
    if (p_a.defuse_mask != p_b.defuse_mask) {
        return false;
    }
    if (p_a.defuse_mask & p_b.defuse_mask & INSTRUCTION_DEFUSE_SOURCE0) {
        if (p_a.source_address0 != p_a.source_address0) {
            return false;
        }
    }
    if (p_a.defuse_mask & p_b.defuse_mask & INSTRUCTION_DEFUSE_SOURCE1) {
        if (p_a.source_address1 != p_a.source_address1) {
            return false;
        }
    }        

    return false;
}

OpExpression OpExpression::from_instruction(const Instruction& instruction) {
    OpExpression expr;

    expr.opcode = instruction.opcode;
    expr.variant_op = instruction.variant_op;
    expr.expression_type = Variant::Type::VARIANT_MAX;
    expr.defuse_mask = instruction.defuse_mask;
    expr.source_address0 = instruction.source_address0;
    expr.source_address1 = instruction.source_address1;

    return expr;
}
