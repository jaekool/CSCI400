
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

#ifndef CUDA_MD5_GPU_H
#define CUDA_MD5_GPU_H

#ifdef  __cplusplus
extern "C" {
#endif

#ifndef uint
	typedef unsigned int uint;
#endif

#ifndef uint64
	typedef unsigned long long uint64;
#endif

extern void init_md5_target_constants(uint *target_cpu);
// A helper to export the kernel call to C++ code not compiled with nvcc
extern int execute_kernel_md5_search(int blocks_x, int blocks_y, int threads_per_block, int shared_mem_required, uint *gpuHash, char *gpuWords);

#ifdef  __cplusplus
}
#endif

#endif /* CUDA_MD5_GPU_H */
