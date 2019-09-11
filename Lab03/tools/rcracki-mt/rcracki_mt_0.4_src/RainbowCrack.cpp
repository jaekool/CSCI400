/*
   RainbowCrack - a general propose implementation of Philippe Oechslin's faster time-memory trade-off technique.

   Copyright (C) Zhu Shuanglei <shuanglei@hotmail.com>

   Modified by Martin Westergaard Jørgensen <martinwj2005@gmail.com> to support indexed and hybrid tables
*/

#ifdef _WIN32
	#pragma warning(disable : 4786)
#endif

#include "CrackEngine.h"
#include "lm2ntlm.h"

#ifdef _WIN32
	#include <io.h>
#else
	#include <sys/types.h>
	#include <sys/stat.h>
	#include <unistd.h>
	#include <dirent.h>
#endif

#include <openssl/md4.h>
#ifdef _WIN32
	#pragma comment(lib, "libeay32.lib")
#endif

//////////////////////////////////////////////////////////////////////

#ifdef _WIN32
void GetTableList(string sWildCharPathName, vector<string>& vPathName)
{
	//vPathName.clear();

	string sPath;
	int n = sWildCharPathName.find_last_of('\\');
	if (n != -1)
		sPath = sWildCharPathName.substr(0, n + 1);

	_finddata_t fd;
	int subDir_count = 0;
	//printf("sWildCharPathName: %s\n", sWildCharPathName.c_str());
	//printf("sPath: %s\n", sPath.c_str());
	long handle = _findfirst(sWildCharPathName.c_str(), &fd);
	if (handle != -1)
	{
		do
		{
			string sName = fd.name;
			//printf("sName: %s\n", sName.c_str());
			//if (sName != "." && sName != ".." && !(fd.attrib & _A_SUBDIR))
			if (sName.size()>4) {
				if (sName.substr(sName.size()-4, 4) == ".rti" && !(fd.attrib & _A_SUBDIR))
				{
					string sPathName = sPath + sName;
					vPathName.push_back(sPathName);
					//printf("sPathName: %s\n", sPathName.c_str());
				}
			}
			if (sName != "." && sName != ".." && (fd.attrib & _A_SUBDIR))
			{
				subDir_count++;
				string sPath_sub = sPath + sName + '\\';
				string sWildCharPathName_sub = sPath_sub + '*';
				_finddata_t fd_sub;
				long handle_sub = _findfirst(sWildCharPathName_sub.c_str(), &fd_sub);
				if (handle_sub != -1)
				{
					do
					{
						string sName_sub = fd_sub.name;
						//printf("sName: %s\n", sName.c_str());
						//if (sName != "." && sName != ".." && !(fd.attrib & _A_SUBDIR))
						if (sName_sub.size()>4) {
							if (sName_sub.substr(sName_sub.size()-4, 4) == ".rti" && !(fd_sub.attrib & _A_SUBDIR))
							{
								string sPathName_sub = sPath_sub + sName_sub;
								vPathName.push_back(sPathName_sub);
								//printf("sPathName_sub: %s\n", sPathName_sub.c_str());
							}
						}
					} while (_findnext(handle_sub, &fd_sub) == 0);

					_findclose(handle_sub);
				}
			}

		} while (_findnext(handle, &fd) == 0);

		_findclose(handle);
	}
	//printf("Found %d rainbowtables (files) in %d sub directories...\n", vPathName.size(), subDir_count);
}
#else
//void GetTableList(int argc, char* argv[], vector<string>& vPathName)
void GetTableList(string sWildCharPathName, vector<string>& vPathName)
{
	//vPathName.clear();

	struct stat buf;
	if (lstat(sWildCharPathName.c_str(), &buf) == 0)
	{
		if (S_ISDIR(buf.st_mode))
		{
			DIR *dir = opendir(sWildCharPathName.c_str());
			if(dir)
			{
				struct dirent *pDir=NULL;
				while((pDir = readdir(dir)) != NULL)
				{
					string filename = "";
					filename += (*pDir).d_name;
					if (filename != "." && filename != "..")
					{
						string new_filename = sWildCharPathName + '/' + filename;
						GetTableList(new_filename, vPathName);
					}
				}
				closedir(dir);
			}
		}
		else if (S_ISREG(buf.st_mode))
		{
			if (sWildCharPathName.size()>4)
			{
				if (sWildCharPathName.substr(sWildCharPathName.size()-4, 4) == ".rti")
				{
					//string sPathName_sub = sPath_sub + sName_sub;
					vPathName.push_back(sWildCharPathName);
					//printf("sPathName_sub: %s\n", sPathName_sub.c_str());
				}
			}
		}
	}
}
#endif

