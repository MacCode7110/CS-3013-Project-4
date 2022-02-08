/*
 ============================================================================
 Name    	: goatmalloc.c
 Author  	: Matthew McAlarney and Dean Cooper
 Version 	:
 Copyright   : Your copyright notice
 Description : Dynamic Memory Allocation Program
 ============================================================================
 */

#include <stdio.h>
#include "goatmalloc.h"
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

void * _arena_start; //_arena_start is of type void * because mmap() returns a pointer to the dynamically mapped region in memory
int product;

int init(size_t requestedsize)
{
	//Initialize variables needed to map a virtual address space (allocate memory) for the arena
	int pagesize = getpagesize();
	product = pagesize * requestedsize;
	int fd = open("/dev/zero", O_RDWR); //open function creates and returns a new file descriptor for the file located at "/dev/zero".

	//If the byte size requested by the user is less than 0, then return ERR_BAD_ARGUMENTS. Otherwise, map a virtual address space for the arena.
	if(requestedsize < 0)
	{
    	return ERR_BAD_ARGUMENTS;
	}
	else
	{
    	_arena_start = mmap(NULL, product, (PROT_READ | PROT_WRITE), MAP_PRIVATE, fd, 0);
    	return (pagesize * requestedsize);
	}

	//Initialize the chunk list:
	//struct __node_t *head = NULL;
}

int destroy()
{
	if(_arena_start != NULL)
	{
		int result = munmap(_arena_start, product);
		return result;
	}
	else
	{
		return ERR_UNINITIALIZED;
	}
}


