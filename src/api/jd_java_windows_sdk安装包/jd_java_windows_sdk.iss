; �ű��� Inno Setup �ű��� ���ɣ�
; �йش��� Inno Setup �ű��ļ�����ϸ��������İ����ĵ���

[Setup]
; ע: AppId��ֵΪ������ʶ��Ӧ�ó���
; ��ҪΪ������װ����ʹ����ͬ��AppIdֵ��
; (�����µ�GUID����� ����|��IDE������GUID��)
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

; ע��: ��Ҫ���κι���ϵͳ�ļ���ʹ�á�Flags: ignoreversion��

