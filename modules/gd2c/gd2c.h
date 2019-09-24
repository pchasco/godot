#ifndef GD2C_RUNTIME_SERVICES_H
#define GD2C_RUNTIME_SERVICES_H

#include "core/reference.h"
#include "modules/gdnative/include/gdnative/gdnative.h"

#ifndef memnew_placement_custom
#define memnew_placement_custom(m_placement, m_class, m_constr) _post_initialize(new (m_placement, sizeof(m_class), "") m_constr)
#endif

/*

*/

extern "C" {
	
}

class GD2CApi : public Reference {
    GDCLASS(GD2CApi, Reference)
public:
    GD2CApi() {};

	String get_api(int major, int minor);

protected:
    static void _bind_methods();
};

#endif
