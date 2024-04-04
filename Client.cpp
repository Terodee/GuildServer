#include ".\client.h"
#include "GlobalDef.h"
#include "XSocket.h"



CClient::CClient(HWND hWnd):
	m_hWnd(hWnd),
	m_bIsProcessingAvailable(FALSE)
{
	m_pXSock = new XSocket(hWnd, DEF_CLIENTSOCKETBLOCKLIMIT);
	m_pXSock->bInitBufferSize(DEF_MSGBUFFERSIZE);
	ZeroMemory(m_cName, sizeof(m_cName));
	ZeroMemory(m_cIPAddress,sizeof(m_cIPAddress));
}

CClient::~CClient(void)
{
	delete m_pXSock;
	m_pXSock = NULL;
}


int CClient::iSendMsg(char * pData, DWORD dwSize, char cKey /* = NULL */)
{
	return m_pXSock->iSendMsg(pData, dwSize, cKey);
}