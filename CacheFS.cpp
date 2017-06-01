// TODO: Valgrind
// TODO: Makefile
// TODO: README
// TODO: Answers + add Answers.pdf to Makefile of tar
// TODO: Check every system call.


/**
 * @file CacheFS.cpp
 * @author Itai Tagar <itagar>
 *
 * @brief An implementation of the Cache File System Library.
 */


/*-----=  Includes  =-----*/


#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstdio>
#include <string>
#include <cstring>
#include <iostream>
#include <map>
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

/**
 * @def BLOCK_START_ALIGNMENT 0
 * @brief A Macro that sets the value of the block start alignment.
 */
#define BLOCK_START_ALIGNMENT 0

/**
 * @def INITIAL_NUMBER_OF_BLOCKS 0
 * @brief A Macro that sets the value of initial number of blocks in the cache.
 */
#define INITIAL_NUMBER_OF_BLOCKS 0

/**
 * @def TMP_PATH "/tmp"
 * @brief A Macro that sets the path of the tmp directory.
 */
#define TMP_PATH "/tmp"

/**
 * @def PATH_SEPARATOR "/"
 * @brief A Macro that sets the path separator character.
 */
#define PATH_SEPARATOR '/'


/*-----=  Type Definitions & Enums  =-----*/

// TODO: Doxygen.
enum BufferSection { NONE, NEW, MID, OLD };

/**
 * @brief Type Definition for the block size type.
 */
typedef size_t blockSize_t;

// TODO :Doxygen.
typedef struct Block
{
    std::string filePath;
    size_t blockNumber;
    BufferSection section;
    char *buffer;
} Block;

/**
 * @brief Type Definition for a vector of blocks type.
 */
typedef std::vector<Block> blocksVector;

/**
 * @brief Type Definition for a Map of file descriptors and their path type.
 */
typedef std::map<int, std::string> filesMap;


/*-----=  System Functions  =-----*/


/**
 * @brief Returns the block size on this system.
 * @return The block size on this system.
 */
static blockSize_t getBlockSize()
{
    struct stat fi;
    stat(TMP_PATH, &fi);
    return (blockSize_t) fi.st_blksize;
}


/*-----=  Library Data  =-----*/


/**
 * @brief The size of a single block in this system.
 */
blockSize_t blockSize = getBlockSize();

// TODO: Doxygen.
int activeBlocks = INITIAL_NUMBER_OF_BLOCKS;

// TODO: Doxygen.
int numberOfBlocks = INITIAL_NUMBER_OF_BLOCKS;

/**
 * @brief A Map which holds to open files during the run of this library.
 */
filesMap openFiles;

/**
 * @brief A Vector of all the blocks which are available in the buffer cache.
 */
blocksVector blocks;


/*-----=  TODO: Organize these functions  =-----*/


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

    // Set the library data of the number of blocks to the given amount.
    numberOfBlocks = blocks_num;

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

/**
 * @brief Validate that the given file path is in the 'tmp' directory.
 * @param realPath The file path to validate.
 * @return 0 if the path is a valid path, otherwise returns -1.
 */
static int validatePath(const char *realPath)
{
    // Check that the given path starts with '/tmp', i.e. the path is in the tmp
    // folder in the file system.
    unsigned int charIndex = 0;
    for ( ; charIndex < strlen(TMP_PATH); ++charIndex)
    {
        if (realPath[charIndex] == TMP_PATH[charIndex])
        {
            continue;
        }
        // In case there is no match in the current char we can return failure.
        return FAILURE_STATE;
    }

    // Check that the next char is '/'.
    if (realPath[charIndex] != PATH_SEPARATOR)
    {
        return FAILURE_STATE;
    }

    return SUCCESS_STATE;
}

// TODO: Doxygen.
static int validateReadArguments()
{

}

/**
 * @brief Determines if a given file ID is an open file in the library.
 *        The function checks if the file ID which represent a file descriptor
 *        is in the open files container.
 * @param file_id The file ID to check.
 * @return 0 in case of validation success, -1 in case of failure.
 */
