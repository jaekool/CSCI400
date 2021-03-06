/*----------------------------------------------------------------------
This source distribution is placed in the public domain by its author,
Ben Buhrow. You may use it for any purpose, free of charge,
without having to notify anyone. I disclaim any responsibility for any
errors.

Optionally, please be nice and tell me if you find this source to be
useful. Again optionally, if you add to the functionality present here
please consider making those additions public too, so that others may 
benefit from your work.	

Some parts of the code (and also this header), included in this 
distribution have been reused from other sources. In particular I 
have benefitted greatly from the work of Jason Papadopoulos's msieve @ 
www.boo.net/~jasonp, Scott Contini's mpqs implementation, and Tom St. 
Denis Tom's Fast Math library.  Many thanks to their kind donation of 
code to the public domain.
       				   --bbuhrow@gmail.com 7/28/10
----------------------------------------------------------------------*/

//definitions
#ifndef HEAD_DEF
#define HEAD_DEF

//#define DEBUG

#define _CRT_SECURE_NO_WARNINGS 

#define VERSION_STRING "1.19.2"

//basics
#define POSITIVE 0
#define NEGATIVE 1

//bases
#define DEC 10
#define HEX 16
#define OCT 8
#define BIN 2

//default maximum size in chars for a str_t
#define GSTR_MAXSIZE 1024

//#define NO_ASM

//support libraries
#include "types.h"

//global typedefs
typedef struct
{
	fp_digit *val;
	int alloc;
	int size;
	int type;
} z;

typedef struct
{
	uint32 *val;
	int alloc;
	int size;
	int type;
} z32;

typedef struct
{
	char *s;		//pointer to beginning of s
	int nchars;		//number of valid characters in s (including \0)
	int alloc;		//bytes allocated to s
} str_t;

typedef struct
{
	char name[40];
	z data;
} uvar_t;

typedef struct
{
	uvar_t *vars;
	int num;
	int alloc;
} uvars_t;

typedef struct
{
	z r;
	z rhat;
	z nhat;
	z one;
} monty;

typedef struct
{
	z factor;
	int count;
} factor_t;

typedef struct
{
	str_t **elements;	//an array of pointers to elements
	int num;			//number of elements
	int size;			//allocated number of elements in stack
	int top;			//the top element
	int type;			//is this a stack (0) or a queue (1)?
} bstack_t;

typedef struct
{
	uint32 hi;
	uint32 low;
} rand_t;

typedef struct
{
	uint32 stg1;
	uint64 stg2;
	uint32 base;
	z *factors;
	uint32 *exp;
	double stg1time;
	double stg2time;
} pm1_data_t;

typedef struct
{
	uint32 stg1;
	uint64 stg2;
	uint32 numbases;
	uint32 base;
	z *factors;
	uint32 *exp;
	double *stg1time;
	double *stg2time;
} pp1_data_t;

typedef struct
{
	uint32 stg1;
	uint64 stg2;
	uint32 numcurves;
	uint32 sigma;
	z *factors;
	uint32 *exp;
	double *stg1time;
	double *stg2time;
} ecm_data_t;

typedef struct
{
	uint32 iterations;
	uint32 numpoly;
	z *factors;
	uint32 *exp;
	double time;
} rho_data_t;

typedef struct
{
	uint32 limit;
	z *factors;
	uint32 *exp;
	double time;
} trial_data_t;

//user dimis:
//http://cboard.cprogramming.com/cplusplus-programming/
//101085-how-measure-time-multi-core-machines-pthreads.html
//
typedef struct {
	long		secs;
	long		usecs;
} TIME_DIFF;

//int qcomp_uint16(const void *x, const void *y);
//int qcomp_int(const void *x, const void *y);

//global variables
int IBASE;
int OBASE;
uint32 BRENT_MAX_IT;
uint32 FMTMAX;
uint32 WILL_STG1_MAX;
uint64 WILL_STG2_MAX;
uint32 ECM_STG1_MAX;
uint64 ECM_STG2_MAX;
uint32 POLLARD_STG1_MAX;
uint64 POLLARD_STG2_MAX;
int ECM_STG2_ISDEFAULT;
int PP1_STG2_ISDEFAULT;
int PM1_STG2_ISDEFAULT;
int VFLAG, LOGFLAG;
uint32 LOWER_POLYPOOL_INDEX;
uint32 UPPER_POLYPOOL_INDEX;
uint32 QS_DUMP_CUTOFF;
uint32 NUM_WITNESSES;
uint32 SIGMA;
int PRIMES_TO_FILE;
int PRIMES_TO_SCREEN;
int THREADS;
int AUTO_FACTOR;
int MSC_ASM;
int SCALE;
int USEBATCHFILE;
int USERSEED;
uint32 L1CACHE, L2CACHE;
uint64 PRIME_THRESHOLD;
uint64 gcounts[10];
uint32 maxbn;
int NO_SIQS_OPT;

//globals for testing siqs
int gbl_override_B_flag;
uint32 gbl_override_B;			//override the # of factor base primes
int gbl_override_tf_flag;
uint32 gbl_override_tf;			//extra reduction of the TF bound by X bits
int gbl_override_time_flag;
uint32 gbl_override_time;		//stop after this many seconds
int gbl_override_rel_flag;
uint32 gbl_override_rel;		//stop after collecting this many relations
int gbl_override_blocks_flag;
uint32 gbl_override_blocks;		//override the # of blocks used
int gbl_override_lpmult_flag;
uint32 gbl_override_lpmult;		//override the large prime multiplier
int gbl_force_DLP;

//global workspace variables
z zZero, zOne, zTwo, zThree;

//this array holds a global store of prime numbers
uint64 *spSOEprimes;	//the primes	
uint64 szSOEp;			//count of primes

//this array holds NUM_P primes in the range P_MIN to P_MAX, and
//can change as needed - always check the range and size to see
//if the primes you need are in there before using it
uint64 *PRIMES;
uint64 NUM_P;
uint64 P_MIN;
uint64 P_MAX;

//a few strings - mostly for logprint stuff
str_t gstr1, gstr2, gstr3;

//factorization log file
char flogname[80];

//input batch file
char batchfilename[80];

//siqs files
char siqs_savefile[80];

//session name
char sessionname[80];

//random seeds and the state of the LCG
rand_t g_rand;
uint64 LCGSTATE;

#ifdef WIN32
//system info
char sysname[MAX_COMPUTERNAME_LENGTH + 1];
int sysname_sz;

#else
//system info
char sysname[80];
int sysname_sz;

#endif

//option flag processing
void applyOpt(char *opt, char *arg);
unsigned process_flags(int argc, char **argv);

#endif //ifndef HEAD_DEF



