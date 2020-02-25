#include "core/os/memory.h"
#include "modules/gdscript/gdscript_functions.h"
#include "gdscript_optimizer.h"
#include "fastvector.h"

FastVector<int> get_function_default_argument_jump_table(const GDScriptFunction *function) {
    Vector<int> defargs;
    for (int i = 0; i < function->get_default_argument_count(); ++i) {
        defargs.push_back(function->get_default_argument_addr(i));
    }
    defargs.sort();

    FastVector<int> result;
    for (int i = 0; i < defargs.size(); ++i) {
       result.push(defargs[i]);
    }

    return result;
}

OpExpression Instruction::get_expression() const {
    OpExpression expr;

    switch (opcode) {
    case GDScriptFunction::Opcode::OPCODE_ASSIGN:
        expr.opcode = opcode;
        expr.operand_address0 = source_address0;
        break;
    case GDScriptFunction::Opcode::OPCODE_OPERATOR:
        expr.opcode = opcode;
        expr.operand_address0 = source_address0;
        expr.operand_address1 = source_address1;
        break;
    default:
        // leave with default expr;
        break;
    }

    return expr;
}

bool Instruction::is_branch() const {
    switch (opcode) {
        case GDScriptFunction::Opcode::OPCODE_JUMP:
        case GDScriptFunction::Opcode::OPCODE_JUMP_IF:
        case GDScriptFunction::Opcode::OPCODE_JUMP_IF_NOT:
        case GDScriptFunction::Opcode::OPCODE_ITERATE:
        case GDScriptFunction::Opcode::OPCODE_ITERATE_BEGIN:
        case GDScriptFunction::Opcode::OPCODE_JUMP_TO_DEF_ARGUMENT:
            return true;
        default:
            return false;
    }
}

String Instruction::to_string() const {
    switch (opcode) {
        case GDScriptFunction::Opcode::OPCODE_OPERATOR: {
            String op;
            switch (variant_op) {
                case Variant::Operator::OP_ADD:
                    op = "+";
                    break;
                case Variant::Operator::OP_SUBTRACT:
                    op = "-";
                    break;
                case Variant::Operator::OP_MULTIPLY:
                    op = "*";
                    break;
                case Variant::Operator::OP_DIVIDE:
                    op = "/";
                    break;
                case Variant::Operator::OP_MODULE:
                    op = "%";
                    break;
                case Variant::Operator::OP_NOT:
                    op = "not";
                    break;
                case Variant::Operator::OP_NEGATE:
                    op = "negate";
                    break;
                case Variant::Operator::OP_AND:
                    op = "and";
                    break;
                case Variant::Operator::OP_OR:
                    op = "or";
                    break;
                case Variant::Operator::OP_XOR:
                    op = "xor";
                    break;
                case Variant::Operator::OP_IN:
                    op = "in";
                    break;
                case Variant::Operator::OP_LESS:
                    op = "<";
                    break;
                case Variant::Operator::OP_LESS_EQUAL:
                    op = "<=";
                    break;
                case Variant::Operator::OP_GREATER:
                    op = ">";
                    break;
                case Variant::Operator::OP_GREATER_EQUAL:
                    op = ">=";
                    break;
                default:
                    op = "¯\\_(ツ)_/¯";
                    break;
            }
            
            return "OPERATOR " + op;
        }            
        case GDScriptFunction::Opcode::OPCODE_JUMP:
            return "JUMP " + itos(branch_ip);
        case GDScriptFunction::Opcode::OPCODE_JUMP_IF:
            return "JUMP_IF " + itos(branch_ip);
        case GDScriptFunction::Opcode::OPCODE_JUMP_IF_NOT:
            return "JUMP_IF_NOT " + itos(branch_ip);
        case GDScriptFunction::Opcode::OPCODE_ITERATE:
            return "ITERATE (ESCAPE " + itos(branch_ip) + ")";
        case GDScriptFunction::Opcode::OPCODE_ITERATE_BEGIN:
            return "ITERATE_BEGIN (ESCAPE " + itos(branch_ip) + ")";
        case GDScriptFunction::Opcode::OPCODE_LINE:
            return "LINE";
        case GDScriptFunction::Opcode::OPCODE_RETURN:
            return "RETURN";
        case GDScriptFunction::Opcode::OPCODE_CALL_RETURN: {
            String args;
            for (int i = 0; i < varargs.size(); ++i) {
                args += " " + itos(varargs[i]);
            }
            return "CALL_RETURN";
        }
        case GDScriptFunction::Opcode::OPCODE_CALL_SELF_BASE: {
            String args;
            for (int i = 0; i < varargs.size(); ++i) {
                args += " " + itos(varargs[i]);
            }
            return "CALL_SELF_BASE " + itos(index_arg) + "(" + args + ")";
        }
        case GDScriptFunction::Opcode::OPCODE_CALL_SELF: {
            String args;
            for (int i = 0; i < varargs.size(); ++i) {
                args += " " + itos(varargs[i]);
            }
            return "CALL_SELF " + itos(index_arg) + "(" + args + ")";
        }
        case GDScriptFunction::Opcode::OPCODE_CALL_BUILT_IN: {
            String args;
            for (int i = 0; i < varargs.size(); ++i) {
                args += " " + itos(varargs[i]);
            }
            return "CALL_BUILT_IN " + itos(index_arg) + "(" + args + ")";
        }
        case GDScriptFunction::Opcode::OPCODE_END:
            return "END";
        case GDScriptFunction::Opcode::OPCODE_CALL: {
            String args;
            for (int i = 0; i < varargs.size(); ++i) {
                args += " " + itos(varargs[i]);
            }
            return "CALL " + itos(index_arg) + "(" + args + ")";
        }
        case GDScriptFunction::Opcode::OPCODE_ASSIGN:
            return "ASSIGN";
        case GDScriptFunction::Opcode::OPCODE_JUMP_TO_DEF_ARGUMENT: {
            String args;
            for (int i = 0; i < varargs.size(); ++i) {
                args += " " + itos(varargs[i]);
            }
            return "JUMP_TO_DEF_ARGUMENT" + args;
        }
        case GDScriptFunction::Opcode::OPCODE_CONSTRUCT_ARRAY: {
            String args;
            for (int i = 0; i < varargs.size(); ++i) {
                args += " " + itos(varargs[i]);
            }
            return "CONSTRUCT_ARRAY [" + args + "]";
        }
        case GDScriptFunction::Opcode::OPCODE_BOX_INT: {
            return "BOX INT " + itos(source_address0) + " into " + itos(target_address);
        }
        case GDScriptFunction::Opcode::OPCODE_BOX_REAL: {
            return "BOX REAL " + itos(source_address0) + " into " + itos(target_address);
        }
        case GDScriptFunction::Opcode::OPCODE_UNBOX_INT: {
            return "UNBOX INT " + itos(source_address0) + " into " + itos(target_address);
        }
        case GDScriptFunction::Opcode::OPCODE_UNBOX_REAL: {
            return "UNBOX REAL " + itos(source_address0) + " into " + itos(target_address);
        }
        default:
            return "Unknown instruction " + itos((int)opcode); 
    }
}

