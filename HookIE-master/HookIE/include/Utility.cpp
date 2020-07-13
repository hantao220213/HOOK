#include "StdAfx.h"

#include "Utility.h"
#include <TlHelp32.h>
#include <string> // std::locale��Ҫ
#include <strsafe.h> // StringCchPrintf ��Ҫ
#include <Windows.h>

// GetModuleFileNameEx ��Ҫ���漸��
#ifndef PSAPI_VERSION
#define PSAPI_VERSION 1
#endif

#include <Psapi.h>  
#pragma comment (lib,"Psapi.lib")  

bool CUtility::m_bInitLog = false;

CUtility::CUtility(void)
{
}


CUtility::~CUtility(void)
{
}

BOOL CUtility::IsWindows64()
{
	typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);
	LPFN_ISWOW64PROCESS fnIsWow64Process = (LPFN_ISWOW64PROCESS)::GetProcAddress(GetModuleHandle(_T("kernel32")), "IsWow64Process");
	BOOL bIsWow64 = FALSE;
	if (fnIsWow64Process)
		if (!fnIsWow64Process(::GetCurrentProcess(), &bIsWow64))
			bIsWow64 = FALSE;
	return bIsWow64;
}

CString CUtility::GetIEPath()
{
	TCHAR szPath[MAX_PATH];
	TCHAR *strLastSlash = NULL;
	GetSystemDirectoryW(szPath, sizeof(szPath) );
	szPath[MAX_PATH - 1] = 0;
	strLastSlash = wcschr( szPath, L'\\' );
	*strLastSlash = 0;
	if ( IsWindows64() )
	{
		wcscat_s( szPath,L"\\program files (x86)\\internet explorer\\iexplore.exe" );
	}
	else
	{
		wcscat_s( szPath,L"\\program files\\internet explorer\\iexplore.exe" );
	}
	return CString(szPath);
}

CString CUtility::GetModulePath(HMODULE hModule)
{
	TCHAR buf[MAX_PATH] = {'\0'};
	CString strDir, strTemp;

	::GetModuleFileName( hModule, buf, MAX_PATH);
	strTemp = buf;
	strDir = strTemp.Left( strTemp.ReverseFind('\\') + 1 );
	return strDir;
}

void CUtility::GetProcessHandle(CString strExePath,std::list<HANDLE>& handleList)
{
	CString exeName ;
	int index= strExePath.ReverseFind('\\');
	exeName = strExePath.Right(strExePath.GetLength()-index-1);

	HANDLE snapHandele = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,NULL);
	if( INVALID_HANDLE_VALUE == snapHandele)
	{
		return;
	}
	PROCESSENTRY32 entry = {0};
	entry.dwSize = sizeof(entry);// ���ȱ��븳ֵ
	BOOL bRet = Process32First(snapHandele,&entry);
	CString  exeTempName;
	while (bRet) 
	{
		exeTempName = (entry.szExeFile);
		if( exeTempName.CompareNoCase(exeName) ==0 )
		{
			HANDLE procHandle=OpenProcess(PROCESS_ALL_ACCESS,FALSE,entry.th32ProcessID);  
			TCHAR exePath[MAX_PATH] = {0};
			if(procHandle)
			{
				if( GetModuleFileNameEx(procHandle,NULL,exePath,MAX_PATH) )
				{
					// ȫ·����ȡ��
					if(CString(exePath).CompareNoCase(strExePath) == 0)
					{
						// ���̾���ҵ�
						handleList.push_back(procHandle);
					}
					else
					{
						CloseHandle(procHandle);
					}
				}
				else
				{
					CloseHandle(procHandle);
				}
				
			}
		}
		bRet = Process32Next(snapHandele,&entry);
	}
	CloseHandle(snapHandele);
	return;
}

void CUtility::InjectDllToExe(CString strDllPath,CString strExePath)
{
	std::list<HANDLE> handleList;
	GetProcessHandle(strExePath,handleList);
	HANDLE targetProc = NULL;

	// ��ȡ����ÿ��EXE���̾��������DLLע��
	for(std::list<HANDLE>::iterator it = handleList.begin(); it != handleList.end(); it++)
	{
		targetProc = *it;
		bool ret = InjectDllToProc(strDllPath, targetProc);
		CloseHandle(targetProc);
		if(ret == false)
		{
			CString temp = _T("CUtility::InjectDllToExe");
			temp.AppendFormat(_T("handle:%d false\n"),targetProc);
			TRACE(temp);
		}
		
	}
	return;
}

