//2006-3-18 created by jcfly

#pragma once
#include <Windows.h>

class XSocket;
class CClient
{
public:
	CClient(HWND hWnd);
	~CClient(void);

	XSocket*	GetSock() const{
		return m_pXSock;
	}

	int iSendMsg(char * pData, DWORD dwSize, char cKey = NULL);

public:
	BOOL	m_bIsProcessingAvailable;
	char	m_cName[11];
	char	m_cIPAddress[16];
private:

	XSocket*	m_pXSock;

	HWND	m_hWnd;
};
