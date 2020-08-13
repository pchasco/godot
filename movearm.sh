#! /bin/sh
scons p=iphone arch=arm64 -j4
cp bin/godot.iphone.debug.arm64 /Users/patrick/Projects/Godot2/moblcade-break-the-blocks/godot_ios_xcode/godot_opt.iphone 
cp -r bin/godot.iphone.debug.arm64.dSYM/ /Users/patrick/Projects/Godot2/moblcade-break-the-blocks/godot_ios_xcode/godot_opt.iphone.dSYM/
