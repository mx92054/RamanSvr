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
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


#pragma comment(lib,"mswsock.lib") 
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
//------------------------------------
CModbusSvr::CModbusSvr()
{
	dwEventTotal = 0 ;
	dwRecvBytes = 0 ;
	Flag = 0 ;
	nPort = PORT ;
	bRunning = false ;

	InitializeCriticalSection(&m_cs) ;
}

//------------------------------------
CModbusSvr::~CModbusSvr()
{
	DeleteCriticalSection(&m_cs) ;
}

//--------------------------------------------------------------------
// Function:  Initialize socket
//  return true when success
//	return false when failure
//--------------------------------------------------------------------
bool CModbusSvr::InitSocket()
{
	WSADATA wsaData ;
	WSAStartup(MAKEWORD(2,2),&wsaData) ;

//-----------------------------------------------------------------------
	ListenSocket = WSASocket(AF_INET,SOCK_STREAM,0,NULL,0,WSA_FLAG_OVERLAPPED) ;
	if ( ListenSocket == INVALID_SOCKET )
	{
		WSACleanup() ;
		return false ;
	}

//-----------------------------------------------------------------------
	SOCKADDR_IN ServerAddr ;
	ServerAddr.sin_family = AF_INET ;
	ServerAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY) ;
	ServerAddr.sin_port = htons(nPort) ;

	if ( bind(ListenSocket,(LPSOCKADDR)&ServerAddr,sizeof(ServerAddr)) == SOCKET_ERROR )
	{
		closesocket(ListenSocket) ;
		WSACleanup() ;
		return false ;
	}

//-----------------------------------------------------------------------
	if ( listen(ListenSocket,SOMAXCONN) == SOCKET_ERROR )
	{
		closesocket(ListenSocket) ;
		WSACleanup() ;
		return false ;
	}


	PostAccept(ListenSocket) ;

	bRunning = true ;
	DWORD dwThreadID ;
	m_hThreadService = CreateThread(NULL,0,ServerAcceptThread,this,0,&dwThreadID) ;

	return true ;
}

//--------------------------------------------------------------------
// Function:  Work thread to handle to response event
//  keep running after initialize socket
//	
//--------------------------------------------------------------------
DWORD WINAPI  CModbusSvr::ServerAcceptThread(void* pParam) 
{
	CModbusSvr* pSvr = (CModbusSvr*)pParam ;
	char buf[100] ;

	while(pSvr->bRunning)
	{
		pSvr->LockClientArray() ;  //lock critical area 

		int nClientCount = pSvr->m_ClientArray.GetSize() ;
		WSAEVENT* pEvent = NULL ;
		if ( nClientCount > 0 )
		{
			pEvent = new WSAEVENT[nClientCount] ;
			for(int i = 0 ; i < nClientCount ; i++)
			{
				pEvent[i] = pSvr->m_ClientArray.GetAt(i)->m_ol.hEvent ;
			}
		}
		pSvr->UnlockClientArray() ;

		if ( pEvent == NULL )
		{
			Sleep(1000) ;
			continue ;
		}

		DWORD dwWaitRet = ::WaitForMultipleObjects(nClientCount,pEvent,FALSE,10000) ;
		if ( dwWaitRet == WAIT_FAILED )
		{

		}
		else if ( dwWaitRet == WAIT_TIMEOUT )
		{

		}
		else
		{
			pSvr->LockClientArray() ;
			CClientInfo* pClient = pSvr->m_ClientArray.GetAt(dwWaitRet - WAIT_OBJECT_0) ;
			WSAResetEvent(pClient->m_ol.hEvent) ;

			switch ( pClient->m_IO_type )
			{
			case IO_MBAP:		//received MBAP 
				if ( pClient->m_ol.InternalHigh == 0 )
				{
					pSvr->OnSocketDisconnect(pClient->sSocket) ;
				}
				else
				{
					unsigned int Len = 	pClient->m_MBAP[4]<< 8 | pClient->m_MBAP[5] ;
					if ( Len > 255 && Len < 2 )
					{
						sprintf(buf,"Recv data format error!") ;
						::SendMessage(pSvr->hWnd,WM_MODBUSSVR,0,(LPARAM)(LPCTSTR)buf) ;
						pSvr->PostRecvMBAP(pClient) ;
					}
					else
					{
						pClient->m_nDataLen = Len - 1;
						pSvr->PostRecvDATA(pClient) ;
					}						
				}
				break ;

			case IO_DATA:	//received DATA
				if ( pClient->m_ol.InternalHigh == 0 )
				{
					pSvr->OnSocketDisconnect(pClient->sSocket) ;
				}
				else
				{
					unsigned int ID,IP,Len ;
					ID = pClient->m_MBAP[0] << 8 | pClient->m_MBAP[1] ;
					IP = pClient->m_MBAP[2] << 8 | pClient->m_MBAP[3] ;
					Len = pClient->m_MBAP[4]<< 8 | pClient->m_MBAP[5] ;
					
					pClient->AttachData() ;

					sprintf(buf,"%s:%d Tx:%d Pt:%d Len:%d,Net:%d Cmd:%d Ad:%d Num:%d",
											pClient->strIP,pClient->nPort,
											ID,IP,Len,pClient->m_MBAP[6],
											pClient->m_nCmd,pClient->m_nAdr,pClient->m_nLen) ;
					::SendMessage(pSvr->hWnd,WM_MODBUSSVR,0,(LPARAM)(LPCTSTR)buf) ;	
						
					pSvr->PostSend(pClient) ;
				}
				break ;

			case IO_SEND:	// finished to send
				if ( pClient->m_ol.InternalHigh == 0 )
				{
					pSvr->OnSocketDisconnect(pClient->sSocket) ;
				}
				else
				{

					pSvr->PostRecvMBAP(pClient) ;
				}
				break ;

			case IO_ACCEPT:		// client havd accept 
				setsockopt(pClient->sAcceptSocket,SOL_SOCKET,SO_UPDATE_ACCEPT_CONTEXT,
							(char*)&pClient->sAcceptSocket,
							sizeof(pClient->sAcceptSocket)) ;
				CClientInfo* pNewClient = pSvr->OnSocketConnected(pClient->sAcceptSocket,NULL) ;
				pSvr->PostRecvMBAP(pNewClient) ;
				pSvr->PostAccept(pClient) ;
				break ;
			}

			pSvr->UnlockClientArray() ;		//unlock critical area
		}

		delete[] pEvent ;
	}

	return 0 ;
}

