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
       				   --bbuhrow@gmail.com 11/24/09
----------------------------------------------------------------------*/


/*
implements a very fast sieve of erathostenes.  Speed enhancements
include a variable mod30 or mod210 wheel, bit packing
and cache blocking.  

cache blocking means that the sieve interval is split up into blocks,
and each block is sieved separately by all primes that are needed.
the size of the blocks and of the information about the sieving
primes and offsets into the next block are carefully chosen so
that everything fits into cache while sieving a block.

bit packing means that a number is represented as prime or not with
a single bit, rather than a byte or word.  this greatly reduces 
the storage requirements of the flags, which makes cache blocking 
more effective.

the mod30 or mod210 wheel means that depending
on the size of limit, either the primes 2,3, and 5 are presieved
or the primes 2,3,5 and 7 are presieved.  in other words, only the 
numbers not divisible by these small primes are 'written down' with flags.
this reduces the storage requirements for the prime flags, which in 
turn makes the cache blocking even more effective.  It also directly 
increases the speed because the slowest sieving steps have been removed.

*/

#include "yafu.h"
#include "soe.h"
#include "common.h"

#ifdef YAFU_64K
#define BLOCKSIZE 65536
#define FLAGSIZE 524288
#else
#define BLOCKSIZE 32768
#define FLAGSIZE 262144
#endif

#if defined(MSC_ASM64X) || defined(GCC_ASM64X)
	#define SOE_UNROLL_LOOPS
#endif

void start_soe_worker_thread(thread_soedata_t *t, uint32 is_master_thread) 
{

	/* create a thread that will process a sieve line. The last line does 
	   not get its own thread (the current thread handles it) */

	if (is_master_thread) {
		return;
	}

	t->command = SOE_COMMAND_INIT;
#if defined(WIN32) || defined(_WIN64)
	t->run_event = CreateEvent(NULL, FALSE, TRUE, NULL);
	t->finish_event = CreateEvent(NULL, FALSE, FALSE, NULL);
	t->thread_id = CreateThread(NULL, 0, soe_worker_thread_main, t, 0, NULL);

	WaitForSingleObject(t->finish_event, INFINITE); /* wait for ready */
#else
	pthread_mutex_init(&t->run_lock, NULL);
	pthread_cond_init(&t->run_cond, NULL);

	pthread_cond_signal(&t->run_cond);
	pthread_mutex_unlock(&t->run_lock);
	pthread_create(&t->thread_id, NULL, soe_worker_thread_main, t);

	pthread_mutex_lock(&t->run_lock); /* wait for ready */
	while (t->command != SOE_COMMAND_WAIT)
		pthread_cond_wait(&t->run_cond, &t->run_lock);
#endif
}

void start_soe_worker_thread64(thread_soedata64_t *t, uint32 is_master_thread) 
{

	/* create a thread that will process a sieve line. The last line does 
	   not get its own thread (the current thread handles it) */

	if (is_master_thread) {
		return;
	}

	t->command = SOE_COMMAND_INIT64;
#if defined(WIN32) || defined(_WIN64)
	t->run_event = CreateEvent(NULL, FALSE, TRUE, NULL);
	t->finish_event = CreateEvent(NULL, FALSE, FALSE, NULL);
	t->thread_id = CreateThread(NULL, 0, soe_worker_thread_main64, t, 0, NULL);

	WaitForSingleObject(t->finish_event, INFINITE); /* wait for ready */
#else
	pthread_mutex_init(&t->run_lock, NULL);
	pthread_cond_init(&t->run_cond, NULL);

	pthread_cond_signal(&t->run_cond);
	pthread_mutex_unlock(&t->run_lock);
	pthread_create(&t->thread_id, NULL, soe_worker_thread_main64, t);

	pthread_mutex_lock(&t->run_lock); /* wait for ready */
	while (t->command != SOE_COMMAND_WAIT64)
		pthread_cond_wait(&t->run_cond, &t->run_lock);
#endif
}

void stop_soe_worker_thread(thread_soedata_t *t, uint32 is_master_thread)
{
	if (is_master_thread) {
		return;
	}

	t->command = SOE_COMMAND_END;
#if defined(WIN32) || defined(_WIN64)
	SetEvent(t->run_event);
	WaitForSingleObject(t->thread_id, INFINITE);
	CloseHandle(t->thread_id);
	CloseHandle(t->run_event);
	CloseHandle(t->finish_event);
#else
	pthread_cond_signal(&t->run_cond);
	pthread_mutex_unlock(&t->run_lock);
	pthread_join(t->thread_id, NULL);
	pthread_cond_destroy(&t->run_cond);
	pthread_mutex_destroy(&t->run_lock);
#endif
}

void stop_soe_worker_thread64(thread_soedata64_t *t, uint32 is_master_thread)
{
	if (is_master_thread) {
		return;
	}

	t->command = SOE_COMMAND_END64;
#if defined(WIN32) || defined(_WIN64)
	SetEvent(t->run_event);
	WaitForSingleObject(t->thread_id, INFINITE);
	CloseHandle(t->thread_id);
	CloseHandle(t->run_event);
	CloseHandle(t->finish_event);
#else
	pthread_cond_signal(&t->run_cond);
	pthread_mutex_unlock(&t->run_lock);
	pthread_join(t->thread_id, NULL);
	pthread_cond_destroy(&t->run_cond);
	pthread_mutex_destroy(&t->run_lock);
#endif
}

