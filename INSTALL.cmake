=========================
Building LyX with CMake
=========================

    Install CMake from www.cmake.org or your distribution (version >= 2.6.4).



3rd party libraries
--------------------

    Install Qt 4 and make sure qmake is found.
        Add the folder with qmake to the environment variable PATH.
        If you've compiled Qt by yourself or qmake is not found after
        installing Qt fix PATH,
        Linux/Unix: export PATH=<your path to qt>/bin:$PATH
        Windows   : set PATH=<your path to qt>\bin;%PATH%
    
    When you wanna run LyX without installing from a out-of-source
    build directory you have to set the environment variable LYX_DIR_20x
    and it must point to the lib dir in the source tree.
        Linux/Unix: export LYX_DIR_20x=<lyx-source>/lib
        Windows   : set LYX_DIR_20x=<lyx-source>\lib

    Windows specific
    
        On Windows install the supplementary modules:
        * Visual Studio 2008: ftp://ftp.lyx.org/pub/lyx/contrib/lyx-windows-deps-msvc2008.zip
        * Visual Studio 2010: ftp://ftp.devel.lyx.org/pub/contrib/windows/bin
                or use the option LYX_DEPENDENCIES_DOWNLOAD, then you have to add
                these paths to your PATH variable:
                  <build-dir>\msvc2010-deps\deps20\bin
                  <build-dir>\msvc2010-deps\deps20\python
                  <build-dir>\msvc2010-deps\deps20\imagemagick
                  <build-dir>\msvc2010-deps\deps20\ghostscript
                  <build-dir>\msvc2010-deps\deps20\gettext-tools

        If cmake couldn't find these modules set GNUWIN32_DIR, eg. 
        -DGNUWIN32_DIR=c:\gnuwin32. By default cmake searches in your 
        program folder. Or use the cmake GUI to set the GNUWIN32_DIR path.

        The build process tries to find aspell on Windows
        in %ProgramFiles%/GnuWin32/ and in /usr/ or in /usr/local 
        under Linux. If it could not find aspell, spell checking
        will be disabled.
        
        
        
Generating build system files
------------------------------

    CMake is a build system file generator. On all systems it could
    generate files for several build systems, for instance Makefiles
    for make, project files for Visual Studio, Xcode, Eclipse.

    Running cmake without any argument lists all supported build
    systems on your system. Passing one of them as -G"<build system name>" 
    argument when running cmake selects this.


    
Building out-of-source
-----------------------

    The standard way of using CMake is to build in a folder which doesn't resides
    in the source tree. This has the advantage, that a complete fresh build could
    be done by simply deleting all files in the build folder and to re-run cmake 
    again.

    Another benefit of out-of-source builds is that several builds (debug, release,
    command-line builds, IDE project files) could all use the same source tree.

    Therefore when using cmake create a folder outside of the source tree and
    select this folder when using CMake's GUI, cmake-gui, or go into this folder
    when you call cmake from the shell.

    

Using cmake
------------

    When calling cmake you must pass the path to the source tree (absolute are relative)
    and optionally the generator (each system has its own default). Additional arguments 
    could be passed with the -D prefix.

    Here some examples, assuming the build folder is in the same folder as the source tree:

    * Makefiles on Linux
        cmake ../trunk/development/cmake
    
    * Project files for QtCreator:
        Open the trunk/development/cmake/CMakeLists.txt file and select the build folder
        or create the files in the command line using the -G"CodeBlocks *" option, eg
            cmake ../trunk/development/cmake -G"CodeBlocks - Unix Makefiles"
    
    * Project files for Xcode
        cmake ../trunk/development/cmake -GXcode
        
    * Project files for Visual Studio 10
        cmake ..\trunk\development\cmake -G"Visual Studio 10"

    * NMake files for Visual Studio
        cmake ..\trunk\development\cmake -G"NMake Makefiles"

    * Makefiles for MinGW
        cmake ..\trunk\development\cmake -G"MinGW Makefiles"


    Daily work:

    * Re-running cmake is simple
        cmake .

    * Adding new files 
        The cmake build system scans the directories, so no need to update any file,
        just re-run cmake. Also the mocing rules are generated.

    * Unused source code file
        Because cmake scans the directories for *.cpp and *.h files it will also
        add files to the build system which are not mentioned to build. To exclude
        them change their ending and re-run cmake.


        
      
