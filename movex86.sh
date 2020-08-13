#! /bin/sh
scons p=iphone arch=x86_64 -j8
cp bin/godot.iphone.debug.x86_64 /Users/patrick/Projects/Godot2/moblcade-break-the-blocks/godot_ios_xcode/godot_opt.iphone 
cp -r bin/godot.iphone.debug.x86_64.dSYM/ /Users/patrick/Projects/Godot2/moblcade-break-the-blocks/godot_ios_xcode/godot_opt.iphone.dSYM/
