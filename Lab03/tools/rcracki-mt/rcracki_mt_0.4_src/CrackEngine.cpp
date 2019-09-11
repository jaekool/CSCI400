/*
   RainbowCrack - a general propose implementation of Philippe Oechslin's faster time-memory trade-off technique.

   Copyright (C) Zhu Shuanglei <shuanglei@hotmail.com>
*/

#ifdef _WIN32
	#pragma warning(disable : 4786)
#endif

#include "CrackEngine.h"

#include <time.h>

CCrackEngine::CCrackEngine()
{
	ResetStatistics();
	writeOutput = false;
}

CCrackEngine::~CCrackEngine()
{
}

//////////////////////////////////////////////////////////////////////

void CCrackEngine::ResetStatistics()
{
	m_fTotalDiskAccessTime               = 0.0f;
	m_fTotalCryptanalysisTime            = 0.0f;
	m_nTotalChainWalkStep                = 0;
	m_nTotalFalseAlarm                   = 0;
	m_nTotalChainWalkStepDueToFalseAlarm = 0;
//	m_nTotalFalseAlarmSkipped			 = 0;
}

RainbowChain *CCrackEngine::BinarySearch(RainbowChain *pChain, int nChainCountRead, uint64 nIndex, IndexChain *pIndex, int nIndexSize, int nIndexStart)
{
	uint64 nPrefix = nIndex >> 16;
	int nLow, nHigh;	
	bool found = false;
	int nChains = 0;
	
	if(nPrefix > pIndex[nIndexSize-1].nPrefix) // check if its in the index file
	{
		return NULL;
	}

	int nBLow = 0;
	int nBHigh = nIndexSize - 1;
	while (nBLow <= nBHigh)
	{
		int nBMid = (nBLow + nBHigh) / 2;
		if (nPrefix == pIndex[nBMid].nPrefix)
		{
			//nLow = nChains;
			int nChains = 0;

			nLow = pIndex[nBMid].nFirstChain;
			nHigh = nLow + pIndex[nBMid].nChainCount;
			if(nLow >= nIndexStart && nLow <= nIndexStart + nChainCountRead) 
			{					
				if(nHigh > nIndexStart + nChainCountRead)
					nHigh = nIndexStart + nChainCountRead;
			}
			else if(nLow < nIndexStart && nHigh >= nIndexStart)
			{
				nLow = nIndexStart;
			}
			else break;					
			found = true;
			break;
		}
		else if (nPrefix < pIndex[nBMid].nPrefix)
			nBHigh = nBMid - 1;
		else
			nBLow = nBMid + 1;
	}
	if(found == true)
	{
		for(int i = nLow - nIndexStart; i < nHigh - nIndexStart; i++)
		{
			int nSIndex = ((int)nIndex) & 0x0000FFFF;

			if (nSIndex == pChain[i].nIndexE)
			{
				return &pChain[i];
			}				
			else if(pChain[i].nIndexE > nSIndex)
				break;
		}
	}	
	return NULL;
}

