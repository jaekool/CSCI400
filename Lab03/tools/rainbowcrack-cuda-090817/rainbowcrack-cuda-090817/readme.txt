
RainbowCrack for CUDA

Introduction
--------------------------------
This software is a demo of the GPU accelerated time-memory tradeoff implementation.
rcrack_cuda.exe and rcrack_cuda_gui.exe programs are included. Rainbow tables can be generated 
with the rtgen.exe program in RainbowCrack software, and then sort with rtsort.exe program.

Configurations supported by rcrack_cuda.exe and rcrack_cuda_gui.exe programs in this software include:
	lm_alpha-numeric#1-7
	ntlm_loweralpha-numeric#1-7
	md5_loweralpha-numeric#1-7
	sha1_loweralpha-numeric#1-7

Rainbow table generation commands:
	rtgen lm alpha-numeric 1 7 0 3800 33554432 0
	rtgen lm alpha-numeric 1 7 1 3800 33554432 0
	rtgen lm alpha-numeric 1 7 2 3800 33554432 0
	rtgen lm alpha-numeric 1 7 3 3800 33554432 0
	rtgen lm alpha-numeric 1 7 4 3800 33554432 0
	rtgen lm alpha-numeric 1 7 5 3800 33554432 0

	rtgen ntlm loweralpha-numeric 1 7 0 3800 33554432 0
	rtgen ntlm loweralpha-numeric 1 7 1 3800 33554432 0
	rtgen ntlm loweralpha-numeric 1 7 2 3800 33554432 0
	rtgen ntlm loweralpha-numeric 1 7 3 3800 33554432 0
	rtgen ntlm loweralpha-numeric 1 7 4 3800 33554432 0
	rtgen ntlm loweralpha-numeric 1 7 5 3800 33554432 0

	rtgen md5 loweralpha-numeric 1 7 0 3800 33554432 0
	rtgen md5 loweralpha-numeric 1 7 1 3800 33554432 0
	rtgen md5 loweralpha-numeric 1 7 2 3800 33554432 0
	rtgen md5 loweralpha-numeric 1 7 3 3800 33554432 0
	rtgen md5 loweralpha-numeric 1 7 4 3800 33554432 0
	rtgen md5 loweralpha-numeric 1 7 5 3800 33554432 0

	rtgen sha1 loweralpha-numeric 1 7 0 3800 33554432 0
	rtgen sha1 loweralpha-numeric 1 7 1 3800 33554432 0
	rtgen sha1 loweralpha-numeric 1 7 2 3800 33554432 0
	rtgen sha1 loweralpha-numeric 1 7 3 3800 33554432 0
	rtgen sha1 loweralpha-numeric 1 7 4 3800 33554432 0
	rtgen sha1 loweralpha-numeric 1 7 5 3800 33554432 0

System Requirements
--------------------------------
Operating System:	Windows XP, Windows Vista
GPU:				nVidia GPU with CUDA enabled driver installed
Memory:				at least 256MB

Notes from NVIDIA's document:
       The use of multiple GPUs as CUDA devices by an application running on a multi-
       GPU system is only guaranteed to work if these GPUs are of the same type. If the
       system is in SLI mode however, only one GPU can be used as a CUDA device since
       all the GPUs are fused at the lowest levels in the driver stack. SLI mode needs to be
       turned off in the control panel for CUDA to be able to see each GPU as separate
       devices.

How to use
--------------------------------
Visit http://project-rainbowcrack.com/ for documentation and update of this software.

Copyright 2003-2009 Project RainbowCrack. All rights reserved.
Official Website: http://project-rainbowcrack.com/
August 17, 2009
