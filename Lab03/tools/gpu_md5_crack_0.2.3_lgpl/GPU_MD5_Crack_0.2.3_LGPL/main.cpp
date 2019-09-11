
/* 
    Copyright (C) 2009  Benjamin Vernoux, titanmkd@gmail.com

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 3 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <signal.h>

#if (defined(WIN32) || defined(__WIN32__) || defined(__WIN32))
#include "getopt.h"
#else
#include <unistd.h>
#endif

#include "global_types.h"
#include "bruteforce.h"
#include "md5_utils.h"
#include "charset.h"

using namespace std; 

const char *version_string = "v0.2.3 09 July 2009 LGPL";

#define REFRESH_RATE_MS 200.0
#define MAX_PWD_INIT_MEM 8000640
#define MAX_PWD_INIT 8000640

#define PWD_SIZE 16
#define MAX_SIZE_PWD PWD_SIZE+1

bool g_stop = false;
char pwd_start[32];
uint64 pwd_nb = 1000000000;

int benchmark(void);

int start_gpu_bruteforce(char *md5_str, char *pwd_start, e_charset charset);
void sighandler(int sig);

static void print_usage(void);
static void print_help(void);
static void print_version(void);

#ifndef PATH_SEPARATOR
# define PATH_SEPARATOR '/'
#endif
const char *exec_name;

static void
print_version(void)
{
  printf("GPU_MD5_Crack %s for BackTrack 4.\n", version_string);
  printf("Copyright (C) 2009 TitanMKD (titanmkd@gmail.com).\n");
}

/* Print the usage message.  */
static void
print_usage (void)
{
  printf("Usage: %s <-h MD5 Hash to bruteforce> or <-l to list all cuda device ID> or <-b to benchmark/test mode>[OPTIONS]\n", exec_name);
  printf("[OPTIONS]:\n");
  printf("-c <charset number>(default 2), 0=[0-9], 1=[a-z], 2=[a-z0-9], 3=[A-Z], 4=[A-Z0-9], 5=[A-Za-z], 6=[A-Za-z0-9], 7=All Printables\n");
  printf("-d <cuda device ID>(default first cuda device) see -l option to list cuda device ID\n");
  printf("-s <Start Password>\n");
  printf("-t <Total number of password to test>(default 1 billion)\n");
  printf("Example: %s -h 098f6bcd4621d373cade4e832627b4f6 -s a -t 1000000000\n", exec_name);
  printf("Program can be aborted at any time with Ctrl C or kill signals\n");
  exit(0);
}

bool is_pwd_start_in_charset_range(e_charset charset, char *pwd, unsigned char pwd_len)
{
	char *cCharset = NULL;
	unsigned char charset_len;
	bool match_char;

	get_charsetCPU_charsetLen(charset, &cCharset, &charset_len);

	for(int i=0; i<pwd_len; i++)
	{
		match_char = false;
		for(int c=0; c<charset_len; c++)
		{
			if(pwd[i] == cCharset[c])
			{
				match_char = true;
				break;
			}
		}
		if(match_char == false)
		{
			return false;
		}
	}

	return true;
}

