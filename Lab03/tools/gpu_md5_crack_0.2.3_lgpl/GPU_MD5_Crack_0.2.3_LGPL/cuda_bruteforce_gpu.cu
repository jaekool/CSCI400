
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

#include "cuda_bruteforce_gpu.h"

typedef union __align__(16) pwd_type
{
	uint4 vect4;
	unsigned int ui[4];
	unsigned char ch[16];
}pwd_t;

/* GPU constant initialized by init_constants_bruteforce_gpu and used by bruteforce_gpu_charset() */
__device__ __constant__ char __align__(16) gpu_charset[128];
__device__ __constant__ unsigned int gpu_charset_len[1];

/* 
Return 0 if no error else return <> 0
*/
int init_constants_bruteforce_gpu(e_charset charset)
{
	cudaError_t err1, err2;
	unsigned int num_char;
	char *cCharset = NULL;
	unsigned char charset_len;
	
	get_charsetCPU_charsetLen(charset, &cCharset, &charset_len);
	num_char = charset_len;
	err1 = cudaMemcpyToSymbol(gpu_charset, cCharset, num_char);
	err2 = cudaMemcpyToSymbol(gpu_charset_len, &num_char, sizeof(num_char));

	if(err1 != cudaSuccess)
	{
		return err1;
	}else if(err2 != cudaSuccess)
	{
		return err2;
	}

	return 0;
}

