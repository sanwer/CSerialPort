/******************************************************************************
**  Filename    CSerialPort.h
**  Purpose     This class can read, write and watch one serial port.
**              It sends messages to its owner when something happends on the port
**              The class creates a thread for reading and writing so the main
**              program is not blocked.
**  Author      Remon Spekreijse, mrlong, liquanhai, viruscamp, itas109, sanwer
******************************************************************************/
#include "stdafx.h"
#include "SerialPort.h"

//
// Constructor
//
CSerialPort::CSerialPort()
{
	m_hComm = NULL;
	// initialize overlapped structure members to zero
	m_ov.Offset = 0;
	m_ov.OffsetHigh = 0;

	// create events
	m_ov.hEvent = NULL;
	m_hWriteEvent = NULL;
	m_hShutdownEvent = NULL;

	m_szWriteBuffer = NULL;
	m_nWriteSize = 1;
	m_bHasWritten = TRUE;

	m_bThreadAlive = FALSE;
	
	InitializeCriticalSection(&m_csCommunicationSync);
}

// Delete dynamic memory
CSerialPort::~CSerialPort()
{
	if(m_hComm == INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_hComm);
		m_hComm = NULL;
	}

	do
	{
		SetEvent(m_hShutdownEvent);
		Sleep(50);
	} while (m_bThreadAlive);

	// if the port is still opened: close it
	if (m_hComm != NULL)
	{
		CloseHandle(m_hComm);
		m_hComm = NULL;
	}
	// Close Handles
	if(m_hShutdownEvent!=NULL)
		CloseHandle( m_hShutdownEvent);
	if(m_ov.hEvent!=NULL)
		CloseHandle( m_ov.hEvent );
	if(m_hWriteEvent!=NULL)
		CloseHandle( m_hWriteEvent );

	//TRACE("Thread ended\n");

	if(m_szWriteBuffer != NULL)
	{
		delete [] m_szWriteBuffer;
		m_szWriteBuffer = NULL;
	}

	DeleteCriticalSection(&m_csCommunicationSync);
}

//
// Initialize the port. This can be port 1 to COMM_MAX_PORT_NUMBER.
//parity: n=none,e=even,o=odd,m=mark,s=space
//databits: 5,6,7,8
//stopbits: 1,1.5,2
DWORD CSerialPort::OpenPort(ISerialPortSink* pSink,
						   UINT  portnr,		// portnumber
						   DWORD  baud,			// baudrate
						   char  parity,		// parity
						   BYTE  databits,		// databits
						   BYTE  stopbits,		// stopbits
						   DWORD dwCommEvents,	// EV_RXCHAR, EV_CTS etc
						   UINT  writebuffersize,// size to the writebuffer
						   DWORD   ReadIntervalTimeout,
						   DWORD   ReadTotalTimeoutMultiplier,
						   DWORD   ReadTotalTimeoutConstant,
						   DWORD   WriteTotalTimeoutMultiplier,
						   DWORD   WriteTotalTimeoutConstant)