int main(int argc, char **argv)
{
	#define STR_ERROR_LEN 1024
	char str_error[STR_ERROR_LEN+1] = "0";
	char MD5_str[MD5_SIZE+2];
	char start_pwd[PWD_SIZE+2];
	int c, res;
	bool is_md5_set = false;
	bool list_gpu_devices = false;
	bool benchmark_opt = false;
	int gpu_device_id = -1;
	e_charset charset;
	int charset_val;
	char *cCharset = NULL;
	unsigned char charset_len;
	md5hash hash_to_crack;
	char str_options[] = "h:lbc:d:s:t:";

	/* Init Start Password at null */
	start_pwd[0] = start_pwd[1] = 0;

	/* Default Charset */
	charset_val = 2; /* [a-z0-9] */
	charset = (e_charset)charset_val;

	print_version();
	/* Construct the name of the executable, without the directory part.  */
	exec_name = strrchr (argv[0], PATH_SEPARATOR);
	if (!exec_name)
		exec_name = argv[0];
	else
		++exec_name;

	while ((c = getopt (argc, argv, str_options)) != -1)
	switch (c)
	{
		/* MD5 Hash Input */
		case 'h':
			is_md5_set = true;
			/* We check MD5 is 32bytes and contain only Hex */
			if( !is_MD5_valid(optarg) ) 
			{
				printf("\nError MD5 Hash, incorrect size(should be 32 bytes) or Hex digit.\n\n");
				print_usage();
			}
			snprintf(MD5_str,MAX_SIZE_MD5,"%s",optarg);
			MD5_str[MD5_SIZE] = 0;
			break;

		/* List all GPU device Id */
		case 'l':
			list_gpu_devices = true;
			break;

		/* Benchmark / test mode */
		case 'b':
			benchmark_opt = true;
			break;

		/* Charset */
		case 'c':
			charset_val = atoi(optarg);
			if(charset_val<0 || charset_val>7)
			{
				printf("\nError Charset, incorrect value should be between 0 and 7\n\n");
				print_usage();
			}
			charset = (e_charset)charset_val;
			break;

		/* GPU device Id to use */
		case 'd':
			gpu_device_id = atoi(optarg);
			break;

		/* Start password */
		case 's':
			snprintf(start_pwd,PWD_SIZE,"%s",optarg);
			break;

		/* Total number of password */
		case 't':
			pwd_nb = atoll(optarg);
			if(pwd_nb < 1)
			{
				printf("\nError total number of password must be > 1.\n\n");
				print_usage();
			}
			break;

		default:
			print_usage();
	}

	/* List GPU devices */
	if(list_gpu_devices)
	{
		print_devices();
		return 0;
	}

	/* Benchmark / Test */
	if(benchmark_opt)
	{
		printf("\nBenchmark Start\n");
		res = benchmark();
		printf("Benchmark End\n");
		return res;
	}

	if(!is_md5_set)
	{
		printf("\nError missing mandatory -h option\n\n");
		print_usage();
	}
	printf("MD5 Hash:%s, Start Password:%s, Total pwd to check:%lld\n",MD5_str, start_pwd, pwd_nb);

	get_charsetCPU_charsetLen(charset, &cCharset, &charset_len);
	printf("Charset used %d:%s\n",charset_val,cCharset);

	/* Check if Password Start is in used Charset range */
	if(!is_pwd_start_in_charset_range(charset, start_pwd, (unsigned char)strlen(start_pwd)))
	{
		printf("\nError Start Password is not in used charset range please change Password Start or Charset.\n\n");
		print_usage();
	}

	if( init_bruteforce_gpu((uint)MAX_PWD_INIT_MEM, gpu_device_id) < 0)
	{
		printf("\nInit Bruteforce error !!\n");
		last_error_string(str_error, STR_ERROR_LEN);
		printf("%s\n",str_error);
		exit_bruteforce();
		return(-1);
	}

	ascii_hash_to_md5hash(MD5_str,&hash_to_crack);
	if( set_md5_gpu_parameters(charset, (uint *)&hash_to_crack) < 0)
	{
		printf("\nSet MD5 GPU parameters error !!\n");
		last_error_string(str_error, STR_ERROR_LEN);
		printf("%s\n",str_error);
		exit_bruteforce();
		return(-1);
	}

	signal(SIGABRT, &sighandler);
	signal(SIGTERM, &sighandler);
	signal(SIGINT, &sighandler);

	if( start_gpu_bruteforce(MD5_str, start_pwd, charset) < 0)
	{
		printf("\nStart GPU Bruteforce error !!\n\n");
	}

	exit_bruteforce();

	/* Check no error on CUDA */
	res = last_error_string(str_error, STR_ERROR_LEN);
	if(res != 0)
	{
	  printf("Error: %s\n",str_error);
	}

	return 0;
}

