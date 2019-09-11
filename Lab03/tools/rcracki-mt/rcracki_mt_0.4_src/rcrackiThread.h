#pragma once
#include "ChainWalkContext.h"
#include "Public.h"
#include "HashSet.h"
//#include <process.h>
#include <pthread.h>

class rcrackiThread
{
private:
	unsigned char* t_TargetHash;
	int t_nPos;
	int t_nRainbowChainLen;
	CChainWalkContext t_cwc;
	vector<uint64> t_vStartPosIndexE;
	int t_ID;
	int t_count;
	uint64* t_pStartPosIndexE;
	int t_nChainWalkStep;
	bool falseAlarmChecker;
	vector<RainbowChain *> t_pChainsFound;
	vector<int> t_nGuessedPoss;
	unsigned char* t_pHash;
	bool foundHash;
	int t_nChainWalkStepDueToFalseAlarm;
	int t_nFalseAlarm;
	string t_Hash;
	string t_Plain;
	string t_Binary;

public:
	rcrackiThread(unsigned char* TargetHash, int thread_id, int nRainbowChainLen, int thread_count, uint64* pStartPosIndexE);
	rcrackiThread(unsigned char* pHash);
	rcrackiThread(void);
	~rcrackiThread(void);

	void SetWork(unsigned char* TargetHash, int nPos, int nRainbowChainLen);
	//static unsigned __stdcall rcrackiThread::rcrackiThreadStaticEntryPoint(void * pThis);
	static void * rcrackiThreadStaticEntryPointPthread(void * pThis);
	int GetIndexCount();
	int GetChainWalkStep();
	uint64 GetIndex(int nPos);
	bool FoundHash();
	void AddAlarmCheck(RainbowChain* pChain, int nGuessedPos);
	int GetChainWalkStepDueToFalseAlarm();
	int GetnFalseAlarm();
	string GetHash();
	string GetPlain();
	string GetBinary();

private:
	void rcrackiThreadEntryPoint();
	void PreCalculate();
	void CheckAlarm();
};