//--------------------------------------------------------------------
// Function:  handle when client conntect to this server
//  return client information
//	send messge to main UI
//--------------------------------------------------------------------
CClientInfo* CModbusSvr::OnSocketConnected(SOCKET sClientSocket,sockaddr_in* saClient)
{
	bool bDelete = false ;
	char buf[100] ;

	if ( saClient == NULL )
	{
		sockaddr_in* pTempClientAddr = new sockaddr_in ;
		int nLen = sizeof(sockaddr_in) ;
		if ( SOCKET_ERROR == getpeername(sClientSocket,(sockaddr*)pTempClientAddr,&nLen) )
		{
			int nErrorCode = ::WSAGetLastError() ;
			if ( SOCKET_ERROR == getpeername(sClientSocket,(sockaddr*)pTempClientAddr,&nLen) )
			{
					sprintf(buf,"get peer name error") ;
					::SendMessage(hWnd,WM_MODBUSSVR,0,(LPARAM)(LPCTSTR)buf) ;	
			}
		}
		saClient = (sockaddr_in*)pTempClientAddr ;
		bDelete = true ;
	}

	u_short uPort = ntohs(((sockaddr_in *)saClient)->sin_port) ;
	CString strIP = inet_ntoa(saClient->sin_addr) ;

	CClientInfo* pClient = new CClientInfo ;
	pClient->nPort = uPort ;
	pClient->strIP = strIP ;
	pClient->sSocket = sClientSocket ;
	pClient->m_IO_type = IO_MBAP ;

	LockClientArray() ;
	m_ClientArray.Add(pClient) ;
//-------------------------------------------------
	UnlockClientArray() ;

	if ( bDelete )
		delete saClient ;
	
	sprintf(buf,"%s:%d connect",pClient->strIP,pClient->nPort) ;
	::SendMessage(hWnd,WM_MODBUSSVR,0,(LPARAM)(LPCTSTR)buf) ;
	return pClient ;
}

//--------------------------------------------------------------------
// Function:  handle when client disconntect to this server
// delete client information  
// send messge to main UI
//--------------------------------------------------------------------
void CModbusSvr::OnSocketDisconnect(SOCKET aClientSocket)
{
	char buf[100] ;

	LockClientArray() ;
	for(int i = 0 ; i < m_ClientArray.GetSize() ; i++) 
	{
		CClientInfo* pClient = m_ClientArray.GetAt(i) ;
		
		if ( pClient->sSocket == aClientSocket )
		{
			sprintf(buf,"%s:%d disconnect",pClient->strIP,pClient->nPort) ;
			::SendMessage(hWnd,WM_MODBUSSVR,0,(LPARAM)(LPCTSTR)buf) ;			
			delete pClient ;
			m_ClientArray.RemoveAt(i) ;
		}
	}
	UnlockClientArray() ;

}

