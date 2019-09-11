
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

#include "cuda_md5_gpu.h"

//
// On-device variable declarations
//

__constant__ uint target[4];		// target hash, if searching for hash matches


void init_md5_target_constants(uint *target_cpu)
{
	if(target_cpu) { cudaMemcpyToSymbol(target, target_cpu, sizeof(target)); };
}

//
// MD5 routines (straight from Wikipedia's MD5 pseudocode description)
//

//////////////////////////////////////////////////////////////////////////////
/////////////       Ron Rivest's MD5 C Implementation       //////////////////
//////////////////////////////////////////////////////////////////////////////

/*
 **********************************************************************
 ** Copyright (C) 1990, RSA Data Security, Inc. All rights reserved. **
 **                                                                  **
 ** License to copy and use this software is granted provided that   **
 ** it is identified as the "RSA Data Security, Inc. MD5 Message     **
 ** Digest Algorithm" in all material mentioning or referencing this **
 ** software or this function.                                       **
 **                                                                  **
 ** License is also granted to make and use derivative works         **
 ** provided that such works are identified as "derived from the RSA **
 ** Data Security, Inc. MD5 Message Digest Algorithm" in all         **
 ** material mentioning or referencing the derived work.             **
 **                                                                  **
 ** RSA Data Security, Inc. makes no representations concerning      **
 ** either the merchantability of this software or the suitability   **
 ** of this software for any particular purpose.  It is provided "as **
 ** is" without express or implied warranty of any kind.             **
 **                                                                  **
 ** These notices must be retained in any copies of any part of this **
 ** documentation and/or software.                                   **
 **********************************************************************
 */


/* F, G and H are basic MD5 functions: selection, majority, parity */
#define F(x, y, z) (((x) & (y)) | ((~x) & (z)))
#define G(x, y, z) (((x) & (z)) | ((y) & (~z)))
#define H(x, y, z) ((x) ^ (y) ^ (z))
#define I(x, y, z) ((y) ^ ((x) | (~z))) 

/* ROTATE_LEFT rotates x left n bits */
#define ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32-(n))))

/* FF, GG, HH, and II transformations for rounds 1, 2, 3, and 4 */
/* Rotation is separate from addition to prevent recomputation */
#define FF(a, b, c, d, x, s, ac) \
  {(a) += F ((b), (c), (d)) + (x) + (uint)(ac); \
   (a) = ROTATE_LEFT ((a), (s)); \
   (a) += (b); \
  }
#define GG(a, b, c, d, x, s, ac) \
  {(a) += G ((b), (c), (d)) + (x) + (uint)(ac); \
   (a) = ROTATE_LEFT ((a), (s)); \
   (a) += (b); \
  }
#define HH(a, b, c, d, x, s, ac) \
  {(a) += H ((b), (c), (d)) + (x) + (uint)(ac); \
   (a) = ROTATE_LEFT ((a), (s)); \
   (a) += (b); \
  }
#define II(a, b, c, d, x, s, ac) \
  {(a) += I ((b), (c), (d)) + (x) + (uint)(ac); \
   (a) = ROTATE_LEFT ((a), (s)); \
   (a) += (b); \
  }

/*
* Basic MD5 step. Transform buf based on i0 to i3 128bits in registers.
 */
