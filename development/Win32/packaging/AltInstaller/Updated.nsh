Function UpdateModifiedFiles

 # list with modified files for LyX 1.6.4
 SetOutPath "$INSTDIR\bin" # always here
 File "${PRODUCT_SOURCEDIR}\bin\lyx.exe" # always here
 File "${PRODUCT_SOURCEDIR}\bin\tex2lyx.exe" # always here
 File "${PRODUCT_SOURCEDIR}\bin\QtCore4.dll"
 File "${PRODUCT_SOURCEDIR}\bin\QtGui4.dll"
 SetOutPath "$INSTDIR\Resources" # always here
 File /r "${PRODUCT_SOURCEDIR}\Resources\locale" # always here
 File "${PRODUCT_SOURCEDIR}\Resources\chkconfig.ltx"
 File "${PRODUCT_SOURCEDIR}\Resources\configure.py"
 File "${PRODUCT_SOURCEDIR}\Resources\symbols"
 SetOutPath "$INSTDIR\Resources\bind"
 File "${PRODUCT_SOURCEDIR}\Resources\bind\cua.bind"
 File "${PRODUCT_SOURCEDIR}\Resources\bind\mac.bind"
 SetOutPath "$INSTDIR\Resources\doc"
 File /r "${PRODUCT_SOURCEDIR}\Resources\doc\ca"
 File /r "${PRODUCT_SOURCEDIR}\Resources\doc\da"
 File /r "${PRODUCT_SOURCEDIR}\Resources\doc\he"
 File /r "${PRODUCT_SOURCEDIR}\Resources\doc\nb"
 File /r "${PRODUCT_SOURCEDIR}\Resources\doc\ro"
 File /r "${PRODUCT_SOURCEDIR}\Resources\doc\sk"
 File /r "${PRODUCT_SOURCEDIR}\Resources\doc\uk"
 File "${PRODUCT_SOURCEDIR}\Resources\doc\Customization.lyx"
 File "${PRODUCT_SOURCEDIR}\Resources\doc\EmbeddedObjects.lyx"
 File "${PRODUCT_SOURCEDIR}\Resources\doc\Extended.lyx"
 File "${PRODUCT_SOURCEDIR}\Resources\doc\Intro.lyx"
 File "${PRODUCT_SOURCEDIR}\Resources\doc\LaTeXConfig.lyx"
 File "${PRODUCT_SOURCEDIR}\Resources\doc\LFUNs.lyx"
 File "${PRODUCT_SOURCEDIR}\Resources\doc\Math.lyx"
 File "${PRODUCT_SOURCEDIR}\Resources\doc\UserGuide.lyx"
 SetOutPath "$INSTDIR\Resources\doc\de"
 File "${PRODUCT_SOURCEDIR}\Resources\doc\de\Customization.lyx"
 File "${PRODUCT_SOURCEDIR}\Resources\doc\de\EmbeddedObjects.lyx"
 File "${PRODUCT_SOURCEDIR}\Resources\doc\de\Extended.lyx"
 File "${PRODUCT_SOURCEDIR}\Resources\doc\de\Intro.lyx"
 File "${PRODUCT_SOURCEDIR}\Resources\doc\de\Math.lyx"
 File "${PRODUCT_SOURCEDIR}\Resources\doc\de\Tutorial.lyx"
 File "${PRODUCT_SOURCEDIR}\Resources\doc\de\UserGuide.lyx"
 SetOutPath "$INSTDIR\Resources\doc\es"
 File "${PRODUCT_SOURCEDIR}\Resources\doc\es\Customization.lyx"
 File "${PRODUCT_SOURCEDIR}\Resources\doc\es\EmbeddedObjects.lyx"
 File "${PRODUCT_SOURCEDIR}\Resources\doc\es\Intro.lyx"
 File "${PRODUCT_SOURCEDIR}\Resources\doc\es\Math.lyx"
 File "${PRODUCT_SOURCEDIR}\Resources\doc\es\UserGuide.lyx"
 SetOutPath "$INSTDIR\Resources\doc\eu"
 File "${PRODUCT_SOURCEDIR}\Resources\doc\eu\Customization.lyx"
 File "${PRODUCT_SOURCEDIR}\Resources\doc\eu\Intro.lyx"
 SetOutPath "$INSTDIR\Resources\doc\fr"
 File "${PRODUCT_SOURCEDIR}\Resources\doc\fr\Customization.lyx"
 File "${PRODUCT_SOURCEDIR}\Resources\doc\fr\EmbeddedObjects.lyx"
 File "${PRODUCT_SOURCEDIR}\Resources\doc\fr\Intro.lyx"
 File "${PRODUCT_SOURCEDIR}\Resources\doc\fr\Math.lyx"
 File "${PRODUCT_SOURCEDIR}\Resources\doc\fr\Shortcuts.lyx"
 File "${PRODUCT_SOURCEDIR}\Resources\doc\fr\Tutorial.lyx"
 File "${PRODUCT_SOURCEDIR}\Resources\doc\fr\UserGuide.lyx"
 SetOutPath "$INSTDIR\Resources\doc\gl"
 File "${PRODUCT_SOURCEDIR}\Resources\doc\gl\Intro.lyx"
 SetOutPath "$INSTDIR\Resources\doc\hu"
 File "${PRODUCT_SOURCEDIR}\Resources\doc\hu\Intro.lyx"
 SetOutPath "$INSTDIR\Resources\doc\id"
 File "${PRODUCT_SOURCEDIR}\Resources\doc\it\Customization.lyx"
 File "${PRODUCT_SOURCEDIR}\Resources\doc\it\Intro.lyx"
 SetOutPath "$INSTDIR\Resources\doc\it"
 File "${PRODUCT_SOURCEDIR}\Resources\doc\id\Intro.lyx"
 SetOutPath "$INSTDIR\Resources\doc\ja"
 File "${PRODUCT_SOURCEDIR}\Resources\doc\ja\Intro.lyx"
 SetOutPath "$INSTDIR\Resources\doc\nl"
 File "${PRODUCT_SOURCEDIR}\Resources\doc\nl\Intro.lyx"
 SetOutPath "$INSTDIR\Resources\doc\pl"
 File "${PRODUCT_SOURCEDIR}\Resources\doc\pl\Intro.lyx"
 SetOutPath "$INSTDIR\Resources\doc\pt"
 File "${PRODUCT_SOURCEDIR}\Resources\doc\pt\Intro.lyx"
 SetOutPath "$INSTDIR\Resources\doc\ru"
 File "${PRODUCT_SOURCEDIR}\Resources\doc\ru\Intro.lyx"
 SetOutPath "$INSTDIR\Resources\doc\sl"
 File "${PRODUCT_SOURCEDIR}\Resources\doc\sl\Intro.lyx"
 SetOutPath "$INSTDIR\Resources\doc\sv"
 File "${PRODUCT_SOURCEDIR}\Resources\doc\sv\Intro.lyx"
 SetOutPath "$INSTDIR\Resources\examples"
 File "${PRODUCT_SOURCEDIR}\Resources\examples\achemso.lyx"
 File /r "${PRODUCT_SOURCEDIR}\Resources\images"
 SetOutPath "$INSTDIR\Resources\layouts"
 File "${PRODUCT_SOURCEDIR}\Resources\layouts\amsbook.layout"
 File "${PRODUCT_SOURCEDIR}\Resources\layouts\amsart.layout"
 File "${PRODUCT_SOURCEDIR}\Resources\layouts\article-beamer.layout"
 File "${PRODUCT_SOURCEDIR}\Resources\layouts\theorems.inc"
 File "${PRODUCT_SOURCEDIR}\Resources\layouts\theorems-ams.module"
 File "${PRODUCT_SOURCEDIR}\Resources\layouts\theorems-chap.module"
 File "${PRODUCT_SOURCEDIR}\Resources\layouts\theorems-sec.module"
 SetOutPath "$INSTDIR\Resources\lyx2lyx" # always here
 File "${PRODUCT_SOURCEDIR}\Resources\lyx2lyx\lyx2lyx_version.py" # always here
 File "${PRODUCT_SOURCEDIR}\Resources\lyx2lyx\lyx_1_5.py"
 File "${PRODUCT_SOURCEDIR}\Resources\lyx2lyx\lyx_1_6.py"
 SetOutPath "$INSTDIR\Resources\scripts"
 File "${PRODUCT_SOURCEDIR}\Resources\scripts\layout2layout.py"
 SetOutPath "$INSTDIR\Resources\ui"
 File "${PRODUCT_SOURCEDIR}\Resources\ui\stdcontext.inc"
 File "${PRODUCT_SOURCEDIR}\Resources\ui\stdtoolbars.inc"
 File "${PRODUCT_SOURCEDIR}\Resources\ui\stdmenus.inc"

FunctionEnd

