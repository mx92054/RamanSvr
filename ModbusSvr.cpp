// ModbusSvr.cpp: implementation of the CModbusSvr class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ModbusSvr.h"
#include "Instruments.h"

#include <memory.h>
#include <mswsock.h>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

#pragma comment(lib, "mswsock.lib")
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
//------------------------------------
CModbusSvr::CModbusSvr()
{
	dwEventTotal = 0;
	dwRecvBytes = 0;
	Flag = 0;
	nPort = PORT;
	bRunning = false;
	bThreadExit = true ;

	try
	{
		CFile myFile(".//reg_data.txt", CFile::modeRead);
		myFile.SeekToBegin();    
		CArchive arLoad(&myFile, CArchive::load);     
		m_data.Serialize(arLoad) ;
		m_data.Init() ;
		arLoad.Close();
		myFile.Close() ;
	}
	catch(CFileException *ex)
	{
		MessageBox(NULL,"Load parameter failure!","Alarm",MB_OK|MB_ICONWARNING) ;
	}

	InitializeCriticalSection(&m_cs);
}

//------------------------------------
CModbusSvr::~CModbusSvr()
{
	bRunning = false;
	while (bThreadExit) ;

	DeleteCriticalSection(&m_cs);

	try
	{
		CFile myFile(".//reg_data.txt", CFile::modeCreate | CFile::modeReadWrite);
		CArchive arStore(&myFile, CArchive::store);         
		m_data.Serialize(arStore);     
		arStore.Close();
		myFile.Close() ;
	}
	catch(CFileException *ex)
	{
		MessageBox(NULL,"Save parameter failure!","Alarm",MB_OK|MB_ICONWARNING) ;
	}

}

//--------------------------------------------------------------------
// Function:  Initialize socket
//  return true when success
//	return false when failure
//--------------------------------------------------------------------
int CModbusSvr::InitSocket()
{
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData); 

	//-----------------------------------------------------------------------
	ListenSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (ListenSocket == INVALID_SOCKET)
	{
		WSACleanup();
		return 1;
	}

	//-----------------------------------------------------------------------
	SOCKADDR_IN ServerAddr;
	ServerAddr.sin_family = AF_INET;
	ServerAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	ServerAddr.sin_port = htons(nPort);

	if (bind(ListenSocket, (LPSOCKADDR)&ServerAddr, sizeof(ServerAddr)) == SOCKET_ERROR)
	{
		closesocket(ListenSocket);
		WSACleanup();
		return 2;
	}

	//-----------------------------------------------------------------------
	if (listen(ListenSocket, SOMAXCONN) == SOCKET_ERROR)
	{
		closesocket(ListenSocket);
		WSACleanup();
		return 3;
	}

	PostAccept(ListenSocket);

	bRunning = true;
	DWORD dwThreadID;
	bThreadExit = true;
	m_hThreadService = CreateThread(NULL, 0, ServerAcceptThread, this, 0, &dwThreadID);

	return 0;
}

