#include "stdafx.h"
#include "Assistant.h"
#include "MainDlg.h"
#include "io.h"
#include "math.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define TIMEER_SEND_TEXT	1
#define TIMEER_SEND_FILE	2

#define BufSize 512


/////////////////////////////////////////////////////////////////////////////
// CMainDlg dialog

CMainDlg::CMainDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CMainDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CMainDlg)
	m_strRecvText = _T("");
	m_strSendText = _T("sanwer.com");
	m_nSendCycleTime = 1000;
	//}}AFX_DATA_INIT
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	m_bOpenedPort=FALSE;
	m_nComId=1;
	m_nBaud=9600;
	m_cParity='N';
	m_nDatabits=8;
	m_nStopbits=1;
	m_dwCommEvents = EV_RXFLAG | EV_RXCHAR;
	
	m_bOnTop=FALSE;

	m_strRecvFile = _T("");
	m_bRecvFile=FALSE;
	m_strSendFile = _T("");
	m_bSendFile=FALSE;
	m_uSendFileTotal=0;
	m_uSendFileSent=0;
	m_uPartFileSend=0;
	m_bAutoSendText=FALSE;

	m_uRecvCount=0;
	m_uSendCount=0;
}


void CMainDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMainDlg)
	DDX_Control(pDX, IDC_COMBO_COMID, m_ctrlComId);
	DDX_Control(pDX, IDC_COMBO_SPEED, m_ctrlSpeed);
	DDX_Control(pDX, IDC_COMBO_PARITY, m_ctrlParity);
	DDX_Control(pDX, IDC_COMBO_DATABITS, m_ctrlDataBits);
	DDX_Control(pDX, IDC_COMBO_STOPBITS, m_ctrlStopBits);
	DDX_Control(pDX, IDC_ICON_PORT_STATUS, m_ctrlPortStatus);
	DDX_Control(pDX, IDC_BUTTON_OPENPORT, m_ctrlOpenPort);
	
	DDX_Control(pDX, IDC_RECV_CHECK_FILEPATH, m_ctrlRecvFilePath);
	DDX_Control(pDX, IDC_RECV_CHECK_HEX, m_ctrRecvHex);
	DDX_Control(pDX, IDC_RECV_CHECK_NOTSHOW, m_ctrlRecvNotShow);
	DDX_Control(pDX, IDC_RECV_EDIT_TEXT, m_ctrRecvText);
	DDX_Text(pDX, IDC_RECV_EDIT_TEXT, m_strRecvText);
	
	DDX_Control(pDX, IDC_SEND_CHECK_FILENAME, m_ctrlSendFilePath);
	DDX_Control(pDX, IDC_SEND_CHECK_HEX, m_ctrlSendHex);
	DDX_Control(pDX, IDC_SEND_CHECK_AUTO, m_ctrlSendAuto);
	DDX_Control(pDX, IDC_SEND_EDIT_CYCLETIME, m_ctrlSendCycleTime);
	DDX_Text(pDX, IDC_SEND_EDIT_CYCLETIME, m_nSendCycleTime);
	DDX_Control(pDX, IDC_SEND_BUTTON_CLEAR, m_ctrlSendClear);
	DDX_Control(pDX, IDC_SEND_EDIT_TEXT, m_ctrlSendText);
	DDX_Text(pDX, IDC_SEND_EDIT_TEXT, m_strSendText);
	DDX_Control(pDX, IDC_SEND_BUTTON, m_ctrlSend);

	DDX_Control(pDX, IDC_BUTTON_DESKTOP, m_ctrlDesktop);
	DDX_Control(pDX, IDC_STATIC_STATUS, m_ctrlStatus);
	DDX_Control(pDX, IDC_STATIC_RXCOUNT, m_ctrlRXCount);
	DDX_Control(pDX, IDC_STATIC_TXCOUNT, m_ctrlTXCount);
	DDX_Control(pDX, IDC_BUTTON_RESET_COUNT, m_ctrlResetCount);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CMainDlg, CDialog)
	//{{AFX_MSG_MAP(CMainDlg)
	ON_WM_DESTROY()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_SYSCOMMAND()
	ON_WM_TIMER()
	ON_MESSAGE(WM_COMM_RXCHAR, OnCommRecvChar)
	ON_MESSAGE(WM_COMM_RXSTR, OnCommRecvString)
	ON_MESSAGE(WM_COMM_TXEMPTY_DETECTED, OnCommBufferSent)
	ON_BN_CLICKED(IDC_BUTTON_OPENPORT, OnButtonOpenPort)
	
	ON_BN_CLICKED(IDC_RECV_CHECK_FILEPATH, OnCheckRecvFilePath)
	ON_BN_CLICKED(IDC_RECV_CHECK_NOTSHOW, OnButtonRecvDisplay)
	ON_BN_CLICKED(IDC_RECV_BUTTON_SAVE, OnButtonRecvSaveData)
	ON_BN_CLICKED(IDC_RECV_BUTTON_CLEAR, OnButtonRecvClear)
	
	ON_BN_CLICKED(IDC_SEND_CHECK_FILENAME, OnCheckSendFilePath)
	ON_EN_CHANGE(IDC_SEND_EDIT_CYCLETIME, OnChangeSendCycleTime)
	ON_BN_CLICKED(IDC_SEND_BUTTON, OnButtonSend)
	ON_EN_CHANGE(IDC_SEND_EDIT_TEXT, OnChangeSendText)
	ON_BN_CLICKED(IDC_SEND_BUTTON_CLEAR, OnButtonSendClear)

	ON_BN_CLICKED(IDC_BUTTON_DESKTOP, OnButtonDesktop)
	ON_BN_CLICKED(IDC_BUTTON_RESET_COUNT, OnButtonResetCount)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CMainDlg::OnDestroy() 
{
	CDialog::OnDestroy();
	m_ctrlSendAuto.SetCheck(0);  //强行关闭自动发送
	KillTimer(TIMEER_SEND_TEXT);   //关闭自动发送定时器
	m_Port.ClosePort();  //关闭串口
	m_strRecvText.Empty();  //清空接收数据字符串
}