void Block::replace_jumps(Block& original_block, Block& target_block) {
    for (int i = 0; i < instructions.size(); ++i) {
        Instruction& inst = instructions[i];
        switch (inst.opcode) {
            case GDScriptFunction::Opcode::OPCODE_JUMP:
            case GDScriptFunction::Opcode::OPCODE_JUMP_IF:
            case GDScriptFunction::Opcode::OPCODE_JUMP_IF_NOT:
            case GDScriptFunction::Opcode::OPCODE_ITERATE:
            case GDScriptFunction::Opcode::OPCODE_ITERATE_BEGIN:
                if (inst.branch_ip == original_block.id) {
                    inst.branch_ip = target_block.id;
                }
                break;
        }
    }

    forward_edges.erase(original_block.id);
    forward_edges.insert(target_block.id);
    target_block.back_edges.insert(id);
    original_block.back_edges.erase(id);
}

void Block::update_def_use() {
    // Iterate over instructions in block. When a variable is assigned a value
    // it goes into the defs set. If it is read then it goes into the use set 
    // if it is not already in the defs set.
    defs.clear();
    uses.clear();

    for (int i = 0; i < instructions.size(); ++i) {
        Instruction inst = instructions[i];

        if ((inst.defuse_mask & INSTRUCTION_DEFUSE_TARGET)) {
            defs.insert(inst.target_address);
        }
        if ((inst.defuse_mask & INSTRUCTION_DEFUSE_SELF)) {
            // We never define self within the body of any function
            uses.insert(((int)GDScriptFunction::Address::ADDR_TYPE_SELF) << ((int)GDScriptFunction::Address::ADDR_BITS) & ((int)GDScriptFunction::Address::ADDR_MASK));
        }
        if ((inst.defuse_mask & INSTRUCTION_DEFUSE_SOURCE0)) {
            if (!defs.has(inst.source_address0)) {
                uses.insert(inst.source_address0);
            }
        }
        if ((inst.defuse_mask & INSTRUCTION_DEFUSE_SOURCE1)) {
            if (!defs.has(inst.source_address1)) {
                uses.insert(inst.source_address1);
            }
        }
        if ((inst.defuse_mask & INSTRUCTION_DEFUSE_INDEX)) {
            if (!defs.has(inst.index_address)) {
                uses.insert(inst.index_address);
            }
        }
        if ((inst.defuse_mask & INSTRUCTION_DEFUSE_VARARGS)) {
            for (int j = 0; j < inst.varargs.size(); ++j) {
                int address = inst.varargs[j];
                if (!defs.has(address)) {
                    uses.insert(address);
                }
            }
        }
    }
}

bool OpExpression::matches(const OpExpression &other) const {
    if (opcode != other.opcode) {
        return false;
    }
    if (result_type != other.result_type) {
        return false;
    }

    if (opcode == GDScriptFunction::Opcode::OPCODE_ASSIGN) {
        return operand_address0 == other.operand_address0;
    }

    if (opcode == GDScriptFunction::Opcode::OPCODE_OPERATOR) {			
        if (variant_operator != other.variant_operator) {
            // Expressions are not same if operator is different
            return false;
        }

        // Check expression type. We do not consider expressions
        // with different types for CSE
        switch (result_type) {
        case Variant::Type::INT:
        case Variant::Type::REAL:
        case Variant::Type::BOOL:
            // Any of these types are supported
            break;
        default:
            return false;
        }

        // Sort operands when possible to potentially match more
        // expressions

        bool is_unary = false;
        int this_address1 = 0;
        int this_address2 = 0;
        int other_address1 = 0;
        int other_address2 = 0;

        switch (variant_operator) {
        // Unary ops
        case Variant::Operator::OP_NEGATE:
            is_unary = true;
            this_address1 = operand_address0;
            other_address1 = other.operand_address0;
            break;

        // Non-commutative operators. a-b != b-a, etc
        // Preserve operand order for comparison
        case Variant::Operator::OP_SUBTRACT:
        case Variant::Operator::OP_DIVIDE:
        case Variant::Operator::OP_MODULE:
            this_address1 = operand_address0;
            this_address2 = operand_address1;
            other_address1 = other.operand_address0;
            other_address2 = other.operand_address1;
            break;

        // Commutative operators. a+b == b+a.
        // Sort operands by address asc
        case Variant::Operator::OP_ADD:
        case Variant::Operator::OP_MULTIPLY:
        case Variant::Operator::OP_AND:
        case Variant::Operator::OP_OR:
        case Variant::Operator::OP_XOR:
        case Variant::Operator::OP_BIT_AND:
        case Variant::Operator::OP_BIT_OR:
        case Variant::Operator::OP_BIT_XOR:
            if (operand_address1 < operand_address0) {
                this_address1 = operand_address1;
                this_address2 = operand_address0;
            } else {
                this_address1 = operand_address0;
                this_address2 = operand_address1;
            }

            if (other.operand_address1 < other.operand_address0) {
                other_address1 = other.operand_address1;
                other_address2 = other.operand_address0;
            } else {
                other_address1 = other.operand_address0;
                other_address2 = other.operand_address1;
            }

        default:
            return false;
        }			

        // If all else has matched, then compare operands.
        // If operands match then these expressions are the same.

        return this_address1 == other_address1
            && (is_unary || this_address2 == other_address2);
    }

    return false;
}


ControlFlowGraph::ControlFlowGraph(const GDScriptFunction *function) {
    _function = function;
}

void ControlFlowGraph::construct() {
    disassemble();
    build_blocks();
}

