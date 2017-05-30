/**
 * @file CacheFS.cpp
 * @author Itai Tagar <itagar>
 *
 * @brief An implementation of the Cache File System Library.
 */


/*-----=  Includes  =-----*/


#include <sys/stat.h>
#include <cstdio>
#include <cassert>
#include <string>
#include <vector>
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


/*-----=  Type Definitions  =-----*/


/**
 * @brief Type Definition for the block size type.
 */
typedef long blockSize_t;


/*-----=  Library Data  =-----*/




/*-----=  Misc. Functions  =-----*/


/**
 * @brief Returns the block size on this system.
 * @return The block size on this system.
 */
static blockSize_t getBlockSize()
{
    struct stat fi;
    stat("/tmp", &fi);
    return fi.st_blksize;
}


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
    return SUCCESS_STATE;
}


int CacheFS_open(const char *pathname)
{
    return SUCCESS_STATE;
}

int CacheFS_close(int file_id)
{
    return SUCCESS_STATE;
}

int CacheFS_pread(int file_id, void *buf, size_t count, off_t offset)
{
    return SUCCESS_STATE;
}

int CacheFS_print_cache(const char *log_path)
{
    return SUCCESS_STATE;
}

int CacheFS_print_stat(const char *log_path)
{
    return SUCCESS_STATE;
}