// not used currently, leaving code for future checkpoints
bool CCrackEngine::CheckAlarm(RainbowChain* pChain, int nGuessedPos, unsigned char* pHash, CHashSet& hs)
{
	CChainWalkContext cwc;
	uint64 nIndexS = pChain->nIndexS >> 16;
	cwc.SetIndex(nIndexS);
	int nPos;
	for (nPos = 0; nPos < nGuessedPos; nPos++)
	{
		cwc.IndexToPlain();
		cwc.PlainToHash();
		cwc.HashToIndex(nPos);
		// Not using checkpoints atm
		/*
		switch(nPos)
		{
		case 5000:
				if((cwc.GetIndex() & 0x00000001) != (pChain->nCheckPoint & 0x00000080) >> 7)
				{
					m_nTotalFalseAlarmSkipped += 10000 - 5000;
//					printf("CheckPoint caught false alarm at position 7600\n");
					return false;
				}
				break;
		case 6000:
				if((cwc.GetIndex() & 0x00000001) != (pChain->nCheckPoint & 0x00000040) >> 6)
				{
//					printf("CheckPoint caught false alarm at position 8200\n");
					m_nTotalFalseAlarmSkipped += 10000 - 6000;
					return false;
				}
				break;

		case 7600:
				if((cwc.GetIndex() & 0x00000001) != (pChain->nCheckPoint & 0x00000020) >> 5)
				{
//					printf("CheckPoint caught false alarm at position 8700\n");
					m_nTotalFalseAlarmSkipped += 10000 - 7600;
					return false;
				}
				break;

		case 8200:
				if((cwc.GetIndex() & 0x00000001) != (pChain->nCheckPoint & 0x00000010) >> 4)
				{
//					printf("CheckPoint caught false alarm at position 9000\n");
					m_nTotalFalseAlarmSkipped += 10000 - 8200;
					return false;
				}
				break;

			case 8700:
				if((cwc.GetIndex() & 0x00000001) != (pChain->nCheckPoint & 0x00000008) >> 3)
				{
//					printf("CheckPoint caught false alarm at position 9300\n");
					m_nTotalFalseAlarmSkipped += 10000 - 8700;
					return false;
				}

				break;
			case 9000:
				if((cwc.GetIndex() & 0x00000001) != (pChain->nCheckPoint & 0x00000004) >> 2)
				{
//					printf("CheckPoint caught false alarm at position 9600\n");
					m_nTotalFalseAlarmSkipped += 10000 - 9000;
					return false;
				}

				break;
			case 9300:
				if((cwc.GetIndex() & 0x00000001) != (pChain->nCheckPoint & 0x00000002) >> 1)
				{
//					printf("CheckPoint caught false alarm at position 9600\n");
					m_nTotalFalseAlarmSkipped += 10000 - 9300;
					return false;
				}
				break;
			case 9600:
				if((cwc.GetIndex() & 0x00000001) != (pChain->nCheckPoint & 0x00000001))
				{
//					printf("CheckPoint caught false alarm at position 9600\n");
					m_nTotalFalseAlarmSkipped += 10000 - 9600;
					return false;
				}
				break;

		}*/
	}
	cwc.IndexToPlain();
	cwc.PlainToHash();
	if (cwc.CheckHash(pHash))
	{
		printf("plaintext of %s is %s\n", cwc.GetHash().c_str(), cwc.GetPlain().c_str());
		hs.SetPlain(cwc.GetHash(), cwc.GetPlain(), cwc.GetBinary());
		return true;
	}

	return false;
}

