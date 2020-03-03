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
	void pass_available_expression_analysis();
	void pass_data_type_analysis();

	// Transform passes
	void pass_jump_threading();
	void pass_dead_block_elimination();
	void pass_dead_assignment_elimination();
	void pass_local_common_subexpression_elimination();
	void pass_global_common_subexpression_elimination();
	void pass_local_insert_redundant_operations();
	void pass_strip_debug();

	void commit();

	ControlFlowGraph *get_cfg() const { return _cfg; }

private:
	void need_data_flow() {
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