void ControlFlowGraph::disassemble() {
    _instructions.clear();

    int ip = 0;
    const int *code = _function->get_code();
    int code_size = _function->get_code_size();
    while (ip < code_size) {
        GDScriptFunction::Opcode opcode = (GDScriptFunction::Opcode)code[ip];
        Instruction inst;
        inst.opcode = opcode;
        int inst_start_ip = ip;

        switch (opcode) {
            case GDScriptFunction::Opcode::OPCODE_OPERATOR:
                inst.variant_op = (Variant::Operator)code[ip + 1];
                inst.source_address0 = code[ip + 2];
                inst.source_address1 = code[ip + 3];
                inst.target_address = code[ip + 4];
                inst.defuse_mask = INSTRUCTION_DEFUSE_TARGET
                    | INSTRUCTION_DEFUSE_SOURCE0
                    | INSTRUCTION_DEFUSE_SOURCE1;
                ip += 5;
                break;
            case GDScriptFunction::Opcode::OPCODE_EXTENDS_TEST:
                inst.source_address0 = code[ip + 1]; // instance
                inst.source_address1 = code[ip + 2]; // type
                inst.target_address = code[ip + 3];
                inst.defuse_mask = INSTRUCTION_DEFUSE_TARGET
                    | INSTRUCTION_DEFUSE_SOURCE0
                    | INSTRUCTION_DEFUSE_SOURCE1;
                ip += 4;
                break;
            case GDScriptFunction::Opcode::OPCODE_IS_BUILTIN:
                inst.source_address0 = code[ip + 1];
                inst.target_address = code[ip + 3];
                inst.type_arg = code[ip + 2];
                inst.defuse_mask = INSTRUCTION_DEFUSE_TARGET
                    | INSTRUCTION_DEFUSE_SOURCE0;
                ip += 4;
                break;
            case GDScriptFunction::Opcode::OPCODE_SET:
                inst.target_address = code[ip + 1];
                inst.index_address = code[ip + 2]; // index
                inst.source_address0 = code[ip + 3]; // value
                inst.defuse_mask = INSTRUCTION_DEFUSE_SOURCE0
                    | INSTRUCTION_DEFUSE_INDEX
                    | INSTRUCTION_DEFUSE_TARGET;
                ip += 4;
                break;
            case GDScriptFunction::Opcode::OPCODE_GET:
                inst.source_address0 = code[ip + 1]; // value
                inst.index_address = code[ip + 2]; // index
                inst.target_address = code[ip + 3];
                inst.defuse_mask = INSTRUCTION_DEFUSE_SOURCE0
                    | INSTRUCTION_DEFUSE_INDEX
                    | INSTRUCTION_DEFUSE_TARGET;
                ip += 4;
                break;
            case GDScriptFunction::Opcode::OPCODE_SET_NAMED:
                inst.source_address0 = code[ip + 1]; // value
                inst.index_arg = code[ip + 2]; // name index
                inst.target_address = code[ip + 3];
                inst.defuse_mask = INSTRUCTION_DEFUSE_SOURCE0
                    | INSTRUCTION_DEFUSE_TARGET;
                ip += 4;
                break;
            case GDScriptFunction::Opcode::OPCODE_GET_NAMED:
                inst.target_address = code[ip + 1];
                inst.source_address0 = code[ip + 3]; // value
                inst.defuse_mask = INSTRUCTION_DEFUSE_SOURCE0
                    | INSTRUCTION_DEFUSE_TARGET;
                ip += 4;
                break;
            case GDScriptFunction::Opcode::OPCODE_SET_MEMBER:
                inst.index_arg = code[ip + 1]; // name index
                inst.source_address0 = code[ip + 2]; // value
                inst.defuse_mask = INSTRUCTION_DEFUSE_SOURCE0
                    | INSTRUCTION_DEFUSE_SELF;
                ip += 3;
                break;
            case GDScriptFunction::Opcode::OPCODE_GET_MEMBER:
                inst.index_arg = code[ip + 1]; // name index
                inst.target_address = code[ip + 2]; // value
                inst.defuse_mask = INSTRUCTION_DEFUSE_TARGET
                    | INSTRUCTION_DEFUSE_SELF;
                ip += 3;
                break;
            case GDScriptFunction::Opcode::OPCODE_ASSIGN:
                inst.target_address = code[ip + 1];
                inst.source_address0 = code[ip + 2]; 
                inst.defuse_mask = INSTRUCTION_DEFUSE_SOURCE0
                    | INSTRUCTION_DEFUSE_TARGET;
                ip += 3;
                break;
            case GDScriptFunction::Opcode::OPCODE_ASSIGN_TRUE:
                inst.target_address = code[ip + 1];
                inst.defuse_mask = INSTRUCTION_DEFUSE_TARGET;
                ip += 2;
                break;
            case GDScriptFunction::Opcode::OPCODE_ASSIGN_FALSE:
                inst.target_address = code[ip + 1];
                inst.defuse_mask = INSTRUCTION_DEFUSE_TARGET;
                ip += 2;
                break;
            case GDScriptFunction::Opcode::OPCODE_ASSIGN_TYPED_BUILTIN:
                inst.type_arg = code[ip + 1];
                inst.target_address = code[ip + 2];
                inst.source_address0 = code[ip + 3];
                inst.defuse_mask = INSTRUCTION_DEFUSE_SOURCE0
                    | INSTRUCTION_DEFUSE_TARGET;
                ip += 4;
                break;
            case GDScriptFunction::Opcode::OPCODE_ASSIGN_TYPED_NATIVE:
                inst.source_address0 = code[ip + 1]; // type
                inst.target_address = code[ip + 2];
                inst.source_address1 = code[ip + 3]; // source
                inst.defuse_mask = INSTRUCTION_DEFUSE_SOURCE0
                    | INSTRUCTION_DEFUSE_SOURCE1
                    | INSTRUCTION_DEFUSE_TARGET;
                ip += 4;
                break;
            case GDScriptFunction::Opcode::OPCODE_ASSIGN_TYPED_SCRIPT:
                inst.source_address0 = code[ip + 1]; // type
                inst.target_address = code[ip + 2];
                inst.source_address1 = code[ip + 3]; // source
                inst.defuse_mask = INSTRUCTION_DEFUSE_SOURCE0
                    | INSTRUCTION_DEFUSE_SOURCE1
                    | INSTRUCTION_DEFUSE_TARGET;
                ip += 4;
                break;
            case GDScriptFunction::Opcode::OPCODE_CAST_TO_BUILTIN:
                inst.type_arg = code[ip + 1]; // variant::type
                inst.source_address0 = code[ip + 2]; // source
                inst.target_address = code[ip + 3];
                inst.defuse_mask = INSTRUCTION_DEFUSE_SOURCE0
                    | INSTRUCTION_DEFUSE_TARGET;
                ip += 4;
                break;
            case GDScriptFunction::Opcode::OPCODE_CAST_TO_NATIVE:
                inst.source_address0 = code[ip + 1]; // to type
                inst.source_address1 = code[ip + 2]; // source
                inst.target_address = code[ip + 3];
                inst.defuse_mask = INSTRUCTION_DEFUSE_SOURCE0
                    | INSTRUCTION_DEFUSE_SOURCE1
                    | INSTRUCTION_DEFUSE_TARGET;
                ip += 4;
                break;
            case GDScriptFunction::Opcode::OPCODE_CAST_TO_SCRIPT:
                inst.source_address0 = code[ip + 1]; // to type
                inst.source_address1 = code[ip + 2]; // source
                inst.target_address = code[ip + 3];
                inst.defuse_mask = INSTRUCTION_DEFUSE_SOURCE0
                    | INSTRUCTION_DEFUSE_SOURCE1
                    | INSTRUCTION_DEFUSE_TARGET;
                ip += 4;
                break;
            case GDScriptFunction::Opcode::OPCODE_CONSTRUCT:
                inst.type_arg = code[ip + 1];
                inst.vararg_count = code[ip + 2];
                for (int i = 0; i < inst.vararg_count; ++i) {
                    inst.varargs.push(code[ip + 3 + i]);
                }
                inst.target_address = code[ip + 3 + inst.vararg_count];
                inst.defuse_mask = INSTRUCTION_DEFUSE_VARARGS
                    | INSTRUCTION_DEFUSE_TARGET;
                ip += 4 + inst.vararg_count;
                break;
            case GDScriptFunction::Opcode::OPCODE_CONSTRUCT_ARRAY:
                inst.vararg_count = code[ip + 1];
                for (int i = 0; i < inst.vararg_count; ++i) {
                    inst.varargs.push(code[ip + 2 + i]);
                }
                inst.target_address = code[ip + 2 + inst.vararg_count];
                inst.defuse_mask = INSTRUCTION_DEFUSE_VARARGS
                    | INSTRUCTION_DEFUSE_TARGET;
                ip += 3 + inst.vararg_count;
                break;
            case GDScriptFunction::Opcode::OPCODE_CONSTRUCT_DICTIONARY:
                inst.vararg_count = code[ip + 1];
                for (int i = 0; i < inst.vararg_count; ++i) {
                    inst.varargs.push(code[ip + 2 + i]);
                }
                inst.target_address = code[ip + 2 + (inst.vararg_count * 2)];
                inst.defuse_mask = INSTRUCTION_DEFUSE_VARARGS
                    | INSTRUCTION_DEFUSE_TARGET;
                ip += 3 + inst.vararg_count;
                break;
            case GDScriptFunction::Opcode::OPCODE_CALL:
            case GDScriptFunction::Opcode::OPCODE_CALL_RETURN:
                inst.vararg_count = code[ip + 1];
                inst.source_address0 = code[ip + 2];
                inst.index_arg = code[ip + 3];
                for (int i = 0; i < inst.vararg_count; ++i) {
                    inst.varargs.push(code[ip + 4 + i]);
                }
                // If this is CALL then whatever value is here will be ignored in execution
                // If this is CALL_RETURN then it will be the location to store result
                inst.target_address = code[ip + 4 + inst.vararg_count];
                inst.defuse_mask = INSTRUCTION_DEFUSE_VARARGS
                    | INSTRUCTION_DEFUSE_SOURCE0;
                if (inst.opcode == GDScriptFunction::Opcode::OPCODE_CALL_RETURN) {
                    inst.defuse_mask |= INSTRUCTION_DEFUSE_TARGET;
                }
                ip += 5 + inst.vararg_count;
                break;
            case GDScriptFunction::Opcode::OPCODE_CALL_BUILT_IN:
                inst.index_arg = code[ip + 1]; // function index
                inst.vararg_count = code[ip + 2];
                for (int i = 0; i < inst.vararg_count; ++i) {
                    inst.varargs.push(code[ip + 3 + i]);
                }
                inst.target_address = code[ip + 3 + inst.vararg_count];
                inst.defuse_mask = INSTRUCTION_DEFUSE_VARARGS
                    | INSTRUCTION_DEFUSE_TARGET;
                ip += 4 + inst.vararg_count;
                break;
            case GDScriptFunction::Opcode::OPCODE_CALL_SELF:
                // ?
                ip += 1;
                break;
            case GDScriptFunction::Opcode::OPCODE_CALL_SELF_BASE:
                inst.index_arg = code[ip + 1];
                inst.vararg_count = code[ip + 2];
                for (int i = 0; i < inst.vararg_count; ++i) {
                    inst.varargs.push(code[ip + 3 + i]);
                }
                inst.target_address = code[ip + 3 + inst.vararg_count + 1];
                inst.defuse_mask = INSTRUCTION_DEFUSE_VARARGS
                    | INSTRUCTION_DEFUSE_TARGET
                    | INSTRUCTION_DEFUSE_SELF;
                ip += 4 + inst.vararg_count;
                break;
            case GDScriptFunction::Opcode::OPCODE_YIELD:
                ip += 2;
                break;
            case GDScriptFunction::Opcode::OPCODE_YIELD_SIGNAL:
                inst.source_address0 = code[ip + 1];
                inst.index_arg = code[ip + 2];
                inst.defuse_mask = INSTRUCTION_DEFUSE_SOURCE0;
                ip += 3;
                break;
            case GDScriptFunction::Opcode::OPCODE_YIELD_RESUME:
                inst.target_address = code[ip + 1];
                inst.defuse_mask = INSTRUCTION_DEFUSE_TARGET;
                break;
            case GDScriptFunction::Opcode::OPCODE_JUMP:
                inst.branch_ip = code[ip + 1];
                ip += 2;
                break;
            case GDScriptFunction::Opcode::OPCODE_JUMP_IF:
                inst.source_address0 = code[ip + 1];
                inst.branch_ip = code[ip + 2];
                inst.defuse_mask = INSTRUCTION_DEFUSE_SOURCE0;
                ip += 3;
                break;
            case GDScriptFunction::Opcode::OPCODE_JUMP_IF_NOT:
                inst.source_address0 = code[ip + 1];
                inst.branch_ip = code[ip + 2];
                inst.defuse_mask = INSTRUCTION_DEFUSE_SOURCE0;
                ip += 3;
                break;
            case GDScriptFunction::Opcode::OPCODE_JUMP_TO_DEF_ARGUMENT:
                ip += 1;
                break;
            case GDScriptFunction::Opcode::OPCODE_ITERATE_BEGIN:
                inst.source_address0 = code[ip + 1]; // counter
                inst.source_address1 = code[ip + 2]; // container
                inst.branch_ip = code[ip + 3];
                inst.target_address = code[ip + 4]; // iterator
                inst.defuse_mask = INSTRUCTION_DEFUSE_SOURCE0
                    | INSTRUCTION_DEFUSE_SOURCE1
                    | INSTRUCTION_DEFUSE_TARGET;
                ip += 5;
                break; 
            case GDScriptFunction::Opcode::OPCODE_ITERATE:
                inst.source_address0 = code[ip + 1]; // counter
                inst.source_address1 = code[ip + 2]; // container
                inst.branch_ip = code[ip + 3];
                inst.target_address = code[ip + 4]; // iterator
                inst.defuse_mask = INSTRUCTION_DEFUSE_SOURCE0
                    | INSTRUCTION_DEFUSE_SOURCE1
                    | INSTRUCTION_DEFUSE_TARGET;
                ip += 5;
                break;          
            case GDScriptFunction::Opcode::OPCODE_ASSERT:
                inst.source_address0 = code[ip + 1]; // test
                inst.source_address1 = code[ip + 2]; // message
                inst.defuse_mask = INSTRUCTION_DEFUSE_SOURCE0
                    | INSTRUCTION_DEFUSE_SOURCE1;
                ip += 3;
                break;
            case GDScriptFunction::Opcode::OPCODE_BREAKPOINT:
                ip += 1;
                break;
            case GDScriptFunction::Opcode::OPCODE_LINE:
                inst.index_arg = code[ip + 1];
                ip += 2;
                break;
            case GDScriptFunction::Opcode::OPCODE_END:
                ip += 1;
                break;
            case GDScriptFunction::Opcode::OPCODE_RETURN:
                inst.source_address0 = code[ip + 1];
                inst.defuse_mask = INSTRUCTION_DEFUSE_SOURCE0;
                ip += 2;
                break;
            case GDScriptFunction::Opcode::OPCODE_BOX_INT:
                inst.source_address0 = code[ip + 1];
                inst.target_address = code[ip + 2];
                inst.defuse_mask = INSTRUCTION_DEFUSE_SOURCE0 
                    | INSTRUCTION_DEFUSE_TARGET;
                ip += 3;        
                break;
            case GDScriptFunction::Opcode::OPCODE_BOX_REAL:
                inst.source_address0 = code[ip + 1];
                inst.target_address = code[ip + 2];
                inst.defuse_mask = INSTRUCTION_DEFUSE_SOURCE0 
                    | INSTRUCTION_DEFUSE_TARGET;
                ip += 3;        
                break;
            case GDScriptFunction::Opcode::OPCODE_UNBOX_INT:
                inst.source_address0 = code[ip + 1];
                inst.target_address = code[ip + 2];
                inst.defuse_mask = INSTRUCTION_DEFUSE_SOURCE0 
                    | INSTRUCTION_DEFUSE_TARGET;
                ip += 3;        
                break;
            case GDScriptFunction::Opcode::OPCODE_UNBOX_REAL:
                inst.source_address0 = code[ip + 1];
                inst.target_address = code[ip + 2];
                inst.defuse_mask = INSTRUCTION_DEFUSE_SOURCE0 
                    | INSTRUCTION_DEFUSE_TARGET;
                ip += 3;        
                break;
            
            default:
                ERR_FAIL_MSG("Invalid opcode");
                ip = 999999999;
                break;
        }

        inst.stride = ip - inst_start_ip;
        _instructions.push(inst);
    }
}

