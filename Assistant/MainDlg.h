#include "SerialPort.h"
#include "PushPin.h"
#include "afxwin.h"
#ifndef _MAINDLG_H_
#define _MAINDLG_H_
#pragma once


class CMainDlg : public CDialog
{
public:
	CMainDlg(CWnd* pParent = NULL);

// Dialog Data
	//{{AFX_DATA(CMainDlg)
	enum { IDD = IDD_MAIN_DIALOG };

	CComboBox m_ctrlComId;
	CComboBox m_ctrlSpeed;
	CComboBox m_ctrlParity;
	CComboBox m_ctrlDataBits;
	CComboBox m_ctrlStopBits;
	CStatic	m_ctrlPortStatus;
	CButton	m_ctrlOpenPort;
	
	CButton m_ctrlRecvFilePath;
	CButton	m_ctrlRecvNotShow;
	CButton	m_ctrRecvHex;
	CEdit m_ctrRecvText;
	CString	m_strRecvText;
	
	CButton m_ctrlSendFilePath;
	CButton	m_ctrlSendHex;
	CButton	m_ctrlSendAuto;
	CEdit m_ctrlSendCycleTime;
	UINT m_nSendCycleTime;
	CButton	m_ctrlSendClear;
	CEdit	m_ctrlSendText;
	CString	m_strSendText;
	CButton m_ctrlSend;

	CPushPinButton m_ctrlDesktop;
	CStatic	m_ctrlStatus;
	CStatic	m_ctrlTXCount;
	CStatic	m_ctrlRXCount;
	CButton	m_ctrlResetCount;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMainDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON	m_hIcon;
	BOOL	m_bOpenedPort;
	int		m_nComId;			//串口号
	int		m_nBaud;			//波特率
	char	m_cParity;			//校验
	int		m_nDatabits;		//数据位
	int		m_nStopbits;		//停止位
	DWORD	m_dwCommEvents;
	HICON m_hIconOn;			//串口打开时的红灯图标句柄
	HICON m_hIconOff;			//串口关闭时的指示图标句柄
	CSerialPort m_Port;			//CSerialPort类对象
	
	BOOL m_bOnTop;				//窗口置顶

	CString	m_strRecvFile;		//接收文件路径
	BOOL	m_bRecvFile;		//是否接收文件
	CFile	m_RecvFile;

	CString	m_strSendFile;		//发送文件的路径
	BOOL	m_bSendFile;		//是否发送文件
	CFile	m_SendFile;
	ULONGLONG m_uSendFileTotal;	//要发文件的总长度
	ULONGLONG m_uSendFileSent;	//文件已发送总长度
	ULONGLONG m_uPartFileSend;	//文件当前发送长度
	BOOL	m_bAutoSendText;	//自动发送文本内容

	ULONGLONG m_uRecvCount;
	ULONGLONG m_uSendCount;

	int Str2Hex(CString str, char *data);
	char HexChar(char c);

	void OnOK();
	void OnCancel();
	void SendText();
	void SendFile();

	//{{AFX_MSG(CMainDlg)
	afx_msg void OnDestroy();
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnTimer(UINT nIDEvent);
	
	afx_msg LONG OnCommRecvChar(WPARAM wParam, LPARAM port);
	afx_msg LONG OnCommRecvString(WPARAM wString, LPARAM lLength);
	afx_msg LONG OnCommBufferSent(WPARAM wParam,LPARAM port);
	
	afx_msg void OnButtonOpenPort();

	afx_msg void OnButtonRecvClear();
	afx_msg void OnButtonRecvDisplay();
	afx_msg void OnButtonSend();
	afx_msg void OnChangeSendCycleTime();
	afx_msg void OnChangeSendText();
	afx_msg void OnButtonSendClear();
	afx_msg void OnButtonRecvSaveData();
	afx_msg void OnCheckRecvFilePath();
	afx_msg void OnCheckSendFilePath();

	afx_msg void OnButtonDesktop();
	afx_msg void OnButtonResetCount();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
	//DECLARE_DYNAMIC_MAP()
};

//{{AFX_INSERT_LOCATION}}

#endif
