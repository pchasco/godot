#ifndef GDSCRIPT_OPTIMIZER_H
#define GDSCRIPT_OPTIMIZER_H

#include "core/vector.h"
#include "gdscript_function.h"

// Never set to less than 1 because pop() returns _data[0] if nothing in vector
#define STACK_MIN_BUFFER_SIZE 8

template <typename T>
class FastVector {
public:
	FastVector() : FastVector(STACK_MIN_BUFFER_SIZE) {}

	FastVector(size_t p_initial_buffer_size) {
		if (p_initial_buffer_size == 0) {
			p_initial_buffer_size = STACK_MIN_BUFFER_SIZE;
		}

		_top = 0;
		_size = p_initial_buffer_size;
		_data = nullptr;
	}

	FastVector(const FastVector<T>& other)
		: FastVector(other._top)
	{
		_top = other._top;
		
		initialize_data();
		
		for (int i = 0; i < _top; ++i) {
			new (_data + i, sizeof(T), "") T(other._data[i]);
		}
	}

	virtual ~FastVector() {
		if (_data != nullptr) {
			clear();
			memfree(_data);
			_data = nullptr;
			_size = 0;
		}
	}

	FastVector<T>& operator=(const FastVector<T>& other) {
		_size = other._size;

		size_t bytes = sizeof(T) * _size;
		if (_data == nullptr) {
			_data = (T*)memalloc(bytes);
		} else {
			clear();
			_data = (T*)memrealloc(_data, bytes);
		}

		_top = other._top;

		for (int i = 0; i < _top; ++i) {
			new (_data + i, sizeof(T), "") T(other._data[i]);
		}

		return *this;
	}

	void push(const T& p_value) {
		if (_top == _size || _data == nullptr) {
			if (_top == _size) {
				_size *= 2;
				if (_size < STACK_MIN_BUFFER_SIZE) {
					_size = STACK_MIN_BUFFER_SIZE;
				}
			}

			if (_data == nullptr) {
				_data = (T*)memalloc(sizeof(T) * _size);
			} else {
				_data = (T*)memrealloc(_data, sizeof(T) * _size);
			}
		}

		new (_data + _top, sizeof(T), "") T(p_value);
		_top += 1;
	}

	void push_many(int count, const T* p_values) {
		for (int i = 0; i < count; ++i) {
			push(*p_values[i]);
		}
	}

	T pop() {
		if (_top <= 0) {
			ERR_FAIL_V_MSG(*((T*)nullptr), "Tried to pop when nothing to pop");
		}

		return _data[--_top];
	}

	bool empty() const {
		return _top == 0;
	}

	T *address_of(int p_index) const {
		if (p_index < 0 || p_index >= _top) {
			ERR_FAIL_V_MSG(nullptr, "Index out of range");
		}

		return &_data[p_index];
	}

	int size() const {
		return _top;
	}

	T &operator[](int p_index) const {
		if (p_index < 0 || p_index >= _top) {
			ERR_FAIL_V_MSG(*((T*)nullptr), "Index out of range");
		}

		return _data[p_index];
	}

	int index_of(const T &p_value) const {
		for (int i = 0; i < _top; ++i) {
			if (_data[i] == p_value) {
				return i;
			}
		}

		return -1;
	}

	bool has(const T &p_value) const {
		return index_of(p_value) >= 0;
	}

	void clear() {
		if (_data != nullptr) {
			for (int i = 0; i < _top; ++i) {
				_data[i].~T();
			}
		}
		
		_top = 0;
	}

	const T *ptr() const {
		return _data;
	}

private:
	void initialize_data() {		
		size_t bytes = sizeof(T) * _size;
		_data = (T*)memalloc(bytes);
	}

	T *_data;
	int _size;
	int _top;
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
	Block() : force_code_size(0) {}

	// During immediate construction of the CFG the id corresponds with
	// the offset of the first instruction in the block of the bytecode.
	// After the CFG is constructed the id has no relationship with its
	// position in the output bytecode.
	int id;
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
	FastVector<int> get_bytecode();
	void remove_dead_blocks();
	void analyze_live_variables();

	void debug_print() const;
	void debug_print_instructions() const;

private:
    void disassemble();
	void build_blocks();
	Block* find_block(const FastVector<Block> *blocks, int id) const;

	const GDScriptFunction *function;
	FastVector<Instruction> instructions;
    FastVector<Block> _blocks;
    int _entry_id = -10000000;
    int _exit_id = -10000000;
};

class GDScriptFunctionOptimizer {
public:
	GDScriptFunctionOptimizer(GDScriptFunction *function);
	~GDScriptFunctionOptimizer() {
		if (_cfg != nullptr) {
			delete _cfg;
		}
	}

	void begin();

	void pass_jump_threading();
	void pass_dead_block_elimination();
	void pass_strip_debug();

	void commit();

	ControlFlowGraph *get_cfg() const { return _cfg; }

private:
	GDScriptFunction *_function;
	ControlFlowGraph *_cfg;
};

#endif