BOOL CMainDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	SetIcon(m_hIcon, TRUE);
	SetIcon(m_hIcon, FALSE);

	m_ctrlComId.SetCurSel(0);
	m_ctrlSpeed.SetCurSel(5);
	m_ctrlParity.SetCurSel(0);
	m_ctrlDataBits.SetCurSel(0);
	m_ctrlStopBits.SetCurSel(0);

	m_hIconOn  = AfxGetApp()->LoadIcon(IDI_ICON_ON);
	m_hIconOff	= AfxGetApp()->LoadIcon(IDI_ICON_OFF);

	m_ctrlPortStatus.SetIcon(m_hIconOff);

	return TRUE;
}

void CMainDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

HCURSOR CMainDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CMainDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	CDialog::OnSysCommand(nID, lParam);
}

void CMainDlg::OnTimer(UINT nIDEvent) 
{
	// TODO: Add your message handler code here and/or call default
	CString strStatus;
	switch(nIDEvent)
	{
	case TIMEER_SEND_TEXT:
		SendText();
		break;
	case TIMEER_SEND_FILE:
		KillTimer(TIMEER_SEND_FILE);
		SendFile();
		break;
	default:
		break;
	}

	CDialog::OnTimer(nIDEvent);
}

void CMainDlg::OnOK() 
{
	//CDialog::OnOK();
}

void CMainDlg::OnCancel() 
{
	CDialog::OnCancel();
}

LONG CMainDlg::OnCommRecvChar(WPARAM wParam, LPARAM port)
{
	m_uRecvCount++;   //接收的字节计数
	CString strTemp;
	strTemp.Format("%u",m_uRecvCount);
	strTemp="RX:"+strTemp;
	m_ctrlRXCount.SetWindowText(strTemp);  //显示接收计数

	if(m_bRecvFile)
	{
		strTemp.Format("%c",wParam);
		m_RecvFile.Write((LPCTSTR)strTemp,strTemp.GetLength());
	}
	else
	{
		if(m_ctrlRecvNotShow.GetCheck())   //如果选择了“停止显示”接收数据，则返回
			return -1;          //注意，这种情况下，计数仍在继续，只是不显示

		//如果没有“自动清空”，数据行达到400后，也自动清空
		//因为数据过多，影响接收速度，显示是最费CPU时间的操作
		if(m_ctrRecvText.GetLineCount()>400)
		{
			m_strRecvText.Empty();
			m_strRecvText="***The Length of the Text is too long, Emptied Automaticly!!!***\r\n";        
			UpdateData(FALSE);
		}
		if(m_ctrRecvHex.GetCheck())
			strTemp.Format("%02X ",wParam);
		else 
			strTemp.Format("%c",wParam);
		//以下是将接收的字符加在字符串的最后，这里费时很多
		//但考虑到数据需要保存成文件，所以没有用List Control
		int nLen=m_ctrRecvText.GetWindowTextLength();
		m_ctrRecvText.SetSel(nLen, nLen);
		m_ctrRecvText.ReplaceSel(strTemp);
		nLen+=strTemp.GetLength();
		m_strRecvText+=strTemp;
	}

	UpdateData(FALSE);
	return 0;
}

