/*
   RainbowCrack - a general propose implementation of Philippe Oechslin's faster time-memory trade-off technique.

   Copyright (C) Zhu Shuanglei <shuanglei@hotmail.com>
*/

#ifdef _WIN32
	#pragma warning(disable : 4786)
#endif

#include "ChainWalkContext.h"

#include <ctype.h>
#include <openssl/rand.h>
#ifdef _WIN32
	#pragma comment(lib, "libeay32.lib")
#endif

//////////////////////////////////////////////////////////////////////

string CChainWalkContext::m_sHashRoutineName;
HASHROUTINE CChainWalkContext::m_pHashRoutine;
int CChainWalkContext::m_nHashLen;
int CChainWalkContext::m_nPlainLenMinTotal = 0;
int CChainWalkContext::m_nPlainLenMaxTotal = 0;
int CChainWalkContext::m_nHybridCharset = 0;
vector<stCharset> CChainWalkContext::m_vCharset;
uint64 CChainWalkContext::m_nPlainSpaceUpToX[MAX_PLAIN_LEN + 1];
uint64 CChainWalkContext::m_nPlainSpaceTotal;
unsigned char CChainWalkContext::m_Salt[MAX_SALT_LEN];
int CChainWalkContext::m_nSaltLen = 0;
int CChainWalkContext::m_nRainbowTableIndex;
uint64 CChainWalkContext::m_nReduceOffset;

//////////////////////////////////////////////////////////////////////

CChainWalkContext::CChainWalkContext()
{
}

CChainWalkContext::~CChainWalkContext()
{
}

bool CChainWalkContext::LoadCharset(string sName)
{
	m_vCharset.clear();
	if (sName == "byte")
	{
		stCharset tCharset;
		int i;
		for (i = 0x00; i <= 0xff; i++)
			tCharset.m_PlainCharset[i] = i;
		tCharset.m_nPlainCharsetLen = 256;
		tCharset.m_sPlainCharsetName = sName;
		tCharset.m_sPlainCharsetContent = "0x00, 0x01, ... 0xff";
		m_vCharset.push_back(tCharset);
		return true;
	}
	if(sName.substr(0, 6) == "hybrid") // Hybrid charset consisting of 2 charsets
	{
		m_nHybridCharset = 1;		
	}
	else
	{
		m_nHybridCharset = 0;
	}
	vector<string> vLine;
	if (ReadLinesFromFile("charset.txt", vLine))
	{
		int i;
		for (i = 0; i < vLine.size(); i++)
		{
			// Filter comment
			if (vLine[i][0] == '#')
				continue;

			vector<string> vPart;
			if (SeperateString(vLine[i], "=", vPart))
			{
				// sCharsetName
				string sCharsetName = TrimString(vPart[0]);
				if (sCharsetName == "")
					continue;
								
				// sCharsetName charset check
				bool fCharsetNameCheckPass = true;
				int j;
				for (j = 0; j < sCharsetName.size(); j++)
				{
					if (   !isalpha(sCharsetName[j])
						&& !isdigit(sCharsetName[j])
						&& (sCharsetName[j] != '-'))
					{
						fCharsetNameCheckPass = false;
						break;
					}
				}
				if (!fCharsetNameCheckPass)
				{
					printf("invalid charset name %s in charset configuration file\n", sCharsetName.c_str());
					continue;
				}

				// sCharsetContent
				string sCharsetContent = TrimString(vPart[1]);
				if (sCharsetContent == "" || sCharsetContent == "[]")
					continue;
				if (sCharsetContent[0] != '[' || sCharsetContent[sCharsetContent.size() - 1] != ']')
				{
					printf("invalid charset content %s in charset configuration file\n", sCharsetContent.c_str());
					continue;
				}
				sCharsetContent = sCharsetContent.substr(1, sCharsetContent.size() - 2);
				if (sCharsetContent.size() > 256)
				{
					printf("charset content %s too long\n", sCharsetContent.c_str());
					continue;
				}

				//printf("%s = [%s]\n", sCharsetName.c_str(), sCharsetContent.c_str());

				// Is it the wanted charset?
				if(m_nHybridCharset == 1)
				{
					vector<tCharset> vCharsets;
					GetHybridCharsets(sName, vCharsets);
					if(sCharsetName == vCharsets[m_vCharset.size()].sName)
					{
						stCharset tCharset = {0};
						tCharset.m_nPlainCharsetLen = sCharsetContent.size();							
						memcpy(tCharset.m_PlainCharset, sCharsetContent.c_str(), tCharset.m_nPlainCharsetLen);
						tCharset.m_sPlainCharsetName = sCharsetName;
						tCharset.m_sPlainCharsetContent = sCharsetContent;	
						tCharset.m_nPlainLenMin = vCharsets[m_vCharset.size()].nPlainLenMin;
						tCharset.m_nPlainLenMax = vCharsets[m_vCharset.size()].nPlainLenMax;
						m_vCharset.push_back(tCharset);
						if(vCharsets.size() == m_vCharset.size())
							return true;
						i = 0; // Start the lookup over again for the next charset
					}						
				}
				else if (sCharsetName == sName)
				{
					stCharset tCharset;
					tCharset.m_nPlainCharsetLen = sCharsetContent.size();							
					memcpy(tCharset.m_PlainCharset, sCharsetContent.c_str(), tCharset.m_nPlainCharsetLen);
					tCharset.m_sPlainCharsetName = sCharsetName;
					tCharset.m_sPlainCharsetContent = sCharsetContent;							
					m_vCharset.push_back(tCharset);
					return true;
				}
			}
		}
		printf("charset %s not found in charset.txt\n", sName.c_str());
	}
	else
		printf("can't open charset configuration file\n");
	return false;
}

