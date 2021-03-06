


// SvrSytemDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "SvrSytem.h"
#include "SvrSytemDlg.h"
#include "afxdialogex.h"
#include "CsvFileHelper.h"
#include "DataStore.h"
#include "Instruments.h" 
#include "ATMCD32D.H"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
  public:
	CAboutDlg();

	// 对话框数据
	enum
	{
		IDD = IDD_ABOUTBOX
	};

  protected:
	virtual void DoDataExchange(CDataExchange *pDX); // DDX/DDV 支持

	// 实现
  protected:
	DECLARE_MESSAGE_MAP()
	//	afx_msg LRESULT OnCommRxchar(WPARAM wParam, LPARAM lParam);
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange *pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
//	ON_MESSAGE(WM_MODBUSSVR, &CAboutDlg::OnModbussvr)
//	ON_MESSAGE(WM_COMM_RXCHAR, &CAboutDlg::OnCommRxchar)
END_MESSAGE_MAP()

// CSvrSytemDlg 对话框

CSvrSytemDlg::CSvrSytemDlg(CWnd *pParent /*=NULL*/)
	: CDialogEx(CSvrSytemDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CSvrSytemDlg::DoDataExchange(CDataExchange *pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LISTMESS, m_lstMess);
	DDX_Control(pDX, IDC_LABAMP, m_labAmp);
	DDX_Control(pDX, IDC_LABSTATUS, m_labStatus);
	DDX_Control(pDX, IDC_LABCODE, m_labCode);
	DDX_Control(pDX, IDC_LABVOLTAGE, m_labVol);
	DDX_Control(pDX, IDC_LABTEMP, m_labTemp);
	DDX_Control(pDX, IDC_CHKCOOL, m_chkCool);
	DDX_Control(pDX, IDC_POWERSW, m_labPowerStatus);
	DDX_Control(pDX, IDC_LABHUMD, m_labHumd);
	DDX_Control(pDX, IDC_LABPOWTEMP, m_labPowTemp);
	DDX_Control(pDX, IDC_BTNPROGRAM, m_btnProgram);
	DDX_Control(pDX, IDC_SVRMODBUS, m_labMbRecv);
	DDX_Control(pDX, IDC_LABSVRSERIAL, m_labSerTX);
	DDX_Control(pDX, IDC_LABCCDSW, m_labCCDSw);
	DDX_Control(pDX, IDC_LABLASERSW, m_labLaserSw);
	DDX_Control(pDX, IDC_LABTMPCODE, m_labTempCode);
	DDX_Control(pDX, IDC_LABPROGSTEP, m_labProgStep);
}

BEGIN_MESSAGE_MAP(CSvrSytemDlg, CDialogEx)
ON_WM_SYSCOMMAND()
ON_WM_PAINT()
ON_WM_QUERYDRAGICON()
ON_BN_CLICKED(IDC_BTNMODBUS, &CSvrSytemDlg::OnBnClickedBtnmodbus)
ON_MESSAGE(WM_MODBUSSVR, &CSvrSytemDlg::OnModbussvr)
ON_WM_DESTROY()
ON_WM_TIMER()
ON_BN_CLICKED(IDC_BTNSERIAL, &CSvrSytemDlg::OnBnClickedBtnserial)
ON_MESSAGE(WM_COMM_RXCHAR, &CSvrSytemDlg::OnCommRxchar)
ON_BN_CLICKED(IDC_BTNCLEAR, &CSvrSytemDlg::OnBnClickedBtnclear)
ON_BN_CLICKED(IDC_BTNINIT, &CSvrSytemDlg::OnBnClickedBtninit)
ON_BN_CLICKED(IDC_BTNSCAN, &CSvrSytemDlg::OnBnClickedBtnscan)
ON_BN_CLICKED(IDC_CHKCOOL, &CSvrSytemDlg::OnBnClickedChkcool)
ON_WM_CTLCOLOR()
ON_BN_CLICKED(IDC_BTNPROGRAM, &CSvrSytemDlg::OnBnClickedBtnprogram)
ON_BN_CLICKED(IDC_BTNCCD, &CSvrSytemDlg::OnBnClickedBtnccd)
ON_BN_CLICKED(IDC_BTNLASER, &CSvrSytemDlg::OnBnClickedBtnlaser)
ON_BN_CLICKED(IDC_BTNGRAPH, &CSvrSytemDlg::OnBnClickedBtngraph)
END_MESSAGE_MAP()