LONG CMainDlg::OnCommRecvString(WPARAM wString, LPARAM lLength)
{
	DWORD nLength = (DWORD)lLength;
	m_uRecvCount+=lLength;   //接收的字节计数
	CString strTemp;
	strTemp.Format("%u",m_uRecvCount);
	strTemp="RX:"+strTemp;
	m_ctrlRXCount.SetWindowText(strTemp);  //显示接收计数
	
	if(m_bRecvFile)
	{
		m_RecvFile.Write((LPCTSTR)wString,lLength);
	}
	else
	{
		if(m_ctrlRecvNotShow.GetCheck())   //如果选择了“停止显示”接收数据，则返回
			return -1;          //注意，这种情况下，计数仍在继续，只是不显示

		//如果没有“自动清空”，数据行达到400后，也自动清空
		//因为数据过多，影响接收速度，显示是最费CPU时间的操作
		if(m_ctrRecvText.GetLineCount()>400)
		{
			m_strRecvText.Empty();
			m_strRecvText="***The Length of the Text is too long, Emptied Automaticly!!!***\r\n";        
			UpdateData(FALSE);
		}

		if(m_ctrRecvHex.GetCheck())
		{
			LPCTSTR lpBuffer = (LPCTSTR)wString;
			for(DWORD i=0;i<nLength;i++)
				strTemp+=(TCHAR)lpBuffer[0];
		}
		else 
			strTemp = (LPCTSTR)wString;

		ULONG nLen=m_ctrRecvText.GetWindowTextLength();
		m_ctrRecvText.SetSel(nLen, nLen);
		m_ctrRecvText.ReplaceSel(strTemp);
		nLen+=strTemp.GetLength();

		m_strRecvText+=strTemp;
	}
	UpdateData(FALSE);
	return 0;
}

LONG CMainDlg::OnCommBufferSent(WPARAM wParam,LPARAM port)
{
	DWORD dwSendLen = (DWORD) wParam;
	
	m_uSendFileSent+=m_uPartFileSend;

	if(m_bSendFile)
	{
		if(dwSendLen == m_uPartFileSend)
		{
			if(m_uSendFileSent == m_uSendFileTotal)
			{
				m_SendFile.Close();
				m_bSendFile=FALSE;
				m_ctrlSend.SetWindowText("发送");
			}
			else if(m_uSendFileSent < m_uSendFileTotal)
			{
				SetTimer(TIMEER_SEND_FILE,50,NULL);
			}
		}
		else
		{
			m_SendFile.Close();
			m_bSendFile=FALSE;
			m_ctrlSend.SetWindowText("发送");
			m_bSendFile=FALSE;
			AfxMessageBox("文件发送失败");
		}
	}

	m_uSendCount+=m_uPartFileSend;
	CString strTemp;
	strTemp.Format("TX:%u",m_uSendCount);
	m_ctrlTXCount.SetWindowText(strTemp);
	UpdateData(FALSE);

	return 0;
}

