/**
 * @file CacheFS.cpp
 * @author Itai Tagar <itagar>
 *
 * @brief An implementation of the Cache File System Library.
 */


/*-----=  Includes  =-----*/


#include <sys/stat.h>
#include <fcntl.h>
#include <cstdio>
#include <stdlib.h>
#include <malloc.h>
#include <unistd.h>
#include <cassert>
#include <string>
#include <iostream>
#include <map>
#include "CacheFS.h"


/*-----=  Definitions  =-----*/


/**
 * @def SUCCESS_STATE 0
 * @brief A Macro that sets the value indicating success state in the library.
 */
#define SUCCESS_STATE 0

/**
 * @def FAILURE_STATE -1
 * @brief A Macro that sets the value indicating failure state in the library.
 */
#define FAILURE_STATE -1

/**
 * @def MINIMUM_BLOCK_NUMBER 1
 * @brief A Macro that sets the minimum value of block number.
 */
#define MINIMUM_BLOCK_NUMBER 1

/**
 * @def PARTITION_LOWER_BOUND 0
 * @brief A Macro that sets the lower bound of partition percentage.
 */
#define PARTITION_LOWER_BOUND 0

/**
 * @def PARTITION_UPPER_BOUND 1
 * @brief A Macro that sets the upper bound of partition percentage.
 */
#define PARTITION_UPPER_BOUND 1

/**
 * @def TMP_PATH "/tmp"
 * @brief A Macro that sets the path of the tmp directory.
 */
#define TMP_PATH "/tmp"


/*-----=  Type Definitions  =-----*/


/**
 * @brief Type Definition for the block size type.
 */
typedef long blockSize_t;

/**
 * @brief Type Definition for the file path type.
 */
typedef std::string filePath_t;


/*-----=  System Functions  =-----*/


/**
 * @brief Returns the block size on this system.
 * @return The block size on this system.
 */
static blockSize_t getBlockSize()
{
    struct stat fi;
    stat(TMP_PATH, &fi);
    return fi.st_blksize;
}


/*-----=  Library Data  =-----*/


// TODO: Doxygen.
blockSize_t blockSize = getBlockSize();

// TODO: Doxygen.
char *bufferCache = nullptr;

// TODO: Doxygen.
int bufferIndex = 0;  // TODO: Magic Number.

// TODO: Doxygen.
std::map<int, filePath_t> openFiles;


/*-----=  Validation Functions  =-----*/


/**
 * @brief Determine if the given partition percentage of f_old and f_new
 *        partitioning is a valid percentage.
 *        The percentage is invalid if it is not a number between 0 to 1 or
 *        if the size of the partition of the blocks is not positive.
 * @param partition Percentage of blocks in a partition (rounding down).
 * @return 0 in case of validation success, -1 in case of failure.
 */
static int validatePartition(double const partition)
{
    // TODO: Check the correctness of this.
    if (PARTITION_LOWER_BOUND < partition && partition < PARTITION_UPPER_BOUND)
    {
        return FAILURE_STATE;
    }
    return SUCCESS_STATE;
}

/**
 * @brief Determine if the given arguments of the init function are valid.
 *        blocks_num is invalid if it's not a positive number.
 *        f_old is invalid if it is not a number between 0 to 1 or if the
 *        size of the partition of the old blocks is not positive.
 *        fNew is invalid if it is not a number between 0 to 1 or if the
 *        size of the partition of the new blocks is not positive.
 *        Also, fOld and fNew are invalid if the fOld + fNew is bigger than 1.
 *        Validation of f_old and f_new should take action only if the selected
 *        algorithm is FBR, which is indicated by the fbrFlag.
 * @param blocks_num The number of blocks in the buffer cache.
 * @param f_old The percentage of blocks in the old partition (rounding down)
 *              relevant in FBR algorithm only
 * @param f_new The percentage of blocks in the new partition (rounding down)
 *              relevant in FBR algorithm only
 * @param fbrFlag Indicates if the selected algorithm is FBR.
 * @return 0 in case of validation success, -1 in case of failure.
 */
static int validateInitArguments(int const blocks_num, double const f_old,
                                 double const f_new, bool const fbrFlag)
{
    // Check that the number of blocks is a positive number.
    if (blocks_num < MINIMUM_BLOCK_NUMBER)
    {
        return FAILURE_STATE;
    }

    // If the selected algorithm is FBR we have to check the values
    // of f_new and f_old as well.
    if (fbrFlag)
    {
        // Check the partition percentage of each partition.
        if (validatePartition(f_old) || validatePartition(f_new))
        {
            return FAILURE_STATE;
        }
        // Check the total amount of the partitions together.
        if ((f_old + f_new) > PARTITION_UPPER_BOUND)
        {
            return FAILURE_STATE;
        }
    }

    return SUCCESS_STATE;
}

// TODO: Doxygen.
static int validateReadArguments()
{

}


/*-----=  TODO: Delete all this  =-----*/


static void printFilesMap()
{
    std::cout << "~~ Print open files ~~" << std::endl;
    for (auto i = openFiles.begin(); i != openFiles.end(); ++i)
    {
        std::cout << i->first << ", " << i->second << std::endl;
    }
    std::cout << std::endl;
}


