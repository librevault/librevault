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

[Languages]
Name: "en"; MessagesFile: "compiler:Default.isl"
Name: "ru"; MessagesFile: "compiler:Languages\Russian.isl"

[Dirs]
Name: "{app}\x32"
Name: "{app}\x64"

[Files]
Source: "release\*"; DestDir: "{app}"; Flags: recursesubdirs

[Icons]
Name: "{commonprograms}\Librevault"; Filename: "{app}\librevault-gui.exe"
Name: "{commondesktop}\Librevault"; Filename: "{app}\librevault-gui.exe"

[Registry]
Root: HKCU; Subkey: "Software\Librevault"; Flags: uninsdeletekey
; Autostart
Root: HKCU; Subkey: "Software\Microsoft\Windows\CurrentVersion\Run"; ValueType: string; ValueName: "Librevault"; ValueData: "{app}\librevault-gui.exe"; Flags: uninsdeletekey

[Run]
Filename: {app}\librevault-gui.exe; Flags: nowait postinstall