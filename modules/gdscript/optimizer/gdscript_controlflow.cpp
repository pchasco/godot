#include "core/os/memory.h"
#include "modules/gdscript/gdscript_functions.h"
#include "gdscript_optimizer.h"
#include "fastvector.h"
#include "distinctstack.h"

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

void Block::replace_jumps(Block& original_block, Block& target_block) {
    for (int i = 0; i < forward_edges.size(); ++i) {
        if (forward_edges[i] == original_block.id) {
            forward_edges[i] = target_block.id;
        }
    }

    original_block.back_edges.erase(id);
    target_block.back_edges.insert(id);
}

void Block::update_def_use() {
    // Iterate over instructions in block. When a variable is assigned a value
    // it goes into the defs set. If it is read then it goes into the use set 
    // if it is not already in the defs set.
    defs.clear();
    uses.clear();

    switch (block_type) {
        case Block::ITERATE_BEGIN:
        case Block::ITERATE:
            defs.insert(iterator_value_address);
            defs.insert(iterator_counter_address);
            uses.insert(iterator_container_address);
            break;
    }

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

    switch (block_type) {
        case Block::BRANCH_IF_NOT:
            if (!defs.has(jump_condition_address)) {
                uses.insert(jump_condition_address);
            }
            break;
    }
}