//--------------------------------------------------------------------
// Function:  handle when client response to accept
// return true when suucess to accept 
// send messge to main UI
//--------------------------------------------------------------------
bool CModbusSvr::PostAccept(SOCKET sListenSocket)
{
	CClientInfo* pClient = new CClientInfo ;
	SOCKET	sClientSocket = WSASocket(AF_INET,SOCK_STREAM,IPPROTO_TCP,NULL,0,WSA_FLAG_OVERLAPPED) ;
	if ( sClientSocket == INVALID_SOCKET )
	{
		WSACleanup() ;
		return false ;
	}	
	pClient->sSocket = sListenSocket ;
	pClient->m_IO_type = IO_ACCEPT ;
	pClient->sAcceptSocket = sClientSocket ;

	DWORD dwBytesReceived = 0 ;
	int nRet = AcceptEx(pClient->sSocket,pClient->sAcceptSocket,pClient->m_MBAP,0,
						sizeof(sockaddr_in) + 16,sizeof(sockaddr_in)+16,&dwBytesReceived,
						&pClient->m_ol) ;
	if ( nRet || ::WSAGetLastError() == ERROR_IO_PENDING)
	{
		LockClientArray() ;
		m_ClientArray.Add(pClient) ;
		UnlockClientArray() ;
		return true ;
	}
	
	return false ;
}

//--------------------------------------------------------------------
// Function:  handle when client response to accept
// return true when suucess to accept 
// send messge to main UI
//--------------------------------------------------------------------
bool CModbusSvr::PostAccept(CClientInfo* pClient)
{
	SOCKET	sClientSocket = WSASocket(AF_INET,SOCK_STREAM,IPPROTO_TCP,NULL,0,WSA_FLAG_OVERLAPPED) ;
	if ( sClientSocket == INVALID_SOCKET )
	{
		WSACleanup() ;
		return false ;
	}	
	pClient->m_IO_type = IO_ACCEPT ;
	pClient->sAcceptSocket = sClientSocket ;

	DWORD dwBytesReceived = 0 ;
	int nRet = AcceptEx(pClient->sSocket,pClient->sAcceptSocket,pClient->m_MBAP,0,
						sizeof(sockaddr_in) + 16,sizeof(sockaddr_in)+16,&dwBytesReceived,
						&pClient->m_ol) ;
	if ( nRet || ::WSAGetLastError() == ERROR_IO_PENDING)
	{
		return true ;
	}

	char buf[100] ;
	sprintf(buf,"AcceptEx error") ;
	::SendMessage(hWnd,WM_MODBUSSVR,0,(LPARAM)(LPCTSTR)buf) ;	
	return false ;
}

