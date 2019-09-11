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
#include "arith.h"
#include "util.h"
#include "qs.h"
#include "factor.h"

void test_dlp_composites()
{
	FILE *in;
	uint64 *comp;
	uint32 *f1;
	uint32 *f2, totBits,minBits,maxBits;
	double t_time;
	clock_t start, stop;
	int i,j,num,correct;
	z tmp,tmp2,t1,t2,t3;
	uint64 f64;
	int64 queue[100];
	fact_obj_t *fobj;

	comp = (uint64 *)malloc(2000000 * sizeof(uint64));
	f1 = (uint32 *)malloc(2000000 * sizeof(uint32));
	f2 = (uint32 *)malloc(2000000 * sizeof(uint32));
	zInit(&tmp);
	zInit(&tmp2);
	zInit(&t2);
	zInit(&t1);
	zInit(&t3);

	in = fopen("pseudoprimes.dat","r");

	start = clock();
	i=0;
	totBits = 0;
	minBits = 999;
	maxBits = 0;
	//read in everything
	while (!feof(in))
	{
#if BITS_PER_DIGIT == 64
		fscanf(in,"%lu,%u,%u",comp+i,f1+i,f2+i);
#else
#ifdef _WIN32
		fscanf(in,"%I64u,%u,%u",comp+i,f1+i,f2+i);
#else
		fscanf(in,"%llu,%u,%u",comp+i,f1+i,f2+i);
#endif
#endif
		sp642z(comp[i],&tmp);
		j = zBits(&tmp);
		totBits += j;
		if ((uint32)j > maxBits)
			maxBits = j;
		if ((uint32)j < minBits && j != 0)
			minBits = j;
		i++;
	}
	num = i;
	fclose(in);
	stop = clock();
	t_time = (double)(stop - start)/(double)CLOCKS_PER_SEC;
	printf("data read in %2.4f sec\n",t_time);
	printf("average bits of input numbers = %.2f\n",(double)totBits/(double)i);
	printf("minimum bits of input numbers = %d\n",minBits);
	printf("maximum bits of input numbers = %d\n",maxBits);


	
	start = clock();

	correct = 0;
	for (i=0;i<num;i++)
	{
		
		if (i%1000 == 0)
		{
			printf("done with %d, %d correct, after %2.2f sec\n",i,
				correct,(double)(clock() - start)/(double)CLOCKS_PER_SEC);
		}
		sp642z(comp[i],&tmp);
		//smallmpqs(&tmp,&tmp2,&t2,&t3);
		//f64 = tmp2.val[0];

		//f64 = sp_shanks_loop(&tmp);
		f64 = (uint64)squfof_jp(&tmp);

		if ( ((uint32)f64 == f1[i]) || ((uint32)f64 == f2[i]))
			correct++;
	}

	stop = clock();
	t_time = (double)(stop - start)/(double)CLOCKS_PER_SEC;
	printf("jason's squfof got %d of %d correct in %2.2f sec\n",correct,num,t_time);
	printf("percent correct = %.2f\n",100.0*(double)correct/(double)num);
	printf("average time per input = %.2f ms\n",1000*t_time/(double)num);

	start = clock();

	correct = 0;
	for (i=0;i<num;i++)
	{
		if (i%1000 == 0)
		{
			printf("done with %d, %d correct, after %2.2f sec\n",i,
				correct,(double)(clock() - start)/(double)CLOCKS_PER_SEC);
		}
		sp642z(comp[i],&tmp);
		//smallmpqs(&tmp,&tmp2,&t2,&t3);
		//f64 = tmp2.val[0];

		f64 = sp_shanks_loop(&tmp,NULL);
		//f64 = (uint64)squfof_jp(&tmp);
		
		
		//VFLAG = 0;
		//BRENT_MAX_IT = 257;
		//mbrent(&tmp,1,&tmp2);
		//f64 = tmp2.val[0];
		//
		//POLLARD_STG1_MAX = 1000;
		//POLLARD_STG2_MAX = 100000;
		//mpollard(&tmp,2,&tmp2);
		//f64 = tmp2.val[0];
		

		if ( ((uint32)f64 == f1[i]) || ((uint32)f64 == f2[i]))
			correct++;
	}

	stop = clock();
	t_time = (double)(stop - start)/(double)CLOCKS_PER_SEC;
	printf("squfof got %d of %d correct in %2.2f sec\n",correct,num,t_time);
	printf("percent correct = %.2f\n",100.0*(double)correct/(double)num);
	printf("average time per input = %.2f ms\n",1000*t_time/(double)num);


	start = clock();

	correct = 0;
	for (i=0;i<num;i++)
	{

		if (i%1000 == 0)
		{
			printf("done with %d, %d correct, after %2.2f sec\n",i,
				correct,(double)(clock() - start)/(double)CLOCKS_PER_SEC);
		}
		sp642z(comp[i],&tmp);
		f64 = (uint64)SQUFOF_alpertron((int64)z264(&tmp),queue);

		if ( ((uint32)f64 == f1[i]) || ((uint32)f64 == f2[i]))
			correct++;
	}

	stop = clock();
	t_time = (double)(stop - start)/(double)CLOCKS_PER_SEC;
	printf("dario's squfof got %d of %d correct in %2.2f sec\n",correct,num,t_time);
	printf("percent correct = %.2f\n",100.0*(double)correct/(double)num);
	printf("average time per input = %.2f ms\n",1000*t_time/(double)num);

	start = clock();

	correct = 0;
	for (i=0;i<num;i++)
	{
		int fact1, fact2;
		if (i%1000 == 0)
		{
			printf("done with %d, %d correct, after %2.2f sec\n",i,
				correct,(double)(clock() - start)/(double)CLOCKS_PER_SEC);
		}
		sp642z(comp[i],&tmp);
		////sp642z(2128691,&tmp);
		squfof_rds((int64)z264(&tmp),&fact1, &fact2);
		//printf("%lu = %u * %u\n",z264(&tmp), fact1, fact2);


		if ( ((uint32)fact1 == f1[i]) || ((uint32)fact1 == f2[i]))
			correct++;
	}

	stop = clock();
	t_time = (double)(stop - start)/(double)CLOCKS_PER_SEC;
	printf("rds's squfof got %d of %d correct in %2.2f sec\n",correct,num,t_time);
	printf("percent correct = %.2f\n",100.0*(double)correct/(double)num);
	printf("average time per input = %.2f ms\n",1000*t_time/(double)num);	

	


	//fobj = (fact_obj_t *)malloc(sizeof(fact_obj_t));
	//init_factobj(fobj);


	//start = clock();

	//correct = 0;
	//num=10000;
	//for (i=0;i<num;i++)
	//{
	//	if (i%1000 == 0)
	//	{
	//		printf("done with %d, %d correct, after %2.2f sec\n",i,
	//			correct,(double)(clock() - start)/(double)CLOCKS_PER_SEC);
	//	}
	//	sp642z(comp[i],&tmp);
	//	zCopy(&tmp,&fobj->N);
	//	fobj->qs_obj.flags = 12345;
	//	pQS(fobj);

	//	for (j=0; j<fobj->qs_obj.num_factors; j++)
	//	{
	//		uint32 fact = (uint32)fobj->qs_obj.factors[j].val[0];
	//		if ( (fact == f1[i]) || (fact == f2[i]))
	//		{
	//			correct++;
	//			break;
	//		}
	//		zFree(&fobj->qs_obj.factors[j]);
	//	}
	//	for (; j < fobj->qs_obj.num_factors; j++)
	//		zFree(&fobj->qs_obj.factors[j]);

	//}
	//fobj->qs_obj.num_factors = 0;
	//free_factobj(fobj);

	//stop = clock();
	//t_time = (double)(stop - start)/(double)CLOCKS_PER_SEC;
	//printf("squfof got %d of %d correct in %2.2f sec\n",correct,num,t_time);
	//printf("percent correct = %.2f\n",100.0*(double)correct/(double)num);
	//printf("average time per input = %.2f ms\n",1000*t_time/(double)num);	


	free(f1);
	free(f2);
	free(comp);
	zFree(&tmp);
	zFree(&tmp2);
	zFree(&t2);
	zFree(&t1);
	zFree(&t3);
	return;
}


