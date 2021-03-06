/*
**	FILENAME			CSerialPort.h
**
**	PURPOSE				This class can read, write and watch one serial port.
**								It sends messages to its owner when something happends on the port
**								The class creates a thread for reading and writing so the main
**								program is not blocked.
**

**	AUTHOR				Chen Ming
**
**
*/

#ifndef __SERIALPORT_H__
#define __SERIALPORT_H__

#define WM_COMM_BREAK_DETECTED WM_USER + 1   // 检测有数据到达的中断
#define WM_COMM_CTS_DETECTED WM_USER + 2	 // 检测可以发送数据状态
#define WM_COMM_DSR_DETECTED WM_USER + 3	 // 检测发送数据已经准备好
#define WM_COMM_ERR_DETECTED WM_USER + 4	 // 发生错误
#define WM_COMM_RING_DETECTED WM_USER + 5	// 检测到响铃
#define WM_COMM_RLSD_DETECTED WM_USER + 6	// The RLSD (receive-line-signal-detect) signal changed state.
#define WM_COMM_RXCHAR WM_USER + 7			 // A character was received and placed in the input buffer.
#define WM_COMM_RXFLAG_DETECTED WM_USER + 8  // The event character was received and placed in the input buffer.
#define WM_COMM_TXEMPTY_DETECTED WM_USER + 9 // The last character in the output buffer was sent.

class CSerialPort
{
  public:
	// contruction and destruction
	//构造函数
	CSerialPort();
	//析构函数
	virtual ~CSerialPort();

	// port initialisation
	//端口初始化
	BOOL InitPort(HWND hwnd, UINT portnr = 1, UINT baud = 19200, char parity = 'N', UINT databits = 8, UINT stopsbits = 1, DWORD dwCommEvents = EV_RXCHAR | EV_CTS, UINT nBufferSize = 512);

	int nPortNum;
	bool m_bOpen;

	// start/stop comm watching
	//启动端口监视
	BOOL StartMonitoring();
	//重启端口监视
	BOOL RestartMonitoring();
	//停止端口监视
	BOOL StopMonitoring();
	//获得写缓冲区大小
	DWORD GetWriteBufferSize();
	//
	DWORD GetCommEvents();
	DCB GetDCB();
	//往端口写数据
	void WriteToPort(char *string);

	void WriteToPort(char *string, int nLen);

	void ClosePort(void);

  protected:
	// protected memberfunctions
	void ProcessErrorMessage(TCHAR *ErrorText);
	static UINT CommThread(LPVOID pParam);
	//static void	ReceiveChar(CSerialPort* port, COMSTAT comstat);
	static void ReceiveChar(CSerialPort *port);
	static void WriteChar(CSerialPort *port);

	// thread
	//通信线程
	CWinThread *m_Thread;

	// synchronisation objects
	//临界区对象
	CRITICAL_SECTION m_csCommunicationSync;
	//通信线程是否存在
	BOOL m_bThreadAlive;

	// handles
	//关闭事件对象句柄
	HANDLE m_hShutdownEvent;
	//串行通信设备句柄
	HANDLE m_hComm;
	//写事件对象句柄
	HANDLE m_hWriteEvent;

	//事件对象数组，一共三个，一个是写入数据事件对象，一个是接收数据事件对象
	//还有一个是串行通信端口关闭事件对象。
	HANDLE m_hEventArray[3];

	//I/O交叠结构体
	OVERLAPPED m_ov;
	//串行设备的超时设定结构体
	COMMTIMEOUTS m_CommTimeouts;
	//串行通信设备的结构体
	DCB m_dcb;
	//所属窗口句柄
	CWnd *m_pOwner;
	HWND m_hWnd ;

	//串口通信设备序号
	UINT m_nPortNr;
	//写缓冲区
	char *m_szWriteBuffer;
	DWORD m_dwFrameLen;

	DWORD m_dwCommEvents;
	//写缓冲区大小
	DWORD m_nWriteBufferSize;
};

#endif __SERIALPORT_H__
