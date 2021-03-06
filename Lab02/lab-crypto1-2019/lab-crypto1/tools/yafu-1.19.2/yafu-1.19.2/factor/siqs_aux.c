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

#include "yafu.h"
#include "qs.h"
#include "soe.h"
#include "util.h"

uint32 make_fb_siqs(static_conf_t *sconf)
{
	//finds the factor base primes, and computes the solutions to the congruence x^2 = N mod p
	//for the QS, these are the starting positions of the sieve relative to the sqrt of N.
	//for the MPQS, additional work using the polynomial coefficents and these congruences 
	//needs to be done to compute the starting positions of the sieve.

	//locals
	int i;
	uint32 b,j,r,k;
	uint32 prime, root1, root2;
	uint8 logp;
	uint32 urange = 10000000;
	uint32 lrange = 0;
	fp_digit f;

	//unpack stuff from static data structure
	fb_list *fb = sconf->factor_base;
	z *n = &sconf->n;
	uint32 mul = sconf->multiplier;
	uint32 *modsqrt = sconf->modsqrt_array;

	GetPRIMESRange(lrange,urange);

	//the 0th and 1st elements in the fb are always 1 and 2, so start searching with 3
	j=2; i=1;
	while (j<fb->B)
	{
		if ((uint32)i > NUM_P)
		{
			lrange = urange + 1;
			urange = lrange + 10000000;
			GetPRIMESRange(lrange,urange);
			i=0;
		}

		prime = (uint32)PRIMES[i];
		r = (uint32)zShortMod(n,(fp_digit)prime);
		if (r == 0)
		{
			if (mul % prime != 0)
			{
				//prime doesn't divide the multiplier, so
				//this prime divides the input, divide it out and bail
				zShortDiv(n,(fp_digit)prime,n);
				return prime;
			}

			//p divides n, which means it divides the multiplier.
			//we can still use it, but it only has one solution to x^2 == n mod p instead
			//of two.  just divide its logprime in half.
			//we also can't find the root using shanks-tonelli, but it will be very small
			//because the multiplier is very small, so just use brute force.
			b = (uint32)zShortMod(n,prime);
			k=0;
			while (1)
			{
				if (((k*k) % prime) == b)
					break;
				k++;
			}
			root1 = k;
			root2 = prime - k;

			//compute logp
			logp = (uint8)(log((double)prime)/log(2.0) + .5)/2;

			//fill in factor base
			fb->list->prime[j] = prime;
			modsqrt[j] = root1;
			fb->list->logprime[j] = logp;

			//store a couple things so we can replace single precision
			//mods with shifts and muls during trial division
			if (prime < 256)
			{
				fb->list->small_inv[j] = (uint32)(((uint64)1 << 32) / (uint64)prime);
				if (floor(MP_RADIX / (double)prime + 0.5) ==
								(double)fb->list->small_inv[j]) {
					fb->list->correction[j] = 1;
				}
				else {
					fb->list->correction[j] = 0;
					fb->list->small_inv[j]++;
				}
			}
			else
			{
				fb->list->small_inv[j] = (uint32)(((uint64)1 << 40) / (uint64)prime);
				if (floor(256 * MP_RADIX / (double)prime + 0.5) ==
								(double)fb->list->small_inv[j]) {
					fb->list->correction[j] = 1;
				}
				else {
					fb->list->correction[j] = 0;
					fb->list->small_inv[j]++;
				}
			}

			j++;
			i++;
			continue;
		}

		b = jacobi_1((fp_digit)r,(fp_digit)prime);
		if (b==1)
		{
			//this prime works
			ShanksTonelli_1((fp_digit)r,(fp_digit)prime,&f);
			root1 = (uint32)f;
			root2 = prime - root1;

			//compute logp
			logp = (uint8)(log((double)prime)/log(2.0) + .5);

			//fill in factor base
			fb->list->prime[j] = prime;
			modsqrt[j] = root1;
			fb->list->logprime[j] = logp;

			//store a couple things so we can replace single precision
			//mods with shifts and muls during trial division
			//this is very fragile... need better range checking.
			if (prime < 256)
			{
				fb->list->small_inv[j] = (uint32)(((uint64)1 << 32) / (uint64)prime);
				if (floor(MP_RADIX / (double)prime + 0.5) ==
								(double)fb->list->small_inv[j]) {
					fb->list->correction[j] = 1;
				}
				else {
					fb->list->correction[j] = 0;
					fb->list->small_inv[j]++;
				}
			}
			else
			{
				fb->list->small_inv[j] = (uint32)(((uint64)1 << 40) / (uint64)prime);
				if (floor(256 * MP_RADIX / (double)prime + 0.5) ==
								(double)fb->list->small_inv[j]) {
					fb->list->correction[j] = 1;
				}
				else {
					fb->list->correction[j] = 0;
					fb->list->small_inv[j]++;
				}
			}

			
			j++;
		}
		i++;
	}

	return 0;
}

