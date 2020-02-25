#ifndef GDSCRIPT_CONTROLFLOW_H
#define GDSCRIPT_CONTROLFLOW_H

#include "core/vector.h"
#include "modules/gdscript/gdscript_function.h"

#include "fastvector.h"

FastVector<int> get_function_default_argument_jump_table(const GDScriptFunction *function);

class OpExpression {
public:
	OpExpression() : 
		opcode(GDScriptFunction::Opcode::OPCODE_END),
		result_type(Variant::Type::VARIANT_MAX)
	{}

	GDScriptFunction::Opcode opcode;
	Variant::Type result_type;
	int operand_address0;
	int operand_address1;
	Variant::Operator variant_operator;

	bool matches(const OpExpression &other) const;
	bool is_empty() const { return opcode == GDScriptFunction::Opcode::OPCODE_END; }
	void set_empty() { opcode = GDScriptFunction::Opcode::OPCODE_END; }
};

#define INSTRUCTION_DEFUSE_TARGET 1
#define INSTRUCTION_DEFUSE_SOURCE0 2
#define INSTRUCTION_DEFUSE_SOURCE1 4
#define INSTRUCTION_DEFUSE_VARARGS 8
#define INSTRUCTION_DEFUSE_INDEX 16
#define INSTRUCTION_DEFUSE_SELF 32

class Instruction {
public:
	Instruction() : omit(false), defuse_mask(0) {}
    bool is_branch() const;
	String to_string() const;

	OpExpression get_expression() const;

public:
    GDScriptFunction::Opcode opcode;
    Variant::Operator variant_op;
    int target_address;
    int source_address0;
	union {
		int source_address1;
		int index_address;
	};
    int index_arg;
    int type_arg;
    int vararg_count;
    int branch_ip;
    int stride;
	bool omit;
	int defuse_mask;
    FastVector<int> varargs;
};

class Block {
public:
	enum Type {
		NORMAL,
		BRANCH,
		TERMINATOR,
	};

	Block() : force_code_size(0) {}

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
	Set<int> forward_edges;

	void replace_jumps(Block& original_block, Block& target_block);
	void update_def_use();
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