//////////////////////////////////////////////////////////////////////

bool CChainWalkContext::SetHashRoutine(string sHashRoutineName)
{
	CHashRoutine hr;
	hr.GetHashRoutine(sHashRoutineName, m_pHashRoutine, m_nHashLen);
	if (m_pHashRoutine != NULL)
	{
		m_sHashRoutineName = sHashRoutineName;
		return true;
	}
	else
		return false;
}

bool CChainWalkContext::SetPlainCharset(string sCharsetName, int nPlainLenMin, int nPlainLenMax)
{
	// m_PlainCharset, m_nPlainCharsetLen, m_sPlainCharsetName, m_sPlainCharsetContent
	if (!LoadCharset(sCharsetName))
		return false;

	if(m_vCharset.size() == 1) // Not hybrid charset
	{
		// m_nPlainLenMin, m_nPlainLenMax
		if (nPlainLenMin < 1 || nPlainLenMax > MAX_PLAIN_LEN || nPlainLenMin > nPlainLenMax)
		{
			printf("invalid plaintext length range: %d - %d\n", nPlainLenMin, nPlainLenMax);
			return false;
		}
		m_vCharset[0].m_nPlainLenMin = nPlainLenMin;
		m_vCharset[0].m_nPlainLenMax = nPlainLenMax;
	}
	// m_nPlainSpaceUpToX
	m_nPlainSpaceUpToX[0] = 0;
	m_nPlainLenMaxTotal = 0;
	m_nPlainLenMinTotal = 0;
	uint64 nTemp = 1;
	int j, k = 1;
	for(j = 0; j < m_vCharset.size(); j++)
	{
		int i;
		m_nPlainLenMaxTotal += m_vCharset[j].m_nPlainLenMax;
		m_nPlainLenMinTotal += m_vCharset[j].m_nPlainLenMin;
		for (i = 1; i <= m_vCharset[j].m_nPlainLenMax; i++)
		{			
			nTemp *= m_vCharset[j].m_nPlainCharsetLen;
			if (i < m_vCharset[j].m_nPlainLenMin)
				m_nPlainSpaceUpToX[k] = 0;
			else
				m_nPlainSpaceUpToX[k] = m_nPlainSpaceUpToX[k - 1] + nTemp;
			k++;
		}		
	}
	// m_nPlainSpaceTotal
	m_nPlainSpaceTotal = m_nPlainSpaceUpToX[m_nPlainLenMaxTotal];

	return true;
}

