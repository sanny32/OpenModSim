#--------------------------------
# Includes

  !include "MUI2.nsh"
  !include "LogicLib.nsh"
  !include "FileFunc.nsh"

#--------------------------------
# Custom defines
  !define MUI_ICON ${ICON_FILE}
  !define NAME "Open ModSim"
  !define PUBLISHER "Alexandr Ananev"
  !define APPFILE "omodsim.exe"
  !define ICOFILE "omodsim.exe"
  !define PACKAGENAME "omodsim_${VERSION}_x64"
  !define SLUG "${NAME} v${VERSION}"
  !define UPDATEURL "https://github.com/sanny32/OpenModSim/releases"

#--------------------------------
# General

  Name "${NAME} v${VERSION}"
  OutFile "${OUTPUT_FILE}"
  InstallDir "$PROGRAMFILES64\${NAME}"
  RequestExecutionLevel admin

#--------------------------------
# Pages
  
  # Installer pages
  !insertmacro MUI_PAGE_LICENSE ${LICENSE_FILE}
  !insertmacro MUI_PAGE_DIRECTORY
  !insertmacro MUI_PAGE_INSTFILES

  # Uninstaller pages
  !insertmacro MUI_UNPAGE_CONFIRM
  !insertmacro MUI_UNPAGE_INSTFILES
  
  # Set UI language
  !insertmacro MUI_LANGUAGE "English"

#-------------------------------- 
# Installer Sections

  Section
    SetOutPath $INSTDIR
    File /r "${BUILD_PATH}\*"
    WriteUninstaller $INSTDIR\uninstall.exe
  SectionEnd

  Section "Write Registry Information"
    # Registry information for add/remove programs
    WriteRegStr HKLM64 "Software\Microsoft\Windows\CurrentVersion\Uninstall\${NAME}" "DisplayName" "${NAME}"
    WriteRegStr HKLM64 "Software\Microsoft\Windows\CurrentVersion\Uninstall\${NAME}" "UninstallString" "$\"$INSTDIR\uninstall.exe$\""
    WriteRegStr HKLM64 "Software\Microsoft\Windows\CurrentVersion\Uninstall\${NAME}" "QuietUninstallString" "$\"$INSTDIR\uninstall.exe$\" /S"
    WriteRegStr HKLM64 "Software\Microsoft\Windows\CurrentVersion\Uninstall\${NAME}" "InstallLocation" "$\"$INSTDIR$\""
    WriteRegStr HKLM64 "Software\Microsoft\Windows\CurrentVersion\Uninstall\${NAME}" "DisplayIcon" "$\"$INSTDIR\${ICOFILE}$\""
    WriteRegStr HKLM64 "Software\Microsoft\Windows\CurrentVersion\Uninstall\${NAME}" "Publisher" "${PUBLISHER}"
    WriteRegStr HKLM64 "Software\Microsoft\Windows\CurrentVersion\Uninstall\${NAME}" "URLUpdateInfo" "${UPDATEURL}"
    WriteRegStr HKLM64 "Software\Microsoft\Windows\CurrentVersion\Uninstall\${NAME}" "DisplayVersion" "${VERSION}"
    WriteRegDWORD HKLM64 "Software\Microsoft\Windows\CurrentVersion\Uninstall\${NAME}" "VersionMajor" ${VERSIONMAJOR}
    WriteRegDWORD HKLM64 "Software\Microsoft\Windows\CurrentVersion\Uninstall\${NAME}" "VersionMinor" ${VERSIONMINOR}
    # There is no option for modifying or repairing the install
    WriteRegDWORD HKLM64 "Software\Microsoft\Windows\CurrentVersion\Uninstall\${NAME}" "NoModify" 1
    WriteRegDWORD HKLM64 "Software\Microsoft\Windows\CurrentVersion\Uninstall\${NAME}" "NoRepair" 1
    # Set the estimated eize so Add/Remove Programs can accurately report the size
    ${GetSize} "$INSTDIR" "/S=0K" $0 $1 $2
    IntFmt $0 "0x%08X" $0
	  WriteRegDWORD HKLM64 "Software\Microsoft\Windows\CurrentVersion\Uninstall\${NAME}" "EstimatedSize" "$0"
  SectionEnd

 Section "Visual Studio Runtime"
    SetOutPath "$INSTDIR"
    ExecWait "$INSTDIR\vc_redist.x64.exe /install /quiet"
 SectionEnd

#--------------------------------
# Section - Shortcut

  Section "Desktop Shortcut" DeskShort
      CreateShortCut "$DESKTOP\${NAME}.lnk" "$INSTDIR\${APPFILE}" "" "$INSTDIR\${ICOFILE}"
  SectionEnd

  Section "Start Menu"
      CreateDirectory "$SMPROGRAMS\${NAME}"
	    CreateShortCut "$SMPROGRAMS\${NAME}\${NAME}.lnk" "$INSTDIR\${APPFILE}" "" "$INSTDIR\${ICOFILE}"
  SectionEnd
#--------------------------------
# Remove empty parent directories

  Function un.RMDirUP
    !define RMDirUP '!insertmacro RMDirUPCall'

    !macro RMDirUPCall _PATH
          push '${_PATH}'
          Call un.RMDirUP
    !macroend

    # $0 - current folder
    ClearErrors

    Exch $0
    # DetailPrint "ASDF - $0\.."
    RMDir "$0\.."

    IfErrors Skip
    ${RMDirUP} "$0\.."
    Skip:

    Pop $0

  FunctionEnd

#--------------------------------
# Section - Uninstaller

Section "Uninstall"
  # Delete Shortcut
  Delete "$DESKTOP\${NAME}.lnk"

# Delete Start Menu Folder
  RMDir /r "$SMPROGRAMS\${NAME}"

  # Delete Uninstall
  Delete "$INSTDIR\uninstall.exe"

  # Delete Folder
  RMDir /r "$INSTDIR"
  ${RMDirUP} "$INSTDIR"

  # Remove uninstaller information from the registry
	DeleteRegKey HKLM64 "Software\Microsoft\Windows\CurrentVersion\Uninstall\${NAME}"
  DeleteRegKey HKCU64 "Software\${NAME}"
SectionEnd