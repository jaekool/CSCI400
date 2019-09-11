/*
   RainbowCrack - a general propose implementation of Philippe Oechslin's faster time-memory trade-off technique.

   Copyright (C) Zhu Shuanglei <shuanglei@hotmail.com>
*/

#ifndef _CRACKENGINE_H
#define _CRACKENGINE_H

#include "Public.h"
#include "HashSet.h"
#include "ChainWalkContext.h"
#include "MemoryPool.h"
#include "ChainWalkSet.h"
#include "rcrackiThread.h"
#ifdef _WIN32
#include <conio.h>
#include <windows.h>
#endif
#include <pthread.h>

class CCrackEngine
{
public:
	CCrackEngine();
	virtual ~CCrackEngine();

private:
	CChainWalkSet m_cws;
	int maxThreads;
	bool writeOutput;
	string outputFile;

	// Statistics
	float m_fTotalDiskAccessTime;
	float m_fTotalCryptanalysisTime;
	int m_nTotalChainWalkStep;
	int m_nTotalFalseAlarm;
	int m_nTotalChainWalkStepDueToFalseAlarm;
	FILE *m_fChains;

private:
	void ResetStatistics();
	RainbowChain *BinarySearch(RainbowChain *pChain, int nChainCountRead, uint64 nIndex, IndexChain *pIndex, int nIndexSize, int nIndexStart);
	void SearchTableChunk(RainbowChain* pChain, int nRainbowChainLen, int nRainbowChainCount, CHashSet& hs, IndexChain *pIndex, int nIndexSize, int nChainStart);
	bool CheckAlarm(RainbowChain* pChain, int nGuessedPos, unsigned char* pHash, CHashSet& hs);

public:
	void SearchRainbowTable(string sPathName, CHashSet& hs);
	void Run(vector<string> vPathName, CHashSet& hs, int i_maxThreads);
	float GetStatTotalDiskAccessTime();
	float GetStatTotalCryptanalysisTime();
	int   GetStatTotalChainWalkStep();
	int   GetStatTotalFalseAlarm();
	int   GetStatTotalChainWalkStepDueToFalseAlarm();
	void setOutputFile(string sPathName);
};

#endif