{
	DWORD dwError = -1L;
	if(pSink == NULL || portnr < 0 || portnr > COMM_MAX_PORT_NUMBER)
		return dwError;

	// if the thread is alive: Kill
	if (m_bThreadAlive)
	{
		do
		{
			SetEvent(m_hShutdownEvent);
			Sleep(50);
		} while (m_bThreadAlive);
		Sleep(50);
	}

	// create events
	if (m_ov.hEvent != NULL)
		ResetEvent(m_ov.hEvent);
	else
		m_ov.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	if (m_hWriteEvent != NULL)
		ResetEvent(m_hWriteEvent);
	else
		m_hWriteEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	if (m_hShutdownEvent != NULL)
		ResetEvent(m_hShutdownEvent);
	else
		m_hShutdownEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	// initialize the event objects
	m_hEventArray[0] = m_hShutdownEvent;	// highest priority
	m_hEventArray[1] = m_hWriteEvent;
	m_hEventArray[2] = m_ov.hEvent;

	if(m_szWriteBuffer != NULL)
		delete [] m_szWriteBuffer;
	m_szWriteBuffer = new char[writebuffersize];

	m_pSink = pSink;
	m_nPortNr = portnr;

	m_nWriteBufferSize = writebuffersize;
	m_dwCommEvents = dwCommEvents;

	BOOL bResult = FALSE;
	TCHAR *szPort = new TCHAR[50];
	//TCHAR *szBaud = new TCHAR[50];

	// now it critical!
	EnterCriticalSection(&m_csCommunicationSync);

	// if the port is already opened: close it
	if (m_hComm != NULL)
	{
		CloseHandle(m_hComm);
		m_hComm = NULL;
	}

	int nStopBits;
	switch(stopbits)
	{
	case ONESTOPBIT:
		nStopBits = 1;
		break;
	case ONE5STOPBITS:
		nStopBits = 1;
		break;
	case TWOSTOPBITS:
		nStopBits = 2;
		break;
	default:
		nStopBits = 1;
		break;
	}

	BYTE bParity;
	switch(parity)
	{
	case 'N':
		bParity = NOPARITY;
		break;
	case 'E':
		bParity = EVENPARITY;
		break;
	case 'O':
		bParity = ODDPARITY;
		break;
	case 'M':
		bParity = MARKPARITY;
		break;
	case 'S':
		bParity = SPACEPARITY;
		break;
	default:
		bParity = NOPARITY;
		break;
	}

	// prepare port strings
	_stprintf_s(szPort,50, _T("\\\\.\\COM%d"), portnr);
	//_stprintf_s(szBaud,50, _T("baud=%d parity=%c data=%d stop=%d"), baud, parity, databits, nStopBits);

	// get a handle to the port
	m_hComm = CreateFile(szPort,						// communication port string (COMX)
						 GENERIC_READ | GENERIC_WRITE,	// read/write types
						 0,								// comm devices must be opened with exclusive access
						 NULL,							// no security attributes
						 OPEN_EXISTING,					// comm devices must use OPEN_EXISTING
						 FILE_FLAG_OVERLAPPED,			// Async I/O
						 0);							// template must be 0 for comm devices

	if (m_hComm == INVALID_HANDLE_VALUE)
	{
		dwError = GetLastError();
		//switch(dwError)
		//{
		//case ERROR_FILE_NOT_FOUND:
		//	Log("串口不存在");
		//	break;
		//case ERROR_ACCESS_DENIED:
		//	Log("串口拒绝访问");
		//	break;
		//}
		// port not found
		delete [] szPort;
		//delete [] szBaud;

		return dwError;
	}

	// set the timeout values
	m_CommTimeouts.ReadIntervalTimeout = ReadIntervalTimeout * 1000;
	m_CommTimeouts.ReadTotalTimeoutMultiplier = ReadTotalTimeoutMultiplier * 1000;
	m_CommTimeouts.ReadTotalTimeoutConstant = ReadTotalTimeoutConstant * 1000;
	m_CommTimeouts.WriteTotalTimeoutMultiplier = WriteTotalTimeoutMultiplier * 1000;
	m_CommTimeouts.WriteTotalTimeoutConstant = WriteTotalTimeoutConstant * 1000;

	// configure
	if (SetCommTimeouts(m_hComm, &m_CommTimeouts))
	{
		if (SetCommMask(m_hComm, dwCommEvents))
		{
			if (GetCommState(m_hComm, &m_dcb))
			{
				m_dcb.EvtChar = 'q';
				m_dcb.fRtsControl = RTS_CONTROL_ENABLE;		// set RTS bit high!
				m_dcb.BaudRate = baud;
				m_dcb.Parity = bParity;
				m_dcb.ByteSize = databits;
				m_dcb.StopBits = stopbits;
				//if (!BuildCommDCB(szBaud, &m_dcb))
				//{
					if (SetCommState(m_hComm, &m_dcb))
						; // normal operation... continue
					else
						ProcessErrorMessage("SetCommState()");
				//}
				//else
				//	ProcessErrorMessage("BuildCommDCB()");
			}
			else
				ProcessErrorMessage("GetCommState()");
		}
		else
			ProcessErrorMessage("SetCommMask()");
	}
	else
		ProcessErrorMessage("SetCommTimeouts()");

	delete [] szPort;
	//delete [] szBaud;

	// flush the port
	PurgeComm(m_hComm, PURGE_RXCLEAR | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_TXABORT);

	// release critical section
	LeaveCriticalSection(&m_csCommunicationSync);

	m_Thread = ::CreateThread (NULL, 0, CommThread, this, 0, NULL);
	if (m_Thread == NULL)
	{
		ClosePort();
		return -2;
	}

	return ERROR_SUCCESS;
}