//打开/关闭串口
void CMainDlg::OnButtonOpenPort() 
{
	if(m_bOpenedPort)  //关闭串口
	{
		if(m_bAutoSendText || m_bSendFile)
		{
			//m_bOpenedPort=!m_bOpenedPort;
			AfxMessageBox("请先停止发送");
			return;
		}

		m_Port.ClosePort();//关闭串口
		m_bOpenedPort=FALSE;
		m_ctrlComId.EnableWindow(TRUE);
		m_ctrlSpeed.EnableWindow(TRUE);
		m_ctrlParity.EnableWindow(TRUE);
		m_ctrlDataBits.EnableWindow(TRUE);
		m_ctrlStopBits.EnableWindow(TRUE);
		m_ctrlPortStatus.SetIcon(m_hIconOff);
		m_ctrlOpenPort.SetWindowText("打开串口");
		m_ctrlPortStatus.SetIcon(m_hIconOff);
		m_ctrlStatus.SetWindowText("COM Port Closed");
	}
	else  //打开串口
	{
		int iSel = 0;
		m_nComId=m_ctrlComId.GetCurSel()+1;
		switch(m_ctrlSpeed.GetCurSel())
		{
		case 0:
			m_nBaud=300;
			break;
		case 1:
			m_nBaud=600;
			break;
		case 2:
			m_nBaud=1200;
			break;
		case 3:
			m_nBaud=2400;
			break;
		case 4:
			m_nBaud=4800;
			break;
		case 5:
			m_nBaud=9600;
			break;
		case 6:
			m_nBaud=19200;
			break;
		case 7:
			m_nBaud=38400;
			break;
		case 8:
			m_nBaud=43000;
			break;
		case 9:
			m_nBaud=56000;
			break;
		case 10:
			m_nBaud=57600;
			break;
		case 11:
			m_nBaud=115200;
			break;
		default:
			m_nBaud=9600;
			break;
		}
		switch(m_ctrlParity.GetCurSel())
		{
		case 0:
			m_cParity='N';
			break;
		case 1:
			m_cParity='O';
			break;
		case 2:
			m_cParity='E';
			break;
		default:
			m_cParity='N';
			break;
		}
		switch(m_ctrlDataBits.GetCurSel())
		{
		case 0:
			m_nDatabits=8;
			break;
		case 1:
			m_nDatabits=7;
			break;
		case 2:
			m_nDatabits=6;
			break;
		}
		switch(m_ctrlStopBits.GetCurSel())
		{
		case 0:
			m_nStopbits=1;
			break;
		case 1:
			m_nStopbits=2;
			break;
		default:
			m_nStopbits=1;
			break;
		}
		if (m_Port.InitPort(m_hWnd, m_nComId, m_nBaud,m_cParity,m_nDatabits,m_nStopbits,m_dwCommEvents,512))
		{
			m_bOpenedPort=TRUE;
			m_Port.StartMonitoring();
			m_ctrlPortStatus.SetIcon(m_hIconOn);
			CString strStatus;
			strStatus.Format("COM%d OPENED,%d,%c,%d,%d",m_nComId, m_nBaud,m_cParity,m_nDatabits,m_nStopbits);
			//"当前状态：串口打开，无奇偶校验，8数据位，1停止位");

			m_ctrlComId.EnableWindow(FALSE);
			m_ctrlSpeed.EnableWindow(FALSE);
			m_ctrlParity.EnableWindow(FALSE);
			m_ctrlDataBits.EnableWindow(FALSE);
			m_ctrlStopBits.EnableWindow(FALSE);
			m_ctrlPortStatus.SetIcon(m_hIconOn);
			m_ctrlStatus.SetWindowText(strStatus);
			m_ctrlOpenPort.SetWindowText("关闭串口");
		}
		else
		{
			AfxMessageBox("没有发现此串口或被占用");
		}
	}
}

//改变文件保存路径
void CMainDlg::OnCheckRecvFilePath() 
{
	if(m_ctrlRecvFilePath.GetCheck())
	{
		static char BASED_CODE szFilter[] = "所有文件(*.*)|*.*||";
		CFileDialog FileDlg(TRUE, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, szFilter);
		if( FileDlg.DoModal() ==IDOK )
		{
			CString strFileName =  FileDlg.GetPathName();
			if(PathFileExists(strFileName))
			{
				 if(IDYES != ::MessageBox(NULL,TEXT("覆盖已有文件？"),
					 TEXT("警告"),MB_ICONWARNING|MB_YESNO))
					 return;
			}
			if(!m_RecvFile.Open((LPCTSTR)strFileName,CFile::modeCreate)) 
			{
				AfxMessageBox("打开文件失败!");
				return;
			}
			m_strRecvFile=strFileName;
			m_bRecvFile=TRUE;
			m_strRecvText.Empty();
			m_ctrRecvText.EnableWindow(FALSE);
			UpdateData(FALSE);
			return;
		}
	}
	m_bRecvFile=FALSE;
	m_RecvFile.Close();
	m_strRecvFile.Empty();
	m_ctrlRecvFilePath.SetCheck(0);
	m_ctrRecvText.EnableWindow(TRUE);
}

