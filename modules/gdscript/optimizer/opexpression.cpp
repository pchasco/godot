#include "core/variant.h"
#include "modules/gdscript/gdscript_function.h"
#include "instruction.h"
#include "opexpression.h"

bool OpExpression::uses(int address) const {
    if (((defuse_mask & INSTRUCTION_DEFUSE_SOURCE0) != 0)
    || ((defuse_mask & INSTRUCTION_DEFUSE_SOURCE1) != 0)) {
        return Instruction::normalize_address(source_address0) == Instruction::normalize_address(address)
            || Instruction::normalize_address(source_address1) == Instruction::normalize_address(address);
    }

    return false;
}

bool operator< (const OpExpression& p_a, const OpExpression& p_b) {
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

bool operator== (const OpExpression& p_a, const OpExpression& p_b) {
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
        if (p_a.source_address0 != p_b.source_address0) {
            return false;
        }
    }
    if (p_a.defuse_mask & p_b.defuse_mask & INSTRUCTION_DEFUSE_SOURCE1) {
        if (p_a.source_address1 != p_b.source_address1) {
            return false;
        }
    }        

    return true;
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

bool OpExpression::is_instruction_expression(const Instruction& instruction) {
    switch (instruction.opcode) {
        case GDScriptFunction::Opcode::OPCODE_OPERATOR:
        case GDScriptFunction::Opcode::OPCODE_ASSIGN:
            return true;
        default:
            return false;
    }
}



AvailableExpression* AvailableExpression::find_available_expression(const FastVector<AvailableExpression>& availables, const OpExpression& expression) {
    for (int i = 0; i < availables.size(); ++i) {
        const AvailableExpression& current = availables[i];
        if (!current.removed) {
            if (current.expression == expression) {
                return availables.address_of(i);
            }
        }
    }

    return nullptr;
}

AvailableExpression* AvailableExpression::find_available_expression_by_target(const FastVector<AvailableExpression>& availables, int target_address) {
    for (int i = 0; i < availables.size(); ++i) {
        const AvailableExpression& current = availables[i];
        if (!current.removed) {
            if (current.target_address == target_address) {
                return availables.address_of(i);
            }
        }
    }

    return nullptr;
}