/*
**	FILENAME			CSerialPort.cpp
**
**	PURPOSE				This class can read, write and watch one serial port.
**								It sends messages to its owner when something happends on the port
**								The class creates a thread for reading and writing so the main
**								program is not blocked.
**
**
**	AUTHOR				Chen Ming
**
**
*/
//这个类完成读、写和监视一个串口通信端口，当串行端口发生事件它会发送一个消息给其所属窗口，
//该类创建一个读数据线程和一个写数据线程，使主程序不会被堵塞。

#include "stdafx.h"
#include "SerialPort.h"
#include <assert.h>
// 构造函数
CSerialPort::CSerialPort()
{
	m_hComm = NULL;

	//初始化交叠数据变量为空。
	m_ov.Offset = 0;
	m_ov.OffsetHigh = 0;

	//初始化事件对象
	m_ov.hEvent = NULL;
	m_hWriteEvent = NULL;
	m_hShutdownEvent = NULL;

	m_szWriteBuffer = NULL;
	m_bThreadAlive = FALSE;
	m_bOpen = false;
}

//析构函数
//删除动态内存
CSerialPort::~CSerialPort()
{
	do
	{
		SetEvent(m_hShutdownEvent);
	} while (m_bThreadAlive);

	TRACE("Thread ended\n");

	CloseHandle(m_hComm);

	delete[] m_szWriteBuffer;
}

//
// Initialize the port. This can be port 1 to 4.
//初始化串行端口，
BOOL CSerialPort::InitPort(HWND hwnd,	 // 接收数据端口所属窗口
						   UINT portnr,			 // 端口号
						   UINT baud,			 // 波特率
						   char parity,			 // 奇偶位
						   UINT databits,		 // 数据位
						   UINT stopbits,		 // 停止位
						   DWORD dwCommEvents,   // 通信事件
						   UINT writebuffersize) // 写缓冲区大小
{

	assert(portnr >= 0 && portnr <= 25);

	if (portnr < 0 || portnr > 25 )
		return false;

	// 如果通信线程存在，就删除
	if (m_bThreadAlive)
	{
		do
		{
			SetEvent(m_hShutdownEvent);
		} while (m_bThreadAlive);
		TRACE("Thread ended\n");
	}

	//创建事件对象
	if (m_ov.hEvent != NULL)
		ResetEvent(m_ov.hEvent);
	m_ov.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	if (m_hWriteEvent != NULL)
		ResetEvent(m_hWriteEvent);
	m_hWriteEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	if (m_hShutdownEvent != NULL)
		ResetEvent(m_hShutdownEvent);
	m_hShutdownEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	//初始化事件对象数组
	m_hEventArray[0] = m_hShutdownEvent;
	m_hEventArray[1] = m_ov.hEvent;
	m_hEventArray[2] = m_hWriteEvent;

	//初始化临界区
	InitializeCriticalSection(&m_csCommunicationSync);

	//为写入数据设置大小并保存所属窗口句柄
	m_hWnd = hwnd;
	if (m_szWriteBuffer != NULL)
		delete[] m_szWriteBuffer;
	m_szWriteBuffer = new char[writebuffersize];

	m_nPortNr = portnr;

	m_nWriteBufferSize = writebuffersize;
	m_dwCommEvents = dwCommEvents;

	BOOL bResult = FALSE;
	TCHAR *szPort = new TCHAR[50];
	TCHAR *szBaud = new TCHAR[50];

	//获得临界区所有权
	EnterCriticalSection(&m_csCommunicationSync);

	//如果端口已经打开，关闭它
	if (m_hComm != NULL)
	{
		CloseHandle(m_hComm);
		m_hComm = NULL;
	}

	//设置串行端口名
	//wsprintf(szPort, "\\\\.\\%s", strPortList[nPortNum-1-portnr]);
	sprintf(szBaud, "baud=%d parity=%c data=%d stop=%d", baud, parity, databits, stopbits);

	// 获得串行端口句柄
	wsprintf(szPort, "\\\\.\\COM%d", portnr);
	m_hComm = CreateFile(szPort,					   // communication port string (COMX)
						 GENERIC_READ | GENERIC_WRITE, // read/write types
						 0,							   // comm devices must be opened with exclusive access
						 NULL,						   // no security attributes
						 OPEN_EXISTING,				   // comm devices must use OPEN_EXISTING
						 FILE_FLAG_OVERLAPPED,		   // Async I/O
						 0);						   // template must be 0 for comm devices

	if (m_hComm == INVALID_HANDLE_VALUE)
	{
		// 端口找不到
		delete[] szPort;
		delete[] szBaud;

		return FALSE;
	}

	// 设置超时值
	m_CommTimeouts.ReadIntervalTimeout = 1000;
	m_CommTimeouts.ReadTotalTimeoutMultiplier = 1000;
	m_CommTimeouts.ReadTotalTimeoutConstant = 1000;
	m_CommTimeouts.WriteTotalTimeoutMultiplier = 1000;
	m_CommTimeouts.WriteTotalTimeoutConstant = 1000;

	//设置串行端口配置
	if (SetCommTimeouts(m_hComm, &m_CommTimeouts))
	{
		if (SetCommMask(m_hComm, dwCommEvents))
		{
			if (GetCommState(m_hComm, &m_dcb))
			{
				if (BuildCommDCB(szBaud, &m_dcb))
				{
					if (SetCommState(m_hComm, &m_dcb))
						;
					else
					{
						ProcessErrorMessage("SetCommState()");
					}
				}
				else
					ProcessErrorMessage("BuildCommDCB()");
			}
			else
				ProcessErrorMessage("GetCommState()");
		}
		else
			ProcessErrorMessage("SetCommMask()");
	}
	else
		ProcessErrorMessage("SetCommTimeouts()");

	delete[] szPort;
	delete[] szBaud;

	// 清除串行端口数据
	PurgeComm(m_hComm, PURGE_RXCLEAR | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_TXABORT);

	//释放临界区所有权
	LeaveCriticalSection(&m_csCommunicationSync);

	TRACE("Initialisation for communicationport %d completed.\nUse Startmonitor to communicate.\n", portnr);

	m_bOpen = true;
	return TRUE;
}

