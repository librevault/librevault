; Librevault Inno Setup script. Requires Inno Setup 5.5.9.

[Setup]
AppName=Librevault
AppVerName=Librevault
AppId=com.librevault.desktop
AppVersion=@librevault_VERSION@
AppPublisher=Librevault Team
AppPublisherURL=https://librevault.com
VersionInfoVersion=@librevault_VERSION@

DisableWelcomePage=no
DisableProgramGroupPage=yes

WizardSmallImageFile=smallimage.bmp

DefaultDirName={userpf}\Librevault

SetupIconFile=librevault.ico
UninstallDisplayIcon={app}\librevault-gui.exe
OutputDir=.
PrivilegesRequired=lowest
ShowLanguageDialog=no

SignTool=signtool
; librevault-daemon is used to stuck sometimes on Windows
CloseApplications=force

[Languages]
Name: "en"; MessagesFile: "compiler:Default.isl"
Name: "ru"; MessagesFile: "compiler:Languages\Russian.isl"

[Dirs]
Name: "{app}\x32"
Name: "{app}\x64"

[Files]
Source: "release\*"; Excludes: "*.exe"; DestDir: "{app}"; Flags: recursesubdirs ignoreversion
Source: "release\*.exe"; DestDir: "{app}"; Flags: signonce recursesubdirs ignoreversion

[Icons]
Name: "{commonprograms}\Librevault"; Filename: "{app}\librevault-gui.exe"
Name: "{commondesktop}\Librevault"; Filename: "{app}\librevault-gui.exe"

[Registry]
Root: HKCU; Subkey: "Software\Librevault"; Flags: uninsdeletekey
; Autostart
Root: HKCU; Subkey: "Software\Microsoft\Windows\CurrentVersion\Run"; ValueType: string; ValueName: "Librevault"; ValueData: "{app}\librevault-gui.exe"; Flags: uninsdeletevalue
; "lvlt:" protocol association
Root: HKCU; Subkey: "Software\Classes\lvlt"; ValueType: string; ValueName: ""; ValueData: "URL:Librevault Protocol"; Flags: uninsdeletekey
Root: HKCU; Subkey: "Software\Classes\lvlt"; ValueType: string; ValueName: "URL Protocol"; ValueData: ""
Root: HKCU; Subkey: "Software\Classes\lvlt\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "librevault-gui.exe"
Root: HKCU; Subkey: "Software\Classes\lvlt\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\librevault-gui.exe"" ""%1"""

[Run]
Filename: {app}\librevault-gui.exe; Description: "Start Librevault"; Flags: nowait postinstall
