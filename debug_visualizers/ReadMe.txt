Natvis files will get compiled into the project's .pdb automatically, so manual deployment should not be necessary.

If manual deployment is still desired, copy the .natvis file to either the user-specific natvis directory
 %USERPROFILE%\Documents\Visual Studio 2017\Visualizers
or the system-wide natvis directory
 (%VSINSTALLDIR%\Common7\Packages\Debugger\Visualizers

See https://docs.microsoft.com/en-us/visualstudio/debugger/create-custom-views-of-native-objects for details.