//
// 通信线程函数
//
UINT CSerialPort::CommThread(LPVOID pParam)
{

	CSerialPort *port = (CSerialPort *)pParam;

	//设置为TRUE表示通信线程已经运行
	port->m_bThreadAlive = TRUE;

	DWORD BytesTransfered = 0;
	DWORD Event = 0;
	DWORD CommEvent = 0;
	DWORD dwError = 0;
	COMSTAT comstat;
	BOOL bResult = TRUE;

	//清除串行端口数据
	if (port->m_hComm) // 判断端口是否打开
		PurgeComm(port->m_hComm, PURGE_RXCLEAR | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_TXABORT);

	// 一个无限循环，一直到线程活动
	for (;;)
	{

		// Make a call to WaitCommEvent().  This call will return immediatly
		// because our port was created as an async port (FILE_FLAG_OVERLAPPED
		// and an m_OverlappedStructerlapped structure specified).  This call will cause the
		// m_OverlappedStructerlapped element m_OverlappedStruct.hEvent, which is part of the m_hEventArray to
		// be placed in a non-signeled state if there are no bytes available to be read,
		// or to a signeled state if there are bytes available.  If this event handle
		// is set to the non-signeled state, it will be set to signeled when a
		// character arrives at the port.

		// we do this for each port!

		bResult = WaitCommEvent(port->m_hComm, &Event, &port->m_ov);

		if (!bResult)
		{
			//如果出错，处理最后一个错误原因
			switch (dwError = GetLastError())
			{
			case ERROR_IO_PENDING:
			{
				break;
			}
			case 87:
			{
				break;
			}
			default:
			{
				port->ProcessErrorMessage("WaitCommEvent()");
				break;
			}
			}
		}
		else
		{

			bResult = ClearCommError(port->m_hComm, &dwError, &comstat);

			if (comstat.cbInQue == 0)
				continue;
		}
		// Main wait function.  This function will normally block the thread
		// until one of nine events occur that require action.
		Event = WaitForMultipleObjects(3, port->m_hEventArray, FALSE, INFINITE);

		switch (Event)
		{
		case 0:
		{
			//关闭事件
			port->m_bThreadAlive = FALSE;
			// 清除线程
			AfxEndThread(100);
			break;
		}
		case 1: //读数据事件
		{
			GetCommMask(port->m_hComm, &CommEvent);
			if (CommEvent & EV_CTS)
				::SendMessage(port->m_hWnd, WM_COMM_CTS_DETECTED, (WPARAM)0, (LPARAM)port->m_nPortNr);
			if (CommEvent & EV_RXFLAG)
				::SendMessage(port->m_hWnd, WM_COMM_RXFLAG_DETECTED, (WPARAM)0, (LPARAM)port->m_nPortNr);
			if (CommEvent & EV_BREAK)
				::SendMessage(port->m_hWnd, WM_COMM_BREAK_DETECTED, (WPARAM)0, (LPARAM)port->m_nPortNr);
			if (CommEvent & EV_ERR)
				::SendMessage(port->m_hWnd, WM_COMM_ERR_DETECTED, (WPARAM)0, (LPARAM)port->m_nPortNr);
			if (CommEvent & EV_RING)
				::SendMessage(port->m_hWnd, WM_COMM_RING_DETECTED, (WPARAM)0, (LPARAM)port->m_nPortNr);

			if (CommEvent & EV_RXCHAR)
				// 从串行端口收到数据
				ReceiveChar(port);

			break;
		}
		case 2: // 写数据事件
		{
			// Write character event from port
			WriteChar(port);
			break;
		}

		} // end switch

	} // close forever loop

	return 0;
}

