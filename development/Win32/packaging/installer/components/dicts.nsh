/*

Aspell dictionaries

*/

#--------------------------------
#Macros

!macro Dictionary FILE LANGNAME LANGCODE SIZE

  Section /o "${LANGNAME}"
  
    AddSize ${SIZE}
  
    Push ${LANGCODE}
    Push "${LANGNAME}"
    Push ${FILE}
    
    Call DownloadDictionary
    
  SectionEnd
  
!macroend

#--------------------------------
#Sections

Section -AspellDicts

  Call GetAspellHive
  Pop $AspellHive

SectionEnd

!define DICT '!insertmacro Dictionary'

${DICT} af-0.50-0 "Afrikaans" af 915
${DICT} ar-1.2-0 "Arabic" ar 4352
${DICT} nb-0.50.1-0 "Bokm�l" nb 3031
${DICT} bg-4.0-0 "Bulgarian" bg 5345
${DICT} ca-20040130-1 "Catal�" ca 1095
${DICT} cs-20040614-1 "Ce�tina" cs 2069
${DICT} da-1.4.42-1 "Dansk" da 3036
${DICT} de-20030222-1 "Deutsch" de 2872
${DICT} en-6.0-0 "English" en 1130
${DICT} es-0.50-2 "Espa�ol" es 2953
${DICT} eo-0.50-2 "Esperanto" eo 3773
${DICT} et-0.1.21-1 "Estonian" et 922
${DICT} fr-0.50-3 "Fran�ais" fr 4331
${DICT} gl-0.50-0 "Galego" gl 2974
${DICT} el-0.50-3 "Greek" el 3204
${DICT} he-1.0-0 "Hebrew" he 2024
${DICT} hr-0.51-0 "Hrvatski" hr 1967
${DICT} is-0.51.1-0 "�slenska" is 1536
${DICT} it-2.2_20050523-0 "Italiano" it 696
${DICT} hu-0.99.4.2-0 "Magyar" hu 1608
${DICT} nl-0.50-2 "Nederlands" nl 1526
${DICT} nn-0.50.1-1 "Nynorsk" nn 1930
${DICT} fa-0.11-0 "Persian" fa 1667
${DICT} pl-6.0_20061121-0 "Polski" pl 1859
${DICT} pt_PT-20070510-0 "Portugu�s" pt_PT 319
${DICT} pt_BR-20070411-0 "Portugu�s do Brasil" pt_BR 2453
${DICT} ro-0.50-2 "Rom�na" ro 341
${DICT} ru-0.99f7-1 "Russian" ru 1458
${DICT} gd-0.1.1-1 "Scottish" gd 194
${DICT} sr-0.02 "Serbian" sr 3686
${DICT} sk-0.52-0 "Slovencina" sk 4602
${DICT} sl-0.50-0 "Sloven�cina" sl 5236
${DICT} fi-0.7-0 "Suomi" fi 659
${DICT} sv-0.51-0 "Svenska" sv 1029
${DICT} tr-0.50-0 "T�rk�e" tr 547
${DICT} uk-1.4.0-0 "Ukrainian" uk 770
${DICT} cy-0.50-3 "Welsh" cy 1771

#--------------------------------
#Functions

Function GetAspellHive

  #Check whether the system or local version of Aspell should be used
  #The patched Aspell uses the same logic

  Push $R0

  ReadRegStr $R0 HKCU "Software\Aspell" "Base Path"

  ${if} $R0 == ""
    StrCpy $R0 HKLM
  ${else}
    StrCpy $R0 HKCU
  ${endif}

  Exch $R0

FunctionEnd

Function DownloadDictionary

  Exch $R0
  Exch 1
  Exch $R1
  Exch 2
  Exch $R2
  Push $R3

  dict_download:

    !insertmacro DownloadFile $R3 ASPELLDICTS aspell6-$R0.exe /aspell6-$R0.exe

    ${if} $R3 != "OK"
      #Download failed
      MessageBox MB_YESNO|MB_ICONEXCLAMATION "$(TEXT_DOWNLOAD_FAILED_DICT) ($R3)" IDYES dict_download
      Goto dict_noinstall
    ${endif}

    install_dict:

      ExecWait '"$PLUGINSDIR\aspell6-$R0.exe" /NoDirChange /AutoClose'

      ${if} $AspellHive == HKLM
        ReadRegStr $R3 HKLM "Software\Aspell\Dictionaries" $R2
      ${else}
  	    ReadRegStr $R3 HKCU "Software\Aspell\Dictionaries" $R2
      ${endif}

	    ${if} $R3 == ""
        MessageBox MB_YESNO|MB_ICONEXCLAMATION "$(TEXT_NOTINSTALLED_DICT)" IDYES install_dict
      ${endif}

      Delete "$PLUGINSDIR\aspell6-$R0.exe"

    dict_noinstall:

  Pop $R3
  Pop $R2
  Pop $R1
  Pop $R0

FunctionEnd
