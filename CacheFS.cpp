// TODO: Valgrind
// TODO: Makefile
// TODO: README
// TODO: Answers + add Answers.pdf to Makefile of tar
// TODO: Check negative value in count parameter while reading.
// TODO: Check Rounding down as described in the documentation for boundaries.
// TODO: Check if in the print stat the line should end with a '.' or not.
// TODO: Compare the log file format.
// TODO: Check who close the log files.
// TODO: Check compilation warnings.


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
#include <cassert>
#include <string>
#include <cstring>
#include <iostream>
#include <map>
#include <list>
#include <algorithm>
#include <fstream>
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
 * @def INITIAL_BOUNDARY 0
 * @brief A Macro that sets the value of initial boundary value for partitions.
 */
#define INITIAL_BOUNDARY 0

/**
 * @def INITIAL_REFERENCE_COUNTER 1
 * @brief A Macro that sets the value of initial reference counter of a block.
 */
#define INITIAL_REFERENCE_COUNTER 1

/**
 * @def INITIAL_BYTES_READ 0
 * @brief A Macro that sets the value of initial bytes number while reading.
 */
#define INITIAL_BYTES_READ 0

/**
 * @def INITIAL_BLOCK_INDEX 0
 * @brief A Macro that sets the value of initial block index.
 */
#define INITIAL_BLOCK_INDEX 0

/**
 * @def INITIAL_STAT_COUNTER 0
 * @brief A Macro that sets the value of initial cache hit/miss stat counter.
 */
#define INITIAL_STAT_COUNTER 0

/**
 * @def TMP_PATH "/tmp"
 * @brief A Macro that sets the path of the tmp directory.
 */
#define TMP_PATH "/tmp"

/**
 * @def PATH_SEPARATOR '/'
 * @brief A Macro that sets the path separator character.
 */
#define PATH_SEPARATOR '/'

/**
 * @def LOG_SEPARATOR " "
 * @brief A Macro that sets a separator in the log file.
 */
#define LOG_SEPARATOR " "

/**
 * @def HIT_STAT_LOG "Hits number: "
 * @brief A Macro that sets the line of hit stat in the log file.
 */
#define HIT_STAT_LOG "Hits number: "

/**
 * @def MISS_STAT_LOG "Misses number: "
 * @brief A Macro that sets the line of miss stat in the log file.
 */
#define MISS_STAT_LOG "Misses number: "


/*-----=  Type Definitions & Enums  =-----*/


/**
 * @brief An Enum of the current section of a block in the buffer cache.
 */
enum BufferSection { NONE, NEW, MID, OLD };

/**
 * @brief Type Definition for the block size type.
 */
typedef size_t blockSize_t;

/**
 * @brief An object of a block in the buffer cache.
 *        A block holds the file path of which it came from and the block number
 *        according to this file. Also, a block holds it's own buffer in the
 *        memory. The block holds other data used by the different algorithms.
 */
typedef struct Block
{
    std::string filePath;
    size_t blockNumber;
    BufferSection section;
    int referenceCounter;
    char *buffer;
} Block;

/**
 * @brief An object of data required in the read procedure. An instance of this
 *        object represent a single call to the read function of this library.
 */
typedef struct ReadData
{
    off_t fileSize;
    size_t startBlock;
    size_t endBlock;
    size_t startRemainder;
    size_t endRemainder;
    size_t blocksToRead;
} ReadData;

/**
 * @brief Type Definition for a list of blocks type.
 */
typedef std::list<Block> blocksList;

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

/**
 * @brief The current amount of active blocks in the buffer cache.
 */
int activeBlocks = INITIAL_NUMBER_OF_BLOCKS;

/**
 * @brief The total amount of blocks which can be in the buffer.
 */
int numberOfBlocks = INITIAL_NUMBER_OF_BLOCKS;

/**
 * @brief The bound index in the buffer cache for the new section.
 */
int newBound = INITIAL_BOUNDARY;

/**
 * @brief The bound index in the buffer cache for the old section.
 */
int oldBound = INITIAL_BOUNDARY;

/**
 * @brief Counter for the number of cache hits in the library run.
 */
int cacheHitCounter = INITIAL_STAT_COUNTER;

/**
 * @brief Counter for the number of cache miss in the library run.
 */
int cacheMissCounter = INITIAL_STAT_COUNTER;

