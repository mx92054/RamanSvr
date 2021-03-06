
// SvrSytemDlg.h : 头文件
//

#pragma once

#include "ModbusSvr.h"
#include "SerialPort.h"
#include "CsvFileHelper.h"
#include "afxwin.h"
#include "PowerBd.h"

#define WM_MODBUSSVR WM_USER + 221

// CSvrSytemDlg 对话框
class CSvrSytemDlg : public CDialogEx
{
	// 构造
  public:
	CSvrSytemDlg(CWnd *pParent = NULL); // 标准构造函数

	// 对话框数据
	enum
	{
		IDD = IDD_SVRSYTEM_DIALOG
	};

  private:
	CModbusSvr	svr;
	CPowerBd	m_pow ;

	int		m_oldCCDPow ;
	int		m_oldCCDStatus ;
	int		m_oldRunStatus;
	CFont	m_font ;
	int		nMdRecv ;
	bool    bFirst ;

	// 实现
  protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual void DoDataExchange(CDataExchange *pDX); // DDX/DDV 支持
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()

  protected:
	afx_msg LRESULT OnModbussvr(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnCommRxchar(WPARAM wParam, LPARAM lParam);

  public:
	CListBox m_lstMess; 
	CStatic m_labAmp;
	CStatic m_labStatus;
	CStatic m_labCode;
	CStatic m_labVol;
	CStatic m_labTemp;
	CStatic m_labPowerStatus;
	CStatic m_labHumd;
	CStatic m_labPowTemp;
	CStatic m_labMbRecv;
	CStatic m_labSerTX;
	CStatic m_labCCDSw;
	CStatic m_labLaserSw;
	CButton m_chkCool;
	CButton m_btnProgram;

	void FormatMessage(char *p);

	afx_msg void OnDestroy();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnBnClickedBtnmodbus();
	afx_msg void OnBnClickedBtnserial();
	afx_msg void OnBnClickedBtninit();
	afx_msg void OnBnClickedBtnscan();
	afx_msg void OnBnClickedChkcool();
	afx_msg void OnBnClickedBtnclear();
	afx_msg void OnBnClickedBtnprogram();
	afx_msg void OnBnClickedBtnccd();
	afx_msg void OnBnClickedBtnlaser();

	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	static BOOL CALLBACK EnumChildProc(HWND hwndChild,LPARAM lParam);
	CStatic m_labTempCode;
	afx_msg void OnBnClickedBtngraph();
	CStatic m_labProgStep;
};