#define NUM_PARAM_ROWS 18
void get_params(static_conf_t *sconf)
{
	int bits,i;
	double scale;
	fb_list *fb = sconf->factor_base;

	//parameter table
	//bits, fb primes, lp mulitplier, 64k blocks
	int param_table[22][4] = {
		{140,	600,	40,	1},
		{149,	875,	40,	1},
		{165,	1228,	50,	1},
		{181,	2247,	50,	1},
		{198,	3485,	60,	2},
		{215,	6357,	60,	2},	
		{232,	12132,	70,	3},
		{248,	26379,	80,	4},
		{265,	42871,	90,	6},
		{281,	55137,	100,	8},
		{298,	65244,	120,	10},
		{310,	78247,	120,	12},
		{320,	90678,	140,	14},
		{330,	105000, 150,    18},
		{340,	125000, 150,    21},
		{350,	155000, 150,    25},
		{360,	195000, 150,    29},
		{370,	250000, 150,    34},
		{380,	310000, 150,    40},
		{390,	380000, 150,    47},
		{400,	460000, 150,    55},
		{410,	550000, 150,    64},
	};

	/*
	int param_table[NUM_PARAM_ROWS][4] = {
		{140,	600,	40,	1},
		{149,	875,	40,	1},
		{165,	1228,	50,	1},
		{181,	2247,	50,	2},
		{198,	3485,	60,	3},
		{215,	6357,	60,	4},	
		{232,	12132,	70,	6},
		{248,	26379,	80,	8},
		{265,	42871,	90,	12},
		{281,	55137,	100,	16},
		{298,	65244,	120,	20},
		{310,	78247,	120,	24},
		{320,	90678,	140,	28},
		{330,	105000, 150,    36},
		{340,	125000, 150,    42},
		{350,	155000, 150,    50},
		{360,	195000, 150,    58},
		{370,	250000, 150,    68},
		{380,	310000, 150,    80},
		{390,	380000, 150,    94},
		{400,	460000, 150,    110},
		{410,	550000, 150,    128},
	};
	
	
	int param_table[NUM_PARAM_ROWS][4] = {
		{150,	853,	40,	1},
		{175,	1865,	50,	2},
		{200,	4014,	60,	3},
		{225,	10242,	70,	6},
		{250,	29736,	85,	12},
		{275,	53064,	95,	17},	
		{298,	65244,	120,	20},
		{310,	78247,	120,	24},
		{320,	90678,	140,	28},
		{330,	105000, 150,    36},
		{340,	125000, 150,    42},
		{350,	155000, 150,    50},
		{360,	195000, 150,    58},
		{370,	250000, 150,    68},
		{380,	310000, 150,    80},
		{390,	380000, 150,    94},
		{400,	460000, 150,    110},
		{410,	550000, 150,    128},
	};
	*/


	//linear interpolation according to bit size to determine
	//factor base bound.  use the closest parameter for lp multiplier
	//and number of blocks.

	bits = sconf->obj->bits;

	fb->B = 0;
	if (bits <= param_table[0][0])
	{
		scale = (double)bits / (double)param_table[0][0];
		fb->B = (uint32)(scale * (double)(param_table[0][1]));		
		sconf->large_mult = 40;
		sconf->num_blocks = 1;
	}
	else
	{
		for (i=0;i<NUM_PARAM_ROWS;i++)
		{
			if (bits > param_table[i][0] && bits <= param_table[i+1][0])
			{
				scale = (double)(param_table[i+1][0] - bits) /
					(double)(param_table[i+1][0] - param_table[i][0]);
				fb->B = param_table[i+1][1] - 
					(uint32)(scale * (double)(param_table[i+1][1] - param_table[i][1]));
				
				//sconf->large_mult = (uint32)((double)param_table[i+1][2] - 
				//	(scale * (double)(param_table[i+1][2] - param_table[i][2])) + 0.5);
				sconf->large_mult = (uint32)((param_table[i+1][2] + param_table[i][2])/2.0 + 0.5);
				//sconf->num_blocks = (uint32)((double)param_table[i+1][3] - 
				//	(scale * (double)(param_table[i+1][3] - param_table[i][3])) + 0.5);
				sconf->num_blocks = (uint32)((param_table[i+1][3] + param_table[i][3])/2.0 + 0.5);
			}
		}
	}

	if (fb->B == 0)
	{
		//off the end of the table, extrapolate based on the slope of 
		//the last two

		scale = (double)(param_table[NUM_PARAM_ROWS-1][1] - param_table[NUM_PARAM_ROWS-2][1]) /
			(double)(param_table[NUM_PARAM_ROWS-1][0] - param_table[NUM_PARAM_ROWS-2][0]);
		fb->B = (uint32)(((double)bits - param_table[NUM_PARAM_ROWS-1][0]) * 
			scale + param_table[NUM_PARAM_ROWS-1][1]);
		sconf->large_mult = param_table[NUM_PARAM_ROWS-1][2];	//reuse last one

		scale = (double)(param_table[NUM_PARAM_ROWS-1][3] - param_table[NUM_PARAM_ROWS-2][3]) /
			(double)(param_table[NUM_PARAM_ROWS-1][0] - param_table[NUM_PARAM_ROWS-2][0]);
		sconf->num_blocks = (uint32)(((double)bits - param_table[NUM_PARAM_ROWS-1][0]) * 
			scale + param_table[NUM_PARAM_ROWS-1][3]);
	}

	if (gbl_override_B_flag)
		fb->B = gbl_override_B;

	if (gbl_override_blocks_flag)
		sconf->num_blocks = gbl_override_blocks;

	if (gbl_override_lpmult_flag)
		sconf->large_mult = gbl_override_lpmult;

	return;
}