bool NormalizeHash(string& sHash)
{
	string sNormalizedHash = sHash;

	if (   sNormalizedHash.size() % 2 != 0
		|| sNormalizedHash.size() < MIN_HASH_LEN * 2
		|| sNormalizedHash.size() > MAX_HASH_LEN * 2)
		return false;

	// Make lower
	int i;
	for (i = 0; i < sNormalizedHash.size(); i++)
	{
		if (sNormalizedHash[i] >= 'A' && sNormalizedHash[i] <= 'F')
			sNormalizedHash[i] = sNormalizedHash[i] - 'A' + 'a';
	}

	// Character check
	for (i = 0; i < sNormalizedHash.size(); i++)
	{
		if (   (sNormalizedHash[i] < 'a' || sNormalizedHash[i] > 'f')
			&& (sNormalizedHash[i] < '0' || sNormalizedHash[i] > '9'))
			return false;
	}

	sHash = sNormalizedHash;
	return true;
}

void LoadLMHashFromPwdumpFile(string sPathName, vector<string>& vUserName, vector<string>& vLMHash, vector<string>& vNTLMHash)
{
	vector<string> vLine;
	if (ReadLinesFromFile(sPathName, vLine))
	{
		int i;
		for (i = 0; i < vLine.size(); i++)
		{
			vector<string> vPart;
			if (SeperateString(vLine[i], "::::", vPart))
			{
				string sUserName = vPart[0];
				string sLMHash   = vPart[2];
				string sNTLMHash = vPart[3];

				if (sLMHash.size() == 32 && sNTLMHash.size() == 32)
				{
					if (NormalizeHash(sLMHash) && NormalizeHash(sNTLMHash))
					{
						vUserName.push_back(sUserName);
						vLMHash.push_back(sLMHash);
						vNTLMHash.push_back(sNTLMHash);
					}
					else
						printf("invalid lm/ntlm hash %s:%s\n", sLMHash.c_str(), sNTLMHash.c_str());
				}
			}
		}
	}
	else
		printf("can't open %s\n", sPathName.c_str());
}

bool NTLMPasswordSeek(unsigned char* pLMPassword, int nLMPasswordLen, int nLMPasswordNext,
					  unsigned char* pNTLMHash, string& sNTLMPassword)
{
	if (nLMPasswordNext == nLMPasswordLen)
	{
		unsigned char md[16];
		MD4(pLMPassword, nLMPasswordLen * 2, md);
		if (memcmp(md, pNTLMHash, 16) == 0)
		{
			sNTLMPassword = "";
			int i;
			for (i = 0; i < nLMPasswordLen; i++)
				sNTLMPassword += char(pLMPassword[i * 2]);
			return true;
		}
		else
			return false;
	}

	if (NTLMPasswordSeek(pLMPassword, nLMPasswordLen, nLMPasswordNext + 1, pNTLMHash, sNTLMPassword))
		return true;

	if (   pLMPassword[nLMPasswordNext * 2] >= 'A'
		&& pLMPassword[nLMPasswordNext * 2] <= 'Z')
	{
		pLMPassword[nLMPasswordNext * 2] = pLMPassword[nLMPasswordNext * 2] - 'A' + 'a';
		if (NTLMPasswordSeek(pLMPassword, nLMPasswordLen, nLMPasswordNext + 1, pNTLMHash, sNTLMPassword))
			return true;
		pLMPassword[nLMPasswordNext * 2] = pLMPassword[nLMPasswordNext * 2] - 'a' + 'A';
	}

	return false;
}

bool LMPasswordCorrectCase(string sLMPassword, unsigned char* pNTLMHash, string& sNTLMPassword)
{
	if (sLMPassword.size() == 0)
	{
		sNTLMPassword = "";
		return true;
	}

	unsigned char* pLMPassword = new unsigned char[sLMPassword.size() * 2];
	int i;
	for (i = 0; i < sLMPassword.size(); i++)
	{
		pLMPassword[i * 2    ] = sLMPassword[i];
		pLMPassword[i * 2 + 1] = 0x00;
	}
	bool fRet = NTLMPasswordSeek(pLMPassword, sLMPassword.size(), 0, pNTLMHash, sNTLMPassword);

	delete pLMPassword;

	return fRet;
}