int benchmark(void)
{
	#define STR_ERROR_LEN 1024
	char str_error[STR_ERROR_LEN+1] = "0";
	char MD5_str[MD5_SIZE+2];
	char start_pwd[PWD_SIZE+2];
	int gpu_device_id = -1;
	e_charset charset;
	int charset_val;
	char *cCharset = NULL;
	unsigned char charset_len;
	md5hash hash_to_crack;

	/* 
	 Benchmark / Test structure
	*/
	struct bench_test
	{
		uint64 total_nb_of_pwd_to_check;
		char start_pwd[PWD_SIZE+2];
		e_charset charset;
		char ascii_hash_to_crack[MD5_SIZE+1];
		char expected_pwd[PWD_SIZE+2];
	};
	#define NB_BENCH_TEST 8
	
	struct bench_test bench[NB_BENCH_TEST] =
	{
		{
			1000000000,
			"1200000000",
			CHAR_NUM,
			"e807f1fcf82d132f9bb018ca6738a19f",
			"1234567890"
		},
		{
			1000000000,
			"",
			CHAR_LOW_AZ,
			"ab4f63f9ac65152575886860dde480a1",
			"azerty"
		},
		{
			1000000000,
			"",
			CHAR_LOW_AZ_NUM,
			"41b9cabe6033932eb3037fc933060adc",
			"azer09"
		},
		{
			1000000000,
			"",
			CHAR_UP_AZ,
			"fd049008572788d60140aaead79336cc",
			"AZBVSD"
		},
		{
			1000000000,
			"",
			CHAR_UP_AZ_NUM,
			"7a552dd9cdd49acc5320bad9c29c9722",
			"AZ09AA"
		},
		{
			1000000000,
			"zCAAAA",
			CHAR_UP_LOW_AZ,
			"aef49f70bb7b923b8bc0a018f916ef64",
			"zaZAab"
		},
		{
			1000000000,
			"zaAAAA",
			CHAR_UP_LOW_AZ_NUM,
			"062cc3b1302759722f48ac0b95b75803",
			"za0ZA9"
		},
		{
			1000000000,
			"a0000",
			CHAR_ALL_PRINTABLE,
			"cf7dcf4c3eeb6255668393242fcce273",
			"a^-*|"
		}
	};

	if( init_bruteforce_gpu(MAX_PWD_INIT_MEM, gpu_device_id) < 0)
	{
		printf("\nInit Bruteforce error !!\n");
		last_error_string(str_error, STR_ERROR_LEN);
		printf("%s\n",str_error);
		exit_bruteforce();
		return -1;
	}

	signal(SIGABRT, &sighandler);
	signal(SIGTERM, &sighandler);
	signal(SIGINT, &sighandler);

	for(int i=0; i < NB_BENCH_TEST; i++)
	{
		printf("******* Test %d Start *******\n", i);
		pwd_nb = bench[i].total_nb_of_pwd_to_check;
		snprintf(start_pwd,PWD_SIZE,"%s",bench[i].start_pwd);
		charset = bench[i].charset;
		charset_val = (int)charset;
		snprintf(MD5_str,MAX_SIZE_MD5,"%s",(char *)&bench[i].ascii_hash_to_crack);
		ascii_hash_to_md5hash(MD5_str,&hash_to_crack);
		printf("Expected Password: %s\n", (char *)&bench[i].expected_pwd);
		printf("MD5 Hash:%s, Start Password:%s, Total pwd to check:%lld\n",MD5_str, start_pwd, pwd_nb);
		get_charsetCPU_charsetLen(charset, &cCharset, &charset_len);
		printf("Charset used %d:%s\n",charset_val,cCharset);

		if( set_md5_gpu_parameters(charset, (uint *)&hash_to_crack) < 0)
		{
			printf("\nSet MD5 GPU parameters error !!\n");
			last_error_string(str_error, STR_ERROR_LEN);
			printf("%s\n",str_error);
			exit_bruteforce();
			return(-1);
		}

		if( start_gpu_bruteforce(MD5_str, start_pwd, charset) < 0)
		{
			printf("\nStart GPU Bruteforce error !!\n\n");
		}
		printf("******* Test %d End *******\n\n", i);

		if(g_stop)
		{
			printf("\nBenhcmark aborted\n\n");
			break;
		}
	}
	return 0;
}

void sighandler(int sig)
{
	printf("\nAborting in progress please wait(signal %d)!!                                      \n", sig);
	fflush(stdout);
	g_stop = true;
	return;
}