__global__ void bruteforce_gpu_charset(char *src_gwords, char *dst_gwords)
{
	unsigned int i;
	unsigned int index_src;
	unsigned int index_dst;
	unsigned int pwd_len;
	unsigned int numchar;

	pwd_t tmp_pwd_table;
	register pwd_t tmp_pwd_char;

	numchar = gpu_charset_len[0];
	uint4 *ul4src = (uint4 *)src_gwords;
	uint4 *ul4dst = (uint4 *)dst_gwords;
	uint4 tmp_vect4;

	index_src = (gridDim.x*blockIdx.y + blockIdx.x)*blockDim.x + threadIdx.x; // assuming blockDim.y = 1 and threadIdx.y = 0, always
	index_dst = index_src*numchar;

	ul4dst = &ul4dst[index_dst];

	tmp_pwd_table.vect4 = ul4src[index_src];

	tmp_pwd_char.vect4.x = 0;
	tmp_pwd_char.vect4.y = 0;
	tmp_pwd_char.vect4.z = 0;

	/* Fill first chars password plain text */
	pwd_len = tmp_pwd_table.vect4.w/8;

	for(i=0; i<pwd_len; i++)
	{
		tmp_pwd_char.ch[i] = gpu_charset[tmp_pwd_table.ch[i]];
	}

	tmp_pwd_char.ch[i] = 0x80; // bit 1 after the message
	tmp_pwd_char.vect4.w = tmp_pwd_table.vect4.w;

	tmp_vect4 = tmp_pwd_char.vect4;

	switch(pwd_len)
	{
		case 1: // Vector 0, Char 0
			//#pragma unroll numchar
			for(i=0; i<numchar; i++)
			{
				tmp_vect4.x = ((tmp_vect4.x & 0xFFFFFF00) | (unsigned int)(gpu_charset[i]));
				*ul4dst = tmp_vect4;
				++ul4dst;		
			}
			break;

		case 2: // Vector 0, Char 1
			//#pragma unroll numchar
			for(i=0; i<numchar; i++)
			{
				tmp_vect4.x = ((tmp_vect4.x & 0xFFFF00FF) | ((unsigned int)(gpu_charset[i])<<8));
				*ul4dst = tmp_vect4;
				++ul4dst;		
			}		
			break;

		case 3: // Vector 0, Char 2
			//#pragma unroll numchar
			for(i=0; i<numchar; i++)
			{
				tmp_vect4.x = ((tmp_vect4.x & 0xFF00FFFF) | ((unsigned int)(gpu_charset[i])<<16));
				*ul4dst = tmp_vect4;
				++ul4dst;		
			}		
			break;

		case 4: // Vector 0, Char 3
			//#pragma unroll numchar
			for(i=0; i<numchar; i++)
			{
				tmp_vect4.x = ((tmp_vect4.x & 0x00FFFFFF) | ((unsigned int)(gpu_charset[i])<<24));
				*ul4dst = tmp_vect4;
				++ul4dst;		
			}		
			break;

		case 5: // Vector 1, Char 0
			//#pragma unroll numchar
			for(i=0; i<numchar; i++)
			{
				tmp_vect4.y = ((tmp_vect4.y & 0xFFFFFF00) | (unsigned int)(gpu_charset[i]));
				*ul4dst = tmp_vect4;
				++ul4dst;		
			}
			break;

		case 6: // Vector 1, Char 1
			//#pragma unroll numchar
			for(i=0; i<numchar; i++)
			{
				tmp_vect4.y = ((tmp_vect4.y & 0xFFFF00FF) | ((unsigned int)(gpu_charset[i])<<8));
				*ul4dst = tmp_vect4;
				++ul4dst;		
			}		
			break;

		case 7: // Vector 1, Char 2
			//#pragma unroll numchar
			for(i=0; i<numchar; i++)
			{
				tmp_vect4.y = ((tmp_vect4.y & 0xFF00FFFF) | ((unsigned int)(gpu_charset[i])<<16));
				*ul4dst = tmp_vect4;
				++ul4dst;		
			}		
			break;

		case 8: // Vector 1, Char 3
			//#pragma unroll numchar
			for(i=0; i<numchar; i++)
			{
				tmp_vect4.y = ((tmp_vect4.y & 0x00FFFFFF) | ((unsigned int)(gpu_charset[i])<<24));
				*ul4dst = tmp_vect4;
				++ul4dst;		
			}		
			break;

		case 9: // Vector 2, Char 0
			//#pragma unroll numchar
			for(i=0; i<numchar; i++)
			{
				tmp_vect4.z = ((tmp_vect4.z & 0xFFFFFF00) | (unsigned int)(gpu_charset[i]));
				*ul4dst = tmp_vect4;
				++ul4dst;		
			}
			break;

		case 10: // Vector 2, Char 1
			//#pragma unroll numchar
			for(i=0; i<numchar; i++)
			{
				tmp_vect4.z = ((tmp_vect4.z & 0xFFFF00FF) | ((unsigned int)(gpu_charset[i])<<8));
				*ul4dst = tmp_vect4;
				++ul4dst;		
			}		
			break;

		case 11: // Vector 2, Char 2
			//#pragma unroll numchar
			for(i=0; i<numchar; i++)
			{
				tmp_vect4.z = ((tmp_vect4.z & 0xFF00FFFF) | ((unsigned int)(gpu_charset[i])<<16));
				*ul4dst = tmp_vect4;
				++ul4dst;		
			}		
			break;

		case 12: // Vector 2, Char 3
			//#pragma unroll numchar
			for(i=0; i<numchar; i++)
			{
				tmp_vect4.z = ((tmp_vect4.z & 0x00FFFFFF) | ((unsigned int)(gpu_charset[i])<<24));
				*ul4dst = tmp_vect4;
				++ul4dst;		
			}		
			break;

	}
}

/*
Return 0 if no error else return <> 0 for error
*/
// A helper to export the kernel call to C++ code not compiled with nvcc
int execute_kernel_bruteforce(int blocks_x, int blocks_y, int threads_per_block, int shared_mem_required, char *src_gwords, char *dst_gwords)
{
	dim3 grid, block;
	cudaError_t err;

	grid.x = blocks_x; grid.y = blocks_y; grid.z = 1;
	block.x = threads_per_block; block.y = 1; block.z = 1;

	bruteforce_gpu_charset<<<grid, block, shared_mem_required>>>(src_gwords, dst_gwords);	

	err = cudaThreadSynchronize();
	return err;
}
