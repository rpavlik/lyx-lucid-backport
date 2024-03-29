/*

gui.nsh

Installer user interface settings

*/

#--------------------------------
# General

Name "${APP_NAME} ${APP_VERSION}"
BrandingText " "

#--------------------------------
# Interface settings

!define MUI_ABORTWARNING
!define MUI_ICON "${SETUP_ICON}"
!define MUI_UNICON "${SETUP_ICON}"
!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_BITMAP "${SETUP_HEADERIMAGE}"
!define MUI_HEADERIMAGE_RIGHT
!define MUI_WELCOMEFINISHPAGE_BITMAP "${SETUP_WIZARDIMAGE}"
!define MUI_UNWELCOMEFINISHPAGE_BITMAP "${SETUP_WIZARDIMAGE}"
!define MUI_COMPONENTSPAGE_NODESC

#--------------------------------
# Pages

# Installer

!define MUI_WELCOMEPAGE_TEXT $(TEXT_WELCOME)
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "${FILES_LICENSE}"
!insertmacro MULTIUSER_PAGE_INSTALLMODE

Page custom PageReinstall PageReinstallValidate
Page custom PageExternalLaTeX PageExternalLaTeXValidate

!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES

# The option to run LyX from the finish page is currently disabled because
# it may run with Administrator priviledges, therefore causing a different
# user directory to be used. This could be fixed by creating a separate
# process without UAC elevation.
# !define MUI_FINISHPAGE_RUN "$INSTDIR\${APP_RUN}"

!define MUI_FINISHPAGE_SHOWREADME
!define MUI_FINISHPAGE_SHOWREADME_NOTCHECKED
!define MUI_FINISHPAGE_SHOWREADME_FUNCTION CreateDesktopShortcut
!define MUI_FINISHPAGE_SHOWREADME_TEXT $(TEXT_FINISH_DESKTOP)
!define MUI_FINISHPAGE_LINK $(TEXT_FINISH_WEBSITE)
!define MUI_FINISHPAGE_LINK_LOCATION "http://www.lyx.org/"
!define MUI_PAGE_CUSTOMFUNCTION_SHOW CheckDesktopShortcut
!insertmacro MUI_PAGE_FINISH

# Uninstaller

!insertmacro MUI_UNPAGE_WELCOME
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_COMPONENTS
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_UNPAGE_FINISH

#--------------------------------
# Installer Languages

!macro LANG LANG_NAME
  # NSIS language file
  !insertmacro MUI_LANGUAGE "${LANG_NAME}"
  # LyX language file
  !insertmacro LANGFILE_INCLUDE_WITHDEFAULT "lang\${LANG_NAME}.nsh" "lang\English.nsh"
!macroend

!insertmacro LANG "english"
!insertmacro LANG "french"
!insertmacro LANG "german"
!insertmacro LANG "italian"

#--------------------------------
# Version information

VIProductVersion "${APP_VERSION_NUMBER}"
VIAddVersionKey /LANG=${LANG_ENGLISH} "ProductName" "${APP_NAME}"
VIAddVersionKey /LANG=${LANG_ENGLISH} "FileDescription" "${APP_INFO}"
VIAddVersionKey /LANG=${LANG_ENGLISH} "FileVersion" "${APP_VERSION}"
VIAddVersionKey /LANG=${LANG_ENGLISH} "LegalCopyright" "${APP_COPYRIGHT}"
