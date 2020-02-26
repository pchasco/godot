#include "core/os/memory.h"
//#include "core/error_macros.h"
#include "gdscript_optimizer.h"
#include "modules/gdscript/gdscript_functions.h"

GDScriptFunctionOptimizer::GDScriptFunctionOptimizer(GDScriptFunction *function) {
    _function = function;
    _cfg = nullptr;
}

void GDScriptFunctionOptimizer::begin() {
    if (_cfg != nullptr) {
        delete _cfg;
        _cfg = nullptr;
    }

    _cfg = new ControlFlowGraph(_function);
    _cfg->construct();
}

void GDScriptFunctionOptimizer::commit() {
    FastVector<int> bytecode = _cfg->get_bytecode();
    _function->code.clear();

	if (bytecode.size() > 0) {
        Vector<int> code;
        for (int i = 0; i < bytecode.size(); ++i) {
            code.push_back(bytecode[i]);
        }

		_function->code = code;
		_function->_code_ptr = &code[0];
		_function->_code_size = code.size();
	} else {
		_function->_code_ptr = NULL;
		_function->_code_size = 0;
	}

    delete _cfg;
    _cfg = nullptr;
}

void GDScriptFunctionOptimizer::pass_strip_debug() {
    ERR_FAIL_COND_MSG(_cfg == nullptr, "ControlFlowGraph not initialized");
    Block *entry_block = _cfg->get_entry_block();
    ERR_FAIL_COND_MSG(_cfg == nullptr, "ControlFlowGraph contains no blocks");
    Block *exit_block = _cfg->get_exit_block();

    FastVector<int> worklist;
    FastVector<int> visited;
 
    // Don't mess with default argument assignment blocks
    FastVector<int> defarg_jumps = get_function_default_argument_jump_table(_function);
    // Although we can change the size of the last of the defarg assignment blocks
    if (!defarg_jumps.empty()) {
        defarg_jumps.pop();
    }
    for (int i = 0; i < defarg_jumps.size(); ++i) {
        visited.push(defarg_jumps[i]);
    }

    worklist.push(entry_block->id);

    while(!worklist.empty()) {
        int block_id = worklist.pop();
        if (visited.has(block_id)) {
            continue;
        }

        visited.push(block_id);

        Block *block = _cfg->find_block(block_id);

        FastVector<Instruction> keep_instructions;

        for (int i = 0; i < block->instructions.size(); ++i) {
            Instruction& inst = block->instructions[i];
            switch (inst.opcode) {
                case GDScriptFunction::Opcode::OPCODE_LINE:
                case GDScriptFunction::Opcode::OPCODE_BREAKPOINT:
                    // Do nothing
                    break;
                default:
                    keep_instructions.push(inst);
                    break;
            }
        }

        block->instructions.clear();
        block->instructions.push_many(keep_instructions.size(), keep_instructions.ptr());

        worklist.push_many(block->forward_edges);
    }
}

void GDScriptFunctionOptimizer::pass_dead_block_elimination() {
    ERR_FAIL_COND_MSG(_cfg == nullptr, "ControlFlowGraph not initialized");
    _cfg->remove_dead_blocks();
}

void GDScriptFunctionOptimizer::pass_jump_threading() {
    ERR_FAIL_COND_MSG(_cfg == nullptr, "ControlFlowGraph not initialized");

    Block *entry_block = _cfg->get_entry_block();
    ERR_FAIL_COND_MSG(_cfg == nullptr, "ControlFlowGraph contains no blocks");

    Block *exit_block = _cfg->get_exit_block();
    FastVector<int> worklist;
    FastVector<int> visited;
    FastVector<int> blocks_to_remove;
    FastVector<int> defarg_jumps = get_function_default_argument_jump_table(_function);

    worklist.push(entry_block->id);

    while(!worklist.empty()) {
        int block_id = worklist.pop();
        if (visited.has(block_id)) {
            continue;
        }

        visited.push(block_id);

        Block *block = _cfg->find_block(block_id);

        bool contains_only_jump = true;
        int jump_count = 0;

        for (int i = 0; i < block->instructions.size(); ++i) {
            Instruction& inst = block->instructions[i];
            switch (inst.opcode) {
                case GDScriptFunction::Opcode::OPCODE_LINE:
                case GDScriptFunction::Opcode::OPCODE_BREAKPOINT:
                    // Do nothing
                    break;
                case GDScriptFunction::Opcode::OPCODE_JUMP:
                    jump_count += 1;
                    break;
                default:
                    contains_only_jump = false;
                    break;
            }

            if (jump_count > 1 || !contains_only_jump) {
                break;
            }
        }

        // If this block only contains a jump then we can
        // remove it if it is not in the default argument jump table (could happen after dead code elimination)
        if (contains_only_jump 
            && jump_count == 1 
            && block->id != entry_block->id
            && block->id != exit_block->id
            && !defarg_jumps.has(block->id)) {
            blocks_to_remove.push(block->id);
        }

        worklist.push_many(block->forward_edges);
    }

    for (int i = 0; i < blocks_to_remove.size(); ++i) {
        Block *block = _cfg->find_block(blocks_to_remove[i]);
        // We know there is only one succ block
        Block *succ = _cfg->find_block(block->forward_edges[0]);

        // For all blocks jumping to this block, just jump to succ block instead
        for (Set<int>::Element *E = block->back_edges.front(); E; E = E->next()) {
            Block *pred = _cfg->find_block(E->get());
            pred->replace_jumps(*block, *succ);
        }

        // We don't remove the block from the cfg here. Dead block
        // removal pass will take care of these.
    }
}

