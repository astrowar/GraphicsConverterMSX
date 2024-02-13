@REM "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
@REM  Debug\ConverterGui.exe Debug\00026-2927110527.png

@REM Build for Visual Studio compiler. Run your copy of vcvars32.bat or vcvarsall.bat to setup command-line compiler.
@set OUT_DIR=Debug
@set OUT_EXE=ConverterGui
@set SDL2_DIR=SDL  
@set INCLUDES=/ISDL\include  /Iimgui /Iimgui\backends 
@set SOURCES=ConverterGui.cpp imgui\backends\imgui_impl_sdl2.cpp imgui\backends\imgui_impl_opengl2.cpp imgui\imgui*.cpp color_interface.cpp converter_msx.cpp color_conv.cpp  colorCiee.cpp
@set LIBS=/LIBPATH:SDL\lib\x64 SDL2.lib SDL2main.lib  SDL2_image.lib opengl32.lib shell32.lib
mkdir %OUT_DIR%
cl  /std:c++17 /openmp /fp:fast /EHsc /nologo /Ox /Zi /MD /utf-8 %INCLUDES% %SOURCES% /Fe%OUT_DIR%/%OUT_EXE%.exe /Fo%OUT_DIR%/ /link %LIBS% /subsystem:console 