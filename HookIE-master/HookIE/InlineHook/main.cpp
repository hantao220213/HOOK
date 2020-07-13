#include "stdafx.h"
#include <string>
#include <iostream>
#include <Windows.h>
#include <Wininet.h>
#pragma comment(lib,"wininet.lib")

#include "InlineHook7.h"
using namespace std;
CInlineHook7 g_inlineHookObj7;

typedef HINTERNET ( WINAPI *InternetConnectWFunc)(
	HINTERNET     hInternet,
	LPCWSTR       lpszServerName,
	INTERNET_PORT nServerPort,
	LPCWSTR       lpszUserName,
	LPCWSTR       lpszPassword,
	DWORD         dwService,
	DWORD         dwFlags,
	DWORD_PTR     dwContext
	);

HINTERNET WINAPI MyInternetConnectW(
	HINTERNET     hInternet,
	LPCWSTR       lpszServerName,
	INTERNET_PORT nServerPort,
	LPCWSTR       lpszUserName,
	LPCWSTR       lpszPassword,
	DWORD         dwService,
	DWORD         dwFlags,
	DWORD_PTR     dwContext
	)
{
	g_inlineHookObj7.UnHook(); // ������ж�ع��� �ٲſ����ٴε��ñ�Hook�ĺ�������Ȼ�������ѭ��
	HINTERNET ret = NULL;
	CString temp;
	temp.Format(_T("���ص�url:%s"),lpszServerName);
	CString url = lpszServerName;
	
	//OutputDebugString(url);
	//if(url.Find(_T("http://"))>=0 || url.Find(_T("https://")) >=0)
	if(url.Find(_T("baidu.com")) >= 0)
	{
		url = _T("www.sogou.com");
	}
	CString xx;
	xx.Format(_T("dwService %s"), dwService);
	MessageBox(NULL,(url)+xx.GetString(),NULL,0);
	MessageBox(NULL, _T("NEXT"), NULL, 0);

	OutputDebugString(temp);
	// ����Ҫ���Ѿ����ڵĿ⣬��Ҫ�����Լ��Ŀ�������WININET.dll��ȥʹ��
	//ret = ::InternetConnectW(hInternet, lpszServerName, nServerPort, lpszUserName, lpszPassword,  dwService,  dwFlags,  dwContext);

	HMODULE hModule = GetModuleHandle(_T("WININET.dll"));
	if(hModule == NULL)
	{
		OutputDebugString(_T("GetModuleHandle false"));
		goto end;
	}
	InternetConnectWFunc func = (InternetConnectWFunc)GetProcAddress(hModule,"InternetConnectW");
	if(func == NULL)
	{
		OutputDebugString(_T("GetProcAddress false"));
		goto end;
	}
	ret = func(hInternet, url, nServerPort, lpszUserName, lpszPassword,  dwService,  dwFlags,  dwContext);
	

end:	
	g_inlineHookObj7.ReHook();
	return ret;
}


VOID InlineHookIE()
{
	OutputDebugString(_T("InlineHookIE into"));
	g_inlineHookObj7.Hook("WININET.dll","InternetConnectW",(FARPROC)MyInternetConnectW);
	return;
}

VOID UnInlineHookIE()
{
	OutputDebugString(_T("UnInlineHookIE into"));
	g_inlineHookObj7.UnHook();
	return;
}

// DLL ���
BOOL APIENTRY DllMain( HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
	)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		{
			InlineHookIE();
		}
		break;
	case DLL_PROCESS_DETACH:
		UnInlineHookIE();
		break;
	}
	return TRUE;
}