int find_alias_index(int *reg_alias, int address) {
    for (int i = 0; i < GDSCRIPT_FUNCTION_REGISTER_COUNT; ++i) {
        if (reg_alias[i] == address) {
            return i;
        }
    }

    return -1;
}

#ifdef NOPE
GDScriptFunction::Opcode get_typed_opcode(Variant::Operator op, Variant::Type type) const {

    switch (type) {
        case Variant::Type::INT:
            switch (operator) {
                case Variant::Operator::OP_ADD:
                    return GDScriptFunction::Opcode::OPCODE_OPERATOR_ADD_INT;
                case Variant::Operator::OP_SUBTRACT:
                    return GDScriptFunction::Opcode::OPCODE_OPERATOR_SUB_INT;
                case Variant::Operator::OP_MULTIPLY:
                    return GDScriptFunction::Opcode::OPCODE_OPERATOR_MUL_INT;
                case Variant::Operator::OP_DIVIDE:
                    return GDScriptFunction::Opcode::OPCODE_OPERATOR_DIV_INT;
                case Variant::Operator::OP_NEGATE:
                    return GDScriptFunction::Opcode::OPCODE_OPERATOR_NEG_INT;
                case Variant::Operator::OP_POSITIVE:
                    return GDScriptFunction::Opcode::OPCODE_OPERATOR_POS_INT;
                case Variant::Operator::OP_MODULE:
                    return GDScriptFunction::Opcode::OPCODE_OPERATOR_MOD_INT;
            }
        case Variant::Type::REAL:
            switch (operator) {
                case Variant::Operator::OP_ADD:
                    return GDScriptFunction::Opcode::OPCODE_OPERATOR_ADD_REAL;
                case Variant::Operator::OP_SUBTRACT:
                    return GDScriptFunction::Opcode::OPCODE_OPERATOR_SUB_REAL;
                case Variant::Operator::OP_MULTIPLY:
                    return GDScriptFunction::Opcode::OPCODE_OPERATOR_MUL_REAL;
                case Variant::Operator::OP_DIVIDE:
                    return GDScriptFunction::Opcode::OPCODE_OPERATOR_DIV_REAL;
                case Variant::Operator::OP_NEGATE:
                    return GDScriptFunction::Opcode::OPCODE_OPERATOR_NEG_REAL;
                case Variant::Operator::OP_POSITIVE:
                    return GDScriptFunction::Opcode::OPCODE_OPERATOR_POS_REAL;
                case Variant::Operator::OP_MODULE:
                    return GDScriptFunction::Opcode::OPCODE_OPERATOR_MOD_REAL;
            }
    }

    return GDScriptFunction::Opcode::OPCODE_OPERATOR;
}
#endif


class AvailableExpression {
public:
    AvailableExpression() : removed(false) {}

    OpExpression expression;
    int destination;
    bool removed;

    bool uses_any(int target_address, int address0, int address1) {
        if (target_address == destination) {
            return true;
        }

        return expression.uses(address0)
            || expression.uses(address1);
    }
};

void GDScriptFunctionOptimizer::pass_local_common_subexpression_elimination() {

}

void GDScriptFunctionOptimizer::pass_global_common_subexpression_elimination() {
}