/*
Input:
md5_str: MD5 String to bruteforce/crack
pwd_start: Start password

*/
int start_gpu_bruteforce(char *md5_str, char *pwd_start, e_charset charset)
{
	double elapsed_time_ms, refresh_elapsed_time_ms, total_elapsed_time_ms;
	uint64 i, nb_try, total_nb_pwd;
	uint counter_time;
	char pwdstr_curr[20];
	char PwdStr[20] = 
	{
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	};
	pwdfound pwd_found;
	md5hash hash_found, hash_to_crack;
	int res;
	#define STR_ERROR_LEN 1024
	char str_error[STR_ERROR_LEN+1] = "0";

	snprintf(pwdstr_curr, 18, "%s", pwd_start);
	pwdstr_curr[19] = 0;

	g_stop = false;

	pwd_found.ui[0] = pwd_found.ui[1] = pwd_found.ui[2] = pwd_found.ui[3] = 0;
	hash_to_crack.ui[0] = hash_to_crack.ui[1] = hash_to_crack.ui[2] = hash_to_crack.ui[3] = 0;
	hash_found = hash_to_crack;

	ascii_hash_to_md5hash(md5_str,&hash_to_crack);

	if(strlen(pwdstr_curr) > 11)
	{
		printf("Error max password start len limited to 11 characters\n");
		return -1;
	}

	elapsed_time_ms = 0;
	counter_time = 0;
	refresh_elapsed_time_ms = 0;
	total_elapsed_time_ms = 0;
	total_nb_pwd = 0;
	if( (pwd_nb % MAX_PWD_INIT) == 0)
	{
		nb_try = pwd_nb/MAX_PWD_INIT;
	}else
		nb_try = (pwd_nb/MAX_PWD_INIT)+1; /* Round to calculate a bit more */

	printf("MD5 brute force started\n");
	for(i=0; i<nb_try; i++)
	{
		elapsed_time_ms = md5_crack_brute_force(charset, (uint *)&hash_to_crack, (unsigned char *)pwdstr_curr, MAX_PWD_INIT, &pwd_found);
		refresh_elapsed_time_ms += elapsed_time_ms;
		total_elapsed_time_ms += elapsed_time_ms;
		total_nb_pwd += MAX_PWD_INIT;

		if( pwd_found.ui[3] == 1)
		{
			*((uint *)&PwdStr[0]) = pwd_found.ui[0];
			*((uint *)&PwdStr[4]) = pwd_found.ui[1];
			*((uint *)&PwdStr[8]) = pwd_found.ui[2];
			*((uint *)&PwdStr[12]) = 0;
			PwdStr[strlen(PwdStr)-1] = 0;
			md5hash_to_ascii_hash(&hash_to_crack, md5_str);
			printf("\nMD5 Cracked pwd=%s hash=%s\n",PwdStr, md5_str);
			printf("Instant %01.02f Mhash/s(%01.02f ms)\n",(MAX_PWD_INIT/elapsed_time_ms)/1000.0, elapsed_time_ms);
			printf("Average %01.02f Mhash/s, Total Time:%01.02fs(%01.02f ms)\n",(total_nb_pwd/total_elapsed_time_ms)/1000.0, total_elapsed_time_ms/1000.0, total_elapsed_time_ms);
			printf("MD5 brute force finished\n");
			return 0;
		}

		counter_time++;
		if(refresh_elapsed_time_ms >= REFRESH_RATE_MS)
		{
			elapsed_time_ms = refresh_elapsed_time_ms / counter_time;
			printf("Progress %lld%%, " ,(i+1)*100/nb_try);
			printf("Pwd:%s, ", pwdstr_curr);
			printf("Instant %01.02f Mhash/s(%01.02f ms)\r",(MAX_PWD_INIT/elapsed_time_ms)/1000.0, elapsed_time_ms);
			fflush(stdout);
			refresh_elapsed_time_ms = 0;
			counter_time = 0;

			/* Check no error on Cuda */
			res = last_error_string(str_error, STR_ERROR_LEN);
			if(res != 0)
			{
				printf("\n\nError: %s\n",str_error);
				fflush(stdout);
				return 0;
			}
		}

		if(g_stop)
		{
			printf("\nProgress %lld%%, " ,(i+1)*100/nb_try);
			printf("Pwd:%s\n", pwdstr_curr);
			printf("Instant %01.02f Mhash/s(%01.02f ms)\n",(MAX_PWD_INIT/elapsed_time_ms)/1000.0, elapsed_time_ms);
			printf("Average %01.02f Mhash/s, Total Time:%01.02fs(%01.02f ms)\n",(total_nb_pwd/total_elapsed_time_ms)/1000.0, total_elapsed_time_ms/1000.0, total_elapsed_time_ms);
			printf("\nMD5 brute force finished, no password found\n");
			fflush(stdout);
			return 0;
		}

	}
	printf("End Pwd:%s                                                           \n", pwdstr_curr);
	printf("Instant %01.02f Mhash/s(%01.02f ms)\n",(MAX_PWD_INIT/elapsed_time_ms)/1000.0, elapsed_time_ms);
	printf("Average %01.02f Mhash/s, Total Time:%01.02fs(%01.02f ms)\n",(total_nb_pwd/total_elapsed_time_ms)/1000.0, total_elapsed_time_ms/1000.0, total_elapsed_time_ms);
	printf("\nMD5 brute force finished, no password found\n");
	return 0;
}