#define ENTRY_BLOCK_ID -1
#define EXIT_BLOCK_ID 200000000

void ControlFlowGraph::build_blocks() {
    FastVector<Block> blocks;
    FastVector<int> worklist;
    FastVector<int> visited;
    FastVector<int> jump_targets;
    
    Block entry;
    entry.id = ENTRY_BLOCK_ID;
    entry.forward_edges.insert(0);
    {
        Instruction jump;
        jump.opcode = GDScriptFunction::Opcode::OPCODE_JUMP;
        jump.branch_ip = 0;
        jump.stride = 2;
        entry.instructions.push(jump);
    }
    _entry_id = entry.id;

    Block exit;
    exit.id = EXIT_BLOCK_ID;
    _exit_id = exit.id;

    blocks.clear();
    blocks.push(entry);
    blocks.push(exit);

    worklist.push(0);

    for (int i = 0; i < _instructions.size(); ++i) {
        Instruction& inst = _instructions[i];

        switch (inst.opcode) {
            case GDScriptFunction::Opcode::OPCODE_JUMP:
            case GDScriptFunction::Opcode::OPCODE_JUMP_IF:
            case GDScriptFunction::Opcode::OPCODE_JUMP_IF_NOT:
            case GDScriptFunction::Opcode::OPCODE_ITERATE:
            case GDScriptFunction::Opcode::OPCODE_ITERATE_BEGIN:
                jump_targets.push(inst.branch_ip);
                break;
            case GDScriptFunction::Opcode::OPCODE_RETURN:
                jump_targets.push(_exit_id);
                break;
            case GDScriptFunction::Opcode::OPCODE_JUMP_TO_DEF_ARGUMENT:
                for (int defarg = 0; defarg < _function->get_default_argument_count(); ++defarg) {
                    int defarg_ip = _function->get_default_argument_addr(defarg);
                    jump_targets.push(defarg_ip);
                }
                break;
        }
    }

    // We will use the jump table to avoid altering some blocks by inserting goto which would
    // change the offset of the next defarg assignment block invalidating the jump table
    FastVector<int> defarg_jump_table = get_function_default_argument_jump_table(_function);
    // It's OK to modify the last block in the jump table
    if (!defarg_jump_table.empty()) {
        defarg_jump_table.pop();
    }

    while (!worklist.empty()) {
        int block_id = worklist.pop();

        if (visited.has(block_id)) {
            continue;
        }

        visited.push(block_id);

        Block block;
        block.id = block_id;

        // Seek to instruction at beginning of block
        int ip = 0;
        int i = 0;
        for (i = 0; i < _instructions.size(); ++i) {
            if (ip == block_id) {
                break;
            }

            ip += _instructions[i].stride;
        }

        int block_start_ip = ip;

        // Take block instructions
        bool done = false;
        while (!done) {
            Instruction& inst = _instructions[i];
            i += 1;

            block.instructions.push(inst);
            ip += inst.stride;

            switch (inst.opcode) {
                case GDScriptFunction::Opcode::OPCODE_JUMP:
                    worklist.push(inst.branch_ip);
                    block.forward_edges.insert(inst.branch_ip);
                    done = true;
                    break;
                case GDScriptFunction::Opcode::OPCODE_JUMP_IF:
                case GDScriptFunction::Opcode::OPCODE_JUMP_IF_NOT:
                case GDScriptFunction::Opcode::OPCODE_ITERATE:
                case GDScriptFunction::Opcode::OPCODE_ITERATE_BEGIN:
                    worklist.push(ip);
                    worklist.push(inst.branch_ip);
                    block.forward_edges.insert(ip);
                    block.forward_edges.insert(inst.branch_ip);
                    done = true;
                    break;
                case GDScriptFunction::Opcode::OPCODE_RETURN:
                case GDScriptFunction::Opcode::OPCODE_END:
                    block.forward_edges.insert(exit.id);
                    done = true;
                    break;
                case GDScriptFunction::Opcode::OPCODE_JUMP_TO_DEF_ARGUMENT:
                    // Can't use defarg_jump_table because we popped one out. Refer back to function
                    for (int defarg = 0; defarg < _function->get_default_argument_count(); ++defarg) {
                        int defarg_ip = _function->get_default_argument_addr(defarg);
                        block.forward_edges.insert(defarg_ip);
                        worklist.push(defarg_ip);
                    }
                    block.forward_edges.insert(ip);
                    worklist.push(ip);
                    done = true;
                    break;
                default:
                    if (jump_targets.has(ip)) {
                        block.forward_edges.insert(ip);
                        done = true;
                    }
                    break;
            } 

            if (done) {
                // Insert jump instruction to next block. Unneeded/unreachable
                // jump instructions will be removed later during
                // optimizations. Also don't alter default argument blocks
                int branch_ip;
                switch (inst.opcode) {
                    case GDScriptFunction::Opcode::OPCODE_END:
                    case GDScriptFunction::Opcode::OPCODE_RETURN:
                        branch_ip = _exit_id;
                        break;
                    default:
                        branch_ip = ip;
                        break;
                }
                if (block.forward_edges.has(branch_ip)
                    && !defarg_jump_table.has(branch_ip)) {
                    Instruction jump; 
                    jump.opcode = GDScriptFunction::Opcode::OPCODE_JUMP;
                    jump.branch_ip = branch_ip;
                    jump.stride = 2;
                    block.instructions.push(jump);
                }

                if (defarg_jump_table.has(block_start_ip)) {
                    block.force_code_size = ip - block_start_ip;
                }
            }
        }   

        blocks.push(block);
    }
    
    // Create back edges
    for (int bi = 0; bi < blocks.size(); ++bi) {
        for (Set<int>::Element *E = blocks[bi].forward_edges.front(); E; E = E->next()) {
            Block *succ = find_block(&blocks, E->get());
            succ->back_edges.insert(blocks[bi].id);
        }
    }

    _blocks = blocks;
}