//  The CommThread Function.
///检查串口-->进入循环{WaitCommEvent(不阻塞询问)询问事件-->如果有事件来到-->到相应处理(关闭\读\写)}
DWORD WINAPI CSerialPort::CommThread(LPVOID pParam)
{
	// Cast the void pointer passed to the thread back to
	// a pointer of CSerialPort class
	CSerialPort *port = (CSerialPort*)pParam;

	// Set the status variable in the dialog class to
	// TRUE to indicate the thread is running.
	port->m_bThreadAlive = TRUE;

	// Misc. variables
	DWORD BytesTransfered = 0;
	DWORD Event = 0;
	DWORD CommEvent = 0;
	DWORD dwError = 0;
	COMSTAT comstat;

	BOOL  bResult = TRUE;

	// Clear comm buffers at startup
	if (port->m_hComm)		// check if the port is opened
		PurgeComm(port->m_hComm, PURGE_RXCLEAR | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_TXABORT);

	// begin forever loop.  This loop will run as long as the thread is alive.
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
			// If WaitCommEvent() returns FALSE, process the last error to determin
			// the reason..
			switch (dwError = GetLastError())
			{
			case ERROR_IO_PENDING: 	//正常情况，没有字符可读 erroe code:997
				{
					// This is a normal return value if there are no bytes
					// to read at the port.
					// Do nothing and continue
					break;
				}
			case ERROR_INVALID_PARAMETER://系统错误 erroe code:87
				{
					// Under Windows NT, this value is returned for some reason.
					// I have not investigated why, but it is also a valid reply
					// Also do nothing and continue.
					break;
				}
			case ERROR_ACCESS_DENIED:///拒绝访问 erroe code:5
				{
					port->m_hComm = INVALID_HANDLE_VALUE;
					break;
				}
			case ERROR_INVALID_HANDLE:///打开串口失败 erroe code:6
				{
					port->m_hComm = INVALID_HANDLE_VALUE;
					break;
				}
			case ERROR_BAD_COMMAND:///连接过程中非法断开 erroe code:22
				{
					port->m_hComm = INVALID_HANDLE_VALUE;
					break;
				}
			default:
				{
					// All other error codes indicate a serious error has
					port->m_hComm = INVALID_HANDLE_VALUE;
					// occured.  Process this error.
					port->ProcessErrorMessage("WaitCommEvent()");
					break;
				}
			}
		}
		else
		{
			// If WaitCommEvent() returns TRUE, check to be sure there are
			// actually bytes in the buffer to read.
			//
			// If you are reading more than one byte at a time from the buffer
			// (which this program does not do) you will have the situation occur
			// where the first byte to arrive will cause the WaitForMultipleObjects()
			// function to stop waiting.  The WaitForMultipleObjects() function
			// resets the event handle in m_OverlappedStruct.hEvent to the non-signelead state
			// as it returns.
			//
			// If in the time between the reset of this event and the call to
			// ReadFile() more bytes arrive, the m_OverlappedStruct.hEvent handle will be set again
			// to the signeled state. When the call to ReadFile() occurs, it will
			// read all of the bytes from the buffer, and the program will
			// loop back around to WaitCommEvent().
			//
			// At this point you will be in the situation where m_OverlappedStruct.hEvent is set,
			// but there are no bytes available to read.  If you proceed and call
			// ReadFile(), it will return immediatly due to the async port setup, but
			// GetOverlappedResults() will not return until the next character arrives.
			//
			// It is not desirable for the GetOverlappedResults() function to be in
			// this state.  The thread shutdown event (event 0) and the WriteFile()
			// event (Event2) will not work if the thread is blocked by GetOverlappedResults().
			//
			// The solution to this is to check the buffer with a call to ClearCommError().
			// This call will reset the event handle, and if there are no bytes to read
			// we can loop back through WaitCommEvent() again, then proceed.
			// If there are really bytes to read, do nothing and proceed.

			bResult = ClearCommError(port->m_hComm, &dwError, &comstat);

			if (comstat.cbInQue == 0)
				continue;
		}	// end if bResult

		// Main wait function.  This function will normally block the thread
		// until one of nine events occur that require action.
		Event = WaitForMultipleObjects(3, port->m_hEventArray, FALSE, INFINITE);

		switch (Event)
		{
		case 0:
			{
				// Shutdown event.  This is event zero so it will be
				// the higest priority and be serviced first.
				CloseHandle(port->m_hComm);
				port->m_hComm=NULL;
				port->m_bThreadAlive = FALSE;

				// Kill this thread.  break is not needed, but makes me feel better.
				::ExitThread(100);

				break;
			}
		case 1: // write event
			{
				// Write character event from port
				WriteChar(port);
				break;
			}
		case 2:	// read event
			{
				GetCommMask(port->m_hComm, &CommEvent);
				if (CommEvent & EV_RXCHAR)
					ReceiveChar(port);

				if (port->m_pSink && CommEvent & EV_CTS) //CTS信号状态发生变化
					port->m_pSink->OnCommEvent(port->m_nPortNr, COMM_CTS_DETECTED, NULL, 0);
				if (port->m_pSink && CommEvent & EV_RXFLAG) //接收到事件字符，并置于输入缓冲区中
					port->m_pSink->OnCommEvent(port->m_nPortNr, COMM_RXFLAG_DETECTED, NULL, 0);
				if (port->m_pSink && CommEvent & EV_BREAK)  //输入中发生中断
					port->m_pSink->OnCommEvent(port->m_nPortNr, COMM_BREAK_DETECTED, NULL, 0);
				if (port->m_pSink && CommEvent & EV_ERR) //发生线路状态错误，线路状态错误包括CE_FRAME,CE_OVERRUN和CE_RXPARITY
					port->m_pSink->OnCommEvent(port->m_nPortNr, COMM_ERR_DETECTED, NULL, 0);
				if (port->m_pSink && CommEvent & EV_RING) //检测到振铃指示
					port->m_pSink->OnCommEvent(port->m_nPortNr, COMM_RING_DETECTED, NULL, 0);

				break;
			}
		default:
			MessageBox(NULL,_T("接收有问题!"),_T("Application Error"),MB_OK|MB_ICONERROR);
			break;

		} // end switch

	} // close forever loop

	return 0;
}