void Usage()
{
	Logo();

	printf("usage: rcracki_mt -h hash rainbow_table_pathname\n");
	printf("       rcracki_mt -l hash_list_file rainbow_table_pathname\n");
	printf("       rcracki_mt -f pwdump_file rainbow_table_pathname\n");
	printf("\n");
	printf("-h hash:                use raw hash as input\n");
	printf("-l hash_list_file:      use hash list file as input, each hash in a line\n");
	printf("-f pwdump_file:         use pwdump file as input, handles lanmanager hash only\n");
	printf("rainbow_table_pathname: pathname(s) of the rainbow table(s)\n");
	printf("\n");
	printf("Extra options:    -t [nr] use this amount of threads/cores, default is 1\n");
	printf("                  -o [output_file] write (temporary) results to this file\n");
	printf("\n");
	printf("example: rcracki_mt -h 5d41402abc4b2a76b9719d911017c592 -t 2 [path]\\MD5\n");
	printf("         rcracki_mt -l hash.txt [path_to_specific_table]\\*\n");
	printf("         rcracki_mt -f hash.txt -t 4 -o results.txt *.rti\n");

}

int main(int argc, char* argv[])
{
//#ifdef _WIN32
	if (argc < 4)
	{
		Usage();
		return 0;
	}
	// vPathName
	vector<string> vPathName;
	string sWildCharPathName = "";
	string sInputType        = "";
	string sInput            = "";
	string outputFile		 = "";
	bool writeOutput		 = false;
	int maxThreads			 = 1;
	
	int i;
	for (i = 1; i < argc; i++)
	{
		string cla = argv[i];
		if (cla == "-h") {
			sInputType = cla;
			i++;
			sInput = argv[i];
		}
		else if (cla == "-l") {
			sInputType = cla;
			i++;
			sInput = argv[i];
		}
		else if (cla == "-f") {
			sInputType = cla;
			i++;
			sInput = argv[i];
		}
		else if (cla == "-t") {
			i++;
			maxThreads = atoi(argv[i]);
		}
		else if (cla == "-o") {
			writeOutput = true;
			i++;
			outputFile = argv[i];
		}
		else {
			GetTableList(cla, vPathName);
		}
	}

	if (maxThreads<1)
		maxThreads = 1;

	printf("Using %d threads for pre-calculation and false alarm checking...\n", maxThreads);

	setvbuf(stdout, NULL, _IONBF,0);
	if (vPathName.size() == 0)
	{
		printf("no rainbow table found\n");
		return 0;
	}
	printf("Found %d rainbowtable files...\n\n", vPathName.size());

	// fCrackerType, vHash, vUserName, vLMHash
	bool fCrackerType;			// true: hash cracker, false: lm cracker
	vector<string> vHash;		// hash cracker
	vector<string> vUserName;	// lm cracker
	vector<string> vLMHash;		// lm cracker
	vector<string> vNTLMHash;	// lm cracker
	if (sInputType == "-h")
	{
		fCrackerType = true;

		string sHash = sInput;
		if (NormalizeHash(sHash))
			vHash.push_back(sHash);
		else
			printf("invalid hash: %s\n", sHash.c_str());
	}
	else if (sInputType == "-l")
	{
		fCrackerType = true;

		string sPathName = sInput;
		vector<string> vLine;
		if (ReadLinesFromFile(sPathName, vLine))
		{
			int i;
			for (i = 0; i < vLine.size(); i++)
			{
				string sHash = vLine[i];
				if (NormalizeHash(sHash))
					vHash.push_back(sHash);
				else
					printf("invalid hash: %s\n", sHash.c_str());
			}
		}
		else
			printf("can't open %s\n", sPathName.c_str());
	}
	else if (sInputType == "-f")
	{
		fCrackerType = false;

		string sPathName = sInput;
		LoadLMHashFromPwdumpFile(sPathName, vUserName, vLMHash, vNTLMHash);
	}
	else
	{
		Usage();
		return 0;
	}
	
	if (fCrackerType && vHash.size() == 0)
		return 0;
	if (!fCrackerType && vLMHash.size() == 0)
		return 0;

	// hs
	CHashSet hs;
	if (fCrackerType)
	{
		int i;
		for (i = 0; i < vHash.size(); i++)
			hs.AddHash(vHash[i]);
	}
	else
	{
		int i;
		for (i = 0; i < vLMHash.size(); i++)
		{
			hs.AddHash(vLMHash[i].substr(0, 16));
			hs.AddHash(vLMHash[i].substr(16, 16));
		}
	}

	// Run
	CCrackEngine ce;
	if (writeOutput)
		ce.setOutputFile(outputFile);
	ce.Run(vPathName, hs, maxThreads);

	// Statistics
	printf("statistics\n");
	printf("-------------------------------------------------------\n");
	printf("plaintext found:          %d of %d (%.2f%%)\n", hs.GetStatHashFound(),
															hs.GetStatHashTotal(),
															100.0f * hs.GetStatHashFound() / hs.GetStatHashTotal());
	printf("total disk access time:   %.2f s\n", ce.GetStatTotalDiskAccessTime());
	printf("total cryptanalysis time: %.2f s\n", ce.GetStatTotalCryptanalysisTime());
	printf("total chain walk step:    %d\n",     ce.GetStatTotalChainWalkStep());
	printf("total false alarm:        %d\n",     ce.GetStatTotalFalseAlarm());
	printf("total chain walk step due to false alarm: %d\n", ce.GetStatTotalChainWalkStepDueToFalseAlarm());
//	printf("total chain walk step skipped due to checkpoints: %d\n", ce.GetStatTotalFalseAlarmSkipped()); // Checkpoints not used - yet
	printf("\n");

	// Result
	printf("result\n");
	printf("-------------------------------------------------------\n");
	if (fCrackerType)
	{
		int i;
		for (i = 0; i < vHash.size(); i++)
		{
			string sPlain, sBinary;
			if (!hs.GetPlain(vHash[i], sPlain, sBinary))
			{
				sPlain  = "<notfound>";
				sBinary = "<notfound>";
			}

			printf("%s  %s  hex:%s\n", vHash[i].c_str(), sPlain.c_str(), sBinary.c_str());
		}
	}
	else
	{
		int i;
		for (i = 0; i < vLMHash.size(); i++)
		{
			string sPlain1, sBinary1;
			bool fPart1Found = hs.GetPlain(vLMHash[i].substr(0, 16), sPlain1, sBinary1);
			if (!fPart1Found)
			{
				sPlain1  = "<notfound>";
				sBinary1 = "<notfound>";
			}

			string sPlain2, sBinary2;
			bool fPart2Found = hs.GetPlain(vLMHash[i].substr(16, 16), sPlain2, sBinary2);
			if (!fPart2Found)
			{
				sPlain2  = "<notfound>";
				sBinary2 = "<notfound>";
			}

			string sPlain = sPlain1 + sPlain2;
			string sBinary = sBinary1 + sBinary2;

			// Correct case
			if (fPart1Found && fPart2Found)
			{
				unsigned char NTLMHash[16];
				int nHashLen;
				ParseHash(vNTLMHash[i], NTLMHash, nHashLen);
				if (nHashLen != 16)
					printf("debug: nHashLen mismatch\n");
				string sNTLMPassword;
				if (LMPasswordCorrectCase(sPlain, NTLMHash, sNTLMPassword))
				{
					sPlain = sNTLMPassword;
					sBinary = HexToStr((const unsigned char*)sNTLMPassword.c_str(), sNTLMPassword.size());
				}
				else
				{
					LM2NTLMcorrector corrector;
					if (corrector.LMPasswordCorrectUnicode(sPlain, NTLMHash, sNTLMPassword))
					{
						sPlain = sNTLMPassword;
						sBinary = corrector.getBinary();
						if (writeOutput)
						{
							if (!writeResultLineToFile(outputFile, vNTLMHash[i].c_str(), sPlain.c_str(), sBinary.c_str()))
								printf("Couldn't write final result to file!\n");
						}
					}
					else {
						printf("case correction for password %s fail!\n", sPlain.c_str());
					}
				}
			}

			// Display
			printf("%-14s  %s  hex:%s\n", vUserName[i].c_str(),
										  sPlain.c_str(),
										  sBinary.c_str());
			
		}
	}

	return 0;
}
