#pragma once
#include <list>



class CUtility
{

public:
	CUtility(void);
	~CUtility(void);
private:
	static bool m_bInitLog;

public:
	// �ж��Ƿ�Ϊ64λ����ϵͳ
	static BOOL IsWindows64();

	// ��ȡIE�������path
	static CString GetIEPath();

	// ��ȡ���̾�����ڵ�·��
	static CString GetModulePath(HMODULE hModule = NULL);

	// ��ȡָ��EXE·���Ľ��̾��
	static void GetProcessHandle(CString strExePath,std::list<HANDLE> &handleList);
	
	// ָ��DLLע�뵽ָ��EXE����
	static void InjectDllToExe(CString strDllPath,CString strExePath);

	// ָ��DLLע�뵽ָ�����̾��
	static bool InjectDllToProc(CString strDllPath, HANDLE targetProc);

	// ָ��DLL��ָ��EXE����ж��
	static void UninstallDllToExe(CString strDllPath,CString strExePath);

	// ָ��DLL��ָ�����̾��ж��
	static bool  UninstallDllToProc(CString strDllPath, HANDLE targetProc);



	static CStringW CUtility::A2Wstring(std::string strA);

	static std::string CUtility::W2Astring(const CString& strUnicode);

	static CString GetErrorMsg(DWORD errorCode);
};

