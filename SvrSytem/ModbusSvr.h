// ModbusSvr.h: interface for the CModbusSvr class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_MODBUSSVR_H__9959007D_9AC5_4C19_9DD4_0FA8A6D9A6B2__INCLUDED_)
#define AFX_MODBUSSVR_H__9959007D_9AC5_4C19_9DD4_0FA8A6D9A6B2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include<Windows.h>
#include<winsock2.h>
#include<ws2tcpip.h>
#include<afxtempl.h>

#include "Instruments.h"

#pragma comment(lib,"ws2_32.lib") 


#define WM_SOCKET		WM_USER+220
#define DATA_BUFSIZE	7
#define PORT 502
#define WM_MODBUSSVR   WM_USER+221


//--------------------------------------------------
enum IO_TYPE
{
	IO_MBAP,
	IO_DATA,
	IO_SEND,
	IO_ACCEPT,
	IO_UNKNOWN
} ;

enum CMD_CODE
 {
    ReadCoils = 1,
    ReadDiscretionsInput = 2,
    ReadHoldingRegisters = 3,
    ReadInputRegisters = 4,
    WriteSingleCoil = 5,
    WriteSingleRegister = 6,
    ReadExceptionStatus = 7,
    Diagnostics = 8,
    GetCommEventCounter = 0x0B,
    GetCommEventLog = 0x0C,
    WriteMultipleCoils = 0x0F,
    WriteMultipleRegisters = 0x10,
    ReportServerID = 0x11,
    ReadFileRecord = 0x14,
    WriteFileRecord = 0x15,
    MaskWriteRegister = 0x16,
    ReadWriteMultipleRegisters = 0x17,
    ReadFIFOQueue = 0x18,
    EncapsulatedInterfaceTransport = 0x2B
} ;

enum ModbusExceptionCode 
{
    IllegalFunction = 0x01,
    IllegalDataAddress = 0x02,
    IllegalDataValue = 0x03,
    SlaveDeviceFalure = 0x04,
    Acknowledge = 0x05,
    SlaveDeviceBusy = 0x06,
    MemoryPartiyError = 0x08,
    GatewayPathUnavailable = 0x0A,
    GatewayTargetDeviceFailedToRespond = 0x0B
 }	;
	
//--------------------------------------------------
struct CClientInfo
{
	CClientInfo() 
	{
		ZeroMemory(&m_ol,sizeof(m_ol)) ;
		ZeroMemory(m_MBAP,7) ;
		ZeroMemory(m_Data,256) ;
		m_ol.hEvent = WSACreateEvent() ;
	}
	
	~CClientInfo()
	{
		WSACloseEvent(m_ol.hEvent) ;
	}

	WSAOVERLAPPED	m_ol ;
	SOCKET			sSocket ;
	SOCKET			sAcceptSocket ;
	CString			strIP ;
	u_short			nPort ;

	CString			GetShowText() 
	{
		CString	str ;
		str.Format("%s:%d",strIP,nPort) ;
		return str ;
	}
	
	unsigned int	m_nDataLen ;
	unsigned char	m_MBAP[7] ;
	unsigned char	m_Data[256] ;
	IO_TYPE			m_IO_type ;

	char			m_nCmd ;
	u_short			m_nAdr ;
	u_short			m_nLen ;

	void AttachData()
	{
		m_nCmd = m_Data[0] ;
		m_nAdr = m_Data[1] << 8 | m_Data[2] ;
		m_nLen = m_Data[3]<< 8 | m_Data[4] ;
	}
} ;


//----------------------------------------------------
class CModbusSvr  
{
public:
	LPCTSTR		lpClientIP ;
	UINT		clientPort ;
	WSABUF		DataBuf ;

	bool		bRunning ;				//Server Running flag
	HANDLE		m_hThreadService ;


	CInstrument m_Spectrum ;

private:
	HWND		hWnd ;
	SOCKET	ListenSocket ;
	int		nPort ;
	SOCKET	AcceptSocket ;


	WSAOVERLAPPED	AcceptOverlapped ;
	WSAEVENT		EventArray[WSA_MAXIMUM_WAIT_EVENTS] ;

	DWORD			dwEventTotal  ;
	DWORD			dwRecvBytes ;
	DWORD			Flag ;


	static DWORD WINAPI   ServerAcceptThread(void* pParam) ;
	

public:
	bool	InitSocket() ;
	bool	PostAccept(SOCKET sListenSocket) ;
	bool	PostAccept(CClientInfo* p) ;
	void	PostRecvMBAP(CClientInfo* p) ;
	void	PostRecvDATA(CClientInfo* p) ;
	void	PostSend(CClientInfo* p) ;

	CClientInfo*	OnSocketConnected(SOCKET sClientSocket,sockaddr_in* saClient) ;
	void	OnSocketDisconnect(SOCKET aClientSocket) ;
	inline void SetWindowHandle(HWND h)		{ hWnd = h ;	m_Spectrum.SetWindowHandle(h) ;		}

public:
	CModbusSvr();
	virtual ~CModbusSvr();

public:
	CArray<CClientInfo *,CClientInfo*> m_ClientArray ;
	CRITICAL_SECTION  m_cs ;
	void	LockClientArray()	{	EnterCriticalSection(&m_cs) ;	}
	void	UnlockClientArray()	{	LeaveCriticalSection(&m_cs) ;	}

	void (*Mark)(CString str) ;

};

#endif // !defined(AFX_MODBUSSVR_H__9959007D_9AC5_4C19_9DD4_0FA8A6D9A6B2__INCLUDED_)
