#ifndef GDSCRIPT_OPTIMIZER_H
#define GDSCRIPT_OPTIMIZER_H

#include "core/vector.h"
#include "modules/gdscript/gdscript_function.h"

#include "fastvector.h"
#include "gdscript_controlflow.h"

class GDScriptFunctionOptimizer {
public:
	GDScriptFunctionOptimizer(GDScriptFunction *function);

	~GDScriptFunctionOptimizer() {
		if (_cfg != nullptr) {
			delete _cfg;
		}
	}

	void begin();
	
	// Analysis passes
	void pass_data_type_analysis();

	// Transform passes

	// Eliminates redundant jumps
	void pass_jump_threading();

	// Eliminates blocks with no code
	void pass_dead_block_elimination();

	// Eliminates assignments whose values are never used. Does not
	// eliminate assignments that may have side effects
	void pass_dead_assignment_elimination();

	// Eliminates recalculation of expressions whose results are
	// already known.
	void pass_local_common_subexpression_elimination();

	// Replaces assignments from one address to another when the source
	// value is the result of an expression with the full expression.
	// When followed by dead assignment elimination and local common
	// subexpression elimination, this eliminates many unneeded assignments
	void pass_local_insert_redundant_operations();

	// Strips debug instructions. Generally unnecessary when GDScript has
	// been built for release
	void pass_strip_debug();

	void commit();

	ControlFlowGraph *get_cfg() const { return _cfg; }

private:
	void requires_data_flow() {
		if (_cfg != nullptr && _is_data_flow_dirty) {
			_cfg->analyze_data_flow();
			_is_data_flow_dirty = false;
		}
	}

	void invalidate_data_flow() {
		_is_data_flow_dirty = true;
	}

	GDScriptFunction *_function;
	ControlFlowGraph *_cfg;
	bool _is_data_flow_dirty;
};

#endif