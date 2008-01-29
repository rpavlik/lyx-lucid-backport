/*

Reinstall options

*/

#--------------------------------
#Page functions

Function PageReinstall

  # Check whether this version is already installed
  # If so, some registry key like Software/lyx15 should exist

  ReadRegStr $R0 SHELL_CONTEXT ${APP_REGKEY} "Version"

  ${if} $R0 != "${APP_VERSION}"
    Abort
  ${endif}

  !insertmacro MUI_HEADER_TEXT $(TEXT_REINSTALL_TITLE) $(TEXT_REINSTALL_SUBTITLE)
  !insertmacro MUI_INSTALLOPTIONS_DISPLAY "reinstall.ini"

FunctionEnd

Function PageReinstallValidate

  !insertmacro MUI_INSTALLOPTIONS_READ $R0 "reinstall.ini" "Field 2" "State"

  # if re-installation, hide many sections
  # FIXME: where is SelectSection defined?
  ${if} $R0 == "1"
    !insertmacro SelectSection ${SecCore}
  ${else}
    !insertmacro UnselectSection ${SecCore}
  ${endif}

FunctionEnd