//
// 启动串行端口通信监视线程
//
BOOL CSerialPort::StartMonitoring()
{
	if (!(m_Thread = AfxBeginThread(CommThread, this)))
		return FALSE;
	TRACE("Thread started\n");
	return TRUE;
}

//
//重启串行端口通信监视线程
//
BOOL CSerialPort::RestartMonitoring()
{
	TRACE("Thread resumed\n");
	m_Thread->ResumeThread();
	return TRUE;
}

//
// 挂起串行端口通信监视线程
//
BOOL CSerialPort::StopMonitoring()
{
	TRACE("Thread suspended\n");
	m_Thread->SuspendThread();
	return TRUE;
}

//
// 处理错误消息
//
void CSerialPort::ProcessErrorMessage(TCHAR *ErrorText)
{
	TCHAR *Temp = new TCHAR[200];

	LPVOID lpMsgBuf;

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(LPSTR)&lpMsgBuf,
		0,
		NULL);

	sprintf(Temp, "WARNING:  %s Failed with the following error: \n%s\nPort: %d\n", ErrorText, lpMsgBuf, m_nPortNr);
	MessageBox(NULL, Temp, "Application Error", MB_ICONSTOP);

	LocalFree(lpMsgBuf);
	delete[] Temp;
}

//
// Write a character.
//
void CSerialPort::WriteChar(CSerialPort *port)
{
	BOOL bWrite = TRUE;
	BOOL bResult = TRUE;

	DWORD BytesSent = 0;

	ResetEvent(port->m_hWriteEvent);

	//获得临界区所有权
	EnterCriticalSection(&port->m_csCommunicationSync);

	if (bWrite)
	{
		//初始化变量
		port->m_ov.Offset = 0;
		port->m_ov.OffsetHigh = 0;

		//清除串行端口数据
		PurgeComm(port->m_hComm, PURGE_RXCLEAR | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_TXABORT);

		bResult = WriteFile(port->m_hComm,		   // Handle to COMM Port
							port->m_szWriteBuffer, // Pointer to message buffer in calling finction
							port->m_dwFrameLen,	// Length of message to send
							&BytesSent,			   // Where to store the number of bytes sent
							&port->m_ov);		   // Overlapped structure

		// 处理所有错误
		if (!bResult)
		{
			DWORD dwError = GetLastError();
			switch (dwError)
			{
			case ERROR_IO_PENDING:
			{
				BytesSent = 0;
				bWrite = FALSE;
				break;
			}
			default:
			{
				port->ProcessErrorMessage("WriteFile()");
			}
			}
		}
		else
		{
			//释放临界区所有权
			LeaveCriticalSection(&port->m_csCommunicationSync);
		}
	} // end if(bWrite)

	if (!bWrite)
	{
		bWrite = TRUE;

		bResult = GetOverlappedResult(port->m_hComm, // Handle to COMM port
									  &port->m_ov,   // Overlapped structure
									  &BytesSent,	// Stores number of bytes sent
									  TRUE);		 // Wait flag

		LeaveCriticalSection(&port->m_csCommunicationSync);

		// 处理错误
		if (!bResult)
		{
			port->ProcessErrorMessage("GetOverlappedResults() in WriteFile()");
		}
	} // end if (!bWrite)

	//判断发送数据是否与要发送数据大小相同
	if (BytesSent != port->m_dwFrameLen)
	{
		TRACE("WARNING: WriteFile() error.. Bytes Sent: %d; Message Length: %d\n", BytesSent, strlen((char *)port->m_szWriteBuffer));
	}

	::SendMessage(port->m_hWnd, WM_COMM_TXEMPTY_DETECTED, (WPARAM)BytesSent, (LPARAM)port->m_nPortNr);
}

