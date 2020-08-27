#ifdef IPHONE_ENABLED

#ifndef HAPTIC_H
#define HAPTIC_H

#include "core/object.h"

#ifdef __OBJC__
@class UIImpactFeedbackGenerator;
typedef UIImpactFeedbackGenerator *UIImpactFeedbackGeneratorPtr;
#else
typedef void *UIImpactFeedbackGeneratorPtr;
#endif

class Haptic : public Object {

	OBJ_TYPE(Haptic, Object);

	static Haptic *instance;
	static void _bind_methods();
    
    UIImpactFeedbackGeneratorPtr impactFeedbackGenerator;

public:
    void prepare();
    void impact_occurred();
    void impact_occurred_with_intensity(float intensity);

	static Haptic *get_singleton();

	Haptic();
	~Haptic();
};

#endif

#endif