//--------------------------------------------------------------------
// Function:  Work thread to handle to response event
//  keep running after initialize socket
//
//--------------------------------------------------------------------
DWORD WINAPI CModbusSvr::ServerAcceptThread(void *pParam)
{
	CModbusSvr *pSvr = (CModbusSvr *)pParam;
	char buf[100];

	while (pSvr->bRunning)
	{
		pSvr->LockClientArray(); //lock critical area

		pSvr->nClient = pSvr->m_ClientArray.GetSize();
		WSAEVENT *pEvent = NULL;
		if (pSvr->nClient > 0)
		{
			pEvent = new WSAEVENT[pSvr->nClient];
			for (int i = 0; i < pSvr->nClient; i++)
				pEvent[i] = pSvr->m_ClientArray.GetAt(i)->m_ol.hEvent;
		}
		pSvr->UnlockClientArray();

		if (pEvent == NULL)
		{
			Sleep(1000);
			continue;
		}

		DWORD dwWaitRet = ::WaitForMultipleObjects(pSvr->nClient, pEvent, FALSE, 1000);
		if (dwWaitRet != WAIT_FAILED && dwWaitRet != WAIT_TIMEOUT )
		{
			//pSvr->LockClientArray();
			CClientInfo *pClient = pSvr->m_ClientArray.GetAt(dwWaitRet - WAIT_OBJECT_0);
			WSAResetEvent(pClient->m_ol.hEvent);

			switch (pClient->m_IO_type)
			{
			case IO_MBAP: //received MBAP
				if (pClient->m_ol.InternalHigh == 0)
					pSvr->OnSocketDisconnect(pClient->sSocket);
				else
				{
					unsigned int Len = pClient->m_MBAP[4] << 8 | pClient->m_MBAP[5];
					if (Len > 255 && Len < 2)
					{
						sprintf_s(buf, "Recv data format error!");
						::SendMessage(pSvr->hWnd, WM_MODBUSSVR, 0, (LPARAM)(LPCTSTR)buf);
						pSvr->PostRecvMBAP(pClient);
					}
					else
					{
						pClient->m_nDataLen = Len - 1;
						pSvr->PostRecvDATA(pClient);
					}
				}
				break;

			case IO_DATA: //received DATA
				if (pClient->m_ol.InternalHigh == 0)
					pSvr->OnSocketDisconnect(pClient->sSocket);
				else
				{
					unsigned int ID, IP, Len;
					ID = pClient->m_MBAP[0] << 8 | pClient->m_MBAP[1];
					IP = pClient->m_MBAP[2] << 8 | pClient->m_MBAP[3];
					Len = pClient->m_MBAP[4] << 8 | pClient->m_MBAP[5];
					pClient->AttachData();

					sprintf_s(buf, "%s:%d Tx:%d Pt:%d Len:%d,Net:%d Cmd:%d Ad:%d Num:%d",
							pClient->strIP, pClient->nPort,
							ID, IP, Len, pClient->m_MBAP[6],
							pClient->m_nCmd, pClient->m_nAdr, pClient->m_nLen);
					::SendMessage(pSvr->hWnd, WM_MODBUSSVR, 0, (LPARAM)(LPCTSTR)buf);

					pSvr->PostSend(pClient);
				}
				break;

			case IO_SEND: // finished to send
				if (pClient->m_ol.InternalHigh == 0)
					pSvr->OnSocketDisconnect(pClient->sSocket);
				else
					pSvr->PostRecvMBAP(pClient);
				break;

			case IO_ACCEPT: // client havd accept
				setsockopt(pClient->sAcceptSocket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT,
						   (char *)&pClient->sAcceptSocket,
						   sizeof(pClient->sAcceptSocket));
				CClientInfo *pNewClient = pSvr->OnSocketConnected(pClient->sAcceptSocket, NULL);
				pSvr->PostRecvMBAP(pNewClient);
				pSvr->PostAccept(pClient);
				break;
			}
			//pSvr->UnlockClientArray(); //unlock critical area
		}

		delete[] pEvent;
	}

	pSvr->LockClientArray() ;
	int n = pSvr->m_ClientArray.GetSize();
	for (int i = 0; i < n; i++)
	{
		CClientInfo *pClient = pSvr->m_ClientArray.GetAt(0);
		closesocket(pClient->sSocket) ;
		pSvr->m_ClientArray.RemoveAt(0);
		delete pClient;
	}
	WSACleanup() ;
	pSvr->UnlockClientArray() ;

	pSvr->bThreadExit = false;
	return 0;
}

