/*************************************************************************/
/*  game_center.mm                                                       */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2019 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2019 Godot Engine contributors (cf. AUTHORS.md)    */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/
#ifdef IPHONE_ENABLED
#include "haptic.h"

#import <UIKit/UIKit.h>

Haptic *Haptic::instance = NULL;

void Haptic::_bind_methods() {
	ObjectTypeDB::bind_method(_MD("prepare"), &Haptic::prepare);
	ObjectTypeDB::bind_method(_MD("impact_occurred"), &Haptic::impact_occurred);
	ObjectTypeDB::bind_method(_MD("impact_occurred_with_intensity"), &Haptic::impact_occurred_with_intensity);
};

void Haptic::prepare() {
    NSLog(@"Haptic:prepare()");

    if (impactFeedbackGenerator == NULL) {
        impactFeedbackGenerator = [[UIImpactFeedbackGenerator alloc] init];
        NSLog(@"Haptic:prepare():initialized");
    }

    [impactFeedbackGenerator prepare];
};

void Haptic::impact_occurred() {
    NSLog(@"Haptic:impact_occurred()");

    if (impactFeedbackGenerator == NULL) {
        return;
    }

    [impactFeedbackGenerator impactOccurred];
    NSLog(@"Haptic:impact_occurred executed");
};

void Haptic::impact_occurred_with_intensity(float intensity) {
    NSLog(@"Haptic:impact_occurred_with_intensity(float intensity)");

    if (impactFeedbackGenerator == NULL) {
        return;
    }

    [impactFeedbackGenerator impactOccurredWithIntensity:intensity];
    NSLog(@"Haptic:prepare():impact_occurred_with_intensity executed");
};

Haptic *Haptic::get_singleton() {
	return instance;
};

Haptic::Haptic() {
	ERR_FAIL_COND(instance != NULL);
	instance = this;
	impactFeedbackGenerator = NULL;
};

Haptic::~Haptic(){
    impactFeedbackGenerator = NULL;
};

#endif