int Block::calculate_bytecode_size(bool include_jump) {
    int size = 0;
    for (int i = 0; i < instructions.size(); ++i) {
        size += instructions[i].stride;
    }

    switch (block_type) {
    case Block::Type::NORMAL:
        if (include_jump) {
            size += 2;
        }
        break;
    case Block::Type::BRANCH_IF_NOT:
        size += 3; // for JUMP_IF
        if (include_jump) {
            size += 2; // for JUMP
        }
        break;
    case Block::Type::DEFARG_ASSIGNMENT:
        size += forward_edges.size();
        if (include_jump) {
            size += 2;
        }
        break;
    case Block::Type::ITERATE:
        size += 5;
        if (include_jump) {
            size += 2;
        }
        break;
    case Block::Type::ITERATE_BEGIN:
        size += 5;
        if (include_jump) {
            size += 2;
        }
        break;
    case Block::Type::TERMINATOR:
        size += 1;
        break;
    }

    return size;
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
        Instruction inst = Instruction::parse(code, ip, code_size);
        ip += inst.stride;
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
    entry.block_type = Block::Type::NORMAL;
    // Entry block always jumps to block with id 0
    entry.forward_edges.push(0);

    _entry_id = entry.id;

    Block exit;
    exit.block_type = Block::Type::TERMINATOR;
    exit.id = EXIT_BLOCK_ID;
    _exit_id = exit.id;

    blocks.push(exit);
    blocks.push(entry);
    worklist.push(0);

    // Precache the list of jump target addresses. We can use this
    // to know whether or not a block is a jump target
    int ip = 0;
    for (int i = 0; i < _instructions.size(); ++i) {
        Instruction& inst = _instructions[i];
        int next_ip = ip + inst.stride;

        switch (inst.opcode) {
            case GDScriptFunction::Opcode::OPCODE_JUMP:
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
            case GDScriptFunction::Opcode::OPCODE_JUMP_IF:
            case GDScriptFunction::Opcode::OPCODE_JUMP_IF_NOT:
                jump_targets.push(inst.branch_ip);
                jump_targets.push(next_ip); 
                break;
            case GDScriptFunction::Opcode::OPCODE_ITERATE:
            case GDScriptFunction::Opcode::OPCODE_ITERATE_BEGIN:
                jump_targets.push(inst.branch_ip);
                jump_targets.push(next_ip); 
                break;
        }

        ip = next_ip;
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
        ERR_FAIL_COND_MSG(i >= _instructions.size(), "Block id not found");

        int block_start_ip = ip;

        // Take block instructions
        bool done = false;
        while (!done) {
            Instruction& inst = _instructions[i];
            i += 1;
            int next_ip = ip + inst.stride;

            // Insert instruction to block
            switch (inst.opcode) {
                // Branch instructions are handled as a special case.
                // We do not insert branch instructions into the block.
                // Branches will be emitted based on the block type. This
                // Simplifies optimizations that insert or remove instructions
                // because they will not need to worry about preserving the
                // order of the terminating jump
                case GDScriptFunction::Opcode::OPCODE_JUMP:
                    worklist.push(inst.branch_ip);
                    block.block_type = Block::Type::NORMAL;
                    block.forward_edges.push(inst.branch_ip);
                    done = true;
                    break;
                case GDScriptFunction::Opcode::OPCODE_JUMP_IF:
                    worklist.push(next_ip);
                    worklist.push(inst.branch_ip);
                    block.block_type = Block::Type::BRANCH_IF_NOT;
                    // Implement JUMP_IF by inverting and using JUMP_IF_NOT
                    // Seems counter-intuitive but generates better code most
                    // of the time.
                    block.forward_edges.push(inst.branch_ip);
                    block.forward_edges.push(next_ip);
                    block.jump_condition_address = inst.source_address0;
                    done = true;
                    break;
                case GDScriptFunction::Opcode::OPCODE_JUMP_IF_NOT:
                    worklist.push(next_ip);
                    worklist.push(inst.branch_ip);
                    block.block_type = Block::Type::BRANCH_IF_NOT;
                    block.forward_edges.push(next_ip);
                    block.forward_edges.push(inst.branch_ip);
                    block.jump_condition_address = inst.source_address0;
                    done = true;
                    break;
                case GDScriptFunction::Opcode::OPCODE_ITERATE:
                    worklist.push(inst.branch_ip);
                    worklist.push(next_ip);
                    block.block_type = Block::Type::ITERATE;
                    block.forward_edges.push(next_ip);
                    block.forward_edges.push(inst.branch_ip);
                    block.iterator_counter_address = inst.source_address0;
                    block.iterator_container_address = inst.source_address1;
                    block.iterator_value_address = inst.target_address;
                    done = true;
                    break;
                case GDScriptFunction::Opcode::OPCODE_ITERATE_BEGIN:
                    worklist.push(inst.branch_ip);
                    worklist.push(next_ip);
                    block.block_type = Block::Type::ITERATE_BEGIN;
                    block.forward_edges.push(next_ip);
                    block.forward_edges.push(inst.branch_ip);
                    block.iterator_counter_address = inst.source_address0;
                    block.iterator_container_address = inst.source_address1;
                    block.iterator_value_address = inst.target_address;
                    done = true;
                    break;
                case GDScriptFunction::Opcode::OPCODE_RETURN:
                case GDScriptFunction::Opcode::OPCODE_END:
                    block.forward_edges.push(exit.id);
                    done = true;
                    break;
                case GDScriptFunction::Opcode::OPCODE_JUMP_TO_DEF_ARGUMENT:
                    // Can't use defarg_jump_table because we popped one out. Refer back to function
                    block.forward_edges.push(next_ip);
                    for (int defarg = 0; defarg < _function->get_default_argument_count(); ++defarg) {
                        int defarg_ip = _function->get_default_argument_addr(defarg);
                        block.forward_edges.push(defarg_ip);
                        worklist.push(defarg_ip);
                    }
                    block.block_type = Block::Type::DEFARG_ASSIGNMENT;
                    worklist.push(next_ip);
                    done = true;
                    break;

                default:
                    // Any other instruction gets pushed to the block instructions
                    block.instructions.push(inst);

                    // We happened upon a jump target address 
                    // without branching from this block. Start a new block
                    // and add an edge to the new block
                    if (jump_targets.has(next_ip)) {
                        block.forward_edges.push(next_ip);
                        worklist.push(next_ip);
                        done = true;
                    }

                    break;
            } 

            ip = next_ip;
        }   

        blocks.push(block);
    }
    
    // Create back edges
    for (int bi = 0; bi < blocks.size(); ++bi) {
        for (int i = 0; i < blocks[bi].forward_edges.size(); ++i) {
            Block *succ = find_block(&blocks, blocks[bi].forward_edges[i]);
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
        for (int j = 0; j < block->forward_edges.size(); ++j) {
            Block *target = find_block(block->forward_edges[j]);
            target->back_edges.erase(block->id);
        }
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
        DistinctStack<int> worklist;

        // We walk the CFG in reverse to determine data flow
        worklist.push(_exit_id);
        
        while(!worklist.empty()) {
            int block_id = worklist.pop();

            Block *block = find_block(block_id);

            // Insert all ins from successive blocks into this block's outs
            for (int i = 0; i < block->forward_edges.size(); ++i) {
                Block *succ = find_block(block->forward_edges[i]);
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

            // Update this block's ins with the jump condition address, if applicable
            switch (block->block_type) {
                case Block::Type::BRANCH_IF_NOT:
                    if (!block->ins.has(block->jump_condition_address)) {
                        block->ins.insert(block->jump_condition_address);
                        repeat = true;
                    }
                    break;
                case Block::Type::ITERATE:
                case Block::Type::ITERATE_BEGIN:
                    if (!block->ins.has(block->iterator_container_address)) {
                        block->ins.insert(block->iterator_container_address);
                        repeat = true;
                    }
                    if (!block->ins.has(block->iterator_counter_address)) {
                        block->ins.insert(block->iterator_counter_address);
                        repeat = true;
                    }
                    break;
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

FastVector<int> ControlFlowGraph::assemble() {
    DistinctStack<int> worklist;
    FastVector<int> bytecode;
    FastVector<int> blocks_in_order;
    Map<int, int> block_ip_index;

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

        Block *block = find_block(block_id);
        if (block == nullptr) {
            break;
        }

        blocks_in_order.push(block_id);
        worklist.push_many(block->forward_edges);
    }

    // Now build an index of the blocks and their starting offset in the bytecode
    int ip = 0;
    for (int block_index = 0; block_index < blocks_in_order.size(); ++block_index) {
        // Remember the beginning ip offset of this block
        int block_id = blocks_in_order[block_index];
        block_ip_index[block_id] = ip;

        // Move ip offset forward by block size. Include the final jump instruction
        // of the block only if the next block is not guaranteed next in bytecode
        Block *block = find_block(block_id);
        if (block->forward_edges.size()
        && block_index < blocks_in_order.size() - 1
        && blocks_in_order[block_index + 1] == block->forward_edges[0]) {
            ip += block->calculate_bytecode_size(false);
        } else {
            if (block->forward_edges.size()) {
                ip += block->calculate_bytecode_size(true);
            } else {
                ip += block->calculate_bytecode_size(false);
            }
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
        
        // As a convenience, get the id of the next block in the list of blocks
        int next_block_id = -2;
        if (block_index < blocks_in_order.size() - 1) {
            next_block_id = blocks_in_order[block_index + 1];
        }

        // This block starts at the end of the bytecode we've already emitted
        int start_block_ip = bytecode.size();

        // Emit instructions
        int ip = start_block_ip;
        for (int i = 0; i < block->instructions.size(); ++i) {
            Instruction& inst = block->instructions[i];
            if (inst.omit) {
                continue;
            }
            
            inst.encode(bytecode);
            ip += inst.stride;
        }

        // Emit jump to next block IF necessary
        int unconditional_jump_block_id = -2;
        switch (block->block_type) {
            case Block::Type::NORMAL:
                // Dont emit the unconditional jump just yet...
                unconditional_jump_block_id = block->forward_edges[0];
                break;
            case Block::Type::BRANCH_IF_NOT: {
                Instruction inst;
                inst.opcode = GDScriptFunction::Opcode::OPCODE_JUMP_IF_NOT;
                inst.source_address0 = block->jump_condition_address;
                inst.defuse_mask = INSTRUCTION_DEFUSE_SOURCE0;
                inst.branch_ip = block_ip_index[block->forward_edges[1]];
                inst.encode(bytecode);
                unconditional_jump_block_id = block->forward_edges[0];
            }
                break;
            case GDScriptFunction::Opcode::OPCODE_JUMP_TO_DEF_ARGUMENT:
                break;
            case Block::Type::ITERATE_BEGIN: {
                Instruction inst;
                inst.opcode = GDScriptFunction::Opcode::OPCODE_ITERATE_BEGIN;
                inst.source_address0 = block->iterator_counter_address;
                inst.source_address1 = block->iterator_container_address;
                inst.branch_ip = block_ip_index[block->forward_edges[1]];
                inst.target_address = block->iterator_value_address;
                inst.defuse_mask = INSTRUCTION_DEFUSE_SOURCE0
                    | INSTRUCTION_DEFUSE_SOURCE1
                    | INSTRUCTION_DEFUSE_TARGET;
                inst.encode(bytecode);
                unconditional_jump_block_id = block->forward_edges[0];
            }
                break; 
            case Block::Type::ITERATE: {
                Instruction inst;
                inst.opcode = GDScriptFunction::Opcode::OPCODE_ITERATE;
                inst.source_address0 = block->iterator_counter_address;
                inst.source_address1 = block->iterator_container_address;
                inst.branch_ip = block_ip_index[block->forward_edges[1]];
                inst.target_address = block->iterator_value_address;
                inst.defuse_mask = INSTRUCTION_DEFUSE_SOURCE0
                    | INSTRUCTION_DEFUSE_SOURCE1
                    | INSTRUCTION_DEFUSE_TARGET;
                unconditional_jump_block_id = block->forward_edges[0];
                inst.encode(bytecode);
            }
                break;          
            case Block::Type::TERMINATOR:
                bytecode.push(GDScriptFunction::Opcode::OPCODE_END);
                break;
        }

        // Emit a jump if the next block is not the next in bytecode
        if (unconditional_jump_block_id >= 0) {
            if (next_block_id != unconditional_jump_block_id) {
                bytecode.push(GDScriptFunction::Opcode::OPCODE_JUMP);
                bytecode.push(block_ip_index[unconditional_jump_block_id]);
            }
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
        print_line("type: " + itos(block->block_type));
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
            for (int i = 0; i < block->forward_edges.size(); ++i) {
                print_line("    " + itos(block->forward_edges[i]));
            }
        }

        print_line("");
    }
}

void ControlFlowGraph::debug_print_instructions() const {
    print_line("------ Instructions ------");
    int ip = 0;
    for (int i = 0; i < _instructions.size(); ++i) {
        const Instruction& inst = _instructions[i];
        print_line(itos(ip) + ": " + inst.to_string());
        ip += inst.stride;
    }
}