bool CChainWalkContext::SetRainbowTableIndex(int nRainbowTableIndex)
{
	if (nRainbowTableIndex < 0)
		return false;
	m_nRainbowTableIndex = nRainbowTableIndex;
	m_nReduceOffset = 65536 * nRainbowTableIndex;

	return true;
}

bool CChainWalkContext::SetSalt(unsigned char *Salt, int nSaltLength)
{
	memcpy(&m_Salt[0], Salt, nSaltLength);
	
	m_nSaltLen = nSaltLength;
//	m_sSalt = sSalt;
	return true;
}

bool CChainWalkContext::SetupWithPathName(string sPathName, int& nRainbowChainLen, int& nRainbowChainCount)
{
	// something like lm_alpha#1-7_0_100x16_test.rt

#ifdef _WIN32
	int nIndex = sPathName.find_last_of('\\');
#else
	int nIndex = sPathName.find_last_of('/');
#endif
	if (nIndex != -1)
		sPathName = sPathName.substr(nIndex + 1);

	if (sPathName.size() < 3)
	{
		printf("%s is not a rainbow table\n", sPathName.c_str());
		return false;
	}
	if (sPathName.substr(sPathName.size() - 4) != ".rti")
	{
		printf("%s is not a rainbow table\n", sPathName.c_str());
		return false;
	}

	// Parse
	vector<string> vPart;
	if (!SeperateString(sPathName, "___x_", vPart))
	{
		printf("filename %s not identified\n", sPathName.c_str());
		return false;
	}

	string sHashRoutineName   = vPart[0];
	int nRainbowTableIndex    = atoi(vPart[2].c_str());
	nRainbowChainLen          = atoi(vPart[3].c_str());
	nRainbowChainCount        = atoi(vPart[4].c_str());

	// Parse charset definition
	string sCharsetDefinition = vPart[1];
	string sCharsetName;
	int nPlainLenMin = 0, nPlainLenMax = 0;		

//	printf("Charset: %s", sCharsetDefinition.c_str());
	
	if(sCharsetDefinition.substr(0, 6) == "hybrid") // Hybrid table
	{
		sCharsetName = sCharsetDefinition;
	}
	else
	{
		if (sCharsetDefinition.find('#') == -1)		// For backward compatibility, "#1-7" is implied
		{			
			sCharsetName = sCharsetDefinition;
			nPlainLenMin = 1;
			nPlainLenMax = 7;
		}
		else
		{
			vector<string> vCharsetDefinitionPart;
			if (!SeperateString(sCharsetDefinition, "#-", vCharsetDefinitionPart))
			{
				printf("filename %s not identified\n", sPathName.c_str());
				return false;	
			}
			else
			{
				sCharsetName = vCharsetDefinitionPart[0];
				nPlainLenMin = atoi(vCharsetDefinitionPart[1].c_str());
				nPlainLenMax = atoi(vCharsetDefinitionPart[2].c_str());
			}
		}
	}
	// Setup
	if (!SetHashRoutine(sHashRoutineName))
	{
		printf("hash routine %s not supported\n", sHashRoutineName.c_str());
		return false;
	}
	if (!SetPlainCharset(sCharsetName, nPlainLenMin, nPlainLenMax))
		return false;
	if (!SetRainbowTableIndex(nRainbowTableIndex))
	{
		printf("invalid rainbow table index %d\n", nRainbowTableIndex);
		return false;
	}
	m_nPlainSpaceTotal = m_nPlainSpaceUpToX[m_nPlainLenMaxTotal];
	return true;
}

string CChainWalkContext::GetHashRoutineName()
{
	return m_sHashRoutineName;
}

int CChainWalkContext::GetHashLen()
{
	return m_nHashLen;
}

