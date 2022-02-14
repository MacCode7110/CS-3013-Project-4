/*
 ============================================================================
 Name    	: goatmalloc.c
 Author  	: Matthew McAlarney and Cooper Dean
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
struct __node_t *head; //head of the arena memory chunks
int result; //calculated size of the arena
int statusno; //denotes the current error status
int CHUNK_SIZE = 64; //size of a memory chunk

int init(size_t requestedsize)
{
	//Initialize variables needed to map a virtual address space (allocate memory) for the arena
	int pagesize = getpagesize();
	int protoresult = (int) (requestedsize / pagesize);
	result = (protoresult * pagesize) + pagesize;
	int fd = open("/dev/zero", O_RDWR); //open function creates and returns a new file descriptor for the file located at "/dev/zero".

	//If the return of a file descriptor contains an error, then indicate that there was a failure in the initialization of the arena.
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
		//Initiliaze the arena using mmap
    	_arena_start = mmap(NULL, result, (PROT_READ | PROT_WRITE), MAP_PRIVATE, fd, 0);

    	//If the arena could not be initialized due to a mapping error, indicate this error to the user
    	if(_arena_start == MAP_FAILED)
    	{
    		perror("...failed in initialization, mmap");
    		return ERR_SYSCALL_FAILED;
    	} //If the requested size is equal to the page size, then return the page size immediately
    	else if(requestedsize == pagesize)
    	{
    		result = pagesize;
    	}

    	//Initialize the chunk list:
        head = _arena_start;
        head->is_free = 1; //Set the head chunk to status free (metadata attribute)
        head->size = result - sizeof(node_t); //Set the size of the head chunk to be the size of the arena minus the size of a single memory chunk node to account for metadata storage

        return result;

	}
}

//destroy() removes the arena memory mapping as a virtual address
int destroy()
{
	//Attempt to destory the arena
	int destroyoutcome = munmap(_arena_start, result);

	//If the arena memory mapping was successfully removed, then return this outcome
	if(destroyoutcome == 0)
	{
		return destroyoutcome;
	}
	else //If the arena cannot be destroyed, then indicate this as an error message to the terminal.
	{
		printf("...error: cannot destroy uninitialized arena. Setting error status\n");
		return ERR_UNINITIALIZED;
	}
}

//walloc() allocates basic memory on the arena mapping based on a parameter size
void* walloc(size_t size) {

	//If the arena was unable to be created, then set statusno to reflect an uninitialized error
	if (!result) {
        statusno = ERR_UNINITIALIZED;
        return NULL;
    }

    //Find the next free chunk with enough space
    struct __node_t *chunk = head;

    //While the next chunk is not free for memory allocation or the argument size for memory allocation is greater than the memory space of the next chunk, move onto the next chunk that satisfies these requirements.
    while (!chunk->is_free || size > chunk->size) {
        chunk = chunk->fwd;
        //If the next chunk does not exist, then set the error status to reflect that the arena is out of memory and return a NULL address to memory.
        if (chunk == NULL) {
            statusno = ERR_OUT_OF_MEMORY;
            return NULL;
        }
    }

    //Chunk Splitting - we always split a smaller chunk from a larger chunk

    //The remaining space of a chunk is equal to the size of the current chunk minus the requested size for memory allocation
    int remaining_space = chunk->size - size;
    //If the remaining space is greater than or equal to the memory of a chunk's metadata plus the size of the memory chunk (if there is enough memory space for the requested allocation, and as long as we are not cutting into the memory of the next chunk and its header/metadata), then allocate a new free chunk by splitting it from the arena and return the address to this new memory chunk.
    if(remaining_space >= (sizeof(node_t) + CHUNK_SIZE)) {
        struct __node_t *next = ((void*)chunk) + sizeof(node_t) + size; //The address to the next free chunk node to be used later = the address to the current chunk + the size of the attached metadata + the requested memory size (in bytes)
        chunk->fwd = next;                     //^metadata always proceeds the size of the allocated memory, so the memory size must be added to the size of the metadata in this order.
        next->bwd = chunk; //This line and the one above work because the address to the current chunk is included in the calculation of the address to the next chunk.

        next->is_free = 1; //The next chunk node is free for use and allocation when walloc() is called again.
        next->size = chunk->size - size - sizeof(node_t); //calculation reduces to size of next free chunk = remaining space - the size of the attached metadata for the next free chunk
        chunk->size = size; //set the size of the newly found and allocated chunk to the parameter size
    }
    chunk->is_free = 0; //the newly allocated chunk is no longer free; essentially, we have manipulated the size of the head chunk to match the parameter size in order to account for the remaining space that will be allocated for the next free chunk.

    return ((void*)chunk) + sizeof(node_t); //returns a pointer to the newly found and allocated chunk, and its metadata
}

void wfree(void *ptr) { //consumes a memory buffer (a chunk); specifically, wfree consumes memory that takes the equation form ((void*)chunk) + sizeof(node_t) based on how walloc() is used in the test cases.
	//Cast from void * to __node_t * to access the given chunk's metadata attributes.
	struct __node_t *cpy = ((struct __node_t *) ptr) - 1; //The size of __node_t is 1, so we need to subtract this value to obtain the correct address to the header of the chunk.
	cpy -> is_free = 1;

	//Coalescing:

	// bwd
	//If there is a free chunk prior to the current chunk, combine the two and change the size accordingly.
	while (cpy != NULL) {
		//Check to see if the previous node/memory chunk is NULL; if it is, break from the while loop.
		if (cpy->bwd == NULL)
			break;
		//Else, check to see if the previous node/memory chunk is free. If it is not, then break from the while loop.
		else if (!cpy->bwd->is_free)
			break;
		//Set the size of the previous memory chunk to the size of the previous memory chunk plus the size of the current memory chunk plus the size of the metadata of the current memory chunk.
		//Set the link to the current memory chunk to the the next memory chunk
		else {
			cpy->bwd->size += cpy->size + sizeof(node_t);
			cpy->bwd->fwd = cpy->fwd;
			//If the next memory chunk is not NULL, then set the current memory chunk to the previous memory chunk
			if (cpy->fwd != NULL)
				cpy->fwd->bwd = cpy->bwd;
			cpy = cpy->bwd;
		}
	}

	// fwd
	//If there is a free chunk after the current chunk, combine the two and change the size accordingly.
	while (cpy->fwd != NULL) {
		//If the the next memory chunk is not free, the break from the while loop.
		if (!cpy->fwd->is_free)
			break;
		else {
			//Set the current memory chunk size to the current memory chunk size plus the size of the next memory chunk plus the size of the metadata.
			cpy->size += cpy->fwd->size + sizeof(node_t);
			//If the memroy chunk two nodes away is not NULL, then set the memory chunk one node away to the current memory chunk.
			if (cpy->fwd->fwd != NULL)
				cpy->fwd->fwd->bwd = cpy;
			//Set the memory chunk one node away to the memory chunk two nodes away.
			cpy->fwd = cpy->fwd->fwd;
		}
	}
}
