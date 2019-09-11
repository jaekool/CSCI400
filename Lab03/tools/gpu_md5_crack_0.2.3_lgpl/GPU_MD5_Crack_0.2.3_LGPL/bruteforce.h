
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

#ifndef BRUTEFORCE_H
#define BRUTEFORCE_H

#ifdef  __cplusplus
#include <string>
#include <vector>
extern "C" {
#endif
#include "global_types.h"
#include "charset.h"
#include "cuda_bruteforce_gpu.h"

	union pwdfound
	{
		uint ui[4];
		char ch[16];
	};

	/* Prototype Function */
	extern int print_devices(void);
	/* Init Memory and Stuff for GPU, should be called only 1 time at init */
	extern int init_bruteforce_gpu(uint max_pwd, int gpu_device);
	extern int last_error_string(char *str, uint str_len);
	extern int exit_bruteforce(void);

	/* To use default GPU_Device set to -1 */
	extern int set_md5_gpu_parameters(e_charset charset, uint *target);

	extern double bruteforce_cpu_gpu(e_charset charset, unsigned char *chr_pwd_table, uint nb_pwd);
	extern double md5_crack_brute_force(e_charset charset, uint *target, unsigned char *pwdstr_curr, uint nb_pwd, pwdfound *pwd_found);

	extern double inline bruteforce_cpu_md5prep(e_charset charset, unsigned char *pwdstr_cur, uint nb_pwd, unsigned char *out_buf, uint out_buf_size);

	// GPU MD5 computation external calls
	extern void init_md5_target_constants(uint *target_cpu);
	extern int execute_kernel_md5_search(int blocks_x, int blocks_y, int threads_per_block, int shared_mem_required, uint *gpuHash, char *gpuWords);

#ifdef  __cplusplus
}
#endif

#endif /* BRUTEFORCE_H */
