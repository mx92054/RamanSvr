
// SvrSytemDlg.h : ͷ�ļ�
//

#pragma once

#include "Instruments.h"
#include "ModbusSvr.h"
#include "afxwin.h"

#define WM_MODBUSSVR   WM_USER+221

// CSvrSytemDlg �Ի���
class CSvrSytemDlg : public CDialogEx
{
// ����
public:
	CSvrSytemDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
	enum { IDD = IDD_SVRSYTEM_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��

private:
	CInstrument ins ;
	CModbusSvr  svr ;

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
	afx_msg void OnBnClickedBtnmodbus();

	CListBox m_lstMess;
protected:
	afx_msg LRESULT OnModbussvr(WPARAM wParam, LPARAM lParam);
};