void inline __device__ md5_transform(const uint4 i0,
							  uint &a, uint &b, uint &c, uint &d
							  )
{
	const uint a0 = 0x67452301;
	const uint b0 = 0xEFCDAB89;
	const uint c0 = 0x98BADCFE;
	const uint d0 = 0x10325476;

/*
#define in0  (i0.x)
#define in1  (i0.y)
#define in2  (i0.z)
#define in3  (i0.w)
#define in4  (i1.x)
#define in5  (i1.y)
#define in6  (i1.z)
#define in7  (i1.w)
#define in8  (i2.x)
#define in9  (i2.y)
#define in10 (i2.z)
#define in11 (i2.w)
#define in12 (i3.x)
#define in13 (i3.y)
#define in14 (i3.z)
#define in15 (i3.w)
*/
#define in0  (i0.x)
#define in1  (i0.y)
#define in2  (i0.z)
#define in3  (0)
#define in4  (0)
#define in5  (0)
#define in6  (0)
#define in7  (0)
#define in8  (0)
#define in9  (0)
#define in10 (0)
#define in11 (0)
#define in12 (0)
#define in13 (0)
#define in14 (i0.w)
#define in15 (0)

	//Initialize hash value for this chunk:
	a = a0;
	b = b0;
	c = c0;
	d = d0;

  /* Round 1 */
#define S11 7
#define S12 12
#define S13 17
#define S14 22
  FF ( a, b, c, d, in0,  S11, 3614090360); /* 1 */
  FF ( d, a, b, c, in1,  S12, 3905402710); /* 2 */
  FF ( c, d, a, b, in2,  S13,  606105819); /* 3 */
  FF ( b, c, d, a, in3,  S14, 3250441966); /* 4 */
  FF ( a, b, c, d, in4,  S11, 4118548399); /* 5 */
  FF ( d, a, b, c, in5,  S12, 1200080426); /* 6 */
  FF ( c, d, a, b, in6,  S13, 2821735955); /* 7 */
  FF ( b, c, d, a, in7,  S14, 4249261313); /* 8 */
  FF ( a, b, c, d, in8,  S11, 1770035416); /* 9 */
  FF ( d, a, b, c, in9,  S12, 2336552879); /* 10 */
  FF ( c, d, a, b, in10, S13, 4294925233); /* 11 */
  FF ( b, c, d, a, in11, S14, 2304563134); /* 12 */
  FF ( a, b, c, d, in12, S11, 1804603682); /* 13 */
  FF ( d, a, b, c, in13, S12, 4254626195); /* 14 */
  FF ( c, d, a, b, in14, S13, 2792965006); /* 15 */
  FF ( b, c, d, a, in15, S14, 1236535329); /* 16 */
 
  /* Round 2 */
#define S21 5
#define S22 9
#define S23 14
#define S24 20
  GG ( a, b, c, d, in1, S21, 4129170786); /* 17 */
  GG ( d, a, b, c, in6, S22, 3225465664); /* 18 */
  GG ( c, d, a, b, in11, S23,  643717713); /* 19 */
  GG ( b, c, d, a, in0, S24, 3921069994); /* 20 */
  GG ( a, b, c, d, in5, S21, 3593408605); /* 21 */
  GG ( d, a, b, c, in10, S22,   38016083); /* 22 */
  GG ( c, d, a, b, in15, S23, 3634488961); /* 23 */
  GG ( b, c, d, a, in4, S24, 3889429448); /* 24 */
  GG ( a, b, c, d, in9, S21,  568446438); /* 25 */
  GG ( d, a, b, c, in14, S22, 3275163606); /* 26 */
  GG ( c, d, a, b, in3, S23, 4107603335); /* 27 */
  GG ( b, c, d, a, in8, S24, 1163531501); /* 28 */
  GG ( a, b, c, d, in13, S21, 2850285829); /* 29 */
  GG ( d, a, b, c, in2, S22, 4243563512); /* 30 */
  GG ( c, d, a, b, in7, S23, 1735328473); /* 31 */
  GG ( b, c, d, a, in12, S24, 2368359562); /* 32 */

  /* Round 3 */
#define S31 4
#define S32 11
#define S33 16
#define S34 23
  HH ( a, b, c, d, in5, S31, 4294588738); /* 33 */
  HH ( d, a, b, c, in8, S32, 2272392833); /* 34 */
  HH ( c, d, a, b, in11, S33, 1839030562); /* 35 */
  HH ( b, c, d, a, in14, S34, 4259657740); /* 36 */
  HH ( a, b, c, d, in1, S31, 2763975236); /* 37 */
  HH ( d, a, b, c, in4, S32, 1272893353); /* 38 */
  HH ( c, d, a, b, in7, S33, 4139469664); /* 39 */
  HH ( b, c, d, a, in10, S34, 3200236656); /* 40 */
  HH ( a, b, c, d, in13, S31,  681279174); /* 41 */
  HH ( d, a, b, c, in0, S32, 3936430074); /* 42 */
  HH ( c, d, a, b, in3, S33, 3572445317); /* 43 */
  HH ( b, c, d, a, in6, S34,   76029189); /* 44 */
  HH ( a, b, c, d, in9, S31, 3654602809); /* 45 */
  HH ( d, a, b, c, in12, S32, 3873151461); /* 46 */
  HH ( c, d, a, b, in15, S33,  530742520); /* 47 */
  HH ( b, c, d, a, in2, S34, 3299628645); /* 48 */

  /* Round 4 */
#define S41 6
#define S42 10
#define S43 15
#define S44 21
  II ( a, b, c, d, in0, S41, 4096336452); /* 49 */
  II ( d, a, b, c, in7, S42, 1126891415); /* 50 */
  II ( c, d, a, b, in14, S43, 2878612391); /* 51 */
  II ( b, c, d, a, in5, S44, 4237533241); /* 52 */
  II ( a, b, c, d, in12, S41, 1700485571); /* 53 */
  II ( d, a, b, c, in3, S42, 2399980690); /* 54 */
  II ( c, d, a, b, in10, S43, 4293915773); /* 55 */
  II ( b, c, d, a, in1, S44, 2240044497); /* 56 */
  II ( a, b, c, d, in8, S41, 1873313359); /* 57 */
  II ( d, a, b, c, in15, S42, 4264355552); /* 58 */
  II ( c, d, a, b, in6, S43, 2734768916); /* 59 */
  II ( b, c, d, a, in13, S44, 1309151649); /* 60 */
  II ( a, b, c, d, in4, S41, 4149444226); /* 61 */
  II ( d, a, b, c, in11, S42, 3174756917); /* 62 */
  II ( c, d, a, b, in2, S43,  718787259); /* 63 */
  II ( b, c, d, a, in9, S44, 3951481745); /* 64 */

	a += a0;
	b += b0;
	c += c0;
	d += d0;

}