int qcomp_siqs(const void *x, const void *y)
{
	siqs_r **xx = (siqs_r **)x;
	siqs_r **yy = (siqs_r **)y;
	
	if (xx[0]->large_prime[0] > yy[0]->large_prime[0])
		return 1;
	else if (xx[0]->large_prime[0] == yy[0]->large_prime[0])
		return 0;
	else
		return -1;
}

void set_aprime_roots(uint32 val, int *qli, int s, sieve_fb_compressed *fb)
{
	int i;
	sieve_fb_compressed *ptr;

	for (i=0;i<s;i++)
	{
		ptr = fb + qli[i];
		ptr->roots = val;
		//ptr->root2 = (uint16)val;
	}
	return;
}

void get_gray_code(siqs_poly *poly) 
{

	int i, v, j, n = poly->s;
	int tmp;

	for (i=1; i< (1 << (n-1)); i++) {
		v = 1;
		j = i;
		while ((j&1)==0)
			v++, j>>=1;
		tmp = i + (1<<v) - 1;
		tmp = (tmp>>v);
		poly->nu[i] = v;
		if (tmp&1)
			poly->gray[i] = -1;
		else
			poly->gray[i] = 1;
	}
	return;
}

uint32 factor_list_add(fact_obj_t *obj, factor_list_t *list, 
				z *new_factor) {

	uint32 i, bitsleft;
	int isnew = 1;

	//look to see if we've already included this one
	for (i=0; i<list->num_factors; i++)
		isnew &= (zCompare(&list->final_factors[i]->factor,new_factor) != 0);

	if (isnew)
	{
		logprint(obj->logfile,
			"prp%d = %s\n",ndigits(new_factor),z2decstr(new_factor,&gstr1));
		list->final_factors[list->num_factors] = (final_factor_t *)malloc(
			sizeof(final_factor_t));
		zInit(&list->final_factors[list->num_factors]->factor);
		zCopy(new_factor,&list->final_factors[list->num_factors]->factor);
		list->final_factors[list->num_factors]->type = PRIME;
		list->num_factors++;
	}

	//now determine if we are done based on the bits of factors found compared to 
	//the bits in the original n
	bitsleft = obj->bits;
	for (i=0; i<list->num_factors; i++)
		bitsleft -= zBits(&list->final_factors[i]->factor);

	return bitsleft;
}

void qs_save(char *buf, FILE *data, int force)
{
	//if the input buf can be fit into the global savebuf, do so.
	//else, dump savebuf to disk and start a new savebuf with buf

	//int len = strlen(savebuf);
	//int len_in = strlen(buf);

	if ((savefile_buf_off + strlen(buf) + 1 >= SAVEFILE_BUF_SIZE) || force) {
		fprintf(data, "%s", savebuf);
		fflush(data);
		savefile_buf_off = 0;
	}

	savefile_buf_off += sprintf(savebuf + savefile_buf_off, "%s", buf);
	
	return;
}

void siqsexit(int sig)
{
	printf("\nAborting...\n");
	SIQS_ABORT = 1;
	return;
}