Build options
--------------

    Options could be passed by the -D prefix when running cmake.
    Available options will be listed on each cmake run.
    Here the options with their default value:
    
    # Available on all systems/compilers
    -- LYX_CPACK                = OFF    : Use the CPack management (Implies LYX_INSTALL option)
    -- LYX_INSTALL              = OFF    : Build install projects/rules (implies a bunch of other options)
    -- LYX_NLS                  = OFF    : Use nls
    -- LYX_ASPELL               = OFF    : Require aspell
    -- LYX_ENCHANT              = OFF    : Require Enchant
    -- LYX_HUNSPELL             = OFF    : Require Hunspell
    -- LYX_DEVEL_VERSION        = OFF    : Build developer version
    -- LYX_RELEASE              = ON     : Build release version, build debug when disabled
    -- LYX_PACKAGE_SUFFIX       = ON     : Use version suffix for packaging
    -- LYX_PCH                  = OFF    : Use precompiled headers
    -- LYX_MERGE_FILES          = OFF    : Merge source files into one compilation unit
    -- LYX_MERGE_REBUILD        = OFF    : Rebuild generated files from merged files build
    -- LYX_QUIET                = OFF    : Don't generate verbose makefiles
    -- LYX_INSTALL_PREFIX       = OFF    : Install path for LyX
    -- LYX_EXTERNAL_LIBINTL     = ON     : Use external libintl
    
    # GCC specific 
    -- LYX_PROFILE              = OFF    : Build profile version
    -- LYX_EXTERNAL_BOOST       = OFF    : Use external boost
    -- LYX_PROGRAM_SUFFIX       = ON     : Append version suffix to binaries
    -- LYX_DEBUG_GLIBC          = OFF    : Enable libstdc++ debug mode
    -- LYX_DEBUG_GLIBC_PEDANTIC = OFF    : Enable libstdc++pedantic debug mode
    -- LYX_STDLIB_DEBUG         = OFF    : Use debug stdlib
    -- LYX_CONCEPT_CHECKS       = OFF    : Enable concept-checks

    # MSVC specific
    -- LYX_CONSOLE              = ON     : Show console on Windows
    -- LYX_VLD                  = OFF    : Use VLD with MSVC
    -- LYX_WALL                 = OFF    : Enable all warnings
    -- LYX_CONFIGURE_CHECKS     = OFF    : Also run configure checks for MSVC
    -- LYX_DEPENDENCIES_DOWNLOAD= OFF    : Download precompiled 3rd party libraries for MSVC 10

    

Using the merged files build
-----------------------------

    When the option 'LYX_MERGE_FILES' is used then for each library a files 
    is generated which includes all source files of this library, this speeds 
    up compilation about factor 5.

    When you heavily work on one file you could comment out the relevant 
    define in the '_allinone_const.C' file, so only the file _allinone_touched.C'
    file will be re-compiled again an again.


    - Adding new files
      When you add new files the merging files have to be rebuild:
        cmake -DLYX_MERGE_REBUILD=1 .
      Or start over by completely  cleaning the build folder.

    - Starting over with same configuration
      Delete all files but CMakeCache.txt and call
        cmake .



Visual Studio C++
------------------
    
    * Warnings: The default warning level of the msvc cmake builds 
      is /W3. To enable /W4 use
        '-DLYX_WALL=1 '
      and 
        '-DLYX_WALL=0'
      switches back to to /W3, 
      To disable a specific warning add it to MSVC_W_DISABLE in
      cmake/CMakeLists.txt. To make the warning an error add it
      to MSVC_W_ERROR.
      
    * Memory leak detection
      For MSVC the usage of 'Visual Leak Detection' could be enabled
      (http://dmoulding.googlepages.com/vld): -DLYX_VLD=1
      
      Building vld requires the 'Debugging Tools For Windows' (~16MB)
      http://www.microsoft.com/whdc/devtools/debugging/default.mspx
      Eventually you have to copy dbghelp.dll from your Visual Studio
      Common7\IDE folder into the binary directory. 
      'vld.ini' is part of lyx.proj where you can configure vld.
      
      The docu for vld could be found in development/Win32/vld, or
      http://www.codeproject.com/tools/visualleakdetector.asp
      
      
    Some tips:

    * the Release build links much faster 
    * for the 'Debug' and 'Release' build all precompiled headers are enabled
      to compile without pch (non file merge mode) This could be used to check 
      if all necessary headers are included.
        * use 'MinSizeRel' which only precompiles the STL and Boost headers
        * use 'RelWithDebInfo' which does not use any precompiled headers



GCC/Windows (Win2k only works with MSYS, XP?)
----------------------------------------------
    QMAKESPEC is needed:
      export QMAKESPEC=win32-g++ (MSYS) or set QMAKESPEC=win32-g++ (CMD)



Ubuntu/Kubuntu
--------------_

    You need additionally these packages:
      * g++
      * cmake
      * qt4-dev-tools

      

Xcode/Mac
----------
    
    Some tips:

    * Xcode prefers UTF8 when opening source files, though LyX usually uses
      Latin1. To fix that select all source files in Xcode and click "Get Info"
      in the context menu. Change the encoding to Latin1.

    * You can run and debug LyX from Xcode. For LyX to find its resources, there
      are two possibilities:
        a) Put a resource directory, e.g. a link to the lib directory of the 
           source tree, at .../trunk/../build/bin/Resources
        b) Select the lyx-qt4 executable in Xcode, click on "Get Info" in the 
           context menu and add "-sysdir a_valid_LyX_resource_directory" 
           pointing e.g. to a valid Contents/Resources of a LyX.app directory.

    * LyX on Mac doesn't look for fonts in the resource directory if the
      executable is not in an .app bundle. Instead you have to create a
      symbolic link to the fonts directory in the place where the executable
      is: ln -s .../trunk/lib/fonts .../trunk/../build/bin/Debug/
      If you don't do that math character will not show up correctly.

    * CMake properly finds the Qt4 library bundles from Trolltech's binary
      Qt4 package for Mac. So no need to compile Qt on your own.



Packaging
----------

    - Source .tar.gz, .tar.bz2, .zip:
        make package_source

    - Binary .tar.gz and install .sh:
        make package

    - Binary .deb:
        create : cpack -G DEB --config CPackConfig.cmake
        list   : dpkg-deb -c lyx-*.deb
        install: dpkg -i lyx-*.deb

    - Binary .rpm:
        create : cpack -G RPM --config CPackConfig.cmake
        list   : rpm -qlp lyx-*.rpm              
        install: rpm -U lyx-*.rpm   

