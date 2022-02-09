
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
int result;

int init(size_t requestedsize)
{
	//Initialize variables needed to map a virtual address space (allocate memory) for the arena
	int pagesize = getpagesize();
	int protoresult = (int) (requestedsize / pagesize);
	result = (protoresult * pagesize) + pagesize;
	int fd = open("/dev/zero", O_RDWR); //open function creates and returns a new file descriptor for the file located at "/dev/zero".

	if(fd == -1)
	{
		perror("...failed in initialization, open");
		return ERR_SYSCALL_FAILED;
	}
	//If the byte size requested by the user is less than 0, then return ERR_BAD_ARGUMENTS. Otherwise, map a virtual address space for the arena.
	else if(((int) requestedsize) < 0)
	{
    	return ERR_BAD_ARGUMENTS;
	}
	else
	{
    	_arena_start = mmap(NULL, result, (PROT_READ | PROT_WRITE), MAP_PRIVATE, fd, 0);

    	if(_arena_start == MAP_FAILED)
    	{
    		perror("...failed in initialization, mmap");
    		return ERR_SYSCALL_FAILED;
    	}
    	else if(requestedsize == pagesize)
    	{
    		result = pagesize;
    		return result;
    	}
    	else
    	{
    		return result;
    	}
	}

	//Initialize the chunk list:
	//struct __node_t *head = NULL;
}

int destroy()
{
	int destroyoutcome = munmap(_arena_start, result);

	if(destroyoutcome == 0)
	{
		return destroyoutcome;
	}
	else
	{
		printf("...error: cannot destroy uninitialized arena. Setting error status\n");
		return ERR_UNINITIALIZED;
	}
}