void CCrackEngine::SearchTableChunk(RainbowChain* pChain, int nRainbowChainLen, int nRainbowChainCount, CHashSet& hs, IndexChain *pIndex, int nIndexSize, int nChainStart)
{
	vector<string> vHash;
	vector<uint64 *> vIndices;
	vector<RainbowChain *> vChains;
	hs.GetLeftHashWithLen(vHash, CChainWalkContext::GetHashLen());
	printf("searching for %d hash%s...\n", vHash.size(),
										   vHash.size() > 1 ? "es" : "");

	int nChainWalkStep = 0;
	int nFalseAlarm = 0;
	int nChainWalkStepDueToFalseAlarm = 0;

	vector<rcrackiThread*> threadPool;
	vector<pthread_t> pThreads;

	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	#ifdef _WIN32
	sched_param param;
	param.sched_priority = THREAD_PRIORITY_BELOW_NORMAL;
	pthread_attr_setschedparam (&attr, &param);
	#endif
	// else set it to 5 or something (for linux)?

	bool pausing = false;

	int nHashIndex;
	for (nHashIndex = 0; nHashIndex < vHash.size(); nHashIndex++)
	{
		#ifdef _WIN32
		if (_kbhit())
		{
			int ch = _getch();
			ch = toupper(ch);
			if (ch == 'P')
			{
				pausing = true;
				printf( "\nPausing, press P again to continue... ");
				clock_t t1 = clock();
				while (pausing)
				{
					if (_kbhit())
					{
						ch = _getch();
						ch = toupper(ch);
						if (ch == 'P')
						{
							printf( " [Continuing]\n");
							pausing = false;
							clock_t t2 = clock();
							float fTime = 1.0f * (t2 - t1) / CLOCKS_PER_SEC;
							m_fTotalCryptanalysisTime -= fTime;
						}
					}
					Sleep(500);
				}
			}
			else
			{
				printf( "\nPress 'P' to pause...\n");
			}
		}
		#endif
		unsigned char TargetHash[MAX_HASH_LEN];
		int nHashLen;
		ParseHash(vHash[nHashIndex], TargetHash, nHashLen);
		if (nHashLen != CChainWalkContext::GetHashLen())
			printf("debug: nHashLen mismatch\n");

		// Request ChainWalk
		bool fNewlyGenerated;
//		printf("Requesting walk...");
		 

		uint64* pStartPosIndexE = m_cws.RequestWalk(TargetHash,
													nHashLen,
													CChainWalkContext::GetHashRoutineName(),
													CChainWalkContext::GetPlainCharsetName(),
													CChainWalkContext::GetPlainLenMin(),
													CChainWalkContext::GetPlainLenMax(),
													CChainWalkContext::GetRainbowTableIndex(),
													nRainbowChainLen,
													fNewlyGenerated);
//		printf("done!\n");
//		printf("debug: using %s walk for %s\n", fNewlyGenerated ? "newly generated" : "existing",
//												vHash[nHashIndex].c_str());

		if (fNewlyGenerated)
		{
			printf("Pre-calculating hash %d of %d.\t\t\r", nHashIndex+1, vHash.size());
			threadPool.clear();
			pThreads.clear();
			
			int thread_ID;
			for (thread_ID = 0; thread_ID < maxThreads; thread_ID++)
			{
				rcrackiThread* r_Thread = new rcrackiThread(TargetHash, thread_ID, nRainbowChainLen, maxThreads, pStartPosIndexE);
				if (r_Thread)
				{
					pthread_t pThread;
					int returnValue = pthread_create( &pThread, &attr, rcrackiThread::rcrackiThreadStaticEntryPointPthread, (void *) r_Thread);

					if( returnValue != 0 )
					{
						printf("pThread creation failed, returnValue: %d\n", returnValue);
					}
					else
					{
						pThreads.push_back(pThread);
					}

					threadPool.push_back(r_Thread);
				}
				else 
				{
					printf("r_Thread creation failed!\n");
				}
			}
			
			//printf("%d r_Threads created\t\t\n", threadPool.size());
			
			for (thread_ID = 0; thread_ID < threadPool.size(); thread_ID++)
			{
				pthread_t pThread = pThreads[thread_ID];
				int returnValue = pthread_join(pThread, NULL);
				if( returnValue != 0 )
				{
					printf("pThread join failed, returnValue: %d\n", returnValue);
				}
					
				rcrackiThread* rThread = threadPool[thread_ID];
				nChainWalkStep += rThread->GetChainWalkStep();
			}

			printf("\t\t\t\t\r");


		}

		printf("Checking false alarms for hash %d of %d.\t\t\r", nHashIndex+1, vHash.size());

		threadPool.clear();
		pThreads.clear();

		int i;
		for (i = 0; i < maxThreads; i++)
		{
			rcrackiThread* r_Thread = new rcrackiThread(TargetHash);
			threadPool.push_back(r_Thread);
		}

		int thread_ID = 0;
		int nPos;
		for (nPos = nRainbowChainLen - 2; nPos >= 0; nPos--)
		{
			uint64 nIndexEOfCurPos = pStartPosIndexE[nPos];
		
			// Search matching nIndexE
			RainbowChain *pChainFound = BinarySearch(pChain, nRainbowChainCount, nIndexEOfCurPos, pIndex, nIndexSize, nChainStart);
			if (pChainFound != NULL) // For perfected indexed tables we only recieve 1 result (huge speed increase!)
			{
				rcrackiThread* rThread = threadPool[thread_ID];
				rThread->AddAlarmCheck(pChainFound, nPos);
				if (thread_ID < maxThreads - 1 ) {
					thread_ID++;
				} else {
					thread_ID = 0;
				}
			}
		}

		for (thread_ID = 0; thread_ID < maxThreads; thread_ID++)
		{
			rcrackiThread* r_Thread = threadPool[thread_ID];
			pthread_t pThread;

			int returnValue = pthread_create( &pThread, &attr, rcrackiThread::rcrackiThreadStaticEntryPointPthread, (void *) r_Thread);

			if( returnValue != 0 )
			{
				printf("pThread creation failed, returnValue: %d\n", returnValue);
			}
			else
			{
				pThreads.push_back(pThread);
			}
		}
		
		//printf("%d r_Threads created\t\t\n", threadPool.size());

		bool foundHashInThread = false;
		for (thread_ID = 0; thread_ID < threadPool.size(); thread_ID++)
		{
			rcrackiThread* rThread = threadPool[thread_ID];
			pthread_t pThread = pThreads[thread_ID];

			int returnValue = pthread_join(pThread, NULL);
			if( returnValue != 0 )
			{
				printf("pThread join failed, returnValue: %d\n", returnValue);
			}

			nChainWalkStepDueToFalseAlarm += rThread->GetChainWalkStepDueToFalseAlarm();
			nFalseAlarm += rThread->GetnFalseAlarm();

			if (rThread->FoundHash() && !foundHashInThread) {
				printf("\t\t\t\t\t\t\r");
				printf("plaintext of %s is %s\n", rThread->GetHash().c_str(), rThread->GetPlain().c_str());
				if (writeOutput)
				{
					if (!writeResultLineToFile(outputFile, rThread->GetHash(), rThread->GetPlain(), rThread->GetBinary()))
						printf("Couldn't write this result to file!\n");
				}
				hs.SetPlain(rThread->GetHash(), rThread->GetPlain(), rThread->GetBinary());
				m_cws.DiscardWalk(pStartPosIndexE);
				foundHashInThread = true;
			}
		}

		pThreads.clear();
		threadPool.clear();

		//printf("\t\t\t\t\r");
		//printf("pChainFounds: %d\n", pChainsFound.size());
//NEXT_HASH:;
	}
	printf("\t\t\t\t\t\t\t\r");
	pThreads.clear();
	threadPool.clear();
	pthread_attr_destroy(&attr);

	//printf("debug: chain walk step: %d\n", nChainWalkStep);
	//printf("debug: false alarm: %d\n", nFalseAlarm);
	//printf("debug: chain walk step due to false alarm: %d\n", nChainWalkStepDueToFalseAlarm);

	m_nTotalChainWalkStep += nChainWalkStep;
	m_nTotalFalseAlarm += nFalseAlarm;
	m_nTotalChainWalkStepDueToFalseAlarm += nChainWalkStepDueToFalseAlarm;
}

