#include <windows.h>
#include "SerialPort.h"

class CMainDlg : public WindowImplBase, public ISerialPortSink
{
public:
	void OpenSP1();

protected:
	void OnCommEvent(UINT nPort,UINT nMsg,PVOID pParam,UINT nLen);
}


void CMainDlg::OpenSP1()
{
	CDuiString sBuffer = ctrlSP1ComId->GetText();
	sBuffer.Replace(_T("COM"),_T(""));
	m_nSP1ComId = _tstoi(sBuffer.GetData());
	if(m_nSP1ComId< 0 ||  m_nSP1ComId > 20)
		m_nSP1ComId = 1;

	sBuffer = ctrlSP1Speed->GetText();
	m_nSP1Baud = _tstoi(sBuffer.GetData());
	if(m_nSP1Baud< 75 ||  m_nSP1Baud > 128000)
		m_nSP1Baud = 9600;

	sBuffer = ctrlSP1Parity->GetText();
	if(sBuffer.CompareNoCase(_T("ODD"))==0)
		m_cSP1Parity = 'O';
	else if(sBuffer.CompareNoCase(_T("EVEN"))==0)
		m_cSP1Parity = 'E';
	else if(sBuffer.CompareNoCase(_T("MARK"))==0)
		m_cSP1Parity = 'M';
	else if(sBuffer.CompareNoCase(_T("SPACE"))==0)
		m_cSP1Parity = 'S';
	else
		m_cSP1Parity='N';

	sBuffer = ctrlSP1Databits->GetText();
	m_nSP1Databits = _tstoi(sBuffer.GetData());
	if(m_nSP1Databits< 4 ||  m_nSP1Databits > 8)
		m_nSP1Databits = 8;

	sBuffer = ctrlSP1Stopbits->GetText();
	if(sBuffer.CompareNoCase(_T("1"))==0)
		m_nSP1Stopbits = ONESTOPBIT;
	else if(sBuffer.CompareNoCase(_T("2"))==0)
		m_nSP1Stopbits = TWOSTOPBITS;
	else
		m_nSP1Stopbits = ONE5STOPBITS;

	sBuffer = ctrlSP1Command->GetText();
	m_nSP1CommandLen = _tstoi(sBuffer.GetData());
	if(m_nSP1CommandLen< 1 ||  m_nSP1CommandLen > 16)
		m_nSP1CommandLen = 16;

	if (m_SP1Port.OpenPort(this,m_nSP1ComId,
		m_nSP1Baud,m_cSP1Parity,m_nSP1Databits,
		m_nSP1Stopbits,m_nSP1Events,512))
	{
		m_bSP1Open=TRUE;
		m_bSP1HasSent = true;
		m_nSP1Send = 0;
		m_nSP1Received = 0;
		btnSP1Open->SetText(_T("Close"));
		SP1Log(_T("COM%d Opened"),m_nSP1ComId);
	}
	else
	{
		SP1Log(_T("COM%d Open Failed"),m_nSP1ComId);
	}
}


void CMainDlg::OnCommEvent(UINT nPort,UINT nMsg,PVOID pParam,UINT nLen)
{
	switch (nMsg)
	{
	case COMM_RXCHAR:
		{
			CDuiString sBuffer,sItem;
			PBYTE lpBuf =(PBYTE)pParam;
			if(nPort == m_nSP1ComId){
				m_SP1RecvLock.Lock();
				UINT nReceivedLen=0,nRecvLen=0;
				while(nReceivedLen<nLen)
				{
					nRecvLen = nLen-nReceivedLen;
					if(nRecvLen >= m_nSP1CommandLen-m_nSP1Received)
						nRecvLen = m_nSP1CommandLen-m_nSP1Received;
					memcpy(m_szSP1Recv+m_nSP1Received,lpBuf+nReceivedLen,nRecvLen);
					nReceivedLen += nRecvLen;
					m_nSP1Received += nRecvLen;
					if(m_nSP1Received == m_nSP1CommandLen)
					{
						SP1Recv(m_szSP1Recv);
						m_nSP1Received=0;
					}
				}
				m_SP1RecvLock.Unlock();
			}
			break;
		}
	case COMM_TXEMPTY_DETECTED:
		{
			if(nPort == m_nSP1ComId){
				m_SP1SendLock.Lock();
				m_bSP1HasSent = true;
				m_SP1SendLock.Unlock();
			}
			break;
		}
	default:
		break;
	}
}