//--------------------------------------------------------------------
// Function:  handle when client conntect to this server
//  return client information
//	send messge to main UI
//--------------------------------------------------------------------
CClientInfo *CModbusSvr::OnSocketConnected(SOCKET sClientSocket, sockaddr_in *saClient)
{
	bool bDelete = false;
	char buf[100];

	if (saClient == NULL)
	{
		sockaddr_in *pTempClientAddr = new sockaddr_in;
		int nLen = sizeof(sockaddr_in);
		if (SOCKET_ERROR == getpeername(sClientSocket, (sockaddr *)pTempClientAddr, &nLen))
		{
			int nErrorCode = ::WSAGetLastError();
			if (SOCKET_ERROR == getpeername(sClientSocket, (sockaddr *)pTempClientAddr, &nLen))
			{
				sprintf_s(buf, "get peer name error");
				::SendMessage(hWnd, WM_MODBUSSVR, 1, (LPARAM)(LPCTSTR)buf);
			}
		}
		saClient = (sockaddr_in *)pTempClientAddr;
		bDelete = true;
	}

	u_short uPort = ntohs(((sockaddr_in *)saClient)->sin_port);
	CString strIP = inet_ntoa(saClient->sin_addr);

	CClientInfo *pClient = new CClientInfo;
	pClient->nPort = uPort;
	pClient->strIP = strIP;
	pClient->sSocket = sClientSocket;
	pClient->m_IO_type = IO_MBAP;

	LockClientArray();
	m_ClientArray.Add(pClient);
	//-------------------------------------------------
	UnlockClientArray();

	if (bDelete)
		delete saClient;

	sprintf_s(buf, "%s:%d connect", pClient->strIP, pClient->nPort);
	::SendMessage(hWnd, WM_MODBUSSVR, 1, (LPARAM)(LPCTSTR)buf);
	return pClient;
}

//--------------------------------------------------------------------
// Function:  handle when client disconntect to this server
// delete client information
// send messge to main UI
//--------------------------------------------------------------------
void CModbusSvr::OnSocketDisconnect(SOCKET aClientSocket)
{
	char buf[100];

	LockClientArray();
	for (int i = 0; i < m_ClientArray.GetSize(); i++)
	{
		CClientInfo *pClient = m_ClientArray.GetAt(i);

		if (pClient->sSocket == aClientSocket)
		{
			sprintf_s(buf, "%s:%d disconnect", pClient->strIP, pClient->nPort);
			::SendMessage(hWnd, WM_MODBUSSVR, 1, (LPARAM)(LPCTSTR)buf);
			delete pClient;
			m_ClientArray.RemoveAt(i);
		}
	}
	UnlockClientArray();
}

//--------------------------------------------------------------------
// Function:  handle when client response to accept
// return true when suucess to accept
// send messge to main UI
//--------------------------------------------------------------------
bool CModbusSvr::PostAccept(SOCKET sListenSocket)
{
	CClientInfo *pClient = new CClientInfo;
	SOCKET sClientSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (sClientSocket == INVALID_SOCKET)
	{
		WSACleanup();
		return false;
	}
	pClient->sSocket = sListenSocket;
	pClient->m_IO_type = IO_ACCEPT;
	pClient->sAcceptSocket = sClientSocket;

	DWORD dwBytesReceived = 0;
	int nRet = AcceptEx(pClient->sSocket, pClient->sAcceptSocket, pClient->m_MBAP, 0,
						sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16, &dwBytesReceived,
						&pClient->m_ol);
	if (nRet || ::WSAGetLastError() == ERROR_IO_PENDING)
	{
		LockClientArray();
		m_ClientArray.Add(pClient);
		UnlockClientArray();
		return true;
	}

	return false;
}

//--------------------------------------------------------------------
// Function:  handle when client response to accept
// return true when suucess to accept
// send messge to main UI
//--------------------------------------------------------------------
bool CModbusSvr::PostAccept(CClientInfo *pClient)
{
	SOCKET sClientSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (sClientSocket == INVALID_SOCKET)
	{
		WSACleanup();
		return false;
	}
	pClient->m_IO_type = IO_ACCEPT;
	pClient->sAcceptSocket = sClientSocket;

	DWORD dwBytesReceived = 0;
	int nRet = AcceptEx(pClient->sSocket, pClient->sAcceptSocket, pClient->m_MBAP, 0,
						sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16, &dwBytesReceived,
						&pClient->m_ol);
	if (nRet || ::WSAGetLastError() == ERROR_IO_PENDING)
	{
		return true;
	}

	char buf[100];
	sprintf_s(buf, "AcceptEx error");
	::SendMessage(hWnd, WM_MODBUSSVR, 1, (LPARAM)(LPCTSTR)buf);
	return false;
}