bool CUtility::InjectDllToProc(CString strDllPath, HANDLE targetProc)
{
	if(targetProc == NULL)
	{
		return false;
	}
	/*
	ע��DLL��˼·���裺
	1. ��Ŀ�����������һ���ڴ�ռ�(ʹ��VirtualAllocEx����) ���DLL��·�����������ִ��LoadLibraryA
	2. ��DLL·��д�뵽Ŀ�����(ʹ��WriteProcessMemory����)
	3. ��ȡLoadLibraryA������ַ(ʹ��GetProcAddress)��������Ϊ�̵߳Ļص�����
	4. ��Ŀ����� �����̲߳�ִ��(ʹ��CreateRemoteThread)
	*/

	std::string temp = W2Astring(strDllPath);
	int dllLen = temp.size();
	const char* pPath = temp.c_str();
	// 1.Ŀ���������ռ�
	LPVOID pDLLPath = VirtualAllocEx(targetProc,NULL,dllLen,MEM_COMMIT,PAGE_READWRITE );
	if( pDLLPath == NULL )
	{
		TRACE(_T("CUtility::InjectDllToProc VirtualAllocEx failed\n"));
		return false;
	}
	SIZE_T wLen = 0;
	// 2.��DLL·��д��Ŀ������ڴ�ռ�
	int ret = WriteProcessMemory(targetProc,pDLLPath,pPath,dllLen,&wLen); // ����pPath����ֱ��ʹ��strDllPath
	if( ret == 0 )
	{
		VirtualFreeEx(targetProc, pDLLPath, dllLen, MEM_DECOMMIT);
		TRACE(_T("CUtility::InjectDllToProc WriteProcessMemory failed\n"));
		return false;
	}
	// 3.��ȡLoadLibraryA������ַ
	FARPROC myLoadLibrary = GetProcAddress(GetModuleHandleA("kernel32.dll"),"LoadLibraryA");
	if( myLoadLibrary == NULL )
	{
		VirtualFreeEx(targetProc, pDLLPath, dllLen, MEM_DECOMMIT);
		TRACE(_T("CUtility::InjectDllToProc GetProcAddress failed\n"));
		return false;
	}
	// 4.��Ŀ�����ִ��LoadLibrary ע��ָ�����߳�
	HANDLE tHandle = CreateRemoteThread(targetProc,NULL,NULL,
		(LPTHREAD_START_ROUTINE)myLoadLibrary,pDLLPath,NULL,NULL);
	if(tHandle == NULL)
	{
		VirtualFreeEx(targetProc, pDLLPath, dllLen, MEM_DECOMMIT);
		TRACE(_T("CUtility::InjectDllToProc CreateRemoteThread failed\n"));
		return false;
	}
	WaitForSingleObject(tHandle,INFINITE);
	VirtualFreeEx(targetProc, pDLLPath, dllLen, MEM_DECOMMIT);
	CloseHandle(tHandle);
	return true;
}

void CUtility::UninstallDllToExe(CString strDllPath,CString strExePath)
{
	std::list<HANDLE> handleList;
	GetProcessHandle(strExePath,handleList);
	HANDLE targetProc = NULL;

	// ��ȡ����ÿ��EXE���̾��������DLLж��
	for(std::list<HANDLE>::iterator it = handleList.begin(); it != handleList.end(); it++)
	{
		targetProc = *it;
		bool ret = UninstallDllToProc(strDllPath, targetProc);
		CloseHandle(targetProc);
		if(ret == false)
		{
			CString temp = _T("CUtility::UninstallDllToExe");
			temp.Format(_T("handle:%d false\n"),targetProc);
			TRACE(temp);
		}
		
	}
	return;
}

