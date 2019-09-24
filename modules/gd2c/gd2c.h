#ifndef __GD2C__
#define __GD2C__

#include "core/reference.h"
#include "modules/gdnative/include/gdnative/gdnative.h"

#ifndef memnew_placement_custom
#define memnew_placement_custom(m_placement, m_class, m_constr) _post_initialize(new (m_placement, sizeof(m_class), "") m_constr)
#endif

class GD2CApi : public Reference {
    GDCLASS(GD2CApi, Reference)
public:
    GD2CApi() {};

	String get_api(int major, int minor);

protected:
    static void _bind_methods();
};

#endif