//停止/继续显示接收数据
void CMainDlg::OnButtonRecvDisplay() 
{
	if(m_ctrlRecvNotShow.GetCheck())
		m_ctrlRecvNotShow.SetWindowText("不显示接收数据");
	else
		m_ctrlRecvNotShow.SetWindowText("显示接收数据");
}

//保存显示数据
void CMainDlg::OnButtonRecvSaveData() 
{
	UpdateData(TRUE);

	CString	m_strFile;
	static char BASED_CODE szFilter[] = "文本文件(*.txt)|*.txt|所有文件(*.*)|*.*||";
	CFileDialog FileDlg(TRUE, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, szFilter);
	if( FileDlg.DoModal() ==IDOK )
	{
		CFile saveFile;
		CString strFileName =  FileDlg.GetPathName();
		if(PathFileExists(strFileName))
		{
			if(IDYES != ::MessageBox(NULL,TEXT("覆盖已有文件？"),
				TEXT("警告"),MB_ICONWARNING|MB_YESNO))
				return;
		}
		if(!saveFile.Open((LPCTSTR)strFileName,CFile::modeCreate | CFile::modeWrite)) 
		{
			AfxMessageBox("创建记录文件失败!");
			return;
		}
		CString str;
		saveFile.Write((LPCTSTR)m_strRecvText,m_strRecvText.GetLength());
		saveFile.Flush();
		saveFile.Close();
		UpdateData(FALSE);
	}
}

//清空接收区
void CMainDlg::OnButtonRecvClear() 
{
	m_strRecvText.Empty();
	UpdateData(FALSE);
}



char CMainDlg::HexChar(char c)
{
	if((c>='0')&&(c<='9'))
		return c-0x30;
	else if((c>='A')&&(c<='F'))
		return c-'A'+10;
	else if((c>='a')&&(c<='f'))
		return c-'a'+10;
	else 
		return 0x10;
}


//将一个字符串作为十六进制串转化为一个字节数组，字节间可用空格分隔，
//返回转换后的字节数组长度，同时字节数组长度自动设置。
int CMainDlg::Str2Hex(CString str, char* data)
{
	int t,t1;
	int rlen=0,len=str.GetLength();
	//data.SetSize(len/2);
	for(int i=0;i<len;)
	{
		char l,h=str[i];
		if(h==' ')
		{
			i++;
			continue;
		}
		i++;
		if(i>=len)
			break;
		l=str[i];
		t=HexChar(h);
		t1=HexChar(l);
		if((t==16)||(t1==16))
			break;
		else 
			t=t*16+t1;
		i++;
		data[rlen]=(char)t;
		rlen++;
	}
	return rlen;

}

//选择要发送的文件
void CMainDlg::OnCheckSendFilePath() 
{
	if(m_ctrlSendFilePath.GetCheck())
	{
		static char BASED_CODE szFilter[] = "所有文件(*.*)|*.*||";
		CFileDialog FileDlg(TRUE, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, szFilter);
		if( FileDlg.DoModal() ==IDOK )
		{
			CString strFileName =  FileDlg.GetPathName();
			m_strSendFile=strFileName;
			m_strSendText.Empty();
			m_ctrlSendText.EnableWindow(FALSE);
			UpdateData(FALSE);//拷贝变量值到控件显示
			return;
		}
	}
	m_strSendFile.Empty();
	m_ctrlSendFilePath.SetCheck(0);
	m_ctrlSendText.EnableWindow(TRUE);
}

void CMainDlg::OnChangeSendCycleTime() 
{
	UpdateData(TRUE);//刷新控件的值到变量
}