bool CUtility::UninstallDllToProc(CString strDllPath, HANDLE targetProc)
{
    /*
    ж�ز����ע��DLL����ʵ�ʲ��.
    ע��DLL�� ��Ŀ�������ִ��LoadLibraryA
    ж��DLL�� ��Ŀ�������ִ��FreeLibrary��������ͬ����ж�ز���Ҫ��Ŀ�����������ռ䣬
    ��ΪFreeLibrary����ΪHMODULE ʵ���Ͼ���һ��ָ��ֵ���������Ѿ����ؾ��Ѿ����ڡ�
    */
	
    if( targetProc == NULL )
    {
        return false;
    }
	DWORD processID = GetProcessId(targetProc);

    // 1. ��ȡж��dll��ģ����
    HANDLE snapHandele = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE ,processID);
    if( INVALID_HANDLE_VALUE == snapHandele)
    {
        return false;
    }
    MODULEENTRY32 entry = {0};
    entry.dwSize = sizeof(entry);// ���ȱ��븳ֵ
    BOOL ret = Module32First(snapHandele,&entry);
    HMODULE dllHandle = NULL;
	CString tempDllPath;
    while (ret) {
        //tempDllPath = entry.szModule;
		tempDllPath = entry.szExePath;
        if(tempDllPath.CompareNoCase((strDllPath)) == 0)
        {
            dllHandle = entry.hModule;
            break;
        }
        ret = Module32Next(snapHandele,&entry);
    }

    CloseHandle(snapHandele);
    if( dllHandle == NULL )
    {
        return false;
    }

    // 2.��ȡFreeLibrary������ַ
    FARPROC myLoadLibrary = GetProcAddress(GetModuleHandleA("kernel32.dll"),"FreeLibrary");
    if( myLoadLibrary == NULL )
    {
        return false;
    }
    // 3.��Ŀ�����ִ��FreeLibrary ж��ָ�����߳�
    HANDLE tHandle = CreateRemoteThread(targetProc,NULL,NULL,
                       (LPTHREAD_START_ROUTINE)myLoadLibrary,dllHandle,NULL,NULL);
    if(tHandle == NULL)
    {
        return false;
    }
    WaitForSingleObject(tHandle,INFINITE);
    CloseHandle(tHandle);
	return true;
}

CStringW CUtility::A2Wstring(std::string strA)

{
	int UnicodeLen = ::MultiByteToWideChar(CP_ACP,0,strA.c_str(),-1,NULL,0);
	wchar_t *pUnicode = new wchar_t[UnicodeLen*1]();
	::MultiByteToWideChar(CP_ACP,0,strA.c_str(),strA.size(),pUnicode,UnicodeLen);
	CString str(pUnicode);
	delete []pUnicode;
	return str;
}

std::string CUtility::W2Astring(const CString& strUnicode)
{
	char *pElementText = NULL;
	int iTextLen ;
	iTextLen = ::WideCharToMultiByte(CP_ACP,0,strUnicode,-1,NULL,0,NULL,NULL);
	pElementText = new char[iTextLen +1];
	memset(pElementText,0,(iTextLen+1)*sizeof(char));
	::WideCharToMultiByte(CP_ACP,0,strUnicode,strUnicode.GetLength(),pElementText,iTextLen,NULL,NULL);
	std::string str(pElementText);
	delete []pElementText;
	return str;
}


CString CUtility::GetErrorMsg(DWORD errorCode)
{
	{   
		// Retrieve the system error message for the last-error code  

		LPVOID lpMsgBuf;  
		LPVOID lpDisplayBuf;  

		FormatMessage(  
			FORMAT_MESSAGE_ALLOCATE_BUFFER |   
			FORMAT_MESSAGE_FROM_SYSTEM |  
			FORMAT_MESSAGE_IGNORE_INSERTS,  
			NULL,  
			errorCode,  
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),  
			(LPTSTR) &lpMsgBuf,  
			0, NULL );  

		// Display the error message and exit the process  

		lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,   
			(lstrlen((LPCTSTR)lpMsgBuf)+40)*sizeof(TCHAR));   

		StringCchPrintf((LPTSTR)lpDisplayBuf,   
			LocalSize(lpDisplayBuf),  
			TEXT("%s"),   
			lpMsgBuf);  
		CString result = (LPTSTR)lpDisplayBuf;
		LocalFree(lpMsgBuf);  
		LocalFree(lpDisplayBuf);     
		return result;
	}
}