//
// If there is a error, give the right message
///如果有错误，给出提示
//
void CSerialPort::ProcessErrorMessage(char* ErrorText)
{
	TCHAR *Temp = new TCHAR[200];

	LPVOID lpMsgBuf;

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(LPTSTR) &lpMsgBuf,
		0,
		NULL
		);

	_stprintf_s(Temp,200,_T("WARNING:  %s Failed with the following error: \n%s\nPort: %d\n"), (TCHAR*)ErrorText, lpMsgBuf, m_nPortNr);
	MessageBox(NULL,Temp,_T("Application Error"),MB_OK|MB_ICONERROR);

	LocalFree(lpMsgBuf);
	delete[] Temp;
}

//
// Write a character.
//
void CSerialPort::WriteChar(CSerialPort* port)
{
	BOOL bWrite = TRUE;
	BOOL bResult = TRUE;

	DWORD BytesSent = 0;
	DWORD SendLen = port->m_nWriteSize;
	ResetEvent(port->m_hWriteEvent);


	// Gain ownership of the critical section
	EnterCriticalSection(&port->m_csCommunicationSync);

	if (bWrite)
	{
		// Initailize variables
		port->m_ov.Offset = 0;
		port->m_ov.OffsetHigh = 0;

		// Clear buffer
		PurgeComm(port->m_hComm, PURGE_RXCLEAR | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_TXABORT);

		bResult = WriteFile(port->m_hComm,							// Handle to COMM Port
			port->m_szWriteBuffer,					// Pointer to message buffer in calling finction
			SendLen,	// add by mrlong
			//strlen((char*)port->m_szWriteBuffer),	// Length of message to send
			&BytesSent,								// Where to store the number of bytes sent
			&port->m_ov);							// Overlapped structure

		// deal with any error codes
		if (!bResult)
		{
			DWORD dwError = GetLastError();
			switch (dwError)
			{
			case ERROR_IO_PENDING:
				{
					// continue to GetOverlappedResults()
					BytesSent = 0;
					bWrite = FALSE;
					break;
				}
			case ERROR_ACCESS_DENIED:///拒绝访问 erroe code:5
				{
					port->m_hComm = INVALID_HANDLE_VALUE;
					break;
				}
			case ERROR_INVALID_HANDLE:///打开串口失败 erroe code:6
				{
					port->m_hComm = INVALID_HANDLE_VALUE;
					break;
				}
			case ERROR_BAD_COMMAND:///连接过程中非法断开 erroe code:22
				{
					port->m_hComm = INVALID_HANDLE_VALUE;
					break;
				}
			default:
				{
					// all other error codes
					port->ProcessErrorMessage("WriteFile()");
				}
			}
		}
		else
		{
			LeaveCriticalSection(&port->m_csCommunicationSync);
		}
	} // end if(bWrite)

	if (!bWrite)
	{
		bWrite = TRUE;

		bResult = GetOverlappedResult(port->m_hComm,	// Handle to COMM port
			&port->m_ov,		// Overlapped structure
			&BytesSent,		// Stores number of bytes sent
			TRUE); 			// Wait flag

		LeaveCriticalSection(&port->m_csCommunicationSync);

		// deal with the error code
		if (!bResult)
		{
			port->ProcessErrorMessage("GetOverlappedResults() in WriteFile()");
		}
	} // end if (!bWrite)

	port->m_bHasWritten = TRUE;
	if(port->m_pSink)
		port->m_pSink->OnCommEvent(port->m_nPortNr, COMM_TXEMPTY_DETECTED, NULL, BytesSent);
}