void GDScriptFunctionOptimizer::pass_register_allocation() {
#ifdef NOPE

    // 1. For each basic block
    // 1. Find all assignments, call_return, call_built_in whose result is a guaranteed
    // known type.
    // 2. For each assignment, insert an unbox operation into a free register. This
    // register is now an alias for the original value
    // 3. 

    _cfg->analyze_data_flow();

    for (int i = 0; i < _cfg->get_block_count(); ++i) {
        Block *block = _cfg->get_block(i);
        FastVector<Instruction> new_instructions;
        FastVector<int> free_int_regs;
        FastVector<int> free_real_regs;
        int real_reg_alias[GDSCRIPT_FUNCTION_REGISTER_COUNT];
        int int_reg_alias[GDSCRIPT_FUNCTION_REGISTER_COUNT];

        for (int j = 0; j < GDSCRIPT_FUNCTION_REGISTER_COUNT; ++j) {
            free_real_regs.push(j);
            free_int_regs.push(j);
            real_reg_alias[j] = 0;
            int_reg_alias[j] = 0;
        }

        for (int j = 0; j < block->instructions.size(); ++j) {
            Instruction& inst = block->instructions[j];
            new_instructions.push(inst);

            // If this instruction assigns a guaranteed type value to something,
            // we can cache the value in a register for possible use later.
            // Unused assignments will be removed in another pass
            switch (inst.opcode) {
                case GDScriptFunction::Opcode::OPCODE_CALL_BUILT_IN:
                    MethodInfo info = GDScriptFunctions::get_info((GDScriptFunctions::Function)inst.index_arg);
                    switch (info.return_val.type) {
                        case Variant::Type::INT:
                            if (free_int_regs.size()) {
                                // Select free register for value
                                int reg_index = free_int_regs.pop();
                                // create int alias
                                int_reg_alias[reg_index] = inst.target_address;

                                Instruction box;
                                box.opcode = GDScriptFunction::Opcode::OPCODE_BOX_INT;
                                box.target_address = (GDScriptFunction::Address::ADDR_TYPE_INT_REG << GDScriptFunction::Address::ADDR_BITS) | reg_index;
                                box.source_address0 = inst.target_address;
                                box.stride = 3;
                                box.defuse_mask = INSTRUCTION_DEFUSE_TARGET
                                    | INSTRUCTION_DEFUSE_SOURCE0;

                                new_instructions.push(box);
                            }
                            break;
                        case Variant::Type::REAL:
                            if (free_real_regs.size()) {
                                // create real alias
                                int reg_index = free_real_regs.pop();
                                float_reg_alias[reg_index] = inst.target_address;

                                Instruction box;
                                box.opcode = GDScriptFunction::Opcode::OPCODE_BOX_REAL;
                                box.target_address = (GDScriptFunction::Address::ADDR_TYPE_REAL_REG << GDScriptFunction::Address::ADDR_BITS) | reg_index;
                                box.source_address0 = inst.target_address;
                                box.stride = 3;
                                box.defuse_mask = INSTRUCTION_DEFUSE_TARGET
                                    | INSTRUCTION_DEFUSE_SOURCE0;

                                new_instructions.push(box);
                            }
                            break;
                        default:
                            // do nothing
                            break;
                    }
                    break;

                case GDScriptFunction::Opcode::OPCODE_OPERATOR:
                    bool is_operator_qualified = false;
                    bool is_unary_operator = false;
                    switch (inst.variant_op) {
                        case Variant::Operator::OP_NEGATE:
                        case Variant::Operator::OP_POSITIVE:
                            is_unary_operator = true;
                        case Variant::Operator::OP_ADD:
                        case Variant::Operator::OP_SUBTRACT:
                        case Variant::Operator::OP_MULTIPLY:
                        case Variant::Operator::OP_DIVIDE:
                        case Variant::Operator::OP_MODULE: 
                            is_operator_qualified = true;
                            break;
                    }
                    
                    // This is an operation we can make typed
                    if (is_operator_qualified) {
                        if (is_unary_operator) {
                            // blah
                        } else {
                            int *reg_sets[] = { int_reg_alias, real_reg_alias };
                            FastVector<int> *free_regs[] = { &free_int_regs, &free_real_regs };

                            for (int ri = 0; ri < 2; ++ri) {
                                // Do we have typed aliases for both operands?
                                int reg0 = find_reg_alias(reg_sets[ri], inst.source_address0);
                                int reg1 = find_reg_alias(reg_sets[ri], inst.source_address1);
                                if (reg0 >= 0 && reg1 >= 0) {
                                    int reg_target = find_reg_alias(reg_sets[ri], inst.target_address);
                                    if (reg_target < 0 && reg_sets[ri]->size()) {
                                        reg_target = reg_sets[ri]->pop();
                                    }
                                    if (reg_target >= 0) {
                                        // This is an operation against two ints
                                        Instruction op;
                                        op.opcode = get_typed_opcode(inst.variant_op, Variant::Type::INT);
                                        op.target_address = (GDScriptFunction::Address::ADDR_TYPE_INT_REG << GDScriptFunction::Address::ADDR_BITS) | reg_target;
                                        op.source_address0 = (GDScriptFunction::Address::ADDR_TYPE_INT_REG << GDScriptFunction::Address::ADDR_BITS) | reg0;
                                        op.source_address1 = (GDScriptFunction::Address::ADDR_TYPE_INT_REG << GDScriptFunction::Address::ADDR_BITS) | reg1;
                                        op.stride = 4;
                                        op.defuse_mask = INSTRUCTION_DEFUSE_TARGET
                                            | INSTRUCTION_DEFUSE_SOURCE0
                                            | INSTRUCTION_DEFUSE_SOURCE1;
                                        new_instructions.push(op);
                                    }
                                }
                            }
                        }
                    }
                    break;
            
                    // Do nothing
                    break;

                default:
                    // Do nothing
                    break;
            }
        }

        // Replace block instructions with new list of instructions.
        // This technically stales the data flow facts we collected but
        // we don't need them since we are only operating within blocks
        block->instructions = new_instructions;
    }
#endif
}