# Animation Data

Here is where the animation backporting tool and the Blender I/O add-ons can be found. MOTLib.py contains code for the utility classes used in all three scripts, anim_backporter.py (as the name suggests) backports version 8 animations to version 7, and the MOT_IMPORT.py and MOT_EXPORT.py files contain the main logic for importing and exporting animations from Blender.

Installing the Blender add-ons is as easy as packing the MOTLib.py, MOT_IMPORT.py, and MOT_EXPORT.py files into a zip file and installing that zip via Blender's preferences menu.

To use the animation backporter, simply download the MOTLib.py and anim_backporter.py files and execute the script via the command line, passing an input and output animation path as arguments. (e.g. ``python anim_backporter.py my_v8_anim.got my_backported_anim.got``)

# Credits
[Me](https://github.com/itsmeft24) - Reverse-engineered the file format and wrote the code lol

[SSBUCarlos](https://github.com/ssbucarlos) - Helped tremendously with the math behind the logic + utilizing the Blender APIs correctly