void modtest(int it)
{
	z res;
	z32 res32;
	double t_time;
	clock_t start, stop;
	int i;
	fp_digit p;
	//test the speed of zMod1 for 64 and 32 bit bases

	//a 300 bit factorization has residues ~ 150 bits
	//which is 3 64 bit words and 5 32 bit words

	zInit(&res);
	zInit32(&res32);
	zRandb(&res,125);
	z64_to_z32(&res,&res32);

	start = clock();
	srand(123);
	for (i=0; i < 100000000; i++)
	{
		p = spRand(1000000,10000000);
		zShortMod(&res,p);
	}
	stop = clock();
	t_time = (double)(stop - start)/(double)CLOCKS_PER_SEC;
	printf("elapsed time for 64 bit mods: %2.2f sec\n",t_time);


	start = clock();
	srand(123);
	for (i=0; i < 100000000; i++)
	{
		p = spRand(1000000,10000000);
		zShortMod32(&res32,p);
	}
	stop = clock();
	t_time = (double)(stop - start)/(double)CLOCKS_PER_SEC;
	printf("elapsed time for 32 bit mods: %2.2f sec\n",t_time);

	return;
}

void test_qsort(void)
{
	//test the speed of qsort in  sorting a few million lists
	//of a few thousand random integers

	uint32 *list;
	int i,j,k;
	const int listsz = 512;
	double t_time;
	clock_t start, stop;

	list = (uint32 *)malloc(listsz * sizeof(uint32));

	for (k=1000;k<1000000;k*=10)
	{
		start = clock();

		for (j=0; j<k; j++)
		{
			for (i=0; i<listsz; i++)
				list[i] = (uint32)spRand(1000000,50000000);
		}

		stop = clock();
		t_time = (double)(stop - start)/(double)CLOCKS_PER_SEC;
		printf("baseline elapsed time for %d lists = %2.2f\n",k,t_time);

		start = clock();

		for (j=0; j<k; j++)
		{
			for (i=0; i<listsz; i++)
				list[i] = (uint32)spRand(1000000,50000000);
			qsort(list,listsz,4,&qcomp_uint32);
		}

		stop = clock();
		t_time = (double)(stop - start)/(double)CLOCKS_PER_SEC;
		printf("elapsed time for %d sorts = %2.2f\n",k,t_time);
	}

	free(list);
	return;
}
	