string CChainWalkContext::GetPlainCharsetName()
{
	return m_vCharset[0].m_sPlainCharsetName;
}

string CChainWalkContext::GetPlainCharsetContent()
{
	return m_vCharset[0].m_sPlainCharsetContent;
}

int CChainWalkContext::GetPlainLenMin()
{
	return m_vCharset[0].m_nPlainLenMin;
}

int CChainWalkContext::GetPlainLenMax()
{
	return m_vCharset[0].m_nPlainLenMax;
}

uint64 CChainWalkContext::GetPlainSpaceTotal()
{
	return m_nPlainSpaceTotal;
}

int CChainWalkContext::GetRainbowTableIndex()
{
	return m_nRainbowTableIndex;
}

void CChainWalkContext::Dump()
{
	printf("hash routine: %s\n", m_sHashRoutineName.c_str());
	printf("hash length: %d\n", m_nHashLen);

	printf("plain charset: ");
	int i;
	for (i = 0; i < m_vCharset[0].m_nPlainCharsetLen; i++)
	{
		if (isprint(m_vCharset[0].m_PlainCharset[i]))
			printf("%c", m_vCharset[0].m_PlainCharset[i]);
		else
			printf("?");
	}
	printf("\n");

	printf("plain charset in hex: ");
	for (i = 0; i < m_vCharset[0].m_nPlainCharsetLen; i++)
		printf("%02x ", m_vCharset[0].m_PlainCharset[i]);
	printf("\n");

	printf("plain length range: %d - %d\n", m_vCharset[0].m_nPlainLenMin, m_vCharset[0].m_nPlainLenMax);
	printf("plain charset name: %s\n", m_vCharset[0].m_sPlainCharsetName.c_str());
	//printf("plain charset content: %s\n", m_sPlainCharsetContent.c_str());
	//for (i = 0; i <= m_nPlainLenMax; i++)
	//	printf("plain space up to %d: %s\n", i, uint64tostr(m_nPlainSpaceUpToX[i]).c_str());
	printf("plain space total: %s\n", uint64tostr(m_nPlainSpaceTotal).c_str());

	printf("rainbow table index: %d\n", m_nRainbowTableIndex);
	printf("reduce offset: %s\n", uint64tostr(m_nReduceOffset).c_str());
	printf("\n");
}

void CChainWalkContext::GenerateRandomIndex()
{
	RAND_bytes((unsigned char*)&m_nIndex, 8);
	m_nIndex = m_nIndex % m_nPlainSpaceTotal;
}

void CChainWalkContext::SetIndex(uint64 nIndex)
{
	m_nIndex = nIndex;
}

void CChainWalkContext::SetHash(unsigned char* pHash)
{
	memcpy(m_Hash, pHash, m_nHashLen);
}