void ControlFlowGraph::remove_dead_blocks() {
    Block *entry_block = get_entry_block();
    ERR_FAIL_COND_MSG(entry_block == nullptr, "ControlFlowGraph contains no blocks");

    Block *exit_block = get_exit_block();
    ERR_FAIL_COND_MSG(entry_block == nullptr, "ControlFlowGraph contains no blocks");

    FastVector<Block*> blocks_to_retain;
    FastVector<Block*> blocks_to_remove;

    // Get list of blocks in default argument jump table. Avoid removing these blocks
    FastVector<int> defarg_jumps = get_function_default_argument_jump_table(_function);
    // In this case we must never completely remove any defarg assignment blocks, even
    // if it only contains a jump. If we can modify the jump table then we could do this.

    for (int block_index = 0; block_index < _blocks.size(); ++block_index) {
        Block *block = _blocks.address_of(block_index);

        // If block has no back edges then we can
        // remove it if it is not in the default argument jump table (could happen after dead code elimination)
        if (block->id != entry_block->id
            && block->id != exit_block->id
            && !defarg_jumps.has(block->id)
            && block->back_edges.empty()) {

            blocks_to_remove.push(block);
        } else {
            blocks_to_retain.push(block);
        }
    }

    // Remove this block from back edges of any blocks
    for (int i = 0; i < blocks_to_remove.size(); ++i) {
        Block *block = blocks_to_remove[i];
        Block *target = find_block(block->forward_edges.front()->get());
        target->back_edges.erase(block->id);
    }

    // Replace _blocks with new list of blocks.
    FastVector<Block> new_blocks;
    for (int i = 0; i < blocks_to_retain.size(); ++i) {
        new_blocks.push(*blocks_to_retain[i]);
    }

    _blocks = new_blocks;
}

