; -- Example2.iss --
; Same as Example1.iss, but creates its icon in the Programs folder of the
; Start Menu instead of in a subfolder, and also creates a desktop icon.

; SEE THE DOCUMENTATION FOR DETAILS ON CREATING .ISS SCRIPT FILES!

[Setup]
AppName=Librevault
AppVerName=Librevault
AppId=com.librevault.desktop
AppVersion=1.5
AppPublisher=Librevault Team
AppPublisherURL=https://librevault.com

DisableWelcomePage=no
DisableProgramGroupPage=yes

WizardSmallImageFile=smallimage.bmp

DefaultDirName={userappdata}\Librevault

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
Root: HKCU; Subkey: "Software\Microsoft\Windows\CurrentVersion\Run"; ValueType: string; ValueName: "Librevault"; ValueData: "{app}"; Flags: uninsdeletekey

[Run]
Filename: {app}\librevault-gui.exe; Flags: nowait postinstall