/**
 * @brief The current cache policy algorithm used by this library.
 */
cache_algo_t cachePolicy;

/**
 * @brief A Map which holds to open files during the run of this library.
 */
filesMap openFiles;

/**
 * @brief A list of all the blocks which are available in the buffer cache.
 */
blocksList blocks;


/*-----=  Misc. Functions  =-----*/


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

/**
 * @brief Release all resources of the blocks that are currently in the buffer.
 */
static void releaseAllBlocks()
{
    for (auto i = blocks.begin(); i != blocks.end(); ++i)
    {
        free(i->buffer);
        i->buffer = nullptr;
    }
}


/*-----=  Init Functions  =-----*/


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
        return SUCCESS_STATE;
    }
    return FAILURE_STATE;
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
 *        algorithm is FBR.
 * @param blocks_num The number of blocks in the buffer cache.
 * @param cache_algo The cache algorithm that will be used.
 * @param f_old The percentage of blocks in the old partition (rounding down)
 *              relevant in FBR algorithm only
 * @param f_new The percentage of blocks in the new partition (rounding down)
 *              relevant in FBR algorithm only
 * @return 0 in case of validation success, -1 in case of failure.
 */
static int validateInitArguments(int const blocks_num, cache_algo_t cache_algo,
                                 double const f_old, double const f_new)
{
    // Check that the number of blocks is a positive number.
    if (blocks_num < MINIMUM_BLOCK_NUMBER)
    {
        return FAILURE_STATE;
    }

    // If the selected algorithm is FBR we have to check the values
    // of f_new and f_old as well.
    if (cache_algo == FBR)
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
 * @brief Resets all the library data to the initial values and clears all the
 *        containers in order to start a fresh run of this library.
 */
static void resetLibraryData()
{
    blockSize = getBlockSize();
    activeBlocks = INITIAL_NUMBER_OF_BLOCKS;
    numberOfBlocks = INITIAL_NUMBER_OF_BLOCKS;
    newBound = INITIAL_BOUNDARY;
    oldBound = INITIAL_BOUNDARY;
    cacheHitCounter = INITIAL_STAT_COUNTER;
    cacheMissCounter = INITIAL_STAT_COUNTER;
    openFiles = filesMap();
    blocks = blocksList();
}

/**
 * @brief Set the new and old boundaries in the buffer cache when FBR is the
 *        selected algorithm.
 * @param blocks_num The total number of blocks in the buffer cache.
 * @param f_old The percentage of blocks in the old partition (rounding down).
 * @param f_new The percentage of blocks in the new partition (rounding down).
 */
static void setBoundaries(int const blocks_num, double const f_old,
                          double const f_new)
{
    int oldBlocks = (int)(blocks_num * f_old);
    int newBlocks = (int)(blocks_num * f_new);
    int midBlocks = blocks_num - newBlocks - oldBlocks;
    // Set the boundaries data of the library.
    newBound = INITIAL_BOUNDARY + newBlocks;
    oldBound = newBound + midBlocks;
}


/*-----=  Open Functions  =-----*/


/**
 * @brief Validate that the given file path is in the 'tmp' directory.
 * @param realPath The file path to validate.
 * @return 0 if the path is a valid path, otherwise returns -1.
 */
static int validatePath(const char *realPath)
{
    assert(realPath != nullptr);
    if (strlen(realPath) < strlen(TMP_PATH))
    {
        return FAILURE_STATE;
    }

    // Check that the given path starts with '/tmp', i.e. the path is in the tmp
    // folder in the file system.
    unsigned int charIndex = 0;
    for ( ; charIndex < strlen(TMP_PATH); ++charIndex)
    {
        assert(charIndex < strlen(realPath));
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


/*-----=  Cache Hit Functions  =-----*/


/**
 * @brief Update the buffer cache in case of a cache hit by LRU policy.
 *        The update takes the given block that caused the cache hit and
 *        place it at the top of the list.
 * @param hitIterator Iterator to the block that caused the cache hit.
 */
static void cacheHitLRU(blocksList::iterator &hitIterator)
{
    assert(hitIterator != blocks.end());
    blocks.splice(blocks.begin(), blocks, hitIterator);
}

/**
 * @brief Update the buffer cache in case of a cache hit by LFU policy.
 *        The update takes the given block that caused the cache hit and
 *        increments it's reference counter by one.
 * @param hitIterator Iterator to the block that caused the cache hit.
 */
static void cacheHitLFU(blocksList::iterator &hitIterator)
{
    assert(hitIterator != blocks.end());
    (hitIterator->referenceCounter)++;
}

/**
 * @brief Update the buffer cache in case of a cache hit by FBR policy.
 *        The update takes the given block that caused the cache hit and
 *        place it at the top of the list. Also, if the block was not in the
 *        NEW section the update increments it's reference counter by one.
 * @param hitIterator Iterator to the block that caused the cache hit.
 */
static void cacheHitFBR(blocksList::iterator &hitIterator)
{
    assert(hitIterator != blocks.end());
    if (hitIterator->section != NEW)
    {
        cacheHitLFU(hitIterator);
    }
    cacheHitLRU(hitIterator);
}

/**
 * @brief Generic function that handles cache hit situation.
 * @param hitIterator Iterator to the block that caused the cache hit.
 */
static void cacheHitPolicy(blocksList::iterator &hitIterator)
{
    assert(hitIterator != blocks.end());
    switch (cachePolicy)
    {
        case FBR:
            cacheHitFBR(hitIterator);
            break;
        case LFU:
            cacheHitLFU(hitIterator);
            break;
        case LRU:
            cacheHitLRU(hitIterator);
            break;
    }
}


/*-----=  Block Removal Functions  =-----*/


/**
 * @brief Comparator of two block object which compares them by their
 *        reference counter.
 * @param lhs The first block to compare.
 * @param rhs The second block to compare.
 * @return true if lhs reference is smaller then the second, false otherwise.
 */
static bool refComparator(const Block &lhs, const Block &rhs)
{
    return lhs.referenceCounter < rhs.referenceCounter;
}

/**
 * @brief Select the block to remove by the LRU policy.
 *        The selected block will be the block at the end of the list.
 * @return Iterator to the block to remove.
 */
static blocksList::iterator removeBlockLRU()
{
    return (--blocks.end());
}

/**
 * @brief Select the block to remove by the LFU policy.
 *        The selected block will be the block with the lowest reference count.
 * @return Iterator to the block to remove.
 */
static blocksList::iterator removeBlockLFU()
{
    return std::min_element(blocks.begin(), blocks.end(), refComparator);
}

/**
 * @brief Select the block to remove by the FBR policy.
 *        The selected block will be the block with the lowest reference count
 *        from the OLD section. If there are more then one, it will return
 *        the least recently used block.
 * @return Iterator to the block to remove.
 */
static blocksList::iterator removeBlockFBR()
{
    // First we reach the OLD section in the list.
    int index = INITIAL_BLOCK_INDEX;
    auto oldIterator = blocks.begin();
    for ( ; oldIterator != blocks.end(); ++oldIterator)
    {
        if (index == oldBound)
        {
            break;  // We are in the start of the old section.
        }
        index++;
    }
    assert(oldIterator != blocks.end());
    // Then we find the block in this section with the lowest reference count
    // and take the least recently used one in case of several.
    auto blockToRemove = oldIterator;
    while (oldIterator != blocks.end())
    {
        if (!refComparator(*blockToRemove, *oldIterator))
        {
            blockToRemove = oldIterator;
        }
        ++oldIterator;
    }

    return blockToRemove;
}

/**
 * @brief Generic function that handles block removal.
 * @return Iterator to the block to remove.
 */
static blocksList::iterator removeBlockPolicy()
{
    switch (cachePolicy)
    {
        case FBR:
            return removeBlockFBR();
        case LFU:
            return removeBlockLFU();
        case LRU:
            return removeBlockLRU();
        default:
            return blocks.end();
    }
}

/**
 * @brief Remove a block from the buffer and release it's resources.
 *        The function also updates the amount of active blocks in the cache.
 * @param blockToRemove Iterator to the block to remove.
 */
static void removeBlock(blocksList::iterator &blockToRemove)
{
    assert(blockToRemove != blocks.end());
    free(blockToRemove->buffer);
    blockToRemove->buffer = nullptr;
    blocks.erase(blockToRemove);
    activeBlocks--;
}


/*-----=  Read Functions  =-----*/


/**
 * @brief Determine if the given arguments for the read procedure are valid.
 *        A file ID is invalid if it wasn't returned by the open procedure of
 *        this library or it was already closed.
 *        The buf is invalid if it is NULL.
 *        The offset is invalid if it is negative.
 * @param file_id The file ID to read from.
 * @param buf The buffer to read the data into.
 * @param offset The offset in the file to start the reading.
 * @return 0 in case of validation success, -1 in case of failure.
 */
static int validateReadArguments(int const file_id, void *buf,
                                 off_t const offset)
{
    if (isFileOpen(file_id))
    {
        // In case of invalid file ID to read from.
        return FAILURE_STATE;
    }
    if (buf == nullptr)
    {
        // In case of invalid buffer.
        return FAILURE_STATE;
    }
    if (offset < BLOCK_START_ALIGNMENT)
    {
        // In case of invalid offset.
        return FAILURE_STATE;
    }
    return SUCCESS_STATE;
}

/**
 * @brief Search in the cache buffer for a given block number in a given
 *        file path. The function search in the blocks list which represent
 *        the active blocks in the cache buffer.
 * @param filePath The file which contains the block to search.
 * @param blockNumber The block number in the file to search.
 * @return An iterator to the desired block in the block list.
 *         If the block doesn't exists the iterator will be the end of the list.
 */
static blocksList::iterator findBlock(const std::string filePath,
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

/**
 * @brief Calculate the number of blocks to read according to the given
 *        data which received by the read function.
 *        If the remainder of a block is none, i.e. there is no remainder in
 *        the final block this means that we don't really need to read it.
 *        In this case we don't include it in our calculations.
 * @param start The start block number.
 * @param end The end block number.
 * @param endRemainder The remainder to read from the final block.
 * @return The amount of blocks to read from a file.
 */
static size_t calculateBlocks(const size_t start, const size_t end,
                              const size_t endRemainder)
{
    return (end - start) + (endRemainder == BLOCK_START_ALIGNMENT ? 0 : 1);
}

/**
 * @brief Calculate to current amount of bytes to read from a single block
 *        according to the given bytes that are left to read and the offset
 *        in this block.
 * @param bytesToRead Amount of bytes that are left to read.
 * @param offsetInBlock The offset in the current block.
 * @return The amount of bytes to read from a current block.
 */
static size_t calculateCurrentCount(size_t const &bytesToRead,
                                    off_t const &offsetInBlock)
{
    if (offsetInBlock + bytesToRead < blockSize)
    {
        return bytesToRead;
    }
    return blockSize - offsetInBlock;
}

/**
 * @brief Allocate a new block in the buffer cache.
 *        This function receives a Block instance and set up it's data according
 *        to the given arguments. It then allocate memory to it's buffer which
 *        will be it's buffer cache, and store the requested block data in it.
 * @param block The block instance to set.
 * @param fileID The file ID to read a block from.
 * @param blockNumber The block number to read to the buffer.
 * @return 0 if the path is a valid path, otherwise returns -1.
 */
static int allocateBlock(Block &block, const int fileID,
                         const size_t blockNumber)
{
    // Set the data of this block.
    block.filePath = openFiles[fileID];
    block.blockNumber = blockNumber;
    block.section = ((cachePolicy == FBR) ? NEW : NONE);
    block.referenceCounter = INITIAL_REFERENCE_COUNTER;

    // Allocate memory of the buffer to this block.
    block.buffer = (char *) aligned_alloc(blockSize, blockSize);
    if (block.buffer == nullptr)
    {
        // In case the memory allocation failed.
        return FAILURE_STATE;
    }

    // Read to this block's buffer the block to read from the file.
    if (pread(fileID, block.buffer, blockSize, blockNumber * blockSize) < 0)
    {
        // If failed we need to free the resources of the buffer.
        free(block.buffer);
        block.buffer = nullptr;
        return FAILURE_STATE;
    }

    return SUCCESS_STATE;
}

/**
 * @brief Setup data in the ReadData object of a current read call.
 * @param readData The ReadDAta object to set.
 * @param fileSize The size of the file.
 * @param bytesToRead The actual bytes to read.
 * @param offset The requested offset by a read call.
 */
static void setupReadData(ReadData &readData, off_t const fileSize,
                          size_t const bytesToRead, off_t const offset)
{
    // Set the size of the file in bytes.
    readData.fileSize = fileSize;
    // Set the start and end block numbers in the file to read.
    readData.startBlock = offset / blockSize;
    readData.endBlock = (bytesToRead + offset) / blockSize;
    // Set the remainder to read from the start block and from the end.
    readData.startRemainder = offset % blockSize;
    readData.endRemainder = (bytesToRead + offset) % blockSize;
    // Set the total amount of blocks to read to the cache from this file.
    readData.blocksToRead = calculateBlocks(readData.startBlock,
                                            readData.endBlock,
                                            readData.endRemainder);
}

/**
 * @brief Set the actual amount of bytes to be read according to the requested
 *        count, offset and the file size. In case of large offset or trivial
 *        count the bytes to read is empty. If the requested amount is larger
 *        then the actual file size we return only the amount available in
 *        the file.
 * @param count The given count of bytes to read.
 * @param offset The given offset to start in the file.
 * @param fileSize The file size of the file to read.
 * @return The actual valid amount of bytes to read.
 */
static size_t getBytesToRead(size_t const count, off_t const offset,
                             off_t const fileSize)
{
    // If there is nothing asked to read by count, or if the given offset
    // is larger then the actual file size then bytes to read is simply nothing.
    if (count <= INITIAL_BYTES_READ || offset > fileSize)
    {
        return INITIAL_BYTES_READ;
    }
    // If the requested amount of data to read exceeds the actual file size
    // we read only the available data.
    if (offset + count > fileSize)
    {
        size_t overhead = (offset + count) - fileSize;
        return count - overhead;
    }
    // In a normal case, bytes to read is the given count.
    return count;
}

/**
 * @brief Updates the section fields of the blocks in the buffer cache.
 *        Called only if the cache policy is FBR.
 */
static void updateSections()
{
    assert(cachePolicy == FBR);
    int index = INITIAL_BLOCK_INDEX;
    for (auto i = blocks.begin(); i != blocks.end(); ++i)
    {
        if (index < newBound)
        {
            // We are in the new section.
            i->section = NEW;
        }
        else if (index >= newBound && index < oldBound)
        {
            // We are in the mid section.
            i->section = MID;
        }
        else
        {
            // We are in the old section.
            i->section = OLD;
        }
        index++;
    }
}

/**
 * @brief An helper function for the read procedure of this library.
 *        This function analyze and calculate all the read data requested by
 *        the user and manage the buffer cache accordingly. It also takes into
 *        account the cache policy selected for this library and in the
 *        required situations it take actions according to the policy
 *        (e.g. remove item/cache hit...)
 * @param file_id The file ID to read from.
 * @param buf The buffer to read the data into.
 * @param count The amount of bytes to read from the file.
 * @param offset The offset in the file to start the reading.
 * @return On success, non negative value represents the number of bytes read.
 *         On failure return -1.
 */
static int readHelper(int const file_id, void *buf, size_t const count,
                      off_t const offset)
{
    std::cout << "/*=-- Current Read Data --=*/" << std::endl;
    std::cout << "Requested to read " << count << " bytes starting from " << offset << " offset." << std::endl;

    // Get data of the file to read.
    const std::string filePath = openFiles[file_id];
    const off_t fileSize = lseek(file_id, 0, SEEK_END);

    // Set variables for this current read call.
    int bytesRead = INITIAL_BYTES_READ;
    size_t bytesToRead = getBytesToRead(count, offset, fileSize);
    if (bytesToRead == INITIAL_BYTES_READ)
    {
        // If there is actually nothing to read.
        return bytesRead;
    }
    // Set a variable for the current buffer to read from into the given buf.
    char *currentBuffer = nullptr;

    // Setup the read data.
    ReadData readData;
    setupReadData(readData, fileSize, bytesToRead, offset);
    std::cout << "-READ DATA- " << std::endl;
    std::cout << "File Size: " << readData.fileSize << std::endl;
    std::cout << "Start Block: " << readData.startBlock << std::endl;
    std::cout << "End Block: " << readData.endBlock << std::endl;
    std::cout << "Start Remainder: " << readData.startRemainder << std::endl;
    std::cout << "End Remainder: " << readData.endRemainder << std::endl;
    std::cout << "Blocks to Read: " << readData.blocksToRead << std::endl;

    // The current number of block in the file to read.
    size_t currentBlockNumber = readData.startBlock;
    // The current offset in the current block.
    off_t offsetInBlock = readData.startRemainder;
    // Iterate all the blocks we need to read to the cache from the file.
    for (unsigned int i = 0; i < readData.blocksToRead; ++i)
    {
        // Calculate the current amount of bytes to read from this block.
        size_t currentCount = calculateCurrentCount(bytesToRead, offsetInBlock);

        std::cout << "Current Block Number: " << currentBlockNumber << std::endl;
        std::cout << "Bytes to Read: " << bytesToRead << std::endl;
        std::cout << "Current Offset: " << offsetInBlock << std::endl;
        std::cout << "Current Count: " << currentCount << std::endl;

        // Check if this block is already in the cache buffer.
        auto blockIterator = findBlock(filePath, currentBlockNumber);
        if (blockIterator != blocks.end())
        {
            std::cout << "Found This Block in the Cache!" << std::endl;
            // Block is already in the buffer cache so we set current
            // buffer accordingly.
            currentBuffer = blockIterator->buffer + offsetInBlock;
            // Update buffer by the cache hit policy of the selected algorithm.
            cacheHitPolicy(blockIterator);
            // Update the cache hit counter.
            cacheHitCounter++;
        }
        else
        {
            // Block is not in the buffer cache.
            // Check if there is place in the cache for a new block.
            assert(activeBlocks <= numberOfBlocks);
            if (activeBlocks == numberOfBlocks)
            {
                // If the cache is full we have to remove a block according
                // to the selected policy.
                auto blockToRemove = removeBlockPolicy();
                assert(blockToRemove != blocks.end());
                std::cout << "Removing Block: <" << blockToRemove->blockNumber << "," << blockToRemove->referenceCounter << "," << blockToRemove->section << ">" << std::endl;
                removeBlock(blockToRemove);
            }

            // There is a place for a new block so we create a new block
            // and allocate it in the buffer cache.
            Block block;
            if (allocateBlock(block, file_id, currentBlockNumber))
            {
                return FAILURE_STATE;
            }
            blocks.push_front(block);
            // Set current buffer accordingly.
            currentBuffer = block.buffer + offsetInBlock;
            // Update the amount of blocks in the cache.
            activeBlocks++;
            // Update the cache miss counter.
            cacheMissCounter++;
        }

        // Copy data from the buffer block in the cache to the given buf.
        memcpy(buf, currentBuffer, currentCount);

        // Update data for the next block reading.
        buf += currentCount;
        bytesRead += currentCount;
        bytesToRead -= currentCount;
        offsetInBlock = (offsetInBlock + currentCount) % blockSize;
        if (cachePolicy == FBR)
        {
            updateSections();
        }
        currentBlockNumber++;

        std::cout << "--Blocks in the cache--" << std::endl;
        for (auto j = blocks.begin(); j != blocks.end(); ++j)
        {
            std::cout << "<" <<  j->blockNumber << "," << j->referenceCounter << "," << j->section << ">  ";
        }
        std::cout << std::endl;
    }

    std::cout << "Bytes Read: " << bytesRead << std::endl << std::endl;
    return bytesRead;
}


/*-----=  Print Logs Functions  =-----*/


/**
 * @brief Print the blocks in the given blocks container to the given file.
 * @param logFile The file to write the data that is printed.
 * @param blocksToPrint The blocks to print.
 */
static void printBlocks(std::ofstream &logFile, const blocksList &blocksToPrint)
{
    for (auto i = blocksToPrint.begin(); i != blocksToPrint.end(); ++i)
    {
        logFile << i->filePath << LOG_SEPARATOR << i->blockNumber << std::endl;
    }
}

/**
 * @brief Print current cache state according to LRU policy.
 * @param logFile The file to write the data that is printed.
 */
static void printCacheLRU(std::ofstream &logFile)
{
    // Print the cache state by the order of the blocks in the list.
    printBlocks(logFile, blocks);
}

/**
 * @brief Print current cache state according to LFU policy.
 * @param logFile The file to write the data that is printed.
 */
static void printCacheLFU(std::ofstream &logFile)
{
    // Print the cache state by order of reference counter.
    blocksList blocksToPrint = blocks;
    blocksToPrint.sort(refComparator);
    blocksToPrint.reverse();
    printBlocks(logFile, blocksToPrint);
}

/**
 * @brief Print current cache state according to FBR policy.
 * @param logFile The file to write the data that is printed.
 */
static void printCacheFBR(std::ofstream &logFile)
{
    // Print the cache state by the order of the blocks in the list.
    printCacheLRU(logFile);
}

/**
 * @brief Generic function that handles printing the blocks in the cache.
 * @param logFile The file to write the data that is printed.
 */
static void printCachePolicy(std::ofstream &logFile)
{
    assert(logFile.good());
    switch (cachePolicy)
    {
        case FBR:
            printCacheFBR(logFile);
            break;
        case LFU:
            printCacheLFU(logFile);
            break;
        case LRU:
            printCacheLRU(logFile);
            break;
    }
}

/**
 * @brief Writes the statistics of the CacheFS to the given log file.
 * @param logFile The file to write the data that is printed.
 */
static void printStats(std::ofstream &logFile)
{
    logFile << HIT_STAT_LOG << cacheHitCounter << std::endl;
    logFile << MISS_STAT_LOG << cacheMissCounter << std::endl;
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
    // First we validate the given arguments.
    if (validateInitArguments(blocks_num, cache_algo, f_old, f_new))
    {
        return FAILURE_STATE;
    }

    // Reset the library data for this current run.
    resetLibraryData();

    // Set the library data by the given arguments.
    numberOfBlocks = blocks_num;
    cachePolicy = cache_algo;

    // Organize the buffer cache according to the policy.
    if (cachePolicy == FBR)
    {
        setBoundaries(blocks_num, f_old, f_new);
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
    releaseAllBlocks();
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
    if (validatePath(realPath))
    {
        // Release memory allocated by 'realpath()'.
        free(realPath);
        realPath = nullptr;
        return FAILURE_STATE;
    }

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
    return SUCCESS_STATE;
}

/**
 * @brief Read data from an open file.
 *        Read should return exactly the number of bytes requested except
 *        on EOF or error.
 * @param file_id The file ID to read from.
 * @param buf The buffer to read the data into.
 * @param count The amount of bytes to read from the file.
 * @param offset The offset in the file to start the reading.
 * @return On success, non negative value represents the number of bytes read.
 *         On failure return -1.
 */
int CacheFS_pread(int file_id, void *buf, size_t count, off_t offset)
{
    // Validate the arguments of this call.
    if (validateReadArguments(file_id, buf, offset))
    {
        return FAILURE_STATE;
    }
    return readHelper(file_id, buf, count, offset);
}

/**
 * @brief This function writes the current state of the cache to a file.
 *        The function writes a line for every block that was used in the cache
 *        (meaning, each block with at least one access).
 *        Each line contains the following values separated by a single space.
 *          1.  Full path of the file
 *          2.  The number of the block. Pay attention: this is not the number
 *              in the cache, but the enumeration within the file itself,
 *              starting with 0 for the first block in each file.
 *        For LRU and LFU The order of the entries is from the last block
 *        that will be evicted from the cache to the first (next) block that
 *        will be evicted. For FBR use the LRU order (the order of the stack).
 * @param log_path A path of the log file. A valid path is either: a path to an
 *                 existing log file or a path to a new file
 *                 (under existing directory).
 * @return 0 in case of success, -1 in case of failure.
 */
int CacheFS_print_cache(const char *log_path)
{
    // Open log file stream.
    std::ofstream logFile(log_path, std::ios_base::app | std::ios_base::out);
    if (logFile.fail())
    {
        return FAILURE_STATE;
    }
    // Print cache state.
    printCachePolicy(logFile);
    return SUCCESS_STATE;
}

/**
 * @brief This function writes the statistics of the CacheFS to a file.
 *        This function writes exactly the following lines:
 *          Hits number: HITS_NUM.
 *          Misses number: MISS_NUM.
 *        Where HITS_NUM is the number of cache-hits, and MISS_NUM is the
 *        number of cache-misses. A cache miss counts the number of
 *        fetched blocks from the disk. A cache hit counts the number of
 *        required blocks that were already stored in the cache
 *        (and therefore we didn't fetch them from the disk again).
 * @param log_path A path of the log file. A valid path is either: a path to an
 *                 existing log file or a path to a new file
 *                 (under existing directory).
 * @return 0 in case of success, -1 in case of failure.
 */
int CacheFS_print_stat(const char *log_path)
{
    // Open log file stream.
    std::ofstream logFile(log_path, std::ios_base::app | std::ios_base::out);
    if (logFile.fail())
    {
        return FAILURE_STATE;
    }
    // Print cache state.
    printStats(logFile);
    return SUCCESS_STATE;
}