void CMainDlg::OnButtonSend() 
{
	if(!m_Port.IsOpen())
	{
		if(m_bAutoSendText)
		{
			KillTimer(TIMEER_SEND_TEXT);
			m_bAutoSendText=FALSE;
			m_ctrlSend.SetWindowText("发送");
		}
		AfxMessageBox("串口没有打开，请打开串口");
		return;
	}
	else
	{
		if(m_bAutoSendText)
		{
			KillTimer(TIMEER_SEND_TEXT);
			m_bAutoSendText=FALSE;
			m_ctrlSendAuto.EnableWindow(TRUE);
			m_ctrlSendCycleTime.EnableWindow(TRUE);
			m_ctrlSend.SetWindowText("发送");
			return;
		}
		if(m_bSendFile)
		{
			KillTimer(TIMEER_SEND_FILE);
			m_SendFile.Close();
			m_bSendFile=FALSE;
			m_ctrlSend.SetWindowText("发送");
			return;
		}

		UpdateData(TRUE);

		//发送文件
		if(m_strSendFile.GetLength() > 0)
		{
			if(!(m_SendFile.Open((LPCTSTR)m_strSendFile,CFile::modeRead))) 
			{
				AfxMessageBox("Open file failed!");
				return;
			}
			m_SendFile.SeekToEnd();
			m_uSendFileTotal = m_SendFile.GetLength();
			m_uSendFileSent = 0;
			m_uPartFileSend=0;
			m_SendFile.SeekToBegin();
			m_bSendFile=TRUE;
			SetTimer(TIMEER_SEND_FILE,50,NULL);
			m_ctrlSend.SetWindowText("停止发送");
		}
		else if(m_strSendText.GetLength() > 0)
		{
			if(m_ctrlSendHex.GetCheck())
			{
				char data[512];
				int len=Str2Hex(m_strSendText,data);
				m_Port.WriteToPort(data,len);
				m_uSendCount+=((m_strSendText.GetLength()+1)/3);
			}
			else 
			{
				m_Port.WriteToPort((LPCTSTR)m_strSendText);
				m_uSendCount+=m_strSendText.GetLength();
			}
			CString strTemp;
			strTemp.Format("TX:%u",m_uSendCount);
			m_ctrlTXCount.SetWindowText(strTemp);

			if(m_ctrlSendAuto.GetCheck() && !m_bAutoSendText)
			{
				SetTimer(TIMEER_SEND_TEXT,m_nSendCycleTime,NULL);
				m_bAutoSendText=TRUE;
				m_ctrlSendAuto.EnableWindow(FALSE);
				m_ctrlSendCycleTime.EnableWindow(FALSE);
				m_ctrlSend.SetWindowText("停止发送");
			}
		}
	}
}

void CMainDlg::SendText() 
{
	if(m_Port.IsOpen() && m_strSendText.GetLength() > 0)
	{
		if(m_ctrlSendHex.GetCheck())
		{
			char data[512];
			int len=Str2Hex(m_strSendText,data);
			m_Port.WriteToPort(data,len);
			m_uSendCount+=((m_strSendText.GetLength()+1)/3);
		}
		else 
		{
			m_Port.WriteToPort((LPCTSTR)m_strSendText);
			m_uSendCount+=m_strSendText.GetLength();
		}
		CString strTemp;
		strTemp.Format("TX:%u",m_uSendCount);
		m_ctrlTXCount.SetWindowText(strTemp);
	}
}

//发送文件
void CMainDlg::SendFile() 
{
	BYTE szBuffer[512];
	
	m_uPartFileSend = m_uSendFileTotal - m_uSendFileSent;
	if(m_uPartFileSend > 512)
		m_uPartFileSend = 512;

	memset(szBuffer, 0, m_uPartFileSend);
	UINT uRead = m_SendFile.Read(szBuffer,m_uPartFileSend);
	if(m_uPartFileSend != uRead)
	{
		m_SendFile.Close();
		return;
	}

	m_Port.WriteToPort(szBuffer,(int)uRead);
}

void CMainDlg::OnChangeSendText() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialog::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	// TODO: Add your control notification handler code here
	UpdateData(TRUE);
}

void CMainDlg::OnButtonSendClear() 
{
	m_strSendText.Empty();
	UpdateData(FALSE);
}

void CMainDlg::OnButtonDesktop() 
{
	m_ctrlDesktop.ProcessClick();
	m_bOnTop=!m_bOnTop;
	if(m_bOnTop)
	{
		SetWindowPos(&wndTopMost, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
	}
	else
	{
		SetWindowPos(&wndBottom, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOREDRAW);
		BringWindowToTop();
	}
}

void CMainDlg::OnButtonResetCount() 
{
	m_uRecvCount=0;
	CString strTemp;
	strTemp.Format("%u",m_uRecvCount);
	strTemp="RX:"+strTemp;
	m_ctrlRXCount.SetWindowText(strTemp);
	m_uSendCount=0;
	strTemp.Format("%u",m_uSendCount);
	strTemp="TX:"+strTemp;
	m_ctrlTXCount.SetWindowText(strTemp);
	
}
