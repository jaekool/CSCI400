#include "rcrackiThread.h"

// create job for pre-calculation
rcrackiThread::rcrackiThread(unsigned char* TargetHash, int thread_id, int nRainbowChainLen, int thread_count, uint64* pStartPosIndexE)
{
	t_TargetHash = TargetHash;
	t_nRainbowChainLen = nRainbowChainLen;
	t_ID = thread_id;
	t_count = thread_count;
	t_pStartPosIndexE = pStartPosIndexE;
	t_nChainWalkStep = 0;
	falseAlarmChecker = false;
}

// create job for false alarm checking
rcrackiThread::rcrackiThread(unsigned char* pHash)
{
	falseAlarmChecker = true;
	t_pChainsFound.clear();
	t_nGuessedPoss.clear();
	t_pHash = pHash;
	t_nChainWalkStepDueToFalseAlarm = 0;
	t_nFalseAlarm = 0;
	foundHash = false;
}

void rcrackiThread::AddAlarmCheck(RainbowChain* pChain, int nGuessedPos)
{
	t_pChainsFound.push_back(pChain);
	t_nGuessedPoss.push_back(nGuessedPos);
}

// Windows (beginthreadex) way of threads
//unsigned __stdcall rcrackiThread::rcrackiThreadStaticEntryPoint(void * pThis)
//{
//	rcrackiThread* pTT = (rcrackiThread*)pThis;
//	pTT->rcrackiThreadEntryPoint();
//	_endthreadex( 2 );
//	return 2;
//}

// entry point for the posix thread
void * rcrackiThread::rcrackiThreadStaticEntryPointPthread(void * pThis)
{
	rcrackiThread* pTT = (rcrackiThread*)pThis;
	pTT->rcrackiThreadEntryPoint();
	pthread_exit(NULL);
	return NULL;
}

// start processing of jobs
void rcrackiThread::rcrackiThreadEntryPoint()
{
	if (falseAlarmChecker) {
		CheckAlarm();
	}
	else {
		PreCalculate();
	}
}

uint64 rcrackiThread::GetIndex(int nPos)
{
	uint64 t_index = t_vStartPosIndexE[nPos - t_ID];
	return t_index;
}

int rcrackiThread::GetChainWalkStep()
{
	return t_nChainWalkStep;
}

int rcrackiThread::GetIndexCount()
{
	return t_vStartPosIndexE.size();
}

rcrackiThread::~rcrackiThread(void)
{
}

void rcrackiThread::PreCalculate()
{
	for (t_nPos = t_nRainbowChainLen - 2 - t_ID; t_nPos >= 0; t_nPos -= t_count)
	{
		t_cwc.SetHash(t_TargetHash);
		t_cwc.HashToIndex(t_nPos);
		int i;
		for (i = t_nPos + 1; i <= t_nRainbowChainLen - 2; i++)
		{
			t_cwc.IndexToPlain();
			t_cwc.PlainToHash();
			t_cwc.HashToIndex(i);
		}
		t_pStartPosIndexE[t_nPos] = t_cwc.GetIndex();
		t_nChainWalkStep += t_nRainbowChainLen - 2 - t_nPos;
	}
}

void rcrackiThread::CheckAlarm()
{
	int i;
	for (i = 0; i < t_pChainsFound.size(); i++)
	{
		RainbowChain* t_pChain = t_pChainsFound[i];
		int t_nGuessedPos = t_nGuessedPoss[i];		
		
		CChainWalkContext cwc;
		//uint64 nIndexS = t_pChain->nIndexS & 0x0000FFFFFFFFFFFF; // for first 6 bytes
		uint64 nIndexS = t_pChain->nIndexS >> 16;
		cwc.SetIndex(nIndexS);
		//cwc.SetIndex(t_pChain->nIndexS);	
		int nPos;
		for (nPos = 0; nPos < t_nGuessedPos; nPos++)
		{
			cwc.IndexToPlain();
			cwc.PlainToHash();
			cwc.HashToIndex(nPos);
		}
		cwc.IndexToPlain();
		cwc.PlainToHash();
		if (cwc.CheckHash(t_pHash))
		{
			t_Hash = cwc.GetHash();
			t_Plain = cwc.GetPlain();
			t_Binary = cwc.GetBinary();

			foundHash = true;
			break;
		}
		else {
			foundHash = false;
			t_nChainWalkStepDueToFalseAlarm += t_nGuessedPos + 1;
			t_nFalseAlarm++;
		}
	}
}

bool rcrackiThread::FoundHash()
{
	return foundHash;
}

int rcrackiThread::GetChainWalkStepDueToFalseAlarm()
{
	return t_nChainWalkStepDueToFalseAlarm;
}

int rcrackiThread::GetnFalseAlarm()
{
	return t_nFalseAlarm;
}

string rcrackiThread::GetHash()
{
	return t_Hash;
}

string rcrackiThread::GetPlain()
{
	return t_Plain;
}

string rcrackiThread::GetBinary()
{
	return t_Binary;
}
