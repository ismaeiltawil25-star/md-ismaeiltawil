; MD Backpack Reader — Inno Setup installer script
; Version: 0.2.0

#define MyAppName "MD Backpack Reader"
#define MyAppVersion "0.3.0"
#define MyAppPublisher "MD Backpack Reader"
#define MyAppExeName "md-backpack-reader.exe"

[Setup]
AppId={{D8F3C1A2-5B7E-4A9C-8F6D-3E2B1A0C9F4E}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
OutputDir=..\..\..\..\dist\installers
OutputBaseFilename=md-backpack-reader-v0.2.0-windows-setup
SetupIconFile=..\..\..\..\assets\icons\md-backpack-reader.ico
Compression=lzma2
SolidCompression=yes
UninstallDisplayIcon={app}\md-backpack-reader.exe
PrivilegesRequired=admin
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "Create a &desktop shortcut"; GroupDescription: "Additional icons:"; Flags: checkedonce

[Files]
Source: "..\..\..\..\dist\windows\md-backpack-reader\md-backpack-reader.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\..\..\..\dist\windows\md-backpack-reader\Qt6Core.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\..\..\..\dist\windows\md-backpack-reader\Qt6Gui.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\..\..\..\dist\windows\md-backpack-reader\Qt6Network.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\..\..\..\dist\windows\md-backpack-reader\Qt6Svg.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\..\..\..\dist\windows\md-backpack-reader\Qt6Widgets.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\..\..\..\dist\windows\md-backpack-reader\D3Dcompiler_47.dll"; DestDir: "{app}"; Flags: ignoreversion

; Qt plugins — generic
Source: "..\..\..\..\dist\windows\md-backpack-reader\generic\*"; DestDir: "{app}\generic"; Flags: ignoreversion recursesubdirs createallsubdirs

; Qt plugins — iconengines
Source: "..\..\..\..\dist\windows\md-backpack-reader\iconengines\*"; DestDir: "{app}\iconengines"; Flags: ignoreversion recursesubdirs createallsubdirs

; Qt plugins — imageformats
Source: "..\..\..\..\dist\windows\md-backpack-reader\imageformats\*"; DestDir: "{app}\imageformats"; Flags: ignoreversion recursesubdirs createallsubdirs

; Qt plugins — networkinformation
Source: "..\..\..\..\dist\windows\md-backpack-reader\networkinformation\*"; DestDir: "{app}\networkinformation"; Flags: ignoreversion recursesubdirs createallsubdirs

; Qt plugins — platforms
Source: "..\..\..\..\dist\windows\md-backpack-reader\platforms\*"; DestDir: "{app}\platforms"; Flags: ignoreversion recursesubdirs createallsubdirs

; Qt plugins — styles
Source: "..\..\..\..\dist\windows\md-backpack-reader\styles\*"; DestDir: "{app}\styles"; Flags: ignoreversion recursesubdirs createallsubdirs

; Qt plugins — tls
Source: "..\..\..\..\dist\windows\md-backpack-reader\tls\*"; DestDir: "{app}\tls"; Flags: ignoreversion recursesubdirs createallsubdirs

; Qt translations
Source: "..\..\..\..\dist\windows\md-backpack-reader\translations\*"; DestDir: "{app}\translations"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{group}\Uninstall {#MyAppName}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "Launch {#MyAppName}"; Flags: postinstall nowait skipifsilent shellexec
