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
	void pass_local_common_subexpression_elimination();
	void pass_global_common_subexpression_elimination();
	void pass_strip_debug();
	void pass_register_allocation();

	void commit();

	ControlFlowGraph *get_cfg() const { return _cfg; }

private:
	GDScriptFunction *_function;
	ControlFlowGraph *_cfg;
};

#endif