void arith_timing()
{

	int i,sz,num=1000000;
	double t_time;
	clock_t start, stop;
	z a,b,c,d,e;

	zInit(&a);
	zInit(&b);
	zInit(&c);
	zInit(&d);
	zInit(&e);
	
	for (sz=50; sz<=500; sz += 50)
	{
		printf("baseline: generating %d random numbers with %d digits: ",num,sz);
		start = clock();
		for (i=0;i<num;i++)
		{
			//zRand(&a,sz);
			//zRand(&b,sz);
		}
		stop = clock();
		t_time = (double)(stop - start)/(double)CLOCKS_PER_SEC;
		printf("elapsed time = %2.3f\n",t_time);
	}

	for (sz=50; sz<=500; sz += 50)
	{
		zRand(&a,sz);
		zRand(&b,sz);
		printf("adding %d random numbers with %d digits: ",num,sz);
		start = clock();
		for (i=0;i<num;i++)
		{
			//zRand(&a,sz);
			//zRand(&b,sz);
			zAdd(&a,&b,&c);
		}
		stop = clock();
		t_time = (double)(stop - start)/(double)CLOCKS_PER_SEC;
		printf("elapsed time = %2.3f\n",t_time);
	}

	for (sz=50; sz<=500; sz += 50)
	{
		zRand(&a,sz);
		zRand(&b,sz);
		printf("subtracting %d random numbers with %d digits: ",num,sz);
		start = clock();
		for (i=0;i<num;i++)
		{
			//zRand(&a,sz);
			//zRand(&b,sz);
			zSub(&a,&b,&c);
		}
		stop = clock();
		t_time = (double)(stop - start)/(double)CLOCKS_PER_SEC;
		printf("elapsed time = %2.3f\n",t_time);
	}

	for (sz=50; sz<=500; sz += 50)
	{
		zRand(&a,sz);
		zRand(&b,sz);
		printf("Multiplying %d random numbers with %d digits: ",num,sz);
		start = clock();
		for (i=0;i<num;i++)
		{
			//zRand(&a,sz);
			//zRand(&b,sz);
			zMul(&a,&b,&c);
		}
		stop = clock();
		t_time = (double)(stop - start)/(double)CLOCKS_PER_SEC;
		printf("elapsed time = %2.3f\n",t_time);
	}

	for (sz=50; sz<=500; sz += 50)
	{
		zRand(&a,sz);
		zRand(&b,sz);
		printf("Squaring %d random numbers with %d digits: ",num,sz);
		start = clock();
		for (i=0;i<num;i++)
		{
			//zRand(&a,sz);
			//zRand(&b,sz);
			zSqr(&a,&c);
		}
		stop = clock();
		t_time = (double)(stop - start)/(double)CLOCKS_PER_SEC;
		printf("elapsed time = %2.3f\n",t_time);
	}

	for (sz=50; sz<=500; sz += 50)
	{
		zRand(&a,sz);
		zRand(&b,sz/2);
		printf("Dividing %d random numbers with %d digits: ",num,sz);
		start = clock();
		for (i=0;i<num;i++)
		{
			//zRand(&a,sz);
			//zRand(&b,sz/2);
			zCopy(&a,&e);
			zDiv(&a,&b,&c,&d);
			zCopy(&e,&a);
		}
		stop = clock();
		t_time = (double)(stop - start)/(double)CLOCKS_PER_SEC;
		printf("elapsed time = %2.3f\n",t_time);
	}

	zFree(&a);
	zFree(&b);
	zFree(&c);
	zFree(&d);
	zFree(&e);

	return;
}
