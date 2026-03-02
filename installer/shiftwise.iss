; ===========================================================================
;  ShiftWise 1.0.0 — Inno Setup 6 Installer Script
;  Walking Fish Software
;
;  Requirements:
;    Inno Setup 6  https://jrsoftware.org/isinfo.php
;
;  How to compile:
;    Option A — Run deploy.bat in the project root (fully automated)
;    Option B — Open this file in the Inno Setup IDE and press F9
;
;  deploy.bat must be run first to populate the ..\deploy\ folder.
; ===========================================================================

#define AppName      "ShiftWise"
#define AppVersion   "1.0.0"
#define AppPublisher "Walking Fish Software"
#define AppExeName   "ShiftWise.exe"
#define DeployDir    "..\deploy"

; ---------------------------------------------------------------------------
[Setup]
; AppId uniquely identifies this application in the Windows registry.
; To regenerate: Inno Setup IDE → Tools → Generate GUID
AppId={{A3F8C2D1-5E7B-4A9F-8C06-1D2E3F4A5B6C}}
AppName={#AppName}
AppVersion={#AppVersion}
AppVerName={#AppName} {#AppVersion}
AppPublisher={#AppPublisher}

; Install into C:\Program Files\Walking Fish Software\ShiftWise
DefaultDirName={autopf}\{#AppPublisher}\{#AppName}
DefaultGroupName={#AppPublisher}\{#AppName}
AllowNoIcons=yes

; Installer output
OutputDir=output
OutputBaseFilename=ShiftWise-{#AppVersion}-Setup
SetupIconFile={#DeployDir}\..\resources\icons\shiftwise.ico

; Compression
Compression=lzma2/ultra64
SolidCompression=yes

; Appearance
WizardStyle=modern

; Windows 10+ 64-bit only
MinVersion=10.0
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible

; Require admin so we can write to Program Files
PrivilegesRequired=admin

; ---------------------------------------------------------------------------
[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

; ---------------------------------------------------------------------------
[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

; ---------------------------------------------------------------------------
[Files]
; Root files — exe, Qt DLLs, MinGW runtime DLLs
Source: "{#DeployDir}\*.exe";  DestDir: "{app}"; Flags: ignoreversion
Source: "{#DeployDir}\*.dll";  DestDir: "{app}"; Flags: ignoreversion

; Qt plug-in folders (must be explicit — wildcard recursesubdirs is unreliable)
Source: "{#DeployDir}\platforms\*";         DestDir: "{app}\platforms";         Flags: ignoreversion
Source: "{#DeployDir}\sqldrivers\*";        DestDir: "{app}\sqldrivers";        Flags: ignoreversion
Source: "{#DeployDir}\styles\*";            DestDir: "{app}\styles";            Flags: ignoreversion
Source: "{#DeployDir}\imageformats\*";      DestDir: "{app}\imageformats";      Flags: ignoreversion
Source: "{#DeployDir}\iconengines\*";       DestDir: "{app}\iconengines";       Flags: ignoreversion
Source: "{#DeployDir}\networkinformation\*"; DestDir: "{app}\networkinformation"; Flags: ignoreversion
Source: "{#DeployDir}\tls\*";              DestDir: "{app}\tls";               Flags: ignoreversion

; ---------------------------------------------------------------------------
[Icons]
Name: "{group}\{#AppName}";           Filename: "{app}\{#AppExeName}"
Name: "{group}\Uninstall {#AppName}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#AppName}";     Filename: "{app}\{#AppExeName}"; Tasks: desktopicon

; ---------------------------------------------------------------------------
[Run]
Filename: "{app}\{#AppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(AppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent
