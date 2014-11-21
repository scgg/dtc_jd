; 脚本由 Inno Setup 脚本向导 生成！
; 有关创建 Inno Setup 脚本文件的详细资料请查阅帮助文档！

[Setup]
; 注: AppId的值为单独标识该应用程序。
; 不要为其他安装程序使用相同的AppId值。
; (生成新的GUID，点击 工具|在IDE中生成GUID。)
AppId={{26215840-E5AA-4ECE-8A65-00015742A722}
AppName=jd_java_windows_sdk
AppVersion=1.1
;AppVerName=jd_java_windows_sdk 1.1
AppPublisher=jd
DefaultDirName={pf}\jd_java_windows_sdk
DefaultGroupName=jd_java_windows_sdk
OutputBaseFilename=setup
Compression=lzma
SolidCompression=yes
ArchitecturesInstallIn64BitMode=x64
[Languages]
Name: "chinesesimp"; MessagesFile: "compiler:Default.isl"

[Files]
Source: "E:\vs_test\ttc_4-3-4\src\api\windows_api\Debug\_dtc_java_api.dll"; DestDir: "{sys}"; Flags: 32bit;Check:not Is64BitInstallMode
Source: "E:\vs_test\ttc_4-3-4\src\api\windows_api\Debug\_dtc_java_api.dll"; DestDir: "{win}\SysWOW64"; Flags: 32bit;Check:Is64BitInstallMode
Source: "E:\vs_test\ttc_4-3-4\src\api\windows_api\x64\Debug\_dtc_java_api.dll"; DestDir: "{win}\System32";Check:Is64BitInstallMode; Flags: 64bit;

; 注意: 不要在任何共享系统文件上使用“Flags: ignoreversion”