// The kernel (this is the entrypoint of GPU code)
// Loads the 64-byte word to be hashed from global to shared memory,
// calls the calculation routine, compares to target and flags if a match is found
__global__ void md5_search_bruteforce(char *gwords, uint *succ)
{

	uint4 i0;
	// compute MD5 hash
	uint a, b, c, d;

	int linidx = (gridDim.x*blockIdx.y + blockIdx.x)*blockDim.x + threadIdx.x; // assuming blockDim.y = 1 and threadIdx.y = 0, always

	uint4 *ul4src = (uint4 *)gwords;
	uint index = linidx;
	/* Load using 128bits access */
	i0 = ul4src[index];

	md5_transform(i0, a, b, c, d);
	if(a == target[0] && b == target[1] && c == target[2] && d == target[3])
	{
		uint4 success_res;
		success_res.x = i0.x;
		success_res.y = i0.y;
		success_res.z = i0.z;
		success_res.w = 1;
		*((uint4 *)succ) = success_res;
	}

}

/*
Return 0 if no error else return <> 0 for error
*/
// A helper to export the kernel call to C++ code not compiled with nvcc
extern "C" 
int execute_kernel_md5_search(int blocks_x, int blocks_y, int threads_per_block, int shared_mem_required, uint *gpuHash, char *gpuWords)
{
	dim3 grid;
	cudaError_t err;

	grid.x = blocks_x; grid.y = blocks_y;
	
	md5_search_bruteforce<<<grid, threads_per_block, shared_mem_required>>>(gpuWords, gpuHash);	

	err = cudaThreadSynchronize();
	return err;
}