// CSvrSytemDlg 消息处理程序
BOOL CSvrSytemDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu *pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);  // 设置大图标
	SetIcon(m_hIcon, FALSE); // 设置小图标

	// TODO: 在此添加额外的初始化代码
	m_font.CreateFont(	16,		// nHeight 
						0,		// nWidth 
						0,		// nEscapement 
						0,		// nOrientation 
						FW_BOLD, // nWeight 
						FALSE,	// bItalic 
						FALSE,	// bUnderline 
						0,		// cStrikeOut 
						ANSI_CHARSET,		// nCharSet 
						OUT_DEFAULT_PRECIS, // nOutPrecision 
						CLIP_DEFAULT_PRECIS,// nClipPrecision 
						DEFAULT_QUALITY,	 // nQuality 
						DEFAULT_PITCH | FF_SWISS, // nPitchAndFamily 
						"Consolas");		// lpszFac 
	EnumChildWindows(m_hWnd, EnumChildProc, (LPARAM)&m_font);

	nMdRecv = 0 ;
	m_oldCCDPow = 0 ;
	bFirst = true ;
	svr.m_data.GetNewton(&svr.m_ccd) ;

	this->OnBnClickedBtnmodbus() ;
	this->OnBnClickedBtnserial() ;
	
	SetTimer(1, 1000, NULL);

	return TRUE; // 除非将焦点设置到控件，否则返回 TRUE
}

void CSvrSytemDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CSvrSytemDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CSvrSytemDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CSvrSytemDlg::OnBnClickedBtnmodbus()
{
	// TODO: 在此添加控件通知处理程序代码
	int r = svr.InitSocket() ;
	if (r == 0)
	{
		FormatMessage("Initial socket success!");
		svr.SetWindowHandle(this->m_hWnd);
	}
	else
	{
		char buf[100] ;
		sprintf_s(buf,"Initial socket failure!(code:%d)",r) ;
		FormatMessage(buf);
	}
}

afx_msg LRESULT CSvrSytemDlg::OnModbussvr(WPARAM wParam, LPARAM lParam)
{
	if ( wParam )
		FormatMessage((char *)lParam);
	nMdRecv++ ;
	return 0;
}

void CSvrSytemDlg::OnDestroy()
{
	CDialogEx::OnDestroy();

	// TODO: 在此处添加消息处理程序代码
	CCsvFileHelper::WriteLogFile(&m_lstMess);
}

void CSvrSytemDlg::FormatMessage(char *p)
{
	if (m_lstMess.GetCount() >= 100)
	{
		CCsvFileHelper::WriteLogFile(&m_lstMess);
		m_lstMess.ResetContent();
	}

	CString str;
	time_t tt = time(NULL);
	tm t; 
	localtime_s(&t, &tt);
	str.Format("%02d:%02d:%02d %s", t.tm_hour, t.tm_min, t.tm_sec,p);
	m_lstMess.AddString(str);
	m_lstMess.SetCurSel(m_lstMess.GetCount()-1) ;
}

void CSvrSytemDlg::OnBnClickedBtnserial()
{
	// TODO: 在此添加控件通知处理程序代码
	m_pow.SetCurWnd(this->m_hWnd) ;
	if ( m_pow.Open() )
		FormatMessage("Start COM1 Success");
	else
		FormatMessage("Start COM1 Failure");
}

afx_msg LRESULT CSvrSytemDlg::OnCommRxchar(WPARAM wParam, LPARAM lParam)
{
	if ( m_pow.RxOneChar((unsigned char)wParam))
	{
		svr.m_data.SetMRegVal(20000,10,m_pow.sRegister) ;
		if ( bFirst )
		{
			svr.m_data.SetMRegVal(30000,5,m_pow.sRegister) ;
			bFirst = false ;
		}
	}
	return 0;
}

void CSvrSytemDlg::OnBnClickedBtnclear()
{
	// TODO: 在此添加控件通知处理程序代码
	m_lstMess.ResetContent() ;
}