void CCrackEngine::SearchRainbowTable(string sPathName, CHashSet& hs)
{
	// FileName
#ifdef _WIN32
	int nIndex = sPathName.find_last_of('\\');
#else
	int nIndex = sPathName.find_last_of('/');
#endif
	string sFileName;
	if (nIndex != -1)
		sFileName = sPathName.substr(nIndex + 1);
	else
		sFileName = sPathName;

	// Info
	printf("%s:\n", sFileName.c_str());

	// Setup
	int nRainbowChainLen, nRainbowChainCount;
	if (!CChainWalkContext::SetupWithPathName(sPathName, nRainbowChainLen, nRainbowChainCount))
		return;
	//printf("keyspace: %u\n", CChainWalkContext::GetPlainSpaceTotal());
	// Already finished?
	if (!hs.AnyHashLeftWithLen(CChainWalkContext::GetHashLen()))
	{
		printf("this table contains hashes with length %d only\n", CChainWalkContext::GetHashLen());
		return;
	}

	// Open
	FILE* file = fopen(sPathName.c_str(), "rb");
	if (file != NULL)
	{
		// File length check
		unsigned int nFileLen = GetFileLen(file);
		if (nFileLen % 8 != 0 || nRainbowChainCount * 8 != nFileLen)
			printf("file length mismatch\n");
		else
		{
			FILE* fIndex = fopen(((string)(sPathName + string(".index"))).c_str(), "rb");
			IndexChain *pIndex = NULL;
			int nIndexSize = 0;
			if(fIndex != NULL)
			{
				// File length check
				unsigned int nFileLen = GetFileLen(fIndex);
				unsigned int nRows = nFileLen / 11;
				unsigned int nSize = nRows * sizeof(IndexChain);
				if (nFileLen % 11 != 0)
					printf("index file length mismatch (%u bytes)\n", nFileLen);
				else
				{
					//printf("index nSize: %d\n", nSize);
					pIndex = (IndexChain*)new unsigned char[nSize];
					memset(pIndex, 0x00, nSize);
					fseek(fIndex, 0, SEEK_SET);
					unsigned char *pData = new unsigned char[11];
					int nRead = 0;
					for(int i = 0; (i * 11) < nFileLen; i++)
					{
						nRead = fread(pData, 1, 11, fIndex);
						if(nRead == 11)
						{
							memcpy(&pIndex[i].nPrefix, &pData[0], 5);
							memcpy(&pIndex[i].nFirstChain, &pData[5], 4);
							memcpy(&pIndex[i].nChainCount, &pData[9], 2);
						}
						else break;
					}
					nIndexSize = nFileLen / 11;
					fclose(fIndex);
					delete pData;
//					printf("debug: Index loaded successfully\n", nFileLen);
				}		
			}
			else 
			{
				printf("Can't load index\n");
				return;
			}
			static CMemoryPool mp;
			unsigned int nAllocatedSize;
			RainbowChain* pChain = (RainbowChain*)mp.Allocate(nFileLen, nAllocatedSize);
			//RainbowChain* pChain = (RainbowChain*)mp.Allocate(nFileLen * 2, nAllocatedSize);
			if (pChain != NULL)
			{
				//nAllocatedSize = nAllocatedSize / 16 * 16;		// Round to 16-byte boundary
				nAllocatedSize = nAllocatedSize / 8 * 8;		// Round to 8-byte boundary

				fseek(file, 0, SEEK_SET);
				bool fVerified = false;
				int nProcessedChains = 0;
				while (true)	// Chunk read loop
				{
					if (ftell(file) == nFileLen)
						break;

					// Load table chunk
					memset(pChain, 0x00, nAllocatedSize);
					printf("reading...\n");
					//printf("%u nAllocatedSize\n", nAllocatedSize);
					unsigned char *pData = new unsigned char[8];
					unsigned char *pSwap = new unsigned char[8];
					unsigned int nDataRead = 0;
					unsigned int nRead = 0;
					clock_t t1 = clock();
					//for(int i = 0; i < nAllocatedSize / 16; i++) // Chain size is 16 bytes
					for(int i = 0; i < nAllocatedSize / 8; i++) // Chain size is 8 bytes
					{
						nRead = fread(pData, 1, 8, file);
						if(nRead == 8)
						{
							nDataRead += nRead;
							//memcpy(&pChain[i].nChain, &pData[0], 8);
							memcpy(&pSwap[0], &pData[6], 2);
							memcpy(&pSwap[2], &pData[0], 6);
							memcpy(&pChain[i], &pSwap[0], 8);
							
							// memcpy(&pChain[i].nCheckPoint, &pData[7], 1);						// Disable checkpoints for now
						}
						else break;
					}
					clock_t t2 = clock();
					delete pData;
					float fTime = 1.0f * (t2 - t1) / CLOCKS_PER_SEC;
					printf("%u bytes read, disk access time: %.2f s\n", nDataRead, fTime);
					m_fTotalDiskAccessTime += fTime;
					int nRainbowChainCountRead = nDataRead / 8;
					// Verify table chunk (Too lazy to implement this)
					
					if (!fVerified)
					{
						printf("verifying the file...\n");

						// Chain length test
						int nIndexToVerify = nRainbowChainCountRead / 2;
						CChainWalkContext cwc;
						uint64 nIndexS = pChain[nIndexToVerify].nIndexS >> 16; // for last 6 bytes
						//uint64 nIndexS = pChain[nIndexToVerify].nIndexS & 0x0000FFFFFFFFFFFF; // for first 6 bytes
						//printf("nIndexS: %s\n", uint64tostr(nIndexS).c_str());
						cwc.SetIndex(nIndexS);
						//cwc.SetIndex(pChain[nIndexToVerify].nIndexS);
						int nPos;
						for (nPos = 0; nPos < nRainbowChainLen - 1; nPos++)
						{
							cwc.IndexToPlain();
							cwc.PlainToHash();
							cwc.HashToIndex(nPos);
						}
						uint64 nEndPoint = 0;
						for(int i = 0; i < nIndexSize; i++)
						{
							if(nIndexToVerify >= pIndex[i].nFirstChain && nIndexToVerify < pIndex[i].nFirstChain + pIndex[i].nChainCount) // We found the matching index
							{ // Now we need to seek nIndexToVerify into the chains
								nEndPoint += pIndex[i].nPrefix << 16;
								//unsigned short nIndexE = pChain[nIndexToVerify].nIndexS >> 48; // for last 2 bytes
								//int nIndexE = pChain[nIndexToVerify].nIndexE;
								//printf("nIndexE: %d\n", pChain[nIndexToVerify].nIndexE);
								//nEndPoint += nIndexE;
								nEndPoint += pChain[nIndexToVerify].nIndexE;
								break;
							}
						}
						if (cwc.GetIndex() != nEndPoint)
						{
							printf("rainbow chain length verify fail\n");
							break;
						}

						// Chain sort test
						// We assume its sorted in the indexing process
						/*
						int i;
						for (i = 0; i < nRainbowChainCountRead - 1; i++)
						{
							if (pChain[i].nIndexE > pChain[i + 1].nIndexE)
								break;
						}
						if (i != nRainbowChainCountRead - 1)
						{
							printf("this file is not sorted\n");
							break;
						}
						*/
						fVerified = true;
					}

					// Search table chunk
					t1 = clock();
					float preTime = m_fTotalCryptanalysisTime;
					SearchTableChunk(pChain, nRainbowChainLen, nRainbowChainCountRead, hs, pIndex, nIndexSize, nProcessedChains);
					float postTime = m_fTotalCryptanalysisTime;
					t2 = clock();
					fTime = 1.0f * (t2 - t1) / CLOCKS_PER_SEC;
					printf("cryptanalysis time: %.2f s\n", fTime + postTime - preTime);
					m_fTotalCryptanalysisTime += fTime;
					nProcessedChains += nRainbowChainCountRead;
					// Already finished?
					if (!hs.AnyHashLeftWithLen(CChainWalkContext::GetHashLen()))
						break;
				}
			}
			else printf("memory allocation fail\n");
			delete pIndex;
		}
		fclose(file);
	}
	else
		printf("can't open file\n");
}