void ControlFlowGraph::analyze_data_flow() {
    for (int i = 0; i < _blocks.size(); ++i) {
        _blocks[i].update_def_use();
        _blocks[i].ins.clear();
        _blocks[i].outs.clear();

        // All use variables must be in the ins set
        for (Set<int>::Element *A = _blocks[i].uses.front(); A; A = A->next()) {
            _blocks[i].ins.insert(A->get());
        }
    }

    // Walk the CFG in reverse to propagate live variables. This must be
    // done repeatedly until a loop is completed without any additional changes.
    while (true) {
        bool repeat = false;
        FastVector<int> worklist;
        FastVector<int> visited;

        // We walk the CFG in reverse to determine data flow
        worklist.push(_exit_id);
        
        while(!worklist.empty()) {
            int block_id = worklist.pop();
            if (visited.has(block_id)) {
                continue;
            }

            visited.push(block_id);

            Block *block = find_block(block_id);

            // Insert all ins from successive blocks into this block's outs
            for (Set<int>::Element *E = block->forward_edges.front(); E; E = E->next()) {
                Block *succ = find_block(E->get());
                for (Set<int>::Element *A = succ->ins.front(); A; A = A->next()) {
                    if (!block->outs.has(A->get())) {
                        block->outs.insert(A->get());
                        repeat = true;
                    }
                }
            }

            // Update this block's ins with any addresses in its outs set that
            // are not defined by this block
            for (Set<int>::Element *A = block->outs.front(); A; A = A->next()) {
                if (!block->defs.has(A->get())) {
                    if (!block->ins.has(A->get())) {
                        block->ins.insert(A->get());
                        repeat = true;
                    }
                }
            }     

            for (Set<int>::Element *E = block->back_edges.front(); E; E = E->next()) {
                worklist.push(E->get());
            }
        }

        if (!repeat) {
            break;
        }
    }
}