//--------------------------------------------------------------------
// Function:  send command about recieving the MBAP inforation
// return true when suucess to exectue
// send messge to main UI
//--------------------------------------------------------------------
void CModbusSvr::PostRecvMBAP(CClientInfo *p)
{
	p->m_IO_type = IO_MBAP;

	WSABUF pBuf;
	pBuf.buf = (char *)p->m_MBAP;
	pBuf.len = 7;
	DWORD cbRecv = 0;
	DWORD dwFlg = 0;
	int nRet = WSARecv(p->sSocket, &pBuf, 1, &cbRecv, &dwFlg, &p->m_ol, NULL);
	if (nRet != 0)
	{
		int nError = WSAGetLastError();
		if (nError != ERROR_IO_PENDING)
		{
			char buf[100];
			sprintf_s(buf, "WSARecv error (MBAP code:%d)", nError);
			::SendMessage(hWnd, WM_MODBUSSVR, 1, (LPARAM)(LPCTSTR)buf);
		}
	}
	else
	{
		//Handle message
	}
}

//--------------------------------------------------------------------
// Function:  send command about recieving the DATA inforation
// return true when suucess to exectue
// send messge to main UI
//--------------------------------------------------------------------
void CModbusSvr::PostRecvDATA(CClientInfo *p)
{
	p->m_IO_type = IO_DATA;

	WSABUF pBuf;
	pBuf.buf = (char *)p->m_Data;
	pBuf.len = p->m_nDataLen;
	DWORD cbRecv = 0;
	DWORD dwFlg = 0;
	int nRet = WSARecv(p->sSocket, &pBuf, 1, &cbRecv, &dwFlg, &p->m_ol, NULL);
	if (nRet != 0)
	{
		int nError = WSAGetLastError();
		if (nError != ERROR_IO_PENDING)
		{
			char buf[100];
			sprintf_s(buf, "WSARecv error (DATA code:%d)", nError);
			::SendMessage(hWnd, WM_MODBUSSVR, 1, (LPARAM)(LPCTSTR)buf);
		}
	}
	else
	{
		//Handle message
	}
}

//--------------------------------------------------------------------
// Function:  send command about send the inforation
// return true when suucess to exectue
// send messge to main UI
//--------------------------------------------------------------------
void CModbusSvr::PostSend(CClientInfo *p)
{
	u_char sendbuf[270];
	WSABUF pBuf;
	pBuf.buf = (char *)sendbuf;
	pBuf.len = 9;
	DWORD cbSend = 0;
	DWORD dwFlag = 0;

	u_short bytelen = 2 * p->m_nLen;
	u_short framelen = bytelen + 3;
	int nChoice = p->m_nCmd;

	//copy MBAP
	for (int i = 0; i < 7; i++)
		sendbuf[i] = p->m_MBAP[i];
	sendbuf[0] = p->m_MBAP[0];
	sendbuf[1] = p->m_MBAP[1]; //Transcation ID
	sendbuf[2] = p->m_MBAP[2];
	sendbuf[3] = p->m_MBAP[3]; //Protocol ID
	sendbuf[4] = 0x00;
	sendbuf[5] = 0x03;			   //MBAP LEN
	sendbuf[6] = 0x01;			   //Modbus tcp
	sendbuf[7] = 0x80 + p->m_nCmd; // cmdno + 0x80

	//Command number support?
	if (p->m_nCmd != 3 && p->m_nCmd != 4 && p->m_nCmd != 6 && p->m_nCmd != 16)
	{
		sendbuf[8] = 0x01;
		pBuf.len = 9;
		goto exec;
	}

	//product response frame
	sendbuf[4] = (framelen & 0xFF00) >> 8;
	sendbuf[5] = framelen & 0x00FF;
	sendbuf[7] = p->m_nCmd;
	sendbuf[8] = (u_char)bytelen;

	switch (nChoice)
	{
	case 3:
	case 4:
		sendbuf[9] = (bytelen & 0xFF00) >> 8;
		sendbuf[10] = bytelen & 0x00FF;
		m_data.GetMRegByChar(p->m_nAdr, p->m_nLen, &sendbuf[9]);
		pBuf.len = bytelen + 9;
		break;

	case 6:
		sendbuf[4] = 0x00;
		sendbuf[5] = 0x06;
		sendbuf[8] = p->m_Data[1];
		sendbuf[9] = p->m_Data[2];
		sendbuf[10] = p->m_Data[3];
		sendbuf[11] = p->m_Data[4];
		m_data.SetRegByChar(p->m_nAdr,p->m_Data+3) ;
		pBuf.len = 12;
		break;

	case 16:
		sendbuf[4] = 0x00;
		sendbuf[5] = 0x06;
		sendbuf[8] = p->m_Data[1];
		sendbuf[9] = p->m_Data[2];
		sendbuf[10] = p->m_Data[3];
		sendbuf[11] = p->m_Data[4];
		m_data.SetMRegByChar(p->m_nAdr,p->m_nLen,p->m_Data + 6) ;
		pBuf.len = 12;
		break;
	}

exec:
	p->m_IO_type = IO_SEND;
	int nRet = WSASend(p->sSocket, &pBuf, 1, &cbSend, dwFlag, &p->m_ol, NULL);
	if (nRet != 0)
	{
		int nError = WSAGetLastError();
		if (nError != ERROR_IO_PENDING)
		{
			char buf[100];
			sprintf_s(buf, "WSASend error (code:%d)", nError);
			::SendMessage(hWnd, WM_MODBUSSVR, 1, (LPARAM)(LPCTSTR)buf);
		}
	}
	else
	{
		//Handle message
	}
}

