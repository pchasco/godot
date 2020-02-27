#ifndef GDSCRIPT_CONTROLFLOW_H
#define GDSCRIPT_CONTROLFLOW_H

#include "core/vector.h"
#include "modules/gdscript/gdscript_function.h"

#include "fastvector.h"
#include "instruction.h"
#include "opexpression.h"

FastVector<int> get_function_default_argument_jump_table(const GDScriptFunction *function);

class Block {
public:
	enum Type {
		NORMAL,
		BRANCH_IF_NOT,
		ITERATE_BEGIN,
		ITERATE,
		DEFARG_ASSIGNMENT,
		TERMINATOR,
	};

	Block() : force_code_size(0), block_type(Type::NORMAL) {}

	// During immediate construction of the CFG the id corresponds with
	// the offset of the first instruction in the block of the bytecode.
	// After the CFG is constructed the id has no relationship with its
	// position in the output bytecode.
	int id;
	
	Type block_type;
	int jump_condition_address;

	int force_code_size;
	FastVector<Instruction> instructions;
	Set<int> defs;
	Set<int> uses;
	Set<int> ins;
	Set<int> outs;
	Set<int> back_edges;
	FastVector<int> forward_edges;

	void replace_jumps(Block& original_block, Block& target_block);
	void update_def_use();
	int calculate_bytecode_size(bool include_jump = true);
};

class ControlFlowGraph {
public:
    ControlFlowGraph(const GDScriptFunction *function);

	void construct();
	Block* find_block(int id) const;
	Block* get_entry_block() const;
	Block* get_exit_block() const;
	Block* get_block(int index) const { return _blocks.address_of(index); }
	int get_block_count() const { return _blocks.size(); }
	FastVector<int> get_bytecode();
	void remove_dead_blocks();
	void analyze_data_flow();

	void debug_print() const;
	void debug_print_instructions() const;

private:
    void disassemble();
	void build_blocks();
	Block* find_block(const FastVector<Block> *blocks, int id) const;

	const GDScriptFunction *_function;
	FastVector<Instruction> _instructions;
    FastVector<Block> _blocks;
    int _entry_id = -10000000;
    int _exit_id = -10000000;
};

#endif