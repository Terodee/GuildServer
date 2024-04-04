//2006-3-31 created by jcfly

#pragma once
#include <time.h>
#include <map>
#include <string>
#include <Windows.h>
using namespace std;

#define	SOV_STATE_NONE		0
#define	SOV_STATE_REIGNING	1
#define	SOV_STATE_VOTE		2


class CClient;
class CGame;

class CSovereign
{
public:
	CSovereign(CGame *pGame);
	~CSovereign(void);
	
	BOOL	bInit(char *pLocation);
	void	OnTimer();
	//Net message
	int		iVote(char *src, char* dest);

	BOOL	bSaveToFile();

	int		iComposeSovInfo(char* pData);

	const	char* GetLocation(){
		return m_cLocation;
	}

	void	StartVote();
	void	EndVote();
private:
	BOOL	bParseFile();
	
	void	SendSovMsgToAll(WORD wMsgType);

private:
	CGame	*m_pGame;

	int		m_iState;
	time_t	m_tBegin;

	char	m_cName[11];
	char	m_cLocation[11];

	map<string, string>	m_mVote;

	char	*m_pData;		//buffer for messages
};
