cd ..\..\scons
call scons install prefix=..\..\build-msvc use_vc=yes frontend=qt4 mode=release version_suffix=15 gettext=system nls=yes extra_inc_path=..\..\lyx-windows-deps-msvc-qt4\include extra_lib_path=..\..\lyx-windows-deps-msvc-qt4\lib extra_bin_path=..\..\lyx-windows-deps-msvc-qt4\bin qt_dir=..\..\lyx-windows-deps-msvc-qt4\qt-4
cd ..\Win32\packaging