//收到数据并通知所属窗口
void CSerialPort::ReceiveChar(CSerialPort *port)
{
	COMSTAT comstat;

	BOOL bRead = TRUE;
	BOOL bResult = TRUE;
	DWORD dwError = 0;
	DWORD BytesRead = 0;
	unsigned char RXBuff;

	for (;;)
	{
		//获得临界区所有权
		EnterCriticalSection(&port->m_csCommunicationSync);

		// ClearCommError() will update the COMSTAT structure and
		// clear any other errors.
		//清除所有错误
		bResult = ClearCommError(port->m_hComm, &dwError, &comstat);

		LeaveCriticalSection(&port->m_csCommunicationSync);

		// start forever loop.  I use this type of loop because I
		// do not know at runtime how many loops this will have to
		// run. My solution is to start a forever loop and to
		// break out of it when I have processed all of the
		// data available.  Be careful with this approach and
		// be sure your loop will exit.
		// My reasons for this are not as clear in this sample
		// as it is in my production code, but I have found this
		// solutiion to be the most efficient way to do this.

		if (comstat.cbInQue == 0)
		{
			//当数据读完跳出
			break;
		}

		EnterCriticalSection(&port->m_csCommunicationSync);

		if (bRead)
		{
			bResult = ReadFile(port->m_hComm, // Handle to COMM port
							   &RXBuff,		  // RX Buffer Pointer
							   1,			  // Read one byte
							   &BytesRead,	// Stores number of bytes read
							   &port->m_ov);  // pointer to the m_ov structure
			// 处理错误
			if (!bResult)
			{
				switch (dwError = GetLastError())
				{
				case ERROR_IO_PENDING:
				{
					bRead = FALSE;
					break;
				}
				default:
				{
					port->ProcessErrorMessage("ReadFile()");
					break;
				}
				}
			}
			else
			{
				//读数据完成
				bRead = TRUE;
			}
		} // close if (bRead)

		if (!bRead)
		{
			bRead = TRUE;
			bResult = GetOverlappedResult(port->m_hComm, // Handle to COMM port
										  &port->m_ov,   // Overlapped structure
										  &BytesRead,	// Stores number of bytes read
										  TRUE);		 // Wait flag

			// 处理错误
			if (!bResult)
			{
				port->ProcessErrorMessage("GetOverlappedResults() in ReadFile()");
			}
		} // close if (!bRead)
		//释放临界区所有权
		LeaveCriticalSection(&port->m_csCommunicationSync);

		// 通知父窗口收到数据
		::SendMessage(port->m_hWnd, WM_COMM_RXCHAR, (WPARAM)RXBuff, (LPARAM)port->m_nPortNr);
	} // end forever loop
}

//写数据到串行端口设备
void CSerialPort::WriteToPort(char *string)
{
	if (m_hComm == 0)
		return;

	memset(m_szWriteBuffer, 0, sizeof(m_szWriteBuffer));
	strcpy(m_szWriteBuffer, string);
	m_dwFrameLen = strlen(string);

	// 为写数据设置事件对象
	SetEvent(m_hWriteEvent);
}

//写数据到串行端口设备
void CSerialPort::WriteToPort(char *string, int len)
{
	if (m_hComm == 0)
		return;

	memset(m_szWriteBuffer, 0, sizeof(m_szWriteBuffer));
	memcpy(m_szWriteBuffer, string, len);
	m_dwFrameLen = len;

	// 为写数据设置事件对象
	SetEvent(m_hWriteEvent);
}

//返回控制设备
DCB CSerialPort::GetDCB()
{
	return m_dcb;
}

// 返回通信设备掩码
DWORD CSerialPort::GetCommEvents()
{
	return m_dwCommEvents;
}
//返回输出缓冲区大小
DWORD CSerialPort::GetWriteBufferSize()
{
	return m_nWriteBufferSize;
}

//关闭串口设备
void CSerialPort::ClosePort(void)
{
	do
	{
		SetEvent(m_hShutdownEvent);
	} while (m_bThreadAlive);

	TRACE("Thread ended\n");

	CloseHandle(m_hComm);

	delete[] m_szWriteBuffer;

	m_hComm = NULL;

	//初始化交叠数据变量为空。
	m_ov.Offset = 0;
	m_ov.OffsetHigh = 0;

	//初始化事件对象
	m_ov.hEvent = NULL;
	m_hWriteEvent = NULL;
	m_hShutdownEvent = NULL;

	m_szWriteBuffer = NULL;
	m_bThreadAlive = FALSE;
	m_bOpen = false;
}
