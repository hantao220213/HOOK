
// HookIEDlg.h : ͷ�ļ�
//

#pragma once
#include "afxwin.h"


// CHookIEDlg �Ի���
class CHookIEDlg : public CDialogEx
{
// ����
public:
	CHookIEDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
	enum { IDD = IDD_HOOKIE_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��


// ʵ��
protected:
	HICON m_hIcon;

	// ���ɵ���Ϣӳ�亯��
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedBtnHookie();
	afx_msg void OnBnClickedBtnUnhookie();
private:
	CButton m_uiHookIEBtn;
	CButton m_uiUnHookIEBtn;
};