FastVector<int> ControlFlowGraph::get_bytecode() {
    FastVector<int> worklist;
    FastVector<int> visited;
    FastVector<int> bytecode;
    FastVector<int> blocks_in_order;
    Map<int, int> block_ip_index;
    int ip = 0;

    // Keep a list of the default argument assignment blocks to refer back to later. We use this
    // to avoid modifying them
    FastVector<int> defarg_jump_table = get_function_default_argument_jump_table(_function);
    // Though we can allow the last defarg assignment block be modified
    if (!defarg_jump_table.empty()) {
        defarg_jump_table.pop();
    }
    
    // First in the list is the entry block. However, entry block is
    // a special case and should have no instructions (until function 
    // to modify defarg jump table is in place).
    // This should not interfere with defarg jump table order
    worklist.push(_entry_id);

    // Now allow the blocks to fall into place wherever.
    // A future enhancement would be to sort the blocks to minimize the number of jumps.
    // Defarg blocks will naturally fall into the correct order because each (except for the last one)
    // will have exactly one entry and one exit and the worklist will only have the next block in it
    // after processing
    while (!worklist.empty()) {
        int block_id = worklist.pop();
        if (visited.has(block_id)) {
            continue;
        }

        visited.push(block_id);

        Block *block = find_block(block_id);
        if (block == nullptr) {
            break;
        }

        blocks_in_order.push(block_id);

        for (Set<int>::Element *E = block->forward_edges.front(); E; E = E->next()) {
            worklist.push(E->get());
        }
    }

    // Now build an index of the blocks and their starting offset in the bytecode
    for (int block_index = 0; block_index < blocks_in_order.size(); ++block_index) {
        int block_id = blocks_in_order[block_index];
        Block *block = find_block(block_id);
        block_ip_index[block_id] = ip;
        int start_block_ip = ip;

        for (int i = 0; i < block->instructions.size(); ++i) {
            Instruction& inst = block->instructions[i];
            
            if (inst.opcode == GDScriptFunction::Opcode::OPCODE_JUMP
                && block_index < blocks_in_order.size() - 1
                && blocks_in_order[block_index + 1] == inst.branch_ip) {

                // As an optimization, if this block ends in an unconditional
                // jump, AND the target of that jump is the next block to be
                // written to bytecode, then we can omit it.
                // Also no need to check the defarg jump table because those
                // blocks will not end in a branch (unless it's the last one,
                // then it's OK to change its size)
                
                inst.omit = true;
            } else {
                inst.omit = false;
                ip += inst.stride;
            }            
        }

        while (ip < block->force_code_size + start_block_ip) {
            ip += 1;
        }
    }

    // Now insert instructions. We replace branch targets with
    // ip looked up in branch_ip_index

    for (int block_index = 0; block_index < blocks_in_order.size(); ++block_index) {
        int block_id = blocks_in_order[block_index];

        Block *block = find_block(block_id);
        if (block == nullptr) {
            break;
        }

        int start_block_ip = bytecode.size();

        for (int i = 0; i < block->instructions.size(); ++i) {
            Instruction& inst = block->instructions[i];
            if (inst.omit) {
                continue;
            }
            
            bytecode.push(inst.opcode);

            switch (inst.opcode) {
                case GDScriptFunction::Opcode::OPCODE_OPERATOR:
                    bytecode.push(inst.variant_op);
                    bytecode.push(inst.source_address0);
                    bytecode.push(inst.source_address1);
                    bytecode.push(inst.target_address);
                    break;
                case GDScriptFunction::Opcode::OPCODE_EXTENDS_TEST:
                    bytecode.push(inst.source_address0);
                    bytecode.push(inst.source_address1);
                    bytecode.push(inst.target_address);
                    break;
                case GDScriptFunction::Opcode::OPCODE_IS_BUILTIN:
                    bytecode.push(inst.source_address0);
                    bytecode.push(inst.target_address);
                    bytecode.push(inst.type_arg);
                    break;
                case GDScriptFunction::Opcode::OPCODE_SET:
                    bytecode.push(inst.target_address);
                    bytecode.push(inst.index_address); // index
                    bytecode.push(inst.source_address0); // value
                    break;
                case GDScriptFunction::Opcode::OPCODE_GET:
                    // 1 before 0 intentional here
                    bytecode.push(inst.source_address0); // value
                    bytecode.push(inst.index_address); // index
                    bytecode.push(inst.target_address);
                    break;
                case GDScriptFunction::Opcode::OPCODE_SET_NAMED:
                    bytecode.push(inst.source_address0); // value
                    bytecode.push(inst.index_arg); // name index
                    bytecode.push(inst.target_address);
                    break;
                case GDScriptFunction::Opcode::OPCODE_GET_NAMED:
                    bytecode.push(inst.target_address);
                    bytecode.push(inst.index_arg); // name index
                    bytecode.push(inst.source_address0); // value
                    break;
                case GDScriptFunction::Opcode::OPCODE_SET_MEMBER:
                    bytecode.push(inst.index_arg); // name index
                    bytecode.push(inst.source_address0); // value
                    break;
                case GDScriptFunction::Opcode::OPCODE_GET_MEMBER:
                    bytecode.push(inst.index_arg); // name index
                    bytecode.push(inst.target_address); // value
                    break;
                case GDScriptFunction::Opcode::OPCODE_ASSIGN:
                    bytecode.push(inst.target_address);
                    bytecode.push(inst.source_address0); 
                    break;
                case GDScriptFunction::Opcode::OPCODE_ASSIGN_TRUE:
                    bytecode.push(inst.target_address);
                    break;
                case GDScriptFunction::Opcode::OPCODE_ASSIGN_FALSE:
                    bytecode.push(inst.target_address);
                    break;
                case GDScriptFunction::Opcode::OPCODE_ASSIGN_TYPED_BUILTIN:
                    bytecode.push(inst.type_arg);
                    bytecode.push(inst.target_address);
                    bytecode.push(inst.source_address0);
                    ip += 4;
                    break;
                case GDScriptFunction::Opcode::OPCODE_ASSIGN_TYPED_NATIVE:
                    bytecode.push(inst.source_address0); // type
                    bytecode.push(inst.target_address);
                    bytecode.push(inst.source_address1); // source
                    ip += 4;
                    break;
                case GDScriptFunction::Opcode::OPCODE_ASSIGN_TYPED_SCRIPT:
                    bytecode.push(inst.source_address0); // type
                    bytecode.push(inst.target_address);
                    bytecode.push(inst.source_address1); // source
                    break;
                case GDScriptFunction::Opcode::OPCODE_CAST_TO_BUILTIN:
                    bytecode.push(inst.type_arg); // variant::type
                    bytecode.push(inst.source_address0); // source
                    bytecode.push(inst.target_address);
                    break;
                case GDScriptFunction::Opcode::OPCODE_CAST_TO_NATIVE:
                    bytecode.push(inst.source_address0); // to type
                    bytecode.push(inst.source_address1); // source
                    bytecode.push(inst.target_address);
                    break;
                case GDScriptFunction::Opcode::OPCODE_CAST_TO_SCRIPT:
                    bytecode.push(inst.source_address0); // to type
                    bytecode.push(inst.source_address1); // source
                    bytecode.push(inst.target_address);
                    break;
                case GDScriptFunction::Opcode::OPCODE_CONSTRUCT:
                    bytecode.push(inst.type_arg);
                    bytecode.push(inst.vararg_count);
                    for (int i = 0; i < inst.vararg_count; ++i) {
                        bytecode.push(inst.varargs[i]);
                    }
                    bytecode.push(inst.target_address);
                    break;
                case GDScriptFunction::Opcode::OPCODE_CONSTRUCT_ARRAY:
                    bytecode.push(inst.vararg_count);
                    for (int i = 0; i < inst.vararg_count; ++i) {
                        bytecode.push(inst.varargs[i]);
                    }
                    bytecode.push(inst.target_address);
                    break;
                case GDScriptFunction::Opcode::OPCODE_CONSTRUCT_DICTIONARY:
                    bytecode.push(inst.vararg_count);
                    for (int i = 0; i < inst.vararg_count; ++i) {
                        bytecode.push(inst.varargs[i]);
                    }
                    bytecode.push(inst.target_address);
                    break;
                case GDScriptFunction::Opcode::OPCODE_CALL:
                case GDScriptFunction::Opcode::OPCODE_CALL_RETURN:
                    bytecode.push(inst.vararg_count);
                    bytecode.push(inst.source_address0);
                    bytecode.push(inst.index_arg);
                    for (int i = 0; i < inst.vararg_count; ++i) {
                        bytecode.push(inst.varargs[i]);
                    }
                    // target_address not used in CALL so it doesn't matter what value target_address has
                    // if op is call_return then target_address will be populated with a valid address
                    bytecode.push(inst.target_address);
                    break;
                case GDScriptFunction::Opcode::OPCODE_CALL_BUILT_IN:
                    bytecode.push(inst.index_arg); // function index
                    bytecode.push(inst.vararg_count);
                    for (int i = 0; i < inst.vararg_count; ++i) {
                        bytecode.push(inst.varargs[i]);
                    }
                    bytecode.push(inst.target_address);
                    break;
                case GDScriptFunction::Opcode::OPCODE_CALL_SELF:
                    // ?
                    break;
                case GDScriptFunction::Opcode::OPCODE_CALL_SELF_BASE:
                    bytecode.push(inst.index_arg);
                    bytecode.push(inst.vararg_count);
                    for (int i = 0; i < inst.vararg_count; ++i) {
                        bytecode.push(inst.varargs[i]);
                    }
                    bytecode.push(inst.target_address);
                    break;
                case GDScriptFunction::Opcode::OPCODE_YIELD:
                    break;
                case GDScriptFunction::Opcode::OPCODE_YIELD_SIGNAL:
                    bytecode.push(inst.source_address0);
                    bytecode.push(inst.index_arg);
                    break;
                case GDScriptFunction::Opcode::OPCODE_YIELD_RESUME:
                    bytecode.push(inst.target_address);
                    break;
                case GDScriptFunction::Opcode::OPCODE_JUMP:
                    bytecode.push(block_ip_index[inst.branch_ip]);
                    break;
                case GDScriptFunction::Opcode::OPCODE_JUMP_IF:
                    bytecode.push(inst.source_address0);
                    bytecode.push(block_ip_index[inst.branch_ip]);
                    break;
                case GDScriptFunction::Opcode::OPCODE_JUMP_IF_NOT:
                    bytecode.push(inst.source_address0);
                    bytecode.push(block_ip_index[inst.branch_ip]);
                    break;
                case GDScriptFunction::Opcode::OPCODE_JUMP_TO_DEF_ARGUMENT:
                    break;
                case GDScriptFunction::Opcode::OPCODE_ITERATE_BEGIN:
                    bytecode.push(inst.source_address0); // counter
                    bytecode.push(inst.source_address1); // container
                    bytecode.push(block_ip_index[inst.branch_ip]);
                    bytecode.push(inst.target_address); // iterator
                    break; 
                case GDScriptFunction::Opcode::OPCODE_ITERATE:
                    bytecode.push(inst.source_address0); // counter
                    bytecode.push(inst.source_address1); // container
                    bytecode.push(block_ip_index[inst.branch_ip]);
                    bytecode.push(inst.target_address); // iterator
                    break;          
                case GDScriptFunction::Opcode::OPCODE_ASSERT:
                    bytecode.push(inst.source_address0); // test
                    bytecode.push(inst.source_address1); // message
                    break;
                case GDScriptFunction::Opcode::OPCODE_BREAKPOINT:
                    break;
                case GDScriptFunction::Opcode::OPCODE_LINE:
                    bytecode.push(inst.index_arg);
                    break;
                case GDScriptFunction::Opcode::OPCODE_END:
                    break;
                case GDScriptFunction::Opcode::OPCODE_RETURN:
                    bytecode.push(inst.source_address0);
                    break;
                case GDScriptFunction::Opcode::OPCODE_BOX_INT:
                case GDScriptFunction::Opcode::OPCODE_BOX_REAL:
                case GDScriptFunction::Opcode::OPCODE_UNBOX_INT:
                case GDScriptFunction::Opcode::OPCODE_UNBOX_REAL:
                    bytecode.push(inst.source_address0);
                    bytecode.push(inst.target_address);
                    break;
                default:
                    ERR_FAIL_V_MSG(bytecode, "Invalid opcode");
                    break;
            }
        }

        // Pad out block with noops. These should not be executed
        while (bytecode.size() - start_block_ip < block->force_code_size) {
            bytecode.push(GDScriptFunction::Opcode::OPCODE_BREAKPOINT);
        }
    }

    return bytecode;
}