/*-----=  Library Implementation  =-----*/


/**
 * @brief Initializes the CacheFS.
 *        CacheFS_init will be called before any other function. CacheFS_init
 *        might be called multiple times, but only with CacheFS_destroy
 *        between them.
 * @param blocks_num The number of blocks in the buffer cache.
 * @param cache_algo The cache algorithm that will be used.
 * @param f_old The percentage of blocks in the old partition (rounding down)
 *              relevant in FBR algorithm only
 * @param f_new The percentage of blocks in the new partition (rounding down)
 *              relevant in FBR algorithm only
 * @return 0 in case of success, negative value in case of failure.
 */
int CacheFS_init(int blocks_num, cache_algo_t cache_algo, double f_old,
                 double f_new)
{
    // Allocate memory for the given amount of blocks number.
    bufferCache = (char *) aligned_alloc(blockSize, blocks_num * blockSize);
    if (bufferCache == nullptr)
    {
        // In case the memory allocation failed.
        return FAILURE_STATE;
    }

    // If the selected algorithm is FBR.
    if (cache_algo == FBR)
    {
        // First we validate the given arguments.
        if (validateInitArguments(blocks_num, f_old, f_new, true))
        {
            return FAILURE_STATE;
        }
    }

    // If the selected algorithm is LRU/LFU.
    else
    {
        // First we validate the given arguments.
        if (validateInitArguments(blocks_num, f_old, f_new, false))
        {
            return FAILURE_STATE;
        }
    }

    return SUCCESS_STATE;
}

/**
 * @brief Destroys the CacheFS. This function releases all the allocated
 *        resources by the library. CacheFS_destroy will be called only
 *        after CacheFS_init (one destroy per one init). After CacheFS_destroy
 *        is called, the next CacheFS's function that will be called
 *        is CacheFS_init. CacheFS_destroy is called only after all the open
 *        files already closed. In other words, it's the user responsibility
 *        to close the files before destroying the CacheFS.
 * @return 0 in case of success, negative value in case of failure.
 */
int CacheFS_destroy()
{
    // Release resources of the buffer cache.
    free(bufferCache);
    bufferCache = nullptr;

    return SUCCESS_STATE;
}

// TODO: Doxygen.
int CacheFS_open(const char *pathname)
{
    // Receive the full path of the file.
    const char *realPath = realpath(pathname, nullptr);
    if (realPath == nullptr)
    {
        // If the real path procedure failed.
        return FAILURE_STATE;
    }
    filePath_t filePath(realPath);
    // TODO: Check if the file in /tmp.

    // Open the file in the given path.
    int fd = open(realPath, O_RDONLY | O_DIRECT | O_SYNC);
    if (fd < SUCCESS_STATE)
    {
        // In case open failed.
        return FAILURE_STATE;
    }

    // Insert the opened file to the open files data container with it's path.
    openFiles[fd] = realPath;
    return fd;
}

// TODO: Doxygen.
int CacheFS_close(int file_id)
{
    return SUCCESS_STATE;
}

// TODO: Doxygen.
int CacheFS_pread(int file_id, void *buf, size_t count, off_t offset)
{
    // Find the given file id in the open files container.
    auto fileIterator = openFiles.find(file_id);
    if (fileIterator == openFiles.end())
    {
        // If the given file id is invalid.
        return FAILURE_STATE;
    }

    // TODO: Check if the block contains data is in the cache.
    // TODO: If the cache does not contains the block, calculate the block with data.
    // TODO: Insert block to cache, check if cache full or not.
    // TODO: If cache is full, remove by policy.
    // TODO: Check if we reach the end of file.

    int bytesRead = 0;
    off_t fileSize = lseek(file_id, 0, SEEK_END);
    std::cout << "File Size: " << fileSize << std::endl;
    off_t startBlock = offset / blockSize;
    off_t endBlock = (count + offset) / blockSize;
    off_t skip = offset % blockSize;
    size_t blocksToRead = (size_t)(endBlock - startBlock) + 1;
    std::cout << "Start Block: " << startBlock << std::endl;
    std::cout << "End Block: " << endBlock << std::endl;
    std::cout << "Skip: " << skip << std::endl;
    std::cout << "Blocks to Read: " << blocksToRead << std::endl;

    // Read to cache buffer entire blocks which contains the requested data.
    int blocksRead = (int) pread(file_id, bufferCache + bufferIndex, blocksToRead*blockSize, startBlock*blockSize);
    std::cout << "Blocks Read: " << blocksRead / blockSize << std::endl;
    std::cout << "Buffer Index: " << bufferIndex << std::endl;

    std::cout << bufferCache << std::endl;

    off_t startPosition = bufferIndex + skip;
    std::cout << "startPosition: " << startPosition << std::endl;

    bufferIndex += blocksRead;

    std::cout << "Bytes Read: " << bytesRead << std::endl;
    return bytesRead;
}

int CacheFS_print_cache(const char *log_path)
{
    return SUCCESS_STATE;
}

int CacheFS_print_stat(const char *log_path)
{
    return SUCCESS_STATE;
}
