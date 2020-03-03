#include "core/os/memory.h"
//#include "core/error_macros.h"
#include "gdscript_optimizer.h"
#include "modules/gdscript/gdscript_functions.h"

GDScriptFunctionOptimizer::GDScriptFunctionOptimizer(GDScriptFunction *function) {
    _function = function;
    _cfg = nullptr;
    _is_data_flow_dirty = false;
}

void GDScriptFunctionOptimizer::begin() {
    if (_cfg != nullptr) {
        delete _cfg;
        _cfg = nullptr;
    }

    _cfg = new ControlFlowGraph(_function);
    _cfg->construct();

    invalidate_data_flow();
}

void GDScriptFunctionOptimizer::commit() {
    FastVector<int> bytecode = _cfg->assemble();
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
    _is_data_flow_dirty = true;
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

    bool made_changes = true;
    while (made_changes) {
        made_changes = false;

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

            // If this block only contains a jump then we can
            // remove it if it is not in the default argument jump table (could happen after dead code elimination)
            if (block->instructions.size() == 0
            && block->block_type == Block::Type::NORMAL
            && block->id != entry_block->id
            && block->id != exit_block->id
            && !defarg_jumps.has(block->id)) {
                blocks_to_remove.push(block->id);
                made_changes = true;
            }

            // If a conditional branch block targets the same address
            // on all its edges we can replace it with a non-conditional
            // block
            if (block->block_type == Block::Type::BRANCH_IF_NOT
            && block->forward_edges[0] == block->forward_edges[1]) {
                block->block_type = Block::Type::NORMAL;
                made_changes = true;
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

        if (made_changes) {
            invalidate_data_flow();
        }
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
};

void GDScriptFunctionOptimizer::pass_available_expression_analysis() {

}

AvailableExpression* find_available_expression(const FastVector<AvailableExpression>& availables, const OpExpression& expression) {
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

AvailableExpression* find_available_expression_by_target(const FastVector<AvailableExpression>& availables, int target_address) {
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

void GDScriptFunctionOptimizer::pass_dead_assignment_elimination() {
    ERR_FAIL_COND_MSG(_cfg == nullptr, "ControlFlowGraph not initialized");
    need_data_flow();

    Block *entry_block = _cfg->get_entry_block();
    ERR_FAIL_COND_MSG(_cfg == nullptr, "ControlFlowGraph contains no blocks");

    Block *exit_block = _cfg->get_exit_block();

    bool made_changes = true;
    while (made_changes) {
        made_changes = false;

        FastVector<int> worklist;
        FastVector<int> visited;
        worklist.push(exit_block->id);

        while(!worklist.empty()) {
            int block_id = worklist.pop();
            if (visited.has(block_id)) {
                continue;
            }

            visited.push(block_id);

            Block *block = _cfg->find_block(block_id);
            ERR_FAIL_COND_MSG(block == nullptr, "ControlFlowGraph cannot find block");

            // Instructions that will remain after LCSE
            FastVector<Instruction> keep;

            // Prime uses with the block out values gathered by data flow analysis
            Set<int> uses;
            for (Set<int>::Element *E = block->outs.front(); E; E = E->next()) {
                uses.insert(E->get());
            }

            // Insert any condition variable used by the block type,
            // which will be used in the last instruction in the basic block
            switch (block->block_type) {
                case Block::Type::BRANCH_IF_NOT:
                    uses.insert(block->jump_condition_address);
                    break;
            }

            // Iterate over instructions in reverse. If value is defined but
            // not in uses set then the assignment is dead
            for (int i = block->instructions.size() - 1; i >= 0; --i) {
                Instruction& inst = block->instructions[i];
                bool will_keep = true;

                if ((inst.defuse_mask & INSTRUCTION_DEFUSE_TARGET) != 0) {
                    if (!inst.may_have_side_effects() && !uses.has(inst.target_address)) {
                        will_keep = false;
                        made_changes = true;
                    }
                }

                if (will_keep) {                
                    if ((inst.defuse_mask & INSTRUCTION_DEFUSE_TARGET) != 0) {
                        uses.erase(inst.target_address);
                    }

                    if ((inst.defuse_mask & INSTRUCTION_DEFUSE_SOURCE0) != 0) {
                        uses.insert(inst.source_address0);
                    }
                    if ((inst.defuse_mask & INSTRUCTION_DEFUSE_SOURCE1) != 0) {
                        uses.insert(inst.source_address1);
                    }
                    if ((inst.defuse_mask & INSTRUCTION_DEFUSE_VARARGS) != 0) {
                        for (int j = 0; j < inst.varargs.size(); ++j) {
                            uses.insert(inst.varargs[j]);
                        }
                    }

                    keep.push(inst);
                }
            }

            // Replace block instructions with keep. We iterated over
            // instructions in reverse order, so we need to push them
            // in reverse order also.
            block->instructions.clear();
            for (int i = keep.size() - 1; i >= 0; --i) {
                block->instructions.push(keep[i]);
            }

            for (Set<int>::Element *E = block->back_edges.front(); E; E = E->next()) {
                worklist.push(E->get());
            }
        }

        if (made_changes) {
            invalidate_data_flow();
        }
    }
}

void GDScriptFunctionOptimizer::pass_local_common_subexpression_elimination() {
    ERR_FAIL_COND_MSG(_cfg == nullptr, "ControlFlowGraph not initialized");
    
    // We need up-to-date data flow for this pass,
    // but this pass will not alter data flow so we will
    // not be invalidating it before exit
    need_data_flow();

    Block *entry_block = _cfg->get_entry_block();
    ERR_FAIL_COND_MSG(_cfg == nullptr, "ControlFlowGraph contains no blocks");

    Block *exit_block = _cfg->get_exit_block();
    FastVector<int> worklist;
    FastVector<int> visited;

    worklist.push(entry_block->id);

    while(!worklist.empty()) {
        int block_id = worklist.pop();
        if (visited.has(block_id)) {
            continue;
        }

        visited.push(block_id);

        Block *block = _cfg->find_block(block_id);
        ERR_FAIL_COND_MSG(block == nullptr, "ControlFlowGraph cannot find block");

        // Instructions that will remain after LCSE
        FastVector<Instruction> keep;
        // Addresses whose value can be found elsewhere because an assignment
        // or expression has been elided
        Map<int, int> swaps;

        // List of currently available expressions
        FastVector<AvailableExpression> availables;

        for (int i = 0; i < block->instructions.size(); ++i) {
            Instruction& inst = block->instructions[i];

            // Check if either expression operand should be directed
            // elsewhere
            bool resort_operands = false;
            if ((inst.defuse_mask & INSTRUCTION_DEFUSE_SOURCE0) != 0) {
                if (swaps.has(inst.source_address0)) {
                    inst.source_address0 = swaps[inst.source_address0];
                    resort_operands = true;
                }
            }
            if ((inst.defuse_mask & INSTRUCTION_DEFUSE_SOURCE1) != 0) {
                if (swaps.has(inst.source_address1)) {
                    inst.source_address1 = swaps[inst.source_address1];
                    resort_operands = true;
                }
            }
            if ((inst.defuse_mask & INSTRUCTION_DEFUSE_VARARGS) != 0) {
                for (int j = 0; j < inst.varargs.size(); ++j) {
                    int arg_address = Instruction::normalize_address(inst.varargs[j]);
                    if (swaps.has(arg_address)) {
                        inst.varargs[j] = swaps[arg_address];
                        // No need to resort just for this
                    }
                }
            }
            if (resort_operands) {
                inst.sort_operands();
            }

            bool will_keep = true;
            bool invalidate = false;

            if (OpExpression::is_instruction_expression(inst)) {
                OpExpression expr = OpExpression::from_instruction(inst);
                AvailableExpression* available = find_available_expression(availables, expr);
                
                // Did not find this expression available, so add it to the list
                if (available == nullptr) {
                    AvailableExpression ae;
                    ae.target_address = inst.target_address;
                    ae.expression = expr;
                    availables.push(ae);
                
                    // Invalidate swap record because the target address
                    // has a new value. 
                    invalidate = true;
                } else {
                    // Expression is available, so don't keep instruction
                    will_keep = false;

                    // Record that the result of this expression can be found
                    // elsewhere
                    swaps.insert(inst.target_address, available->target_address);
                }
            } else {
                if ((inst.defuse_mask & INSTRUCTION_DEFUSE_TARGET) != 0) {
                    invalidate = true;
                }
            }

            if (invalidate) {
                for (int j = 0; j < availables.size(); ++j) {
                    AvailableExpression& ae = availables[j];
                    if (ae.expression.uses(inst.target_address)) {
                        ae.removed = true;
                    }
                }

                // If we have elided this assignment we need to store it before
                // we can change its source value. If the assignment is not used
                // later it will be removed by dead assignment elimination pass.
                for (Map<int, int>::Element *E = swaps.front(); E; E = E->next()) {
                    if (E->value() == inst.target_address) {
                        Instruction assignment;
                        assignment.opcode = GDScriptFunction::Opcode::OPCODE_ASSIGN;
                        assignment.target_address = E->key();
                        assignment.source_address0 = E->value();
                        assignment.defuse_mask = INSTRUCTION_DEFUSE_SOURCE0;
                        assignment.stride = 3; // todo - don't hard-code
                        keep.push(assignment);
                    }
                }
            
                swaps.erase(inst.target_address);
            }

            if (will_keep) {
                keep.push(inst);
            }
        }

        block->instructions.clear();
        block->instructions.push_many(keep);

        // Any swaps that are still alive and used by subsequent blocks
        // need to be stored back in their original destinations
        for (Map<int, int>::Element *E = swaps.front(); E; E = E->next()) {
            if (block->outs.has(E->key())) {
                Instruction assignment;
                assignment.opcode = GDScriptFunction::Opcode::OPCODE_ASSIGN;
                assignment.target_address = E->key();
                assignment.source_address0 = E->value();
                assignment.defuse_mask = INSTRUCTION_DEFUSE_SOURCE0;
                assignment.stride = 3; // todo - don't hard-code
                block->instructions.push(assignment);
            }
        }

        worklist.push_many(block->forward_edges);
    }
}

void GDScriptFunctionOptimizer::pass_local_insert_redundant_operations() {
    ERR_FAIL_COND_MSG(_cfg == nullptr, "ControlFlowGraph not initialized");
    // Do not need accurate data flow for this pass
    // Will not invalidate data flow in this pass

    Block *entry_block = _cfg->get_entry_block();
    ERR_FAIL_COND_MSG(_cfg == nullptr, "ControlFlowGraph contains no blocks");

    Block *exit_block = _cfg->get_exit_block();
    FastVector<int> worklist;
    FastVector<int> visited;

    worklist.push(entry_block->id);

    while(!worklist.empty()) {
        int block_id = worklist.pop();
        if (visited.has(block_id)) {
            continue;
        }

        visited.push(block_id);

        Block *block = _cfg->find_block(block_id);
        ERR_FAIL_COND_MSG(block == nullptr, "ControlFlowGraph cannot find block");

        // Instructions that will remain after LCSE
        FastVector<Instruction> keep;

        // List of currently available expressions
        FastVector<AvailableExpression> availables;

        for (int i = 0; i < block->instructions.size(); ++i) {
            Instruction& inst = block->instructions[i];

            bool will_keep = true;
            bool invalidate = false;

            if (inst.opcode == GDScriptFunction::Opcode::OPCODE_ASSIGN) {
                AvailableExpression* available = find_available_expression_by_target(availables, inst.source_address0);
                
                // If this value is being calculated elsewhere then duplicate
                // the expression.
                if (available != nullptr) {
                    will_keep = false;

                    Instruction assignment;
                    assignment.opcode = available->expression.opcode;
                    assignment.variant_op = available->expression.variant_op;
                    assignment.defuse_mask = available->expression.defuse_mask;
                    assignment.target_address = inst.target_address;
                    assignment.source_address0 = available->expression.source_address0;
                    assignment.source_address1 = available->expression.source_address1;
                    assignment.stride = 5; // todo: remove hard code
                    keep.push(assignment);
                }
            } else {
                if (OpExpression::is_instruction_expression(inst)) {
                    OpExpression expr = OpExpression::from_instruction(inst);
                    AvailableExpression* available = find_available_expression(availables, expr);

                    // Did not find this expression available, so add it to the list
                    if (available == nullptr) {
                        AvailableExpression ae;
                        ae.target_address = inst.target_address;
                        ae.expression = expr;
                        availables.push(ae); 

                        invalidate = true;
                    }
                } else {
                    if ((inst.defuse_mask & INSTRUCTION_DEFUSE_TARGET) != 0) {
                        invalidate = true;
                    }
                }
            }
            if (invalidate) {   
                for (int j = 0; j < availables.size(); ++j) {
                    AvailableExpression& ae = availables[j];
                    if (ae.expression.uses(inst.target_address)) {
                        ae.removed = true;
                    }
                }
            }

            if (will_keep) {
                keep.push(inst);
            }
        }

        block->instructions.clear();
        block->instructions.push_many(keep);

        worklist.push_many(block->forward_edges);
    }
}

void GDScriptFunctionOptimizer::pass_global_common_subexpression_elimination() {

}