//--------------------------------------------------------------------
// Function:  send command about recieving the MBAP inforation
// return true when suucess to exectue 
// send messge to main UI
//--------------------------------------------------------------------
void CModbusSvr::PostRecvMBAP(CClientInfo* p)
{
	p->m_IO_type = IO_MBAP ;

	WSABUF pBuf ;
	pBuf.buf = (char*)p->m_MBAP ;
	pBuf.len = 7 ;
	DWORD cbRecv = 0 ;
	DWORD dwFlg = 0 ;
	int nRet = WSARecv(p->sSocket,&pBuf,1,&cbRecv,&dwFlg,&p->m_ol,NULL) ;
	if ( nRet != 0 )
	{
		int nError = WSAGetLastError() ;
		if ( nError != ERROR_IO_PENDING )
		{
			char buf[100] ;
			sprintf(buf,"WSARecv error (MBAP code:%d)",nError) ;
			::SendMessage(hWnd,WM_MODBUSSVR,0,(LPARAM)(LPCTSTR)buf) ;	
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
void CModbusSvr::PostRecvDATA(CClientInfo* p)
{
	p->m_IO_type = IO_DATA ;

	WSABUF pBuf ;
	pBuf.buf = (char*)p->m_Data ;
	pBuf.len = p->m_nDataLen ;
	DWORD cbRecv = 0 ;
	DWORD dwFlg = 0 ;
	int nRet = WSARecv(p->sSocket,&pBuf,1,&cbRecv,&dwFlg,&p->m_ol,NULL) ;
	if ( nRet != 0 )
	{
		int nError = WSAGetLastError() ;
		if ( nError != ERROR_IO_PENDING )
		{
			char buf[100] ;
			sprintf(buf,"WSARecv error (DATA code:%d)",nError) ;
			::SendMessage(hWnd,WM_MODBUSSVR,0,(LPARAM)(LPCTSTR)buf) ;
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
void CModbusSvr::PostSend(CClientInfo* p)
{
	u_char sendbuf[270] ;
	WSABUF pBuf ;
	pBuf.buf = (char*)sendbuf ;
	pBuf.len = 9 ;
	DWORD cbSend = 0 ;
	DWORD dwFlag = 0 ;

	u_short bytelen = 2*p->m_nLen ;
	u_short framelen = bytelen + 3 ;
	int nChoice = p->m_nCmd ;

	//copy MBAP
	for(int i = 0 ; i < 7 ; i++ )
		sendbuf[i] = p->m_MBAP[i] ;	
	sendbuf[0] = p->m_MBAP[0] ;		sendbuf[1] = p->m_MBAP[1] ;		//Transcation ID
	sendbuf[2] = p->m_MBAP[2] ;		sendbuf[3] = p->m_MBAP[3] ;		//Protocol ID
	sendbuf[4] = 0x00 ;				sendbuf[5] = 0x03 ;				//MBAP LEN
	sendbuf[6] = 0x01 ;												//Modbus tcp
	sendbuf[7] = 0x80 + p->m_nCmd ;									// cmdno + 0x80

	//Command number support?
	if ( p->m_nCmd != 3 && p->m_nCmd != 4 && p->m_nCmd != 6 && p->m_nCmd != 16 )
	{
		sendbuf[8] = 0x01 ;
		pBuf.len = 9 ;
		goto exec ;
	}

	//Address is valid?
	if ( (p->m_nCmd == 3  && !m_Spectrum.IsValidAddressOfGet(p->m_nAdr,p->m_nLen) ) ||
		 (p->m_nCmd == 4  && !m_Spectrum.IsValidAddressOfGet(p->m_nAdr,p->m_nLen) ) ||
		 (p->m_nCmd == 6  && !m_Spectrum.IsValidAddressOfSet(p->m_nAdr,1) )		    ||
		 (p->m_nCmd == 16 && !m_Spectrum.IsValidAddressOfSet(p->m_nAdr,p->m_nLen) ) ||
		 (p->m_nCmd != 6  && p->m_nLen > 125) )
	{
		sendbuf[8] = 0x02 ;
		pBuf.len = 9 ;
		goto exec ;
	}

	//product response frame
	sendbuf[4] = (framelen & 0xFF00) >> 8 ;
	sendbuf[5] = framelen & 0x00FF ;
	sendbuf[7] = p->m_nCmd ;
	sendbuf[8] = (u_char)bytelen ;

	switch ( nChoice )
	{
	case 3:
	case 4:
		sendbuf[9] = (bytelen & 0xFF00) >> 8 ;
		sendbuf[10] = bytelen & 0x00FF ;
		m_Spectrum.GetMultReg(p->m_nAdr,p->m_nLen,&sendbuf[9]) ;
		pBuf.len = bytelen + 9 ;
		break ;
	case 6:
		sendbuf[4] = 0x00 ;				sendbuf[5] = 0x06 ;
		sendbuf[8] = p->m_Data[1] ;		sendbuf[9] = p->m_Data[2] ;		 
		sendbuf[10] = p->m_Data[3] ;	sendbuf[11] = p->m_Data[4] ;
		m_Spectrum.SetMultReg(p->m_nAdr, 1, &p->m_Data[3]) ;
		pBuf.len = 12 ;
		break ;
	case 16:
		sendbuf[4] = 0x00 ;				sendbuf[5] = 0x06 ;
		sendbuf[8] = p->m_Data[1] ;		sendbuf[9] = p->m_Data[2] ;		 
		sendbuf[10] = p->m_Data[3] ;	sendbuf[11] = p->m_Data[4] ;
		m_Spectrum.SetMultReg(p->m_nAdr, p->m_nLen, &p->m_Data[6]) ;
		pBuf.len = 12 ;
		break ;
	}


exec:
	p->m_IO_type = IO_SEND ;
	int nRet = WSASend(p->sSocket,&pBuf,1,&cbSend,dwFlag,&p->m_ol,NULL) ;
	if ( nRet != 0 )
	{
		int nError = WSAGetLastError() ;
		if ( nError != ERROR_IO_PENDING )
		{
			char buf[100] ;
			sprintf(buf,"WSASend error (code:%d)",nError) ;
			::SendMessage(hWnd,WM_MODBUSSVR,0,(LPARAM)(LPCTSTR)buf) ;	
		}
	}
	else
	{
		//Handle message
	}
}




//end of file