Block* ControlFlowGraph::find_block(const FastVector<Block> *blocks, int id) const {
    for (int i = 0; i < blocks->size(); ++i) {
        if ((*blocks)[i].id == id) {
            return blocks->address_of(i);
        }
    }

    return nullptr;
}

Block* ControlFlowGraph::find_block(int id) const {
    return find_block(&_blocks, id);
}

Block* ControlFlowGraph::get_entry_block() const {
    return find_block(_entry_id);
}

Block* ControlFlowGraph::get_exit_block() const {
    return find_block(_exit_id);
}

void ControlFlowGraph::debug_print() const {
    print_line("------ CFG -----------------------------");
    print_line("Name: " + _function->get_name());
    print_line("Blocks: " + itos(_blocks.size()));

    for (int block_index = 0; block_index < _blocks.size(); ++block_index) {
        Block *block = _blocks.address_of(block_index);
        if (block == nullptr) {
            break;
        }

        print_line("------ Block ------");
        print_line("id: " + itos(block->id));
        print_line("back edges: " + itos(block->back_edges.size()));
        print_line("forward edges: " + itos(block->forward_edges.size()));
        print_line("ins: " + itos(block->ins.size()));
        for (Set<int>::Element *E = block->ins.front(); E; E = E->next()) {
            print_line("    " + itos(E->get()));
        }
        print_line("outs: " + itos(block->outs.size()));
        for (Set<int>::Element *E = block->outs.front(); E; E = E->next()) {
            print_line("    " + itos(E->get()));
        }
        print_line("instructions: " + itos(block->instructions.size()));
        if (block->instructions.size() > 0) {
            for (int i = 0; i < block->instructions.size(); ++i) {
                print_line("    " + block->instructions[i].to_string());
            }
        } else {
            print_line("    none");
        }
        print_line("forward edges:");
        if (block->forward_edges.empty()) {
            print_line("    none");
        } else {
            for (Set<int>::Element *E = block->forward_edges.front(); E; E = E->next()) {
                print_line("    " + itos(E->get()));
            }
        }

        print_line("");
    }
}

void ControlFlowGraph::debug_print_instructions() const {
    print_line("------ Instructions ------");
    int ip = 0;
    for (int i = 0; i < _instructions.size(); ++i) {
        Instruction& inst = _instructions[i];
        print_line(itos(ip) + ": " + inst.to_string());
        ip += inst.stride;
    }
}