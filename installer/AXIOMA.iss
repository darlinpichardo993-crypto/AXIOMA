; Instalador de AXIOMA - Made By ORYMY
[Setup]
AppId={{8F3B5E1A-6C2D-4A57-9B0E-A10000000001}
AppName=AXIOMA
AppVersion=1.0.0
AppPublisher=ORYMY
DefaultDirName={autopf}\ORYMY\AXIOMA
DefaultGroupName=ORYMY
DisableProgramGroupPage=yes
OutputDir=..\dist
OutputBaseFilename=setup
Compression=lzma2
SolidCompression=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
WizardStyle=modern
UninstallDisplayName=AXIOMA (Made By ORYMY)

[Files]
; Plugin VST3 -> carpeta estándar de VST3 (FL Studio, Ableton, Reaper, etc.)
Source: "..\build\AXIOMA_artefacts\Release\VST3\AXIOMA.vst3\*"; DestDir: "{commoncf64}\VST3\AXIOMA.vst3"; Flags: recursesubdirs createallsubdirs ignoreversion
; Versión independiente (opcional)
Source: "..\build\AXIOMA_artefacts\Release\Standalone\AXIOMA.exe"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\AXIOMA (Standalone)"; Filename: "{app}\AXIOMA.exe"
Name: "{group}\Desinstalar AXIOMA"; Filename: "{uninstallexe}"

[Run]
Filename: "{app}\AXIOMA.exe"; Description: "Abrir AXIOMA"; Flags: nowait postinstall skipifsilent unchecked
