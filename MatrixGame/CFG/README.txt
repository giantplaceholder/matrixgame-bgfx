This "CFG/" directory's structure:

"./robots/iface.txt"
"./robots/data.txt"
"./robots_rename_to_use.dat"

===================================================== EXE ===================================================== 

This "CFG/" directory should reside in the same folder as the final "MatrixGame.exe" if you build for .EXE

The "robots_rename_to_use.dat" is a compressed BlockPar database file which consists of 
"./robots/iface.txt" and "./robots/data.txt" configuration files and is optional for .EXE

If the game cannot find "./robots.dat" it will use "./robots/iface.txt" and "./robots/data.txt" instead.
Therefore, if you want to use the .dat file instead of the .txt files you would need to rename it:

"./robots_rename_to_use.dat" -> "./robots.dat"

Alternatively the "robots.dat" file can be taken from "$(SPACE_RANGERS_DIR)/CFG/$(LANG)/robots.dat".

But it is better to use .txt files because you can tweak them easily.

===================================================== DLL ===================================================== 

If you build for .DLL to use with Space Rangers 2 then only "robots.dat" file can be used!

And it should already be present at "$(SPACE_RANGERS_DIR)/CFG/$(LANG)/robots.dat"

If you want to build a new "robots.dat" file using modified ".txt" config files you can do so using "BUILDCFG"
console command inside of DEBUG .EXE version developer in-game console (press "~" to open).

===============================================================================================================

TODO: allow usage of .txt config files when building for .DLL
 