//-----------------------------------------------------------------------------------------------
bool CModbusSvr::OnTimer_1s() 
{
	bool bRet = false ;
	int  n = -1 ;
	char buf[100] ;
	int step;

	m_data.SetCCDErr(m_ccd.GetCCDError()) ;
	m_data.SetCCDStatus(m_ccd.GetCCDStatus()) ;
	m_data.SetRegVal(RI_CCDISDATA,m_ccd.IsCCDDataValid()) ;
	m_data.SetRegVal(RI_CCDTEMPSTAT,m_ccd.GetCoolStatus()) ;
	m_data.SetRegVal(RI_CCDTEMPVAL,m_ccd.GetCoolTemp()) ;
	
	//拉曼扫描完成，传送数据
	if ( m_ccd.bValidData && m_data.GetProgStep() == 0)
	{
		m_data.SetMRegVal(RI_SCANDATA,PIXEL_FLAG,(short*)m_ccd.pixel_data) ;
		m_ccd.bValidData = false ;
		bRet = true ;
	}		

	//更新时间寄存器数据
	m_data.DTimeGet() ;

	if ( m_data.GetRegVal(RH_PROGSART) )
	{
		//检查是否到了开机时间
		n = m_data.IsTimeForPower(m_nScanGrp,m_nScanNum) ;
		if ( n != -1 )
		{
			m_nBootNo = n ;
			m_nScanNo = 1 ;
			m_nTry = 0 ;
			sprintf_s(buf, "Power On and start CCD( Group: %d)", n + 1);
			::SendMessage(hWnd, WM_MODBUSSVR, 1, (LPARAM)(LPCTSTR)buf);
			short reg[6] ;
			m_data.GetMRegVal(RH_ACQMODE1 + 6*m_nScanGrp,6,reg) ;
			m_ccd.TransSetting(reg) ;
			m_data.SwitchCCD(true) ;
			m_data.SetProgStep(1) ;
		}
		m_data.IncProgStep() ;
	}

	step = m_data.GetProgStep() ;
	if ( step > 5 && step < 50) 
	{
		if ( step > 50 )
		{
			if ( m_nTry++ < 3 )
			{
				m_ccd.InitialCCD() ;
				m_data.SetProgStep(1) ;
			}
			else
			{
				sprintf_s(buf, "Initialize faiure ( Group: %d)", m_nBootNo + 1);
				::SendMessage(hWnd, WM_MODBUSSVR, 1, (LPARAM)(LPCTSTR)buf);
				m_data.SetProgStep(5000) ;
			}
		}
		else
		{
			if ( m_ccd.GetCCDStatus() == 10)
			{
				sprintf_s(buf, "Finish init CCD and cool ( Group: %d)", m_nBootNo + 1);
				::SendMessage(hWnd, WM_MODBUSSVR, 1, (LPARAM)(LPCTSTR)buf);
				m_ccd.Cooler(true) ;
				m_data.SetProgStep(1000) ;
			}
		}
	}
		
	//near set temperature, switch on laser
	if ( step > 1005 && step < 2000 )
	{
		if ( step > 1900)
		{
			sprintf_s(buf, "Cooler faiure ( Group: %d)", m_nBootNo + 1);
			::SendMessage(hWnd, WM_MODBUSSVR, 1, (LPARAM)(LPCTSTR)buf);
			m_data.SetProgStep(4000) ;			
		}
		else
			if ( m_ccd.GetCoolTemp() < (m_ccd.GetCoolSet() + 5) )
			{
				sprintf_s(buf, "Switch on laser ( Group: %d)", m_nBootNo + 1);
				::SendMessage(hWnd, WM_MODBUSSVR, 1, (LPARAM)(LPCTSTR)buf);
				m_data.SwitchLaser(true) ;
				m_data.SetProgStep(2000) ;
			}
	}

	if ( step > 2005 && step < 3000 )
	{
		//start sacaning-----------
		if ( m_ccd.GetCoolTemp() < m_ccd.GetCoolSet() )
		{
			sprintf_s(buf, "Scan start/%d ( Group: %d)", m_nScanNo, m_nBootNo + 1);
			::SendMessage(hWnd, WM_MODBUSSVR, 1, (LPARAM)(LPCTSTR)buf);
			m_data.SetProgStep(3000) ;
			m_data.SetRegVal(RH_SCANING,1) ;
		}
	}

	if ( step > 3005 && step < 4000 )
	{
		if ( step > 3900)
		{
			sprintf_s(buf, "Scan faiure ( Group: %d)", m_nBootNo + 1);
			::SendMessage(hWnd, WM_MODBUSSVR, 1, (LPARAM)(LPCTSTR)buf);
			m_data.SwitchLaser(false) ;
			m_ccd.Cooler(false) ;
			m_data.SetProgStep(4000) ;			
		}	
		else
			//拉曼扫描完成，传送数据
			if ( m_ccd.GetCCDStatus() == 30)
			{
				sprintf_s(buf, "Scan finish /%d ( Group: %d)",m_nScanNo, m_nBootNo + 1);
				::SendMessage(hWnd, WM_MODBUSSVR, 1, (LPARAM)(LPCTSTR)buf);
				//扫描次数判断
				if ( m_nScanNo++ < m_nScanNum  )
				{
					sprintf_s(buf, "Scan start/%d ( Group: %d)", m_nScanNo, m_nBootNo + 1);
					::SendMessage(hWnd, WM_MODBUSSVR, 1, (LPARAM)(LPCTSTR)buf);
					m_data.SetRegVal(RH_SCANING,1) ;
					m_data.SetProgStep(3000) ;
				}
				else
				{
					m_data.SwitchLaser(false) ;
					m_ccd.Cooler(false) ;
					m_data.SetProgStep(4000) ;
				}
			}
	}

	//waiting util temp > -20
	if ( step > 4005 && step < 5000 )
	{
		//near set temperature, switch on laser
		if ( m_ccd.GetCoolTemp() > 180 )
		{
			sprintf_s(buf, "CCD Temp higher than -20 ( Group: %d)", m_nBootNo + 1);
			::SendMessage(hWnd, WM_MODBUSSVR, 1, (LPARAM)(LPCTSTR)buf);
			m_data.SetProgStep(5000) ;	
		}
	}

	//switch off camera
	if ( step > 5002  )
	{
		sprintf_s(buf, "Switch off CCD ( Group: %d)", m_nBootNo + 1);
		::SendMessage(hWnd, WM_MODBUSSVR, 1, (LPARAM)(LPCTSTR)buf);
		m_data.SwitchCCD(false) ;
		m_data.SwitchLaser(false) ;
	}
	return bRet ;
}

//end of file