void CChainWalkContext::IndexToPlain()
{
	int i;
	m_nPlainLen = 0;
	for (i = m_nPlainLenMaxTotal - 1; i >= m_nPlainLenMinTotal - 1; i--)
	{
		if (m_nIndex >= m_nPlainSpaceUpToX[i])
		{
			m_nPlainLen = i + 1;
			break;
		}
	}
	if(m_nPlainLen == 0)
		m_nPlainLen = m_nPlainLenMinTotal;
	uint64 nIndexOfX = m_nIndex - m_nPlainSpaceUpToX[m_nPlainLen - 1];

#ifdef _WIN64
	
	// Slow version
	for (i = m_nPlainLen - 1; i >= 0; i--)
	{
		int nCharsetLen = 0;
		for(int j = 0; j < m_vCharset.size(); i++)
		{
			nCharsetLen += m_vCharset[j].m_nPlainLenMax;
			if(i < nCharsetLen) // We found the correct charset
			{
				m_Plain[i] = m_vCharset[j].m_PlainCharset[nIndexOfX % m_nPlainCharsetLen];
				nIndexOfX /= m_vCharset[j].m_nPlainCharsetLen;
			}
		}
	}
#else


	// Fast version
	for (i = m_nPlainLen - 1; i >= 0; i--)
	{
#ifdef _WIN32
		if (nIndexOfX < 0x100000000I64)
			break;
#else
		if (nIndexOfX < 0x100000000llu)
			break;
#endif
		int nCharsetLen = 0;
		for(int j = 0; j < m_vCharset.size(); j++)
		{
			nCharsetLen += m_vCharset[j].m_nPlainLenMax;
			if(i < nCharsetLen) // We found the correct charset
			{
				m_Plain[i] = m_vCharset[j].m_PlainCharset[nIndexOfX % m_vCharset[j].m_nPlainCharsetLen];
				nIndexOfX /= m_vCharset[j].m_nPlainCharsetLen;
				break;
			}
		}
	}

	unsigned int nIndexOfX32 = (unsigned int)nIndexOfX;
	for (; i >= 0; i--)
	{
		int nCharsetLen = 0;
		for(int j = 0; j < m_vCharset.size(); j++)
		{
			nCharsetLen += m_vCharset[j].m_nPlainLenMax;
			if(i < nCharsetLen) // We found the correct charset
			{

//		m_Plain[i] = m_PlainCharset[nIndexOfX32 % m_vCharset[j].m_nPlainCharsetLen];
//		nIndexOfX32 /= m_vCharset[j].m_nPlainCharsetLen;

		unsigned int nPlainCharsetLen = m_vCharset[j].m_nPlainCharsetLen;
		unsigned int nTemp;
#ifdef _WIN32
		__asm
		{
			mov eax, nIndexOfX32
			xor edx, edx
			div nPlainCharsetLen
			mov nIndexOfX32, eax
			mov nTemp, edx
		}
		m_Plain[i] = m_vCharset[j].m_PlainCharset[nTemp];
#else
		__asm__ __volatile__ (	"mov %2, %%eax;"
								"xor %%edx, %%edx;"
								"divl %3;"
								"mov %%eax, %0;"
								"mov %%edx, %1;"
								: "=m"(nIndexOfX32), "=m"(nTemp)
								: "m"(nIndexOfX32), "m"(nPlainCharsetLen)
								: "%eax", "%edx"
							 );
		m_Plain[i] = m_vCharset[j].m_PlainCharset[nTemp];
#endif
		break;
			}
		}
	}
#endif
}

void CChainWalkContext::PlainToHash()
{	
	m_pHashRoutine(m_Plain, m_nPlainLen, m_Hash);
}

void CChainWalkContext::HashToIndex(int nPos)
{
	m_nIndex = (*(uint64*)m_Hash + m_nReduceOffset + nPos) % m_nPlainSpaceTotal;
}

uint64 CChainWalkContext::GetIndex()
{
	return m_nIndex;
}
const uint64 *CChainWalkContext::GetIndexPtr()
{
	return &m_nIndex;
}

string CChainWalkContext::GetPlain()
{
	string sRet;
	int i;
	for (i = 0; i < m_nPlainLen; i++)
	{
		char c = m_Plain[i];
		if (c >= 32 && c <= 126)
			sRet += c;
		else
			sRet += '?';
	}
	
	return sRet;
}

string CChainWalkContext::GetBinary()
{
	return HexToStr(m_Plain, m_nPlainLen);
}
/*
string CChainWalkContext::GetPlainBinary()
{
	string sRet;
	sRet += GetPlain();
	int i;
	for (i = 0; i < m_nPlainLenMax - m_nPlainLen; i++)
		sRet += ' ';

	sRet += "|";

	sRet += GetBinary();
	for (i = 0; i < m_nPlainLenMax - m_nPlainLen; i++)
		sRet += "  ";

	return sRet;
}
*/
string CChainWalkContext::GetHash()
{
	return HexToStr(m_Hash, m_nHashLen);
}

bool CChainWalkContext::CheckHash(unsigned char* pHash)
{
	if (memcmp(m_Hash, pHash, m_nHashLen) == 0)
		return true;

	return false;
}