void CSvrSytemDlg::OnBnClickedBtninit()
{
	// TODO: 在此添加控件通知处理程序代码
	svr.m_ccd.InitialCCD() ;
}

void CSvrSytemDlg::OnBnClickedBtnscan()
{
	// TODO: 在此添加控件通知处理程序代码
	svr.m_data.SetRegVal(RH_SCANING,1) ;
}

void CSvrSytemDlg::OnBnClickedChkcool()
{
	// TODO: 在此添加控件通知处理程序代码
	if ( m_chkCool.GetCheck() )
	{
		svr.m_data.SetRegVal(RH_CCDCOOLER,1) ;
	}
	else
	{
		svr.m_data.SetRegVal(RH_CCDCOOLER,0) ;
	}
}


HBRUSH CSvrSytemDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);

	// TODO:  在此更改 DC 的任何特性
	int id = pWnd->GetDlgCtrlID() ;
	switch (id)
	{
	case IDC_LABVOLTAGE:
	case IDC_LABAMP:
	case IDC_LABPOWTEMP:
	case IDC_LABHUMD:
			pDC->SetBkColor(RGB(118, 238, 240)) ;
			break ;
	case IDC_LABPROGRAM:
			if (svr.m_data.GetRegVal(RI_PROGFLAG))
				pDC->SetBkColor(RGB(0,255,0)) ;
			else
				pDC->SetBkColor(RGB(242,242,242)) ;
			break ;
	case IDC_LABCCDSW:
			if (svr.m_data.IsCCDOn())
				pDC->SetBkColor(RGB(0,255,0)) ;
			else
				pDC->SetBkColor(RGB(242,242,242)) ;
			break ;
	case IDC_LABLASERSW:
			if (svr.m_data.IsLaserOn())
				pDC->SetBkColor(RGB(0,255,0)) ;
			else
				pDC->SetBkColor(RGB(242,242,242)) ;
			break ;
	}

	// TODO:  如果默认的不是所需画笔，则返回另一个画笔
	return hbr;
}

BOOL CALLBACK CSvrSytemDlg::EnumChildProc(HWND hwndChild, LPARAM lParam)
{
    CFont *pFont = (CFont*)lParam;
    CWnd *pWnd = CWnd::FromHandle(hwndChild);
    pWnd->SetFont(pFont);
    return TRUE;
}
void CSvrSytemDlg::OnBnClickedBtnprogram()
{
	// TODO: 在此添加控件通知处理程序代码
	if ( svr.m_data.GetRegVal(RI_PROGFLAG) )
		svr.m_data.SetRegVal(RH_PROGSART,0) ;
	else
		svr.m_data.SetRegVal(RH_PROGSART,1) ;
}

void CSvrSytemDlg::OnBnClickedBtnccd()
{
	// TODO: 在此添加控件通知处理程序代码
	if ( svr.m_data.IsCCDOn() )
		svr.m_data.SwitchCCD(false) ;
	else
		svr.m_data.SwitchCCD(true) ;
}

void CSvrSytemDlg::OnBnClickedBtnlaser()
{
	// TODO: 在此添加控件通知处理程序代码
	if ( svr.m_data.IsLaserOn() )
		svr.m_data.SwitchLaser(false) ;
	else
		svr.m_data.SwitchLaser(true) ;
}

void CSvrSytemDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	short val ;
	char  buf[100] ;

	if (nIDEvent == 1) //1s timer
	{
		//svr.m_Spectrum.OnTimer_1s();
		if ( svr.OnTimer_1s() )
			OnBnClickedBtngraph();

		for(int i=0 ; i < 5 ; i++)
		{
			m_pow.sSwitch[i] = svr.m_data.GetRegVal(30000 + i) ;
		}
		m_pow.Translate() ;

		//update valtage
		sprintf_s(buf,"%4.2f",svr.m_data.GetRegVal(20005)/100.0f) ;
		m_labVol.SetWindowText(buf);
		sprintf_s(buf,"%4.2f",svr.m_data.GetRegVal(20006)/100.0f) ;
		m_labAmp.SetWindowText(buf);
		sprintf_s(buf,"%4.1f",svr.m_data.GetRegVal(20007)/10.0f) ;
		m_labPowTemp.SetWindowText(buf);
		sprintf_s(buf,"%4.1f",svr.m_data.GetRegVal(20008)/10.0f) ;
		m_labHumd.SetWindowText(buf);

		sprintf_s(buf,"%d",svr.m_data.GetRegVal(RI_CCDERROR)) ;
		m_labCode.SetWindowText(buf);	
		sprintf_s(buf,"%d",svr.m_data.GetRegVal(RI_CCDTEMPVAL)-200) ;
		m_labTemp.SetWindowText(buf);	

		sprintf_s(buf,"%d/%d",nMdRecv,svr.nClient-1) ;
		m_labMbRecv.SetWindowText(buf) ;
		sprintf_s(buf,"%d/%d",m_pow.nTxNo,m_pow.nRxNo) ;
		m_labSerTX.SetWindowText(buf) ;

		int temp = 0 ;
		svr.m_ccd.GetTempValue(&temp) ;
		sprintf_s(buf,"%d/%d",temp,svr.m_ccd.GetCoolStatus()) ;
		m_labTemp.SetWindowText(buf) ;
		sprintf_s(buf,"%d",svr.m_ccd.GetCoolErr()) ;
		m_labTempCode.SetWindowText(buf) ;

		sprintf_s(buf,"%d %d %d %d %d | %d %d %d %d %d",svr.m_data.GetRegVal(RI_SW1),
			svr.m_data.GetRegVal(RI_SW2),svr.m_data.GetRegVal(RI_SW3),svr.m_data.GetRegVal(RI_SW4),
			svr.m_data.GetRegVal(RI_SW5),svr.m_data.GetRegVal(RH_SW1),svr.m_data.GetRegVal(RH_SW2),
			svr.m_data.GetRegVal(RH_SW3),svr.m_data.GetRegVal(RH_SW4),svr.m_data.GetRegVal(RH_SW5));
		m_labPowerStatus.SetWindowText(buf) ;

		if ( svr.m_data.IsCCDOn() )
			m_labCCDSw.SetWindowText("ON") ;
		else
			m_labCCDSw.SetWindowText("OFF") ;

		if ( svr.m_data.IsLaserOn())
			m_labLaserSw.SetWindowText("ON") ;
		else
			m_labLaserSw.SetWindowText("OFF") ;

		val = svr.m_ccd.GetCoolStatus() ;
		if ( val == 1)
			m_chkCool.SetCheck(1) ;
		else
			m_chkCool.SetCheck(0) ;
		
		//------------------------------------------------------------------------------------------------------------
		short status = svr.m_data.GetRegVal(RI_CCDSTATUS) ;
		if ( status != m_oldCCDStatus )
		{
			sprintf_s(buf,"%d",status) ;
			m_labStatus.SetWindowText(buf); 
	
			switch ( status )
			{
			case 1:  FormatMessage("Start init CCD"); break ;
			case 10: FormatMessage("Initial CCD success");
					svr.m_ccd.Cooler(true) ;
					break ;
			case 11:FormatMessage("Setting parameter");	break ;
			case 17: FormatMessage("Start scaning");	break ;
			case 18: FormatMessage("Finish scaning");	break ;
			case 20: FormatMessage("Scaning data");		break ;
			case 30: FormatMessage("Write to file") ;
			}
		}
		m_oldCCDStatus = status;

		status = svr.m_data.IsCCDOn() ;
		if ( status && !m_oldCCDPow)
		{
			svr.m_ccd.InitialCCD() ;
		}
		m_oldCCDPow = status ;
		

		//------------------------------------------------
		status = svr.m_data.GetRegVal(RI_PROGFLAG);
		if (m_oldRunStatus != status)
		{
			m_oldRunStatus = status;
			if (status)
				GetDlgItem(IDC_LABPROGRAM)->SetWindowText("Program Run ");
			else
				GetDlgItem(IDC_LABPROGRAM)->SetWindowText("Program Stop");
		}
		
		sprintf_s(buf,"%d",svr.m_data.GetProgStep()) ;
		m_labProgStep.SetWindowText(buf) ;
	}

	CDialogEx::OnTimer(nIDEvent);
}

void CSvrSytemDlg::OnBnClickedBtngraph()
{

}