//
// Character received. Inform the owner
//
void CSerialPort::ReceiveChar(CSerialPort* port)
{
	BOOL  bRead = TRUE;
	BOOL  bResult = TRUE;
	DWORD dwError = 0;
	DWORD BytesRead = 0;
	COMSTAT comstat;

	for (;;)
	{
		//add by liquanhai 2011-11-06  防止死锁
		if(WaitForSingleObject(port->m_hShutdownEvent,0)==WAIT_OBJECT_0)
			return;

		// Gain ownership of the comm port critical section.
		// This process guarantees no other part of this program
		// is using the port object.

		EnterCriticalSection(&port->m_csCommunicationSync);

		// ClearCommError() will update the COMSTAT structure and
		// clear any other errors.
		///更新COMSTAT

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

		///所有字符均被读出，中断循环
		//0xcccccccc表示串口异常了，会导致RXBuff指针初始化错误
		if (comstat.cbInQue == 0 || comstat.cbInQue == 0xcccccccc)
		{
			// break out when all bytes have been read
			break;
		}

		//如果遇到'\0'，那么数据会被截断，实际数据全部读取只是没有显示完全，这个时候使用memcpy才能全部获取
		unsigned char* RXBuff = new unsigned char[comstat.cbInQue+1];
		if(RXBuff == NULL)
		{
			return;
		}
		RXBuff[comstat.cbInQue] = '\0';//附加字符串结束符

		EnterCriticalSection(&port->m_csCommunicationSync);

		if (bRead)
		{
			///串口读出，读出缓冲区中字节
			bResult = ReadFile(port->m_hComm,	// Handle to COMM port
				RXBuff,							// RX Buffer Pointer
				comstat.cbInQue,				// Read cbInQue len byte
				&BytesRead,						// Stores number of bytes read
				&port->m_ov);					// pointer to the m_ov structure
			// deal with the error code
			///若返回错误，错误处理
			if (!bResult)
			{
				switch (dwError = GetLastError())
				{
				case ERROR_IO_PENDING:
					{
						// asynchronous i/o is still in progress
						// Proceed on to GetOverlappedResults();
						///异步IO仍在进行
						bRead = FALSE;
						break;
					}
				case ERROR_ACCESS_DENIED:///拒绝访问 erroe code:5
					{
						port->m_hComm = INVALID_HANDLE_VALUE;
						break;
					}
				case ERROR_INVALID_HANDLE:///打开串口失败 erroe code:6
					{
						port->m_hComm = INVALID_HANDLE_VALUE;
						break;
					}
				case ERROR_BAD_COMMAND:///连接过程中非法断开 erroe code:22
					{
						port->m_hComm = INVALID_HANDLE_VALUE;
						break;
					}
				default:
					{
						// Another error has occured.  Process this error.
						port->ProcessErrorMessage("ReadFile()");
						break;
						//return;///防止读写数据时，串口非正常断开导致死循环一直执行。add by itas109 2014-01-09 与上面liquanhai添加防死锁的代码差不多
					}
				}
			}
			else///ReadFile返回TRUE
			{
				// ReadFile() returned complete. It is not necessary to call GetOverlappedResults()
				bRead = TRUE;
			}
		}  // close if (bRead)

		///异步IO操作仍在进行，需要调用GetOverlappedResult查询
		if (!bRead)
		{
			bRead = TRUE;
			bResult = GetOverlappedResult(port->m_hComm,	// Handle to COMM port
				&port->m_ov,		// Overlapped structure
				&BytesRead,		// Stores number of bytes read
				TRUE); 			// Wait flag

			// deal with the error code
			if (!bResult)
			{
				port->ProcessErrorMessage("GetOverlappedResults() in ReadFile()");
			}
		}  // close if (!bRead)

		LeaveCriticalSection(&port->m_csCommunicationSync);

		// notify parent that some byte was received
		if(port->m_pSink)
			port->m_pSink->OnCommEvent(port->m_nPortNr, COMM_RXCHAR, (PVOID) RXBuff, comstat.cbInQue);
		delete[] RXBuff;
		RXBuff = NULL;

	} // end forever loop

}

