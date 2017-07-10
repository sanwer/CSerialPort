/******************************************************************************
**  Filename    CSerialPort.h
**  Purpose     This class can read, write and watch one serial port.
**              It sends messages to its owner when something happends on the port
**              The class creates a thread for reading and writing so the main
**              program is not blocked.
**  Author      Remon Spekreijse, mrlong, liquanhai, viruscamp, itas109, sanwer
******************************************************************************/
#ifndef __SERIALPORT_H__
#define __SERIALPORT_H__

#define COMM_BREAK_DETECTED		1	// A break was detected on input.
#define COMM_CTS_DETECTED		2	// The CTS (clear-to-send) signal changed state.
#define COMM_DSR_DETECTED		3	// The DSR (data-set-ready) signal changed state.
#define COMM_ERR_DETECTED		4	// A line-status error occurred. Line-status errors are CE_FRAME, CE_OVERRUN, and CE_RXPARITY.
#define COMM_RING_DETECTED		5	// A ring indicator was detected.
#define COMM_RLSD_DETECTED		6	// The RLSD (receive-line-signal-detect) signal changed state.
#define COMM_RXCHAR				7	// A character was received and placed in the input buffer.
#define COMM_RXFLAG_DETECTED	8	// The event character was received and placed in the input buffer.
#define COMM_TXEMPTY_DETECTED	9	// The last character in the output buffer was sent.

#define COMM_MAX_PORT_NUMBER 200   //最大串口总个数

class ISerialPortSink
{
public:
	virtual void OnCommEvent(UINT nPort,UINT nMsg,PVOID pParam,UINT nLen) = 0;
};

class CSerialPort
{
public:
	CSerialPort();
	virtual ~CSerialPort();

	// port initialisation
	DWORD	OpenPort(ISerialPortSink* pSink=NULL,
					 UINT portnr = 1,
					 DWORD baud = 9600,
					 char parity = NOPARITY,
					 BYTE databits = 8,
					 BYTE stopsbits = ONESTOPBIT,
					 DWORD dwCommEvents = EV_RXCHAR | EV_CTS, UINT nBufferSize = 512,
					 DWORD ReadIntervalTimeout = 1000,
					 DWORD ReadTotalTimeoutMultiplier = 1000,
					 DWORD ReadTotalTimeoutConstant = 1000,
					 DWORD WriteTotalTimeoutMultiplier = 1000,
					 DWORD WriteTotalTimeoutConstant = 1000);
	DWORD	GetWriteBufferSize(){return m_nWriteBufferSize;}//获取写缓冲大小
	DWORD	GetCommEvents(){return m_dwCommEvents;}//获取事件
	DCB		GetDCB(){return m_dcb;}//获取DCB
	BOOL	WriteToPort(LPBYTE lpBuffer, int nBytesToWrite);
	void	ClosePort();
	BOOL	IsOpen();

protected:
	void	ProcessErrorMessage(char* ErrorText);///错误处理
	static	DWORD WINAPI CommThread(LPVOID pParam);///线程函数
	static void	ReceiveChar(CSerialPort* port);
	static void	WriteChar(CSerialPort* port);

	// thread
	HANDLE				m_Thread;

	// synchronisation objects
	CRITICAL_SECTION	m_csCommunicationSync;///临界资源
	BOOL				m_bThreadAlive;///监视线程运行标志

	// handles
	HANDLE				m_hShutdownEvent;  //stop发生的事件
	HANDLE				m_hComm;		   // 串口句柄
	HANDLE				m_hWriteEvent;	 // write

	// Event array.
	// One element is used for each event. There are two event handles for each port.
	// A Write event and a receive character event which is located in the overlapped structure (m_ov.hEvent).
	// There is a general shutdown when the port is closed.
	///事件数组，包括一个写事件，接收事件，关闭事件
	///一个元素用于一个事件。有两个事件线程处理端口。
	///写事件和接收字符事件位于overlapped结构体（m_ov.hEvent）中
	///当端口关闭时，有一个通用的关闭。
	HANDLE				m_hEventArray[3];

	// structures
	OVERLAPPED			m_ov;///异步I/O
	COMMTIMEOUTS		m_CommTimeouts;///超时设置
	DCB					m_dcb;///设备控制块

	// misc
	UINT				m_nPortNr;		///串口号
	DWORD				m_dwCommEvents;
	char*				m_szWriteBuffer;///写缓冲区
	DWORD				m_nWriteBufferSize;///写缓冲大小
	int					m_nWriteSize;//写入字节数
	BOOL				m_bHasWritten;///写入标志

private:
	ISerialPortSink* m_pSink;
};

#endif
