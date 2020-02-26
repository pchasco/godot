#include "core/os/memory.h"
#include "instruction.h"

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

String Instruction::arguments_to_string() const {
    String result;

    if ((defuse_mask & INSTRUCTION_DEFUSE_SOURCE0) != 0) {
        result += itos(source_address0);
    }
    if ((defuse_mask & INSTRUCTION_DEFUSE_SOURCE1) != 0) {
        result += ", " + itos(source_address1);
    }

    return result;
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
            
            return "OPERATOR " + itos(target_address) + " = (" + op + ", " + arguments_to_string() + ")";
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
            return "ASSIGN " + itos(target_address) + " = " + arguments_to_string();
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


Instruction Instruction::parse(const int* code, int index, const int buffer_size) {
   
#define BOUNDS_CHECK(need) ERR_FAIL_COND_V_MSG(index+need>=buffer_size, inst, "Error parsing bytecode") 
   
    GDScriptFunction::Opcode opcode = (GDScriptFunction::Opcode)code[index];
    Instruction inst;
    inst.opcode = opcode;
    int inst_start_ip = index;

    switch (opcode) {
        case GDScriptFunction::Opcode::OPCODE_OPERATOR:
            BOUNDS_CHECK(5);
            inst.variant_op = (Variant::Operator)code[index + 1];
            inst.source_address0 = code[index + 2];
            inst.source_address1 = code[index + 3];
            inst.target_address = code[index + 4];
            inst.defuse_mask = INSTRUCTION_DEFUSE_TARGET
                | INSTRUCTION_DEFUSE_SOURCE0
                | INSTRUCTION_DEFUSE_SOURCE1;
            index += 5;
            break;
        case GDScriptFunction::Opcode::OPCODE_EXTENDS_TEST:
            BOUNDS_CHECK(4);
            inst.source_address0 = code[index + 1]; // instance
            inst.source_address1 = code[index + 2]; // type
            inst.target_address = code[index + 3];
            inst.defuse_mask = INSTRUCTION_DEFUSE_TARGET
                | INSTRUCTION_DEFUSE_SOURCE0
                | INSTRUCTION_DEFUSE_SOURCE1;
            index += 4;
            break;
        case GDScriptFunction::Opcode::OPCODE_IS_BUILTIN:
            BOUNDS_CHECK(4);
            inst.source_address0 = code[index + 1];
            inst.target_address = code[index + 3];
            inst.type_arg = code[index + 2];
            inst.defuse_mask = INSTRUCTION_DEFUSE_TARGET
                | INSTRUCTION_DEFUSE_SOURCE0;
            index += 4;
            break;
        case GDScriptFunction::Opcode::OPCODE_SET:
            BOUNDS_CHECK(4);
            inst.target_address = code[index + 1];
            inst.index_address = code[index + 2]; // index
            inst.source_address0 = code[index + 3]; // value
            inst.defuse_mask = INSTRUCTION_DEFUSE_SOURCE0
                | INSTRUCTION_DEFUSE_INDEX
                | INSTRUCTION_DEFUSE_TARGET;
            index += 4;
            break;
        case GDScriptFunction::Opcode::OPCODE_GET:
            BOUNDS_CHECK(4);
            inst.source_address0 = code[index + 1]; // value
            inst.index_address = code[index + 2]; // index
            inst.target_address = code[index + 3];
            inst.defuse_mask = INSTRUCTION_DEFUSE_SOURCE0
                | INSTRUCTION_DEFUSE_INDEX
                | INSTRUCTION_DEFUSE_TARGET;
            index += 4;
            break;
        case GDScriptFunction::Opcode::OPCODE_SET_NAMED:
            BOUNDS_CHECK(4);
            inst.source_address0 = code[index + 1]; // value
            inst.index_arg = code[index + 2]; // name index
            inst.target_address = code[index + 3];
            inst.defuse_mask = INSTRUCTION_DEFUSE_SOURCE0
                | INSTRUCTION_DEFUSE_TARGET;
            index += 4;
            break;
        case GDScriptFunction::Opcode::OPCODE_GET_NAMED:
            BOUNDS_CHECK(4);
            inst.target_address = code[index + 1];
            inst.source_address0 = code[index + 3]; // value
            inst.defuse_mask = INSTRUCTION_DEFUSE_SOURCE0
                | INSTRUCTION_DEFUSE_TARGET;
            index += 4;
            break;
        case GDScriptFunction::Opcode::OPCODE_SET_MEMBER:
            BOUNDS_CHECK(3);
            inst.index_arg = code[index + 1]; // name index
            inst.source_address0 = code[index + 2]; // value
            inst.defuse_mask = INSTRUCTION_DEFUSE_SOURCE0
                | INSTRUCTION_DEFUSE_SELF;
            index += 3;
            break;
        case GDScriptFunction::Opcode::OPCODE_GET_MEMBER:
            BOUNDS_CHECK(3);
            inst.index_arg = code[index + 1]; // name index
            inst.target_address = code[index + 2]; // value
            inst.defuse_mask = INSTRUCTION_DEFUSE_TARGET
                | INSTRUCTION_DEFUSE_SELF;
            index += 3;
            break;
        case GDScriptFunction::Opcode::OPCODE_ASSIGN:
            BOUNDS_CHECK(3);
            inst.target_address = code[index + 1];
            inst.source_address0 = code[index + 2]; 
            inst.defuse_mask = INSTRUCTION_DEFUSE_SOURCE0
                | INSTRUCTION_DEFUSE_TARGET;
            index += 3;
            break;
        case GDScriptFunction::Opcode::OPCODE_ASSIGN_TRUE:
            BOUNDS_CHECK(2);
            inst.target_address = code[index + 1];
            inst.defuse_mask = INSTRUCTION_DEFUSE_TARGET;
            index += 2;
            break;
        case GDScriptFunction::Opcode::OPCODE_ASSIGN_FALSE:
            BOUNDS_CHECK(2);
            inst.target_address = code[index + 1];
            inst.defuse_mask = INSTRUCTION_DEFUSE_TARGET;
            index += 2;
            break;
        case GDScriptFunction::Opcode::OPCODE_ASSIGN_TYPED_BUILTIN:
            BOUNDS_CHECK(4);
            inst.type_arg = code[index + 1];
            inst.target_address = code[index + 2];
            inst.source_address0 = code[index + 3];
            inst.defuse_mask = INSTRUCTION_DEFUSE_SOURCE0
                | INSTRUCTION_DEFUSE_TARGET;
            index += 4;
            break;
        case GDScriptFunction::Opcode::OPCODE_ASSIGN_TYPED_NATIVE:
            BOUNDS_CHECK(4);
            inst.source_address0 = code[index + 1]; // type
            inst.target_address = code[index + 2];
            inst.source_address1 = code[index + 3]; // source
            inst.defuse_mask = INSTRUCTION_DEFUSE_SOURCE0
                | INSTRUCTION_DEFUSE_SOURCE1
                | INSTRUCTION_DEFUSE_TARGET;
            index += 4;
            break;
        case GDScriptFunction::Opcode::OPCODE_ASSIGN_TYPED_SCRIPT:
            BOUNDS_CHECK(4);
            inst.source_address0 = code[index + 1]; // type
            inst.target_address = code[index + 2];
            inst.source_address1 = code[index + 3]; // source
            inst.defuse_mask = INSTRUCTION_DEFUSE_SOURCE0
                | INSTRUCTION_DEFUSE_SOURCE1
                | INSTRUCTION_DEFUSE_TARGET;
            index += 4;
            break;
        case GDScriptFunction::Opcode::OPCODE_CAST_TO_BUILTIN:
            BOUNDS_CHECK(4);
            inst.type_arg = code[index + 1]; // variant::type
            inst.source_address0 = code[index + 2]; // source
            inst.target_address = code[index + 3];
            inst.defuse_mask = INSTRUCTION_DEFUSE_SOURCE0
                | INSTRUCTION_DEFUSE_TARGET;
            index += 4;
            break;
        case GDScriptFunction::Opcode::OPCODE_CAST_TO_NATIVE:
            BOUNDS_CHECK(4);
            inst.source_address0 = code[index + 1]; // to type
            inst.source_address1 = code[index + 2]; // source
            inst.target_address = code[index + 3];
            inst.defuse_mask = INSTRUCTION_DEFUSE_SOURCE0
                | INSTRUCTION_DEFUSE_SOURCE1
                | INSTRUCTION_DEFUSE_TARGET;
            index += 4;
            break;
        case GDScriptFunction::Opcode::OPCODE_CAST_TO_SCRIPT:
            BOUNDS_CHECK(4);
            inst.source_address0 = code[index + 1]; // to type
            inst.source_address1 = code[index + 2]; // source
            inst.target_address = code[index + 3];
            inst.defuse_mask = INSTRUCTION_DEFUSE_SOURCE0
                | INSTRUCTION_DEFUSE_SOURCE1
                | INSTRUCTION_DEFUSE_TARGET;
            index += 4;
            break;
        case GDScriptFunction::Opcode::OPCODE_CONSTRUCT:
            BOUNDS_CHECK(2);
            inst.type_arg = code[index + 1];
            inst.vararg_count = code[index + 2];
            BOUNDS_CHECK(4 + inst.vararg_count);
            for (int i = 0; i < inst.vararg_count; ++i) {
                inst.varargs.push(code[index + 3 + i]);
            }
            inst.target_address = code[index + 3 + inst.vararg_count];
            inst.defuse_mask = INSTRUCTION_DEFUSE_VARARGS
                | INSTRUCTION_DEFUSE_TARGET;
            index += 4 + inst.vararg_count;
            break;
        case GDScriptFunction::Opcode::OPCODE_CONSTRUCT_ARRAY:
            BOUNDS_CHECK(1);
            inst.vararg_count = code[index + 1];
            BOUNDS_CHECK(3 + inst.vararg_count);
            for (int i = 0; i < inst.vararg_count; ++i) {
                inst.varargs.push(code[index + 2 + i]);
            }
            inst.target_address = code[index + 2 + inst.vararg_count];
            inst.defuse_mask = INSTRUCTION_DEFUSE_VARARGS
                | INSTRUCTION_DEFUSE_TARGET;
            index += 3 + inst.vararg_count;
            break;
        case GDScriptFunction::Opcode::OPCODE_CONSTRUCT_DICTIONARY:
            BOUNDS_CHECK(1);
            inst.vararg_count = code[index + 1];
            BOUNDS_CHECK(inst.vararg_count + 3);
            for (int i = 0; i < inst.vararg_count; ++i) {
                inst.varargs.push(code[index + 2 + i]);
            }
            inst.target_address = code[index + 2 + (inst.vararg_count * 2)];
            inst.defuse_mask = INSTRUCTION_DEFUSE_VARARGS
                | INSTRUCTION_DEFUSE_TARGET;
            index += 3 + inst.vararg_count;
            break;
        case GDScriptFunction::Opcode::OPCODE_CALL:
        case GDScriptFunction::Opcode::OPCODE_CALL_RETURN:
            BOUNDS_CHECK(1);
            inst.vararg_count = code[index + 1];
            BOUNDS_CHECK(5 + inst.vararg_count);
            inst.source_address0 = code[index + 2];
            inst.index_arg = code[index + 3];
            for (int i = 0; i < inst.vararg_count; ++i) {
                inst.varargs.push(code[index + 4 + i]);
            }
            // If this is CALL then whatever value is here will be ignored in execution
            // If this is CALL_RETURN then it will be the location to store result
            inst.target_address = code[index + 4 + inst.vararg_count];
            inst.defuse_mask = INSTRUCTION_DEFUSE_VARARGS
                | INSTRUCTION_DEFUSE_SOURCE0;
            if (inst.opcode == GDScriptFunction::Opcode::OPCODE_CALL_RETURN) {
                inst.defuse_mask |= INSTRUCTION_DEFUSE_TARGET;
            }
            index += 5 + inst.vararg_count;
            break;
        case GDScriptFunction::Opcode::OPCODE_CALL_BUILT_IN:
            BOUNDS_CHECK(2);
            inst.index_arg = code[index + 1]; // function index
            inst.vararg_count = code[index + 2];
            BOUNDS_CHECK(4 + inst.vararg_count);
            for (int i = 0; i < inst.vararg_count; ++i) {
                inst.varargs.push(code[index + 3 + i]);
            }
            inst.target_address = code[index + 3 + inst.vararg_count];
            inst.defuse_mask = INSTRUCTION_DEFUSE_VARARGS
                | INSTRUCTION_DEFUSE_TARGET;
            index += 4 + inst.vararg_count;
            break;
        case GDScriptFunction::Opcode::OPCODE_CALL_SELF:
            BOUNDS_CHECK(1);
            // ?
            index += 1;
            break;
        case GDScriptFunction::Opcode::OPCODE_CALL_SELF_BASE:
            BOUNDS_CHECK(2);
            inst.index_arg = code[index + 1];
            inst.vararg_count = code[index + 2];
            BOUNDS_CHECK(4 + inst.vararg_count);
            for (int i = 0; i < inst.vararg_count; ++i) {
                inst.varargs.push(code[index + 3 + i]);
            }
            inst.target_address = code[index + 3 + inst.vararg_count + 1];
            inst.defuse_mask = INSTRUCTION_DEFUSE_VARARGS
                | INSTRUCTION_DEFUSE_TARGET
                | INSTRUCTION_DEFUSE_SELF;
            index += 4 + inst.vararg_count;
            break;
        case GDScriptFunction::Opcode::OPCODE_YIELD:
            BOUNDS_CHECK(2);
            index += 2;
            break;
        case GDScriptFunction::Opcode::OPCODE_YIELD_SIGNAL:
            BOUNDS_CHECK(3);
            inst.source_address0 = code[index + 1];
            inst.index_arg = code[index + 2];
            inst.defuse_mask = INSTRUCTION_DEFUSE_SOURCE0;
            index += 3;
            break;
        case GDScriptFunction::Opcode::OPCODE_YIELD_RESUME:
            BOUNDS_CHECK(2);
            inst.target_address = code[index + 1];
            inst.defuse_mask = INSTRUCTION_DEFUSE_TARGET;
            index += 2;
            break;
        case GDScriptFunction::Opcode::OPCODE_JUMP:
            BOUNDS_CHECK(2);
            inst.branch_ip = code[index + 1];
            index += 2;
            break;
        case GDScriptFunction::Opcode::OPCODE_JUMP_IF:
            BOUNDS_CHECK(3);
            inst.source_address0 = code[index + 1];
            inst.branch_ip = code[index + 2];
            inst.defuse_mask = INSTRUCTION_DEFUSE_SOURCE0;
            index += 3;
            break;
        case GDScriptFunction::Opcode::OPCODE_JUMP_IF_NOT:
            BOUNDS_CHECK(3);
            inst.source_address0 = code[index + 1];
            inst.branch_ip = code[index + 2];
            inst.defuse_mask = INSTRUCTION_DEFUSE_SOURCE0;
            index += 3;
            break;
        case GDScriptFunction::Opcode::OPCODE_JUMP_TO_DEF_ARGUMENT:
            BOUNDS_CHECK(1);
            index += 1;
            break;
        case GDScriptFunction::Opcode::OPCODE_ITERATE_BEGIN:
            BOUNDS_CHECK(5);
            inst.source_address0 = code[index + 1]; // counter
            inst.source_address1 = code[index + 2]; // container
            inst.branch_ip = code[index + 3];
            inst.target_address = code[index + 4]; // iterator
            inst.defuse_mask = INSTRUCTION_DEFUSE_SOURCE0
                | INSTRUCTION_DEFUSE_SOURCE1
                | INSTRUCTION_DEFUSE_TARGET;
            index += 5;
            break; 
        case GDScriptFunction::Opcode::OPCODE_ITERATE:
            BOUNDS_CHECK(5);
            inst.source_address0 = code[index + 1]; // counter
            inst.source_address1 = code[index + 2]; // container
            inst.branch_ip = code[index + 3];
            inst.target_address = code[index + 4]; // iterator
            inst.defuse_mask = INSTRUCTION_DEFUSE_SOURCE0
                | INSTRUCTION_DEFUSE_SOURCE1
                | INSTRUCTION_DEFUSE_TARGET;
            index += 5;
            break;          
        case GDScriptFunction::Opcode::OPCODE_ASSERT:
            BOUNDS_CHECK(3);
            inst.source_address0 = code[index + 1]; // test
            inst.source_address1 = code[index + 2]; // message
            inst.defuse_mask = INSTRUCTION_DEFUSE_SOURCE0
                | INSTRUCTION_DEFUSE_SOURCE1;
            index += 3;
            break;
        case GDScriptFunction::Opcode::OPCODE_BREAKPOINT:
            BOUNDS_CHECK(2);
            index += 1;
            break;
        case GDScriptFunction::Opcode::OPCODE_LINE:
            BOUNDS_CHECK(1);
            inst.index_arg = code[index + 1];
            index += 2;
            break;
        case GDScriptFunction::Opcode::OPCODE_END:
            BOUNDS_CHECK(0);
            index += 1;
            break;
        case GDScriptFunction::Opcode::OPCODE_RETURN:
            BOUNDS_CHECK(2);
            inst.source_address0 = code[index + 1];
            inst.defuse_mask = INSTRUCTION_DEFUSE_SOURCE0;
            index += 2;
            break;
        case GDScriptFunction::Opcode::OPCODE_BOX_INT:
            BOUNDS_CHECK(3);
            inst.source_address0 = code[index + 1];
            inst.target_address = code[index + 2];
            inst.defuse_mask = INSTRUCTION_DEFUSE_SOURCE0 
                | INSTRUCTION_DEFUSE_TARGET;
            index += 3;        
            break;
        case GDScriptFunction::Opcode::OPCODE_BOX_REAL:
            BOUNDS_CHECK(3);
            inst.source_address0 = code[index + 1];
            inst.target_address = code[index + 2];
            inst.defuse_mask = INSTRUCTION_DEFUSE_SOURCE0 
                | INSTRUCTION_DEFUSE_TARGET;
            index += 3;        
            break;
        case GDScriptFunction::Opcode::OPCODE_UNBOX_INT:
            BOUNDS_CHECK(3);
            inst.source_address0 = code[index + 1];
            inst.target_address = code[index + 2];
            inst.defuse_mask = INSTRUCTION_DEFUSE_SOURCE0 
                | INSTRUCTION_DEFUSE_TARGET;
            index += 3;        
            break;
        case GDScriptFunction::Opcode::OPCODE_UNBOX_REAL:
            BOUNDS_CHECK(3);
            inst.source_address0 = code[index + 1];
            inst.target_address = code[index + 2];
            inst.defuse_mask = INSTRUCTION_DEFUSE_SOURCE0 
                | INSTRUCTION_DEFUSE_TARGET;
            index += 3;        
            break;
        
        default:
            ERR_FAIL_V_MSG(inst, "Invalid opcode");
            index = 999999999;
            break;
    }

    inst.stride = index - inst_start_ip;

    return inst;
}