#if defined(WIN32) || defined(_WIN64)
DWORD WINAPI soe_worker_thread_main(LPVOID thread_data) {
#else
void *soe_worker_thread_main(void *thread_data) {
#endif
	thread_soedata_t *t = (thread_soedata_t *)thread_data;

	while(1) {

		/* wait forever for work to do */
#if defined(WIN32) || defined(_WIN64)
		WaitForSingleObject(t->run_event, INFINITE);
#else
		pthread_mutex_lock(&t->run_lock);
		while (t->command == SOE_COMMAND_WAIT) {
			pthread_cond_wait(&t->run_cond, &t->run_lock);
		}
#endif
		/* do work */

		if (t->command == SOE_COMMAND_RUN)
			sieve_line(t);
		else if (t->command == SOE_COMMAND_RUN_COUNT)
			count_line(t);
		else if (t->command == SOE_COMMAND_END)
			break;

		/* signal completion */

		t->command = SOE_COMMAND_WAIT;
#if defined(WIN32) || defined(_WIN64)
		SetEvent(t->finish_event);
#else
		pthread_cond_signal(&t->run_cond);
		pthread_mutex_unlock(&t->run_lock);
#endif
	}

#if defined(WIN32) || defined(_WIN64)
	return 0;
#else
	return NULL;
#endif
}

#if defined(WIN32) || defined(_WIN64)
DWORD WINAPI soe_worker_thread_main64(LPVOID thread_data) {
#else
void *soe_worker_thread_main64(void *thread_data) {
#endif
	thread_soedata64_t *t = (thread_soedata64_t *)thread_data;

	while(1) {

		/* wait forever for work to do */
#if defined(WIN32) || defined(_WIN64)
		WaitForSingleObject(t->run_event, INFINITE);
#else
		pthread_mutex_lock(&t->run_lock);
		while (t->command == SOE_COMMAND_WAIT64) {
			pthread_cond_wait(&t->run_cond, &t->run_lock);
		}
#endif
		/* do work */

		if (t->command == SOE_COMMAND_RUN64)
			sieve_line64(t);
		else if (t->command == SOE_COMMAND_RUN_COUNT64)
			count_line64(t);
		else if (t->command == SOE_COMMAND_END64)
			break;

		/* signal completion */

		t->command = SOE_COMMAND_WAIT64;
#if defined(WIN32) || defined(_WIN64)
		SetEvent(t->finish_event);
#else
		pthread_cond_signal(&t->run_cond);
		pthread_mutex_unlock(&t->run_lock);
#endif
	}

#if defined(WIN32) || defined(_WIN64)
	return 0;
#else
	return NULL;
#endif
}

//masks for removing single bits in a byte
const uint8 masks[8] = {0x7F, 0xBF, 0xDF, 0xEF, 0xF7, 0xFB, 0xFD, 0xFE};
uint8 nmasks[8];
const uint32 bitsNbyte=8;


int spSOE(uint32 *primes, uint32 lowlimit, uint32 *highlimit, int count)
{
	/*
	finds primes up to 'limit' using the Sieve of Erathostenes
	using the mod30 or mod210 wheel, bit packing, and block sieving

	return the number of primes found less than 'limit'
	'limit' is modified to an integer slightly larger, due to 
	cache blocking and the wheel - it is easier to finish a block
	rather than check in mid loop to see if the limit has been
	exceeded.

	if count == 1, then the primes are simply counted, and not 
	explicitly calculated and saved in *primes.
	*/

	/* TODO
	* add another layer of blocks to cut down on gathering time.  
		i.e. alternate between sieving and gathering when limit is large
	*/

	//*********************** VARIABLES ******************************//
	
	//variables used for the wheel
	uint32 numclasses,prodN,startprime;

	//variables for blocking up the line structures
	uint32 numflags, numbytes, numlinebytes;

	//misc
	uint32 i,j,k,it=0,num_p,sp;
	
	//main flag array
	uint8 **line;

	//structure of static info
	soe_staticdata_t sdata;

	//thread data holds all data needed during sieving
	thread_soedata_t *thread_data;		//an array of thread data objects

	//*********************** CODE ******************************//

	sdata.orig_hlimit = *highlimit;
	sdata.orig_llimit = lowlimit;

	if (*highlimit - lowlimit < 1000000)
		*highlimit = lowlimit + 1000000;

	//more efficient to sieve using mod210 when the range is big
	if ((*highlimit - lowlimit) >= 0x80000000)
	{
		numclasses=48;
		prodN=210;
		startprime=4;
	}
	else
	{
		numclasses=8;
		prodN=30;
		startprime=3;
	}

	sdata.prodN = prodN;
	sdata.startprime = startprime;

	//allocate the residue classes.  
	sdata.rclass = (uint8 *)malloc(numclasses * sizeof(uint8));

	//create the selection masks
	for (i=0;i<bitsNbyte;i++)
		nmasks[i] = ~masks[i];

	//store the primes used to sieve the rest of the flags
	//with the max sieve range set by the size of uint32, the number of primes
	//needed is fixed.
	sdata.sieve_p = (uint16 *)malloc(6542 * sizeof(uint16));
	if (sdata.sieve_p == NULL)
	{
		printf("error allocating sieve_p\n");
		exit(-1);
	}

	//get primes to sieve with
	sp = tiny_soe(65536,sdata.sieve_p);
	sdata.pboundi = sp;

	if (*highlimit < 65536)
	{
		numlinebytes = 0;
		goto done;
	}

	//find the bound of primes we'll need to sieve with to get to limit
	sdata.pbound = (uint32)sqrt(*highlimit);
	for (i=0;i<sp;i++)
	{
		if (sdata.sieve_p[i] > sdata.pbound)
		{
			sdata.pboundi = i;
			break;
		}
	}

	//find the residue classes
	k=0;
	for (i=1;i<prodN;i++)
	{
		if (spGCD(i,prodN) == 1)
		{
			sdata.rclass[k] = (uint8)i;
			k++;
		}
	}
	
	//temporarily set lowlimit to the first multiple of numclasses*prodN < lowlimit
	lowlimit = (lowlimit/(numclasses*prodN))*(numclasses*prodN);
	sdata.lowlimit = lowlimit;

	//reallocate flag structure for wheel and block sieving
	//starting at lowlimit, we need a flag for every 'numresidues' numbers out of 'prodN' up to 
	//limit.  round limit up to make this a whole number.
	numflags = (*highlimit - lowlimit)/prodN;
	numflags += ((numflags % prodN) != 0);
	numflags *= numclasses;

	//since we can pack 8 flags in a byte, we need numflags/8 bytes allocated.
	numbytes = numflags / bitsNbyte + ((numflags % bitsNbyte) != 0);

	//since there are 8 lines to sieve over, each line will contain (numflags/8)/8 bytes
	//so round numflags/8 up to the nearest multiple of 8
	numlinebytes = numbytes/numclasses + ((numbytes % numclasses) != 0);
	*highlimit = (uint32)((uint64)numlinebytes * (uint64)prodN * (uint64)bitsNbyte + lowlimit);
	sdata.highlimit = *highlimit;
	sdata.numlinebytes = numlinebytes;
	
	//we will sieve over this many bytes of flags, for each line, block by block.
	//allocate the lines
	line = (uint8 **)malloc(numclasses * sizeof(uint8 *));
	for (i=0;i<numclasses;i++)
	{
		line[i] = (uint8 *)malloc(numlinebytes * sizeof(uint8));
		memset(line[i],255,numlinebytes);
	}

	//a block consists of 32768 bytes of flags
	//which holds 262144 flags.
	sdata.blocks = numlinebytes/BLOCKSIZE;
	sdata.partial_block_b = (numlinebytes % BLOCKSIZE)*bitsNbyte;

	//allocate a bound for each block
	sdata.pbounds = (uint32 *)malloc((sdata.blocks + (sdata.partial_block_b > 0))*sizeof(uint32));

	//special case, 1 is not a prime
	if (lowlimit <= 1)
		line[0][0] = 0x7F;

	sdata.blk_r = FLAGSIZE*prodN;
	sdata.pbounds[0] = sdata.pboundi;
	it=0;

	if (0)
	{	
		printf("temporary high limit = %u\n",*highlimit);
		printf("sieve bound = %u, bound index = %d\n",sdata.pbound,sdata.pboundi);
		printf("found %d residue classes\n",numclasses);
		printf("numlinebytes = %d\n",numlinebytes);
		printf("lineblocks = %d\n",sdata.blocks);
		printf("partial limit = %d\n",sdata.partial_block_b);
		printf("block range = %d\n",sdata.blk_r);
	}

	thread_data = (thread_soedata_t *)malloc(THREADS * sizeof(thread_soedata_t));
	for (i=0; i<THREADS; i++)
	{
		thread_soedata_t *thread = thread_data + i;

		//we'll need to store the offset into the next block for each prime
		thread->ddata.offsets = (uint16 *)malloc(6542 * sizeof(uint16));
		if (thread->ddata.offsets == NULL)
		{
			printf("error allocating offsets\n");
			exit(-1);
		}

		thread->linecount = 0;
		thread->sdata = sdata;
		thread->line = line;
	}

	/* activate the threads one at a time. The last is the
	   master thread (i.e. not a thread at all). */

	for (i = 0; i < THREADS - 1; i++)
		start_soe_worker_thread(thread_data + i, 0);

	start_soe_worker_thread(thread_data + i, 1);

	//main sieve, line by line
	k = 0;	//count total lines processed
	
	while (1)
	{
		//assign lines to the threads
		j = 0;	//how many lines to process this pass
		for (i = 0; i < THREADS; i++)
		{
			thread_soedata_t *t = thread_data + i;

			t->current_line = k;
			k++;
			j++;

			if (k >= numclasses)
				break;
		}

		//process the lines
		for (i = 0; i < j; i++) 
		{
			thread_soedata_t *t = thread_data + i;

			if (i == THREADS - 1) {
				sieve_line(t);
			}
			else {
				t->command = SOE_COMMAND_RUN;
#if defined(WIN32) || defined(_WIN64)
				SetEvent(t->run_event);
#else
				pthread_cond_signal(&t->run_cond);
				pthread_mutex_unlock(&t->run_lock);
#endif
			}
		}

		//wait for each thread to finish
		for (i = 0; i < j; i++) {
			thread_soedata_t *t = thread_data + i;

			if (i < THREADS - 1) {
#if defined(WIN32) || defined(_WIN64)
				WaitForSingleObject(t->finish_event, INFINITE);
#else
				pthread_mutex_lock(&t->run_lock);
				while (t->command != SOE_COMMAND_WAIT)
					pthread_cond_wait(&t->run_cond, &t->run_lock);
#endif
			}
		}

		if (k >= numclasses)
			break;
	}

	//stop the worker threads and free stuff not needed anymore
	for (i=0; i<THREADS - 1; i++)
	{
		stop_soe_worker_thread(thread_data + i, 0);
		free(thread_data[i].ddata.offsets);
	}
	stop_soe_worker_thread(thread_data + i, 1);
	free(thread_data[i].ddata.offsets);


	/*
	//extract primes from those already found to continue sieving the blocks > 2^32


	for (i=0;i<numlinebytes;i++)
		{
			for (j=0;j<bitsNbyte;j++)
			{
				for (k=0;k<numclasses;k++)
				{
					if (soe.line[k][i] & nmasks[j])
					{
						primes[it] = prodN*(i*bitsNbyte + j) + soe.rclass[k] + lowlimit;
						if (primes[it] >= soe.orig_llimit)
							it++;
					}
				}
			}
		}


	//main sieve, line by line
	for (lcount=0;lcount<numclasses;lcount++)
	{
		lblk_b = lowlimit;
		ublk_b = blk_r + rclass[lcount] + lblk_b - prodN;
		blk_b_sqrt = (uint32)(sqrt(ublk_b + prodN)) + 1;
		j=0;

		//for the current line, find the offsets past the low limit
		get_offsets16(startprime,sieve_p,blk_b_sqrt,lblk_b,ublk_b,blk_r,prodN,rclass,lcount,offsets,pboundi,pbounds,j);
		
		//now we have primes and offsets for every prime, for this line.
		//proceed to sieve the entire flag set, block by block, for this line
		//sieve_16(line,lcount,startprime,lineblocks,sieve_p,offsets,flagblocklimit,flagblocksize,masks,partial_limit,pbounds,pboundi);
		flagblock = line[lcount];
		for (i=0;i<lineblocks;i++)
		{
			//sieve the block with each effective prime
			for (j=startprime;j<pbounds[i];j++)
			{
				prime = sieve_p[j];
				for (k=offsets[j];k<flagblocklimit;k+=prime)
					flagblock[k>>3] &= masks[k&7];

				offsets[j]= (uint16)(k - flagblocklimit);
			}

			flagblock += flagblocksize;
		}

		//and the last partial block, don't worry about updating the offsets
		if (partial_limit > 0)
		{
			for (i=startprime;i<pboundi;i++)
			{
				if (offsets[i]<partial_limit)
				{
					prime = sieve_p[i];
					for (j=offsets[i];j<partial_limit;j+=prime)
						flagblock[j>>3] &= masks[j&7];
				}
			}
		}

	}
	*/

done:

	if (count)
	{
		//the sieve primes are not in the line array, so they must be added
		//in if necessary	
		if (sdata.pbound > lowlimit)
		{
			i=0;
			while (i<sdata.pbounds[0])
			{ 
				if (sdata.sieve_p[i] > lowlimit)
					it++;
				i++;
			}
		}
		
		//limit tweaking to make the sieve more efficient means we need to
		//check the first and last few primes in the line structures and discard
		//those outside the original requested boundaries.

		//just count the primes rather than compute them
		//still cache inefficient, but less so.

		for (i = 0; i < THREADS - 1; i++)
			start_soe_worker_thread(thread_data + i, 0);

		start_soe_worker_thread(thread_data + i, 1);
		
		k = 0;	//count total lines processed
		while (1)
		{
			//assign lines to the threads
			j = 0;	//how many lines to process this pass
			for (i = 0; i < THREADS; i++)
			{
				thread_soedata_t *t = thread_data + i;

				t->current_line = k;
				k++;
				j++;

				if (k >= numclasses)
					break;
			}

			//process the lines
			for (i = 0; i < j; i++) 
			{
				thread_soedata_t *t = thread_data + i;

				if (i == THREADS - 1) {
					count_line(t);
				}
				else {
					t->command = SOE_COMMAND_RUN_COUNT;
#if defined(WIN32) || defined(_WIN64)
					SetEvent(t->run_event);
#else
					pthread_cond_signal(&t->run_cond);
					pthread_mutex_unlock(&t->run_lock);
#endif
				}
			}

			//wait for each thread to finish
			for (i = 0; i < j; i++) {
				thread_soedata_t *t = thread_data + i;

				if (i < THREADS - 1) {
#if defined(WIN32) || defined(_WIN64)
					WaitForSingleObject(t->finish_event, INFINITE);
#else
					pthread_mutex_lock(&t->run_lock);
					while (t->command != SOE_COMMAND_WAIT)
						pthread_cond_wait(&t->run_cond, &t->run_lock);
#endif
				}
			}

			//accumulate the results from each line
			for (i = 0; i < j; i++)
				it += thread_data[i].linecount;

			if (k >= numclasses)
				break;
		}

		//stop the worker threads and free stuff not needed anymore
		for (i=0; i<THREADS - 1; i++)
			stop_soe_worker_thread(thread_data + i, 0);
		stop_soe_worker_thread(thread_data + i, 1);
		
		num_p = it;
	}
	else
	{
		//the sieve primes are not in the line array, so they must be added
		//in if necessary
		it=0;
		
		if (sdata.pbound > lowlimit)
		{
			i=0;
			while (i<sdata.pbounds[0])
			{ 
				if (sdata.sieve_p[i] > lowlimit)
				{
					primes[it] = (uint32)sdata.sieve_p[it];
					it++;
				}
				i++;
			}
		}
		

		//this will find all the primes in order, but since it jumps from line to line (column search)
		//it is very cache inefficient
		for (i=0;i<numlinebytes;i++)
		{
			for (j=0;j<bitsNbyte;j++)
			{
				for (k=0;k<numclasses;k++)
				{
					if (line[k][i] & nmasks[j])
					{
						primes[it] = prodN*(i*bitsNbyte + j) + sdata.rclass[k] + lowlimit;
						if (primes[it] >= sdata.orig_llimit)
							it++;
					}
				}
			}
		}

		//because we rounded limit up to get a whole number of bytes to sieve, 
		//the last few primes are probably bigger than limit.  ignore them.	
		for (num_p = it ; primes[num_p - 1] > sdata.orig_hlimit ; num_p--) {}

		primes = (uint32 *)realloc(primes,num_p * sizeof(uint32));
	}

	for (i=0;i<numclasses;i++)
		free(line[i]);
	free(line);

	free(thread_data);
	free(sdata.sieve_p);
	free(sdata.rclass);
	free(sdata.pbounds);

	return num_p;
}

void count_line(thread_soedata_t *thread_data)
{
	//extract stuff from the thread data structure
	soe_staticdata_t *sdata = &thread_data->sdata;
	uint8 **line = thread_data->line;
	uint32 current_line = thread_data->current_line;
	uint32 numlinebytes = sdata->numlinebytes;
	uint32 lowlimit = sdata->lowlimit;
	uint32 prodN = sdata->prodN;
	uint64 *flagblock64 = (uint64 *)line[current_line];
	uint8 *flagblock;
	uint32 i, k, it = 0, prime32;
	int done;
	int ix,kx;

	for (i=0;i<(numlinebytes >> 3);i++)
	{
		/* Convert to 64-bit unsigned integer */    
		uint64 x = flagblock64[i];
	    
		/*  Employ bit population counter algorithm from Henry S. Warren's
		 *  "Hacker's Delight" book, chapter 5.   Added one more shift-n-add
		 *  to accomdate 64 bit values.
		 */
		x = x - ((x >> 1) & 0x5555555555555555);
		x = (x & 0x3333333333333333) + ((x >> 2) & 0x3333333333333333);
		x = (x + (x >> 4)) & 0x0F0F0F0F0F0F0F0F;
		x = x + (x >> 8);
		x = x + (x >> 16);
		x = x + (x >> 32);

		it += (x & 0x000000000000003F);
	}

	//potentially misses the last few bytes
	//use the simpler baseline method to get these few
	flagblock = line[current_line];
	for (k=0; k<(numlinebytes & 7);k++)
	{
		it += (flagblock[(i<<3)+k] & nmasks[0]) >> 7;
		it += (flagblock[(i<<3)+k] & nmasks[1]) >> 6;
		it += (flagblock[(i<<3)+k] & nmasks[2]) >> 5;
		it += (flagblock[(i<<3)+k] & nmasks[3]) >> 4;
		it += (flagblock[(i<<3)+k] & nmasks[4]) >> 3;
		it += (flagblock[(i<<3)+k] & nmasks[5]) >> 2;
		it += (flagblock[(i<<3)+k] & nmasks[6]) >> 1;
		it += (flagblock[(i<<3)+k] & nmasks[7]);
	}

	//eliminate the primes flaged that are above or below the
	//actual requested limits, as both of these can change to 
	//facilitate sieving we'll need to compute them, and
	//decrement the counter if so.
	//this is a scarily nested loop, but it only should iterate
	//a few times.
	done = 0;
	for (ix=numlinebytes-1;ix>=0 && !done;ix--)
	{
		for (kx=bitsNbyte-1;kx>=0;kx--)
		{
			if (line[current_line][ix] & nmasks[kx])
			{
				prime32 = prodN*(ix*bitsNbyte + kx) + sdata->rclass[current_line] + lowlimit;
				if (prime32 > sdata->orig_hlimit)
					it--;
				else
				{
					done = 1;
					break;
				}
			}
		}
	}
	done = 0;
	for (ix=0;ix<numlinebytes && !done;ix++)
	{
		for (kx=0;kx<8;kx++)
		{
			if (line[current_line][ix] & nmasks[kx])
			{
				prime32 = prodN*(ix*bitsNbyte + kx) + sdata->rclass[current_line] + lowlimit;
				if (prime32 < sdata->orig_llimit)
					it--;
				else
				{
					done = 1;
					break;
				}
			}
		}
	}

	thread_data->linecount = it;

	return;
}

void sieve_line(thread_soedata_t *thread_data)
{
	//extract stuff from the thread data structure
	soe_dynamicdata_t *ddata = &thread_data->ddata;
	soe_staticdata_t *sdata = &thread_data->sdata;
	uint8 **line = thread_data->line;
	uint32 current_line = thread_data->current_line;

	uint8 *flagblock;
	uint32 startprime = sdata->startprime;
	uint32 i,j,k;
	uint16 prime;

	ddata->lblk_b = sdata->lowlimit + sdata->rclass[current_line];
	ddata->ublk_b = sdata->blk_r + ddata->lblk_b - sdata->prodN;
	ddata->blk_b_sqrt = (uint32)(sqrt(ddata->ublk_b + sdata->prodN)) + 1;

	//for the current line, find the offsets past the low limit
	get_offsets16(thread_data);

#ifdef SOE_UNROLL_LOOPS

	flagblock = line[current_line];
	for (i=0;i<sdata->blocks;i++)
	{
		//sieve the block with each effective prime
		uint32 maxP;
		
		//unroll the loop: all primes less than this max hit the interval at least 8 times
		maxP = FLAGSIZE >> 3;
		
		for (j=startprime;j<sdata->pbounds[i];j++)
		{
			uint32 tmpP;
			uint32 stop;
			uint32 p2, p4;

			prime = sdata->sieve_p[j];
			if (prime > maxP)
				break;

			tmpP = prime << 3;
			stop = FLAGSIZE - tmpP + prime;
			k=ddata->offsets[j];
			p2 = prime<<1;
			p4 = prime<<2;
			while (k < stop)
			{
				flagblock[k>>3] &= masks[k&7];								//0 * prime
				flagblock[(k+prime)>>3] &= masks[(k+prime)&7];				//1 * prime
				flagblock[(k+p2)>>3] &= masks[(k+p2)&7];					//2 * prime
				flagblock[(k+prime+p2)>>3] &= masks[(k+prime+p2)&7];		//3 * prime
				flagblock[(k+p4)>>3] &= masks[(k+p4)&7];					//4 * prime
				flagblock[(k+prime+p4)>>3] &= masks[(k+prime+p4)&7];		//5 * prime
				flagblock[(k+p2+p4)>>3] &= masks[(k+p2+p4)&7];				//6 * prime
				flagblock[(k+prime+p2+p4)>>3] &= masks[(k+prime+p2+p4)&7];	//7 * prime
				k += (prime << 3);											//advance
			}

			for (;k<FLAGSIZE;k+=prime)								//finish
				flagblock[k>>3] &= masks[k&7];

			ddata->offsets[j]= (uint16)(k - FLAGSIZE);
		}

		//unroll the loop: all primes less than this max hit the interval at least 4 times
		maxP = FLAGSIZE >> 2;

		for (;j<sdata->pbounds[i];j++)
		{
			uint32 tmpP;
			uint32 stop;
			uint32 p2;

			prime = sdata->sieve_p[j];
			if (prime > maxP)
				break;

			tmpP = prime << 2;
			stop = FLAGSIZE - tmpP + prime;
			k=ddata->offsets[j];
			p2 = prime<<1;
			while (k < stop)
			{
				flagblock[k>>3] &= masks[k&7];								//0 * prime
				flagblock[(k+prime)>>3] &= masks[(k+prime)&7];				//1 * prime
				flagblock[(k+p2)>>3] &= masks[(k+p2)&7];					//2 * prime
				flagblock[(k+prime+p2)>>3] &= masks[(k+prime+p2)&7];		//3 * prime
				k += (prime << 2);											//advance
			}

			for (;k<FLAGSIZE;k+=prime)								//finish
				flagblock[k>>3] &= masks[k&7];

			ddata->offsets[j]= (uint16)(k - FLAGSIZE);
		}

		//finish with primes greater than (flagblocklimit >> 2)
		for (;j<sdata->pbounds[i];j++)
		{
			prime = sdata->sieve_p[j];
			for (k=ddata->offsets[j];k<FLAGSIZE;k+=prime)
				flagblock[k>>3] &= masks[k&7];

			ddata->offsets[j]= (uint16)(k - FLAGSIZE);
		}
		flagblock += BLOCKSIZE;
	}

	//and the last partial block, don't worry about updating the offsets
	if (sdata->partial_block_b > 0)
	{
		for (i=startprime;i<sdata->pboundi;i++)
		{
			prime = sdata->sieve_p[i];
			for (j=ddata->offsets[i];j<sdata->partial_block_b;j+=prime)
				flagblock[j>>3] &= masks[j&7];
		}
	}
#else
	flagblock = line[current_line];
	for (i=0;i<sdata->blocks;i++)
	{
		for (j=startprime;j<sdata->pbounds[i];j++)
		{
			prime = sdata->sieve_p[j];
			for (k=ddata->offsets[j];k<FLAGSIZE;k+=prime)
				flagblock[k>>3] &= masks[k&7];

			ddata->offsets[j]= (uint16)(k - FLAGSIZE);
		}
		flagblock += BLOCKSIZE;
	}

	//and the last partial block, don't worry about updating the offsets
	if (sdata->partial_block_b > 0)
	{
		for (i=startprime;i<sdata->pboundi;i++)
		{
			prime = sdata->sieve_p[i];
			for (j=ddata->offsets[i];j<sdata->partial_block_b;j+=prime)
				flagblock[j>>3] &= masks[j&7];
		}
	}
#endif

	return;
}

uint64 spSOE64(uint64 *primes, uint64 lowlimit, uint64 *highlimit, int count)
{
	/*
	finds primes up to 'limit' using the Sieve of Erathostenes
	using the mod30 or mod210 wheel, bit packing, and block sieving

	return the number of primes found less than 'limit'
	'limit' is modified to an integer slightly larger, due to 
	cache blocking and the wheel - it is easier to finish a block
	rather than check in mid loop to see if the limit has been
	exceeded.

	if count == 1, then the primes are simply counted, and not 
	explicitly calculated and saved in *primes.
	*/

	/* TODO
	* add another layer of blocks to cut down on gathering time.  
		i.e. alternate between sieving and gathering when limit is large
	*/

	//*********************** VARIABLES ******************************//
	
	//variables used for the wheel
	uint64 numclasses,prodN,startprime;

	//variables for blocking up the line structures
	uint64 numflags, numbytes, numlinebytes;

	//misc
	uint64 i,j,k,it=0,num_p,sp;
	
	//main flag array
	uint8 **line;

	//structure of static info
	soe_staticdata64_t sdata;

	//thread data holds all data needed during sieving
	thread_soedata64_t *thread_data;		//an array of thread data objects

	//*********************** CODE ******************************//

	sdata.orig_hlimit = *highlimit;
	sdata.orig_llimit = lowlimit;

	if (*highlimit - lowlimit < 1000000)
		*highlimit = lowlimit + 1000000;

	//more efficient to sieve using mod210 when the range is big
	if ((*highlimit - lowlimit) >= 0x80000000)
	{
		numclasses=48;
		prodN=210;
		startprime=4;

		//numclasses=480;
		//prodN=2310;
		//startprime=5;
	}
	else
	{
		numclasses=8;
		prodN=30;
		startprime=3;
	}
		

	sdata.prodN = prodN;
	sdata.startprime = startprime;

	//allocate the residue classes.  
	sdata.rclass = (uint8 *)malloc(numclasses * sizeof(uint8));

	//create the selection masks
	for (i=0;i<bitsNbyte;i++)
		nmasks[i] = ~masks[i];

	//store the primes used to sieve the rest of the flags
	//with the max sieve range set by the size of uint32, the number of primes
	//needed is fixed.
	//find the bound of primes we'll need to sieve with to get to limit
	sdata.pbound = (uint64)(sqrt(*highlimit) + 1);

	//just set this number high, for now.
	sdata.sieve_p = (uint32 *)malloc(1000000 * sizeof(uint32));
	if (sdata.sieve_p == NULL)
	{
		printf("error allocating sieve_p\n");
		exit(-1);
	}

	//get primes to sieve with
	sp = tiny_soe32(sdata.pbound,sdata.sieve_p);
	sdata.pboundi = sp;

	if (*highlimit < 65536)
	{
		numlinebytes = 0;
		goto done;
	}

	/*
	for (i=0;i<sp;i++)
	{
		if (sdata.sieve_p[i] > sdata.pbound)
		{
			sdata.pboundi = i;
			break;
		}
	}
	*/

	//find the residue classes
	k=0;
	//printf("residue classes = ");
	for (i=1;i<prodN;i++)
	{
		if (spGCD(i,prodN) == 1)
		{
			//printf("%lu ",i);
			sdata.rclass[k] = (uint8)i;
			k++;
		}
	}
	//printf("\nfound %lu residue classes\n",k);
	
	//temporarily set lowlimit to the first multiple of numclasses*prodN < lowlimit
	lowlimit = (lowlimit/(numclasses*prodN))*(numclasses*prodN);
	sdata.lowlimit = lowlimit;

	//reallocate flag structure for wheel and block sieving
	//starting at lowlimit, we need a flag for every 'numresidues' numbers out of 'prodN' up to 
	//limit.  round limit up to make this a whole number.
	numflags = (*highlimit - lowlimit)/prodN;
	numflags += ((numflags % prodN) != 0);
	numflags *= numclasses;

	//since we can pack 8 flags in a byte, we need numflags/8 bytes allocated.
	numbytes = numflags / bitsNbyte + ((numflags % bitsNbyte) != 0);

	//since there are 8 lines to sieve over, each line will contain (numflags/8)/8 bytes
	//so round numflags/8 up to the nearest multiple of 8
	numlinebytes = numbytes/numclasses + ((numbytes % numclasses) != 0);
	*highlimit = (uint64)((uint64)numlinebytes * (uint64)prodN * (uint64)bitsNbyte + lowlimit);
	sdata.highlimit = *highlimit;
	sdata.numlinebytes = numlinebytes;
	
	//we will sieve over this many bytes of flags, for each line, block by block.
	//allocate the lines
	line = (uint8 **)malloc(numclasses * sizeof(uint8 *));
	for (i=0;i<numclasses;i++)
	{
		line[i] = (uint8 *)malloc(numlinebytes * sizeof(uint8));
		memset(line[i],255,numlinebytes);
	}

	//a block consists of 32768 bytes of flags
	//which holds 262144 flags.
	sdata.blocks = numlinebytes/BLOCKSIZE;
	sdata.partial_block_b = (numlinebytes % BLOCKSIZE)*bitsNbyte;

	//allocate a bound for each block
	sdata.pbounds = (uint32 *)malloc((sdata.blocks + (sdata.partial_block_b > 0))*sizeof(uint64));

	//special case, 1 is not a prime
	if (lowlimit <= 1)
		line[0][0] = 0x7F;

	sdata.blk_r = FLAGSIZE*prodN;
	sdata.pbounds[0] = sdata.pboundi;
	it=0;

	if (0)
	{	
		printf("temporary high limit = %lu\n",*highlimit);
		printf("sieve bound = %lu, bound index = %lu\n",sdata.pbound,sdata.pboundi);
		printf("found %lu residue classes\n",numclasses);
		printf("numlinebytes = %lu\n",numlinebytes);
		printf("lineblocks = %lu\n",sdata.blocks);
		printf("partial limit = %lu\n",sdata.partial_block_b);
		printf("block range = %lu\n",sdata.blk_r);
	}

	thread_data = (thread_soedata64_t *)malloc(THREADS * sizeof(thread_soedata64_t));
	for (i=0; i<THREADS; i++)
	{
		thread_soedata64_t *thread = thread_data + i;

		//we'll need to store the offset into the next block for each prime
		thread->ddata.offsets = (uint32 *)malloc(1000000 * sizeof(uint32));
		if (thread->ddata.offsets == NULL)
		{
			printf("error allocating offsets\n");
			exit(-1);
		}

		thread->linecount = 0;
		thread->sdata = sdata;
		thread->line = line;
	}

	/* activate the threads one at a time. The last is the
	   master thread (i.e. not a thread at all). */

	for (i = 0; i < THREADS - 1; i++)
		start_soe_worker_thread64(thread_data + i, 0);

	start_soe_worker_thread64(thread_data + i, 1);

	//main sieve, line by line
	k = 0;	//count total lines processed
	
	while (1)
	{
		//assign lines to the threads
		j = 0;	//how many lines to process this pass
		for (i = 0; i < THREADS; i++)
		{
			thread_soedata64_t *t = thread_data + i;

			t->current_line = k;
			k++;
			j++;

			if (k >= numclasses)
				break;
		}

		//process the lines
		for (i = 0; i < j; i++) 
		{
			thread_soedata64_t *t = thread_data + i;

			if (i == THREADS - 1) {
				sieve_line64(t);
			}
			else {
				t->command = SOE_COMMAND_RUN64;
#if defined(WIN32) || defined(_WIN64)
				SetEvent(t->run_event);
#else
				pthread_cond_signal(&t->run_cond);
				pthread_mutex_unlock(&t->run_lock);
#endif
			}
		}

		//wait for each thread to finish
		for (i = 0; i < j; i++) {
			thread_soedata64_t *t = thread_data + i;

			if (i < THREADS - 1) {
#if defined(WIN32) || defined(_WIN64)
				WaitForSingleObject(t->finish_event, INFINITE);
#else
				pthread_mutex_lock(&t->run_lock);
				while (t->command != SOE_COMMAND_WAIT64)
					pthread_cond_wait(&t->run_cond, &t->run_lock);
#endif
			}
		}

		if (k >= numclasses)
			break;
	}

	//stop the worker threads and free stuff not needed anymore
	for (i=0; i<THREADS - 1; i++)
	{
		stop_soe_worker_thread64(thread_data + i, 0);
		free(thread_data[i].ddata.offsets);
	}
	stop_soe_worker_thread64(thread_data + i, 1);
	free(thread_data[i].ddata.offsets);

done:

	if (count)
	{
		//the sieve primes are not in the line array, so they must be added
		//in if necessary	
		if (sdata.pbound > lowlimit)
		{
			i=0;
			while (i<sdata.pbounds[0])
			{ 
				if (sdata.sieve_p[i] > lowlimit)
					it++;
				i++;
			}
		}
		
		//limit tweaking to make the sieve more efficient means we need to
		//check the first and last few primes in the line structures and discard
		//those outside the original requested boundaries.

		//just count the primes rather than compute them
		//still cache inefficient, but less so.

		for (i = 0; i < THREADS - 1; i++)
			start_soe_worker_thread64(thread_data + i, 0);

		start_soe_worker_thread64(thread_data + i, 1);
		
		k = 0;	//count total lines processed
		while (1)
		{
			//assign lines to the threads
			j = 0;	//how many lines to process this pass
			for (i = 0; i < THREADS; i++)
			{
				thread_soedata64_t *t = thread_data + i;

				t->current_line = k;
				k++;
				j++;

				if (k >= numclasses)
					break;
			}

			//process the lines
			for (i = 0; i < j; i++) 
			{
				thread_soedata64_t *t = thread_data + i;

				if (i == THREADS - 1) {
					count_line64(t);
				}
				else {
					t->command = SOE_COMMAND_RUN_COUNT64;
#if defined(WIN32) || defined(_WIN64)
					SetEvent(t->run_event);
#else
					pthread_cond_signal(&t->run_cond);
					pthread_mutex_unlock(&t->run_lock);
#endif
				}
			}

			//wait for each thread to finish
			for (i = 0; i < j; i++) {
				thread_soedata64_t *t = thread_data + i;

				if (i < THREADS - 1) {
#if defined(WIN32) || defined(_WIN64)
					WaitForSingleObject(t->finish_event, INFINITE);
#else
					pthread_mutex_lock(&t->run_lock);
					while (t->command != SOE_COMMAND_WAIT64)
						pthread_cond_wait(&t->run_cond, &t->run_lock);
#endif
				}
			}

			//accumulate the results from each line
			for (i = 0; i < j; i++)
				it += thread_data[i].linecount;

			if (k >= numclasses)
				break;
		}

		//stop the worker threads and free stuff not needed anymore
		for (i=0; i<THREADS - 1; i++)
			stop_soe_worker_thread64(thread_data + i, 0);
		stop_soe_worker_thread64(thread_data + i, 1);
		
		num_p = it;
	}
	else
	{
		//the sieve primes are not in the line array, so they must be added
		//in if necessary
		it=0;
		
		if (sdata.pbound > lowlimit)
		{
			i=0;
			while (i<sdata.pbounds[0])
			{ 
				if (sdata.sieve_p[i] > lowlimit)
				{
					primes[it] = (uint64)sdata.sieve_p[it];
					it++;
				}
				i++;
			}
		}
		

		//this will find all the primes in order, but since it jumps from line to line (column search)
		//it is very cache inefficient
		for (i=0;i<numlinebytes;i++)
		{
			for (j=0;j<bitsNbyte;j++)
			{
				for (k=0;k<numclasses;k++)
				{
					if (line[k][i] & nmasks[j])
					{
						primes[it] = prodN*(i*bitsNbyte + j) + sdata.rclass[k] + lowlimit;
						if (primes[it] >= sdata.orig_llimit)
							it++;
					}
				}
			}
		}

		//because we rounded limit up to get a whole number of bytes to sieve, 
		//the last few primes are probably bigger than limit.  ignore them.	
		for (num_p = it ; primes[num_p - 1] > sdata.orig_hlimit ; num_p--) {}

		primes = (uint64 *)realloc(primes,num_p * sizeof(uint64));
	}

	for (i=0;i<numclasses;i++)
		free(line[i]);
	free(line);

	free(thread_data);
	free(sdata.sieve_p);
	free(sdata.rclass);
	free(sdata.pbounds);

	return num_p;
}

void count_line64(thread_soedata64_t *thread_data)
{
	//extract stuff from the thread data structure
	soe_staticdata64_t *sdata = &thread_data->sdata;
	uint8 **line = thread_data->line;
	uint32 current_line = thread_data->current_line;
	uint64 numlinebytes = sdata->numlinebytes;
	uint64 lowlimit = sdata->lowlimit;
	uint64 prodN = sdata->prodN;
	uint64 *flagblock64 = (uint64 *)line[current_line];
	uint8 *flagblock;
	uint64 prime;
	uint64 i, k, it = 0;
	int done;
	int ix,kx;

	for (i=0;i<(numlinebytes >> 3);i++)
	{
		/* Convert to 64-bit unsigned integer */    
		uint64 x = flagblock64[i];
	    
		/*  Employ bit population counter algorithm from Henry S. Warren's
		 *  "Hacker's Delight" book, chapter 5.   Added one more shift-n-add
		 *  to accomdate 64 bit values.
		 */
		x = x - ((x >> 1) & 0x5555555555555555);
		x = (x & 0x3333333333333333) + ((x >> 2) & 0x3333333333333333);
		x = (x + (x >> 4)) & 0x0F0F0F0F0F0F0F0F;
		x = x + (x >> 8);
		x = x + (x >> 16);
		x = x + (x >> 32);

		it += (x & 0x000000000000003F);
	}

	//potentially misses the last few bytes
	//use the simpler baseline method to get these few
	flagblock = line[current_line];
	for (k=0; k<(numlinebytes & 7);k++)
	{
		it += (flagblock[(i<<3)+k] & nmasks[0]) >> 7;
		it += (flagblock[(i<<3)+k] & nmasks[1]) >> 6;
		it += (flagblock[(i<<3)+k] & nmasks[2]) >> 5;
		it += (flagblock[(i<<3)+k] & nmasks[3]) >> 4;
		it += (flagblock[(i<<3)+k] & nmasks[4]) >> 3;
		it += (flagblock[(i<<3)+k] & nmasks[5]) >> 2;
		it += (flagblock[(i<<3)+k] & nmasks[6]) >> 1;
		it += (flagblock[(i<<3)+k] & nmasks[7]);
	}

	//eliminate the primes flaged that are above or below the
	//actual requested limits, as both of these can change to 
	//facilitate sieving we'll need to compute them, and
	//decrement the counter if so.
	//this is a scarily nested loop, but it only should iterate
	//a few times.
	done = 0;
	for (ix=numlinebytes-1;ix>=0 && !done;ix--)
	{
		for (kx=bitsNbyte-1;kx>=0;kx--)
		{
			if (line[current_line][ix] & nmasks[kx])
			{
				prime = prodN*(ix*bitsNbyte + kx) + sdata->rclass[current_line] + lowlimit;
				if (prime > sdata->orig_hlimit)
					it--;
				else
				{
					done = 1;
					break;
				}
			}
		}
	}
	done = 0;
	for (ix=0;ix<numlinebytes && !done;ix++)
	{
		for (kx=0;kx<8;kx++)
		{
			if (line[current_line][ix] & nmasks[kx])
			{
				prime = prodN*(ix*bitsNbyte + kx) + sdata->rclass[current_line] + lowlimit;
				if (prime < sdata->orig_llimit)
					it--;
				else
				{
					done = 1;
					break;
				}
			}
		}
	}

	thread_data->linecount = it;

	return;
}

void sieve_line64(thread_soedata64_t *thread_data)
{
	//extract stuff from the thread data structure
	soe_dynamicdata64_t *ddata = &thread_data->ddata;
	soe_staticdata64_t *sdata = &thread_data->sdata;
	uint8 **line = thread_data->line;
	uint32 current_line = thread_data->current_line;

	uint8 *flagblock;
	uint64 startprime = sdata->startprime;
	uint64 i,j,k;
	uint32 prime;

	ddata->lblk_b = sdata->lowlimit + sdata->rclass[current_line];
	ddata->ublk_b = sdata->blk_r + ddata->lblk_b - sdata->prodN;
	ddata->blk_b_sqrt = (uint32)(sqrt(ddata->ublk_b + sdata->prodN)) + 1;

	//for the current line, find the offsets past the low limit
	get_offsets32(thread_data);

#ifdef SOE_UNROLL_LOOPS

	flagblock = line[current_line];
	for (i=0;i<sdata->blocks;i++)
	{
		//sieve the block with each effective prime
		uint32 maxP;
		
		//unroll the loop: all primes less than this max hit the interval at least 8 times
		maxP = FLAGSIZE >> 3;
		
		for (j=startprime;j<sdata->pbounds[i];j++)
		{
			uint32 tmpP;
			uint64 stop;
			uint64 p2, p4;

			prime = sdata->sieve_p[j];
			if (prime > maxP)
				break;

			tmpP = prime << 3;
			stop = FLAGSIZE - tmpP + prime;
			k=ddata->offsets[j];
			p2 = prime<<1;
			p4 = prime<<2;
			while (k < stop)
			{
				flagblock[k>>3] &= masks[k&7];								//0 * prime
				flagblock[(k+prime)>>3] &= masks[(k+prime)&7];				//1 * prime
				flagblock[(k+p2)>>3] &= masks[(k+p2)&7];					//2 * prime
				flagblock[(k+prime+p2)>>3] &= masks[(k+prime+p2)&7];		//3 * prime
				flagblock[(k+p4)>>3] &= masks[(k+p4)&7];					//4 * prime
				flagblock[(k+prime+p4)>>3] &= masks[(k+prime+p4)&7];		//5 * prime
				flagblock[(k+p2+p4)>>3] &= masks[(k+p2+p4)&7];				//6 * prime
				flagblock[(k+prime+p2+p4)>>3] &= masks[(k+prime+p2+p4)&7];	//7 * prime
				k += (prime << 3);											//advance
			}

			for (;k<FLAGSIZE;k+=prime)								//finish
				flagblock[k>>3] &= masks[k&7];

			ddata->offsets[j]= (uint32)(k - FLAGSIZE);
		}

		//unroll the loop: all primes less than this max hit the interval at least 4 times
		maxP = FLAGSIZE >> 2;

		for (;j<sdata->pbounds[i];j++)
		{
			uint32 tmpP;
			uint64 stop;
			uint64 p2;

			prime = sdata->sieve_p[j];
			if (prime > maxP)
				break;

			tmpP = prime << 2;
			stop = FLAGSIZE - tmpP + prime;
			k=ddata->offsets[j];
			p2 = prime<<1;
			while (k < stop)
			{
				flagblock[k>>3] &= masks[k&7];								//0 * prime
				flagblock[(k+prime)>>3] &= masks[(k+prime)&7];				//1 * prime
				flagblock[(k+p2)>>3] &= masks[(k+p2)&7];					//2 * prime
				flagblock[(k+prime+p2)>>3] &= masks[(k+prime+p2)&7];		//3 * prime
				k += (prime << 2);											//advance
			}

			for (;k<FLAGSIZE;k+=prime)								//finish
				flagblock[k>>3] &= masks[k&7];

			ddata->offsets[j]= (uint32)(k - FLAGSIZE);
		}

		//finish with primes greater than (flagblocklimit >> 2)
		for (;j<sdata->pbounds[i];j++)
		{
			prime = sdata->sieve_p[j];
			for (k=ddata->offsets[j];k<FLAGSIZE;k+=prime)
				flagblock[k>>3] &= masks[k&7];

			ddata->offsets[j]= (uint32)(k - FLAGSIZE);
		}
		flagblock += BLOCKSIZE;
	}

	//and the last partial block, don't worry about updating the offsets
	if (sdata->partial_block_b > 0)
	{
		for (i=startprime;i<sdata->pboundi;i++)
		{
			prime = sdata->sieve_p[i];
			for (j=ddata->offsets[i];j<sdata->partial_block_b;j+=prime)
				flagblock[j>>3] &= masks[j&7];
		}
	}
#else
	flagblock = line[current_line];
	for (i=0;i<sdata->blocks;i++)
	{
		for (j=startprime;j<sdata->pbounds[i];j++)
		{
			prime = sdata->sieve_p[j];
			for (k=ddata->offsets[j];k<FLAGSIZE;k+=prime)
				flagblock[k>>3] &= masks[k&7];

			ddata->offsets[j]= (uint32)(k - FLAGSIZE);
		}
		flagblock += BLOCKSIZE;
	}

	//and the last partial block, don't worry about updating the offsets
	if (sdata->partial_block_b > 0)
	{
		for (i=startprime;i<sdata->pboundi;i++)
		{
			prime = sdata->sieve_p[i];
			for (j=ddata->offsets[i];j<sdata->partial_block_b;j+=prime)
				flagblock[j>>3] &= masks[j&7];
		}
	}
#endif

	return;
}


void get_offsets16(thread_soedata_t *thread_data)
{
	//extract stuff from the thread data structure
	soe_dynamicdata_t *ddata = &thread_data->ddata;
	soe_staticdata_t *sdata = &thread_data->sdata;

	uint32 i,startprime = sdata->startprime, prodN = sdata->prodN, block=0;
	uint16 prime;
	uint32 tmp;
	uint64 tmp2;
	int r,s;

	for (i=startprime;i<sdata->pboundi;i++)
	{
		prime = sdata->sieve_p[i];
		//find the first multiple of the prime which is greater than 'block1' and equal
		//to the residue class mod 'prodN'.  

		//if the prime is greater than the limit at which it is necessary to sieve
		//a block, start that prime in the next block.
		if (sdata->sieve_p[i] > ddata->blk_b_sqrt)
		{
			sdata->pbounds[block] = i;
			block++;
			ddata->lblk_b = ddata->ublk_b + prodN;
			ddata->ublk_b += sdata->blk_r;
			ddata->blk_b_sqrt = (uint32)(sqrt(ddata->ublk_b + prodN)) + 1;
		}

		//solving the congruence: rclass[current_line] == kp mod prodN for k
		//eGCD gives r and s such that r*p + s*prodN = gcd(p,prodN).
		//then k = r*class/gcd(p,prodN) is a solution.
		//the gcd of p and prodN is always 1 by construction of prodN and choice of p.  
		//therefore k = r * class is a solution.  furthermore, since the gcd is 1, there
		//is only one solution.
		xGCD_1(prime,prodN,&r,&s,&tmp);

		if (s < 0)
			tmp2 = (uint64)(-1 * s) * (uint64)ddata->lblk_b;
		else
			tmp2 = (uint64)(prime - s) * (uint64)ddata->lblk_b;

		ddata->offsets[i] = (uint16)(tmp2 % (uint64)prime);
	}

	return;
}

void get_offsets32(thread_soedata64_t *thread_data)
{
	//extract stuff from the thread data structure
	soe_dynamicdata64_t *ddata = &thread_data->ddata;
	soe_staticdata64_t *sdata = &thread_data->sdata;

	uint64 i,startprime = sdata->startprime, prodN = sdata->prodN, block=0;
	uint32 prime;
	int tmp;
	uint64 tmp2;
	int r,s;

	for (i=startprime;i<sdata->pboundi;i++)
	{
		prime = sdata->sieve_p[i];
		//find the first multiple of the prime which is greater than 'block1' and equal
		//to the residue class mod 'prodN'.  

		//if the prime is greater than the limit at which it is necessary to sieve
		//a block, start that prime in the next block.
		if (sdata->sieve_p[i] > ddata->blk_b_sqrt)
		{
			sdata->pbounds[block] = i;
			block++;
			ddata->lblk_b = ddata->ublk_b + prodN;
			ddata->ublk_b += sdata->blk_r;
			ddata->blk_b_sqrt = (uint64)(sqrt((int64)(ddata->ublk_b + prodN))) + 1;
		}

		//solving the congruence: rclass[current_line] == kp mod prodN for k
		//eGCD gives r and s such that r*p + s*prodN = gcd(p,prodN).
		//then k = r*class/gcd(p,prodN) is a solution.
		//the gcd of p and prodN is always 1 by construction of prodN and choice of p.  
		//therefore k = r * class is a solution.  furthermore, since the gcd is 1, there
		//is only one solution.
		xGCD_1((int)prime,(int)prodN,&r,&s,&tmp);

		if (s < 0)
			tmp2 = (uint64)(-1 * s) * (uint64)ddata->lblk_b;
		else
			tmp2 = (uint64)(prime - s) * (uint64)ddata->lblk_b;

		ddata->offsets[i] = (uint32)(tmp2 % (uint64)prime);
	}

	return;
}

int tiny_soe(uint32 limit, uint16 *primes)
{
	//simple sieve of erathosthenes for small limits - not efficient
	//for large limits.
	uint8 *flags;
	uint16 prime;
	uint32 i,j;
	int it;

	//allocate flags
	flags = (uint8 *)malloc(limit/2 * sizeof(uint8));
	if (flags == NULL)
		printf("error allocating flags\n");
	memset(flags,1,limit/2);

	//find the sieving primes, don't bother with offsets, we'll need to find those
	//separately for each line in the main sieve.
	primes[0] = 2;
	it=1;
	
	//sieve using primes less than the sqrt of limit
	//flags are created only for odd numbers (mod2)
	for (i=1;i<(uint32)(sqrt(limit)/2+1);i++)
	{
		if (flags[i] > 0)
		{
			prime = (uint16)(2*i + 1);
			for (j=i+prime;j<limit/2;j+=prime)
				flags[j]=0;

			primes[it]=prime;
			it++;
		}
	}

	//now find all the prime flags and compute the sieving primes
	//the last few will exceed uint16, we can fix this later.
	for (;i<limit/2;i++)
	{
		if (flags[i] == 1)
		{
			primes[it] = (uint16)(2*i + 1);
			it++;
		}
	}

	free(flags);
	return it;
}

int tiny_soe32(uint32 limit, uint32 *primes)
{
	//simple sieve of erathosthenes for small limits - not efficient
	//for large limits.
	uint8 *flags;
	uint32 prime;
	uint32 i,j;
	int it;

	//allocate flags
	flags = (uint8 *)malloc(limit/2 * sizeof(uint8));
	if (flags == NULL)
		printf("error allocating flags\n");
	memset(flags,1,limit/2);

	//find the sieving primes, don't bother with offsets, we'll need to find those
	//separately for each line in the main sieve.
	primes[0] = 2;
	it=1;
	
	//sieve using primes less than the sqrt of block1
	//flags are created only for odd numbers (mod2)
	for (i=1;i<(uint32)(sqrt(limit)/2+1);i++)
	{
		if (flags[i] > 0)
		{
			prime = (uint32)(2*i + 1);
			for (j=i+prime;j<limit/2;j+=prime)
				flags[j]=0;

			primes[it]=prime;
			it++;
		}
	}

	//now find all the prime flags and compute the sieving primes
	//the last few will exceed uint16, we can fix this later.
	for (;i<limit/2;i++)
	{
		if (flags[i] == 1)
		{
			primes[it] = (uint32)(2*i + 1);
			it++;
		}
	}

	free(flags);
	return it;
}

void GetPRIMESRange(uint32 lowlimit, uint32 highlimit)
{
	int i;
	
	//reallocate array based on conservative estimate of the number of 
	//primes in the interval
	i = (int)(highlimit/log(highlimit)*1.2);
	if (lowlimit != 0)
		i -= (int)(lowlimit/log(lowlimit));

	PRIMES = (uint32 *)realloc(PRIMES,(size_t) (i*sizeof(uint32)));
	if (PRIMES == NULL)
	{
		printf("unable to allocate %u bytes\n",(uint32)(i*sizeof(uint32)));
		exit(1);
	}

	//find the primes in the interval
	NUM_P = spSOE(PRIMES,lowlimit,&highlimit,0);

	//as it exists now, the sieve may return primes less than lowlimit.  cut these out
	/*
	for (i=0;i<NUM_P;i++)
	{
		if (PRIMES[i] > lowlimit)
			break;
	}
	PRIMES = (uint32 *)realloc(&PRIMES[i],(NUM_P - i) * sizeof(uint32));
	NUM_P -= i;
	*/

	//reset the global constants
	P_MIN = lowlimit;
	P_MAX = highlimit; //PRIMES[NUM_P - 1];

	return;
}

uint32 soe_wrapper(uint32 lowlimit, uint32 highlimit, int count)
{
	//public interface to the sieve.  necessary because in order to keep the 
	//sieve efficient it must sieve larger blocks of numbers than a user may want,
	//and because the program keeps a cache of primes on hand which may or may 
	//not contain the range of interest.  Manage this on-hand cache and any addition
	//sieving needed.
	uint32 retval, tmpl, tmph,i=0;

	//in hex, or mingw complains
	if ((lowlimit > 0xFFFFFEFF) || (highlimit > 0xFFFFFEFF))
	{
		printf("sieve limit of 4294967039\n");
		return 0;
	}

	if (highlimit < lowlimit)
	{
		printf("error: lowlimit must be less than highlimit\n");
		return 0;
	}

	if (count)
	{
		if (lowlimit < P_MIN || lowlimit > P_MAX || highlimit > P_MAX)
		{
			//requested range is not covered by the current range
			tmpl = lowlimit;
			tmph = highlimit;

			//this needs to be a range of at least 1e6
			if ((tmph - tmpl) < 1000000)
			{
				//check that increasing the range doesn't exceed the sieve 
				//limit
				if ((4294967039 - 1000000) < tmpl)
				{
					//it does, so adjust the lowlimit down rather than the 
					//highlimit up
					tmpl = 4294967039 - 1000000;
					tmph = 4294967039;
				}
				else
				{
					tmph = tmpl + 1000000;
				}

				//since this is a small range, we need to explicitly
				//find the primes and count them.
				GetPRIMESRange(tmpl,tmph);
				retval = 0;
				for (i = 0; i < NUM_P; i++)
				{
					if (PRIMES[i] >= lowlimit && PRIMES[i] <= highlimit)
						retval++;
				}
			}
			else
			{
				//we don't need to mess with the requested range,
				//so spSOE will return the requested count directly
				retval = spSOE(NULL,lowlimit,&highlimit,1);
			}

		}
		else
		{
			// the requested range is covered by the current range
			// just count them
			retval = 0;
			for (i = 0; i < NUM_P; i++)
			{
				if (PRIMES[i] >= lowlimit && PRIMES[i] <= highlimit)
					retval++;
			}
		}
	}
	else
	{
		if (lowlimit < P_MIN || lowlimit > P_MAX || highlimit > P_MAX)
		{
			//requested range is not covered by the current range
			tmpl = lowlimit;
			tmph = highlimit;

			//this needs to be a range of at least 1e6
			if (tmph - tmpl < 1000000)
			{
				//check that increasing the range doesn't exceed the sieve 
				//limit
				if (4294967039 - 1000000 < tmpl)
				{
					//it does, so adjust the lowlimit down rather than the 
					//highlimit up
					tmpl = 4294967039 - 1000000;
					tmph = 4294967039;
				}
				else
				{
					tmph = tmpl + 1000000;
				}

				//since this is a small range, we need to 
				//find a bigger range and count them.
				GetPRIMESRange(tmpl,tmph);
				retval = 0;
				for (i = 0; i < NUM_P; i++)
				{
					if (PRIMES[i] >= lowlimit && PRIMES[i] <= highlimit)
						retval++;
				}
			}
			else
			{
				//we don't need to mess with the requested range,
				//so GetPRIMESRange will return the requested range directly
				//and the count will be in NUM_P
				GetPRIMESRange(lowlimit,highlimit);
				retval = NUM_P;
			}
		}
		else
		{
			// the requested range is covered by the current range
			// just count them
			retval = 0;
			for (i = 0; i < NUM_P; i++)
			{
				if (PRIMES[i] >= lowlimit && PRIMES[i] <= highlimit)
					retval++;
			}
		}

		// now dump the requested range of primes to a file, or the
		// screen, both, or neither, depending on the state of a couple
		// global configuration variables
		if (PRIMES_TO_FILE)
		{
			FILE *out;
			out = fopen("primes.dat","w");
			if (out == NULL)
			{
				printf("can't open primes.dat for writing\n");
			}
			else
			{
				for (i = 0; i < NUM_P; i++)
				{
					if (PRIMES[i] >= lowlimit && PRIMES[i] <= highlimit)
						fprintf(out,"%u\n",PRIMES[i]);
				}
				fclose(out);
			}
		}

		if (PRIMES_TO_SCREEN)
		{
			for (i = 0; i < NUM_P; i++)
			{
				if (PRIMES[i] >= lowlimit && PRIMES[i] <= highlimit)
					printf("%u ",PRIMES[i]);
			}
			printf("\n");
		}

			
	}

	return retval;
}

#ifdef NOTDEF
int sieve_to_bitdepth(z *startn, z *stopn, int depth, uint32 *offsets)
{
	//for now, just use a fixed depth of 2^16

	//sieve an interval of numbers from start to stop, using
	//all primes less than 2^depth
	
	//masks for removing single bits in a byte
	const uint8 masks[8] = {0x7F, 0xBF, 0xDF, 0xEF, 0xF7, 0xFB, 0xFD, 0xFE};
	//define bits per byte
	const uint32 bitsNbyte=8;
	//masks for selecting single bits in a byte
	uint8 nmasks[8];
	
	//variables used for the wheel
	uint32 numclasses,prodN,startprime,lcount;

	//variables for blocking up the line structures
	uint32 numflags, numbytes, numlinebytes; //,lineblocks,partial_limit;

	//misc
	uint32 i,j,k,it,sp;
	uint16 prime;
	uint8 *flagblock;
	z lowlimit;
	z highlimit, w1;
	uint32 correction,offsettemp;
	uint32 range;

	//sieving structures
	soe_sieve16_t sieve16;

	soe_t soe;

	//timing variables
	clock_t start, stop;
	double t;
	//clock_t start2, stop2;
	double t2;

	start = clock();

	zInit(&w1);
	zInit(&lowlimit);
	zInit(&highlimit);
	numclasses=8;
	prodN=30;
	startprime=3;

	//allocate the residue classes.  
	soe.rclass = (uint8 *)malloc(numclasses * sizeof(uint8));

	//create the selection masks
	for (i=0;i<bitsNbyte;i++)
		nmasks[i] = ~masks[i];
	
	//we'll need to store the offset into the next block for each prime
	sieve16.offsets = (uint16 *)malloc(6542 * sizeof(uint16));
	if (sieve16.offsets == NULL)
		printf("error allocating offsets\n");

	//store the primes used to sieve the rest of the flags
	//with the max sieve range set by the size of uint32, the number of primes
	//needed is fixed.
	sieve16.sieve_p = (uint16 *)malloc(6542 * sizeof(uint16));
	if (sieve16.sieve_p == NULL)
		printf("error allocating sieve_p\n");

	//use these primes
	//GetPRIMESRange(0,10000000);
	
	//get primes to sieve with
	sp = tiny_soe(65536,sieve16.sieve_p);
	soe.pboundi = 6542;

	//find the residue classes
	k=0;
	for (i=1;i<prodN;i++)
	{
		if (spGCD(i,prodN) == 1)
		{
			soe.rclass[k] = (uint8)i;
			k++;
		}
	}
	
	//temporarily set lowlimit to the first multiple of numclasses*prodN < lowlimit
	zShortDiv(startn,numclasses*prodN,&lowlimit);
	zShortMul(&lowlimit,numclasses*prodN,&lowlimit);
	zSub(startn,&lowlimit,&w1);
	correction = w1.val[0];
	//lowlimit = (lowlimit/(numclasses*prodN))*(numclasses*prodN);

	//reallocate flag structure for wheel and block sieving
	//starting at lowlimit, we need a flag for every 'numresidues' numbers out of 'prodN' up to 
	//limit.  round limit up to make this a whole number.
	//numflags = (*highlimit - lowlimit)/prodN;
	zSub(stopn,&lowlimit,&w1);
	if (w1.size > 1)
	{
		printf("interval too large\n");
		zFree(&w1);
		zFree(&lowlimit);
		zFree(&highlimit);
		return 0;
	}
	range = w1.val[0];
	numflags = range/prodN;
	numflags += ((numflags % prodN) != 0);
	numflags *= numclasses;

	//since we can pack 8 flags in a byte, we need numflags/8 bytes allocated.
	numbytes = numflags / bitsNbyte + ((numflags % bitsNbyte) != 0);

	//since there are 8 lines to sieve over, each line will contain (numflags/8)/8 bytes
	//so round numflags/8 up to the nearest multiple of 8
	numlinebytes = numbytes/numclasses + ((numbytes % numclasses) != 0);
	k = (uint32)((uint64)numlinebytes * (uint64)prodN * (uint64)bitsNbyte);
	zShortAdd(&lowlimit,k,&highlimit);

	start = clock();
	//we will sieve over this many bytes of flags, for each line, block by block.
	//allocate the lines
	soe.line = (uint8 **)malloc(numclasses * sizeof(uint8 *));
	for (i=0;i<numclasses;i++)
	{
		soe.line[i] = (uint8 *)malloc(numlinebytes * sizeof(uint8));
		memset(soe.line[i],255,numlinebytes);
	}

	//a block consists of 32768 bytes of flags
	//which holds 262144 flags.
	sieve16.blocks = numlinebytes/BLOCKSIZE;
	sieve16.partial_block_b = (numlinebytes % BLOCKSIZE)*bitsNbyte;

	if (0)
	{	
		printf("found %d residue classes\n",numclasses);
		printf("numlinebytes = %d\n",numlinebytes);
		printf("lineblocks = %d\n",sieve16.blocks);
		printf("partial limit = %d\n",sieve16.partial_block_b);
	}
	
	sieve16.blk_r = FLAGSIZE*prodN;
	it=0;

	stop = clock();
	t = (double)(stop - start)/(double)CLOCKS_PER_SEC;
	if (0)
		printf("elapsed time for init = %6.5f\n",t);
	
	t=t2=0;
	//main sieve, line by line
	start = clock();
	for (lcount=0;lcount<numclasses;lcount++)
	{
		//for the current line, find the offsets past the low limit
		for (i=startprime;i<soe.pboundi;i++)
		{
			prime = sieve16.sieve_p[i];
			//find the first multiple of the prime which is greater than 'block1' and equal
			//to the residue class mod 'prodN'.  

			//solving the congruence: rclass[lcount] == kp mod prodN for k
			//use the eGCD?

			//tmp = sieve16.lblk_b/prime;
			//tmp *= prime;
			zShortDiv(&lowlimit,prime,&w1);
			zShortMul(&w1,prime,&w1);
			do
			{
				//tmp += prime;
				zShortAdd(&w1,prime,&w1);
			} while (zShortMod(&w1,prodN) != soe.rclass[lcount]); 
			//while ((tmp % prodN) != soe.rclass[lcount]);

			//now find out how much bigger this is than 'lowlimit'
			//in steps of 'prodN'.  this is exactly the offset in flags.
			zSub(&w1,&lowlimit,&w1);
			zShortDiv(&w1,prodN,&w1);
			//sieve16.offsets[i] = (uint16)((tmp - sieve16.lblk_b)/prodN);
			sieve16.offsets[i] = (uint16)(w1.val[0]);
		}

		//now we have primes and offsets for every prime, for this line.
		//proceed to sieve the entire flag set, block by block, for this line	
		flagblock = soe.line[lcount];
		for (i=0;i<sieve16.blocks;i++)
		{
			//sieve the block with each prime
			for (j=startprime;j<soe.pboundi;j++)
			{
				prime = sieve16.sieve_p[j];
				for (k=sieve16.offsets[j];k<FLAGSIZE;k+=prime)
					flagblock[k>>3] &= masks[k&7];

				sieve16.offsets[j]= (uint16)(k - FLAGSIZE);
			}

			flagblock += BLOCKSIZE;
		}

		//and the last partial block, don't worry about updating the offsets
		if (sieve16.partial_block_b > 0)
		{
			for (i=startprime;i<soe.pboundi;i++)
			{
				prime = sieve16.sieve_p[i];
				for (j=sieve16.offsets[i];j<sieve16.partial_block_b;j+=prime)
					flagblock[j>>3] &= masks[j&7];
			}
		}
	}
	stop = clock();
	t += (double)(stop - start)/(double)CLOCKS_PER_SEC;

	//now we have a block of flags, not all of which will be prime depending
	//on the bitdepth and magnitude of the range being sieved.
	
	//rearrange the sieve lines so they are in order with respect to the original
	//starting value requested, and return a single array of offsets from start.

	it=0;
	for (j=0;j<numclasses;j++)
	{
		flagblock = soe.line[j];
		//offsets will not be ordered, but qsort could fix this if necessary
		//for a nominal fee.
		for (k=0; k<numlinebytes;k++)
		{
			//find out how many 30 counts this is away from the beginning (bits)
			//add the residue class
			//subtract the correction
			//result is the offset from start
			if ((flagblock[k] & nmasks[0]) >> 7)
			{
				offsettemp = prodN*(k*bitsNbyte) + soe.rclass[j];
				if (offsettemp > correction)
					offsets[it++] = offsettemp - correction;
			}
			if ((flagblock[k] & nmasks[1]) >> 6)
			{
				offsettemp = prodN*(k*bitsNbyte + 1) + soe.rclass[j];
				if (offsettemp > correction)
					offsets[it++] = offsettemp - correction;
			}
			if ((flagblock[k] & nmasks[2]) >> 5)
			{
				offsettemp = prodN*(k*bitsNbyte + 2) + soe.rclass[j];
				if (offsettemp > correction)
					offsets[it++] = offsettemp - correction;
			}
			if ((flagblock[k] & nmasks[3]) >> 4)
			{
				offsettemp = prodN*(k*bitsNbyte + 3) + soe.rclass[j];
				if (offsettemp > correction)
					offsets[it++] = offsettemp - correction;
			}
			if ((flagblock[k] & nmasks[4]) >> 3)
			{
				offsettemp = prodN*(k*bitsNbyte + 4) + soe.rclass[j];
				if (offsettemp > correction)
					offsets[it++] = offsettemp - correction;
			}
			if ((flagblock[k] & nmasks[5]) >> 2)
			{
				offsettemp = prodN*(k*bitsNbyte + 5) + soe.rclass[j];
				if (offsettemp > correction)
					offsets[it++] = offsettemp - correction;
			}
			if ((flagblock[k] & nmasks[6]) >> 1)
			{
				offsettemp = prodN*(k*bitsNbyte + 6) + soe.rclass[j];
				if (offsettemp > correction)
					offsets[it++] = offsettemp - correction;
			}
			if ((flagblock[k] & nmasks[7]))
			{
				offsettemp = prodN*(k*bitsNbyte + 7) + soe.rclass[j];
				if (offsettemp > correction)
					offsets[it++] = offsettemp - correction;
			}
		}
	}

	//printf("found %d primes\n",it);
	//offsets = (uint32 *)realloc(offsets,it*sizeof(uint32));

	zFree(&w1);
	zFree(&lowlimit);
	zFree(&highlimit);
	return it;
}
#endif