void CCrackEngine::Run(vector<string> vPathName, CHashSet& hs, int i_maxThreads)
{
	maxThreads = i_maxThreads;
	// Reset statistics
	ResetStatistics();

	// Sort vPathName (CChainWalkSet need it)
	int i, j;
	for (i = 0; i < vPathName.size() - 1; i++)
		for (j = 0; j < vPathName.size() - i - 1; j++)
		{
			if (vPathName[j] > vPathName[j + 1])
			{
				string sTemp;
				sTemp = vPathName[j];
				vPathName[j] = vPathName[j + 1];
				vPathName[j + 1] = sTemp;
			}
		}

	// Run
	for (i = 0; i < vPathName.size() && hs.AnyhashLeft(); i++)
	{
		SearchRainbowTable(vPathName[i], hs);
		printf("\n");
	}
}

void CCrackEngine::setOutputFile(string sPathName)
{
	writeOutput = true;
	outputFile = sPathName;
}

float CCrackEngine::GetStatTotalDiskAccessTime()
{
	return m_fTotalDiskAccessTime;
}
/*float CCrackEngine::GetWastedTime()
{
	return m_fIndexTime;
}*/
float CCrackEngine::GetStatTotalCryptanalysisTime()
{
	return m_fTotalCryptanalysisTime;
}

int CCrackEngine::GetStatTotalChainWalkStep()
{
	return m_nTotalChainWalkStep;
}

int CCrackEngine::GetStatTotalFalseAlarm()
{
	return m_nTotalFalseAlarm;
}

int CCrackEngine::GetStatTotalChainWalkStepDueToFalseAlarm()
{
	return m_nTotalChainWalkStepDueToFalseAlarm;
}