BOOL CSerialPort::IsOpen()
{
	return m_hComm != NULL && m_hComm!= INVALID_HANDLE_VALUE;
}

void CSerialPort::ClosePort()
{
	//串口句柄无效
	if(m_hComm == INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_hComm);
		m_hComm = NULL;
	}

	do
	{
		SetEvent(m_hShutdownEvent);
		//防止死锁
		Sleep(50);
	} while (m_bThreadAlive);

	// if the port is still opened: close it
	if (m_hComm != NULL)
	{
		CloseHandle(m_hComm);
		m_hComm = NULL;
	}

	// Close Handles
	if(m_hShutdownEvent!=NULL)
	{
		ResetEvent(m_hShutdownEvent);
	}
	if(m_ov.hEvent!=NULL)
	{
		ResetEvent(m_ov.hEvent);
	}
	if(m_hWriteEvent!=NULL)
	{
		ResetEvent(m_hWriteEvent);
	}

	if(m_szWriteBuffer != NULL)
	{
		delete [] m_szWriteBuffer;
		m_szWriteBuffer = NULL;
	}
}

// Write a buffer to the port
BOOL CSerialPort::WriteToPort(LPBYTE lpBuffer, int nBytesToWrite)
{
	if(m_hComm == NULL || !m_bHasWritten)
		return FALSE;

	m_bHasWritten = FALSE;
	memset(m_szWriteBuffer, 0, sizeof(m_szWriteBuffer));
	memcpy_s(m_szWriteBuffer,m_nWriteBufferSize,lpBuffer,m_nWriteBufferSize);
	m_nWriteSize=nBytesToWrite;

	// set event for write
	SetEvent(m_hWriteEvent);

	return TRUE;
}