static int isFileOpen(const int file_id)
{
    // Find the given file id in the open files container.
    auto fileIterator = openFiles.find(file_id);
    if (fileIterator == openFiles.end())
    {
        // If the given file id is invalid.
        return FAILURE_STATE;
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
        // TODO: Set algorithm to FBR and set the buffer to new/mid/old.
    }

    // If the selected algorithm is LRU/LFU.
    else
    {
        // First we validate the given arguments.
        if (validateInitArguments(blocks_num, f_old, f_new, false))
        {
            return FAILURE_STATE;
        }
        // TODO: Set algorithm to LRU/LFU and leave buffer as is.
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

/**
 * @brief File open operation. Receives a path for a file, opens it,
 *        and returns an ID for accessing the file later. The ID in this
 *        implementation is the file descriptor.
 * @param pathname The path to the file that will be opened.
 * @return On success the file descriptor is returned, -1 on failure.
 */
int CacheFS_open(const char *pathname)
{
    // Receive the full path of the file.
    char *realPath = realpath(pathname, nullptr);
    if (realPath == nullptr)
    {
        // If the real path procedure failed.
        return FAILURE_STATE;
    }

    // Check if the given file to open is in the /tmp directory.
    // TODO: Uncomment this.
//    if (validatePath(realPath))
//    {
//        // Release memory allocated by 'realpath()'.
//        free(realPath);
//        realPath = nullptr;
//        return FAILURE_STATE;
//    }

    // Open the file in the given path.
    int fd = open(realPath, O_RDONLY | O_DIRECT | O_SYNC);
    if (fd < SUCCESS_STATE)
    {
        // In case open failed.
        // Release memory allocated by 'realpath()'.
        free(realPath);
        realPath = nullptr;
        return FAILURE_STATE;
    }

    // Insert the file to the open files data container with it's path.
    openFiles[fd] = std::string(realPath);

    // Release memory allocated by 'realpath()'.
    free(realPath);
    realPath = nullptr;

    return fd;
}

/**
 * @brief File close operation.
 * @param file_id The file ID to close.
 * @return 0 in case of success, negative value in case of failure.
 */
int CacheFS_close(int file_id)
{
    if (isFileOpen(file_id))
    {
        // In case of invalid file ID to close.
        return FAILURE_STATE;
    }

    // Close the file stream.
    if (close(file_id))
    {
        return FAILURE_STATE;
    }

    // Remove this file from the open files container.
    openFiles.erase(file_id);

    // TODO: Release all resources of blocks of this file.

    return SUCCESS_STATE;
}

// TODO: Doxygen.
static blocksVector::iterator findBlock(const std::string filePath,
                                        const size_t blockNumber)
{
    auto blockIterator = blocks.begin();
    for ( ; blockIterator != blocks.end(); ++blockIterator)
    {
        if (filePath.compare(blockIterator->filePath))
        {
            // If the file path is not equal to the block of file.
            continue;
        }
        // The current block is from the requested file path.
        // Now we check if the block itself is the requested block.
        if (blockNumber == blockIterator->blockNumber)
        {
            // The block is in the buffer cache.
            break;
        }
    }
    return blockIterator;
}

// TODO: Doxygen.
static size_t calculateBlocks(const size_t start, const size_t end,
                              const size_t endRemainder)
{
    return (end - start) + (endRemainder == BLOCK_START_ALIGNMENT ? 0 : 1);
}

// TODO: Doxygen.
int CacheFS_pread(int file_id, void *buf, size_t count, off_t offset)
{
    if (isFileOpen(file_id))
    {
        // In case of invalid file ID to read from.
        return FAILURE_STATE;
    }

    // Get the path of the file to read.
    std::string filePath = openFiles[file_id];

    // TODO: Check if the block contains data is in the cache.
    // TODO: If the cache does not contains the block, calculate the block with data.
    // TODO: Insert block to cache, check if cache full or not.
    // TODO: If cache is full, remove by policy.
    // TODO: Check if we reach the end of file.
    // TODO: Move memory from cache to buf.

    int bytesRead = 0;

    off_t fileSize = lseek(file_id, 0, SEEK_END);
    std::cout << "File Size: " << fileSize << std::endl;

    // Retrieve the start and end block numbers in the file to read.
    size_t startBlock = offset / blockSize;
    size_t endBlock = (count + offset) / blockSize;
    // Calculate the remainder to read from the start block and from the end.
    size_t startRemainder = offset % blockSize;
    size_t endRemainder = (count + offset) % blockSize;
    // Calculate the total amount of blocks to read to the cache.
    size_t blocksToRead = calculateBlocks(startBlock, endBlock, endRemainder);


    std::cout << "Start Block: " << startBlock << std::endl;
    std::cout << "End Block: " << endBlock << std::endl;
    std::cout << "Start Remainder: " << startRemainder << std::endl;
    std::cout << "End Remainder: " << endRemainder << std::endl;
    std::cout << "Blocks to Read: " << blocksToRead << std::endl;


    size_t blockNumber = startBlock;
    size_t offsetInBlock = startRemainder;
    for (unsigned int i = 0; i < blocksToRead; ++i)
    {
        std::cout << "Current Block Number: " << blockNumber << std::endl;
        // First check if this block is already in the cache buffer.
        auto blockIterator = findBlock(filePath, blockNumber);
        if (blockIterator != blocks.end())
        {
            // Block is already in the buffer cache.
            std::cout << "FOUND BLOCK IN CACHE" << std::endl;
            // Copy data from the block in the cache to the given buffer.
//            memcpy(buf, blockIterator->bufferCache + offsetInBlock, count);
//            bytesRead +=
        }
        else
        {
            // Block is not in the buffer cache.
            // Check if there is place in the cache for a new block.
            // If there is we simply read the block from the file and allocate
            // it in the cache. If the cache is full we have to remove a block
            // according to the current selected policy.
            if (activeBlocks < numberOfBlocks)
            {
                // There is a place for a new block so we create a new block
                // and allocate it in the buffer cache.
                Block block;
                block.filePath = filePath;
                block.blockNumber = blockNumber;
                block.buffer = (char *) aligned_alloc(blockSize, blockSize);
                if (block.buffer == nullptr)
                {
                    // In case the memory allocation failed.
                    return FAILURE_STATE;
                }
                // Read to this block's buffer the block to read from the file.
                pread(file_id, block.buffer, blockSize, blockNumber * blockSize);
                blocks.push_back(block);
            }
            blockNumber++;
        }
    }

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
