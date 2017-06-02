/**
 * @file LRUTest.cpp
 * @author Itai Tagar <itagar>
 *
 * @brief Test for CacheFS using LRU.
 */


#include <iostream>
#include <fstream>
#include <cstring>
#include "CacheFS.h"


#define BUFFER_SIZE 100000
#define CHAPTER_1 "/tmp/TheBoyWhoLived"
#define NOT_TMP "TheBoyWhoLived"


int main(int argc, char *argv[])
{
    for (int i = 0; i < 3; ++i)
    {
        std::cout << "Starting FBR Test..." << i << std::endl;
        try
        {
            char *buf = new char[BUFFER_SIZE];
            if (!CacheFS_init(10, FBR, 1, 0))
            {
                std::cerr << "You should check the partitions" << std::endl;
                return -1;
            }
            if (CacheFS_init(4, FBR, 0.5, 0.5))
            {
                std::cerr << "Error in legal CacheFS_init" << std::endl;
                return -1;
            }

            // Open the same file twice.
            int f1 = CacheFS_open(CHAPTER_1);
            int f2 = CacheFS_open(CHAPTER_1);
            if (f1 < 0 || f2 < 0)
            {
                std::cerr << "You have a problem in opening legal files" << std::endl;
                return -1;
            }
            // Open file that not in /tmp.
            int f3 = CacheFS_open(NOT_TMP);
            if (f3 >= 0)
            {
                std::cerr << "You should not support files that are not in /tmp" << std::endl;
                return -1;
            }

            // Read from the second file the first block.
            if (CacheFS_pread(f2, buf, 150, 0) != 150)
            {
                std::cerr << "Error in CacheFS_pread while legally reading" << std::endl;
                return -1;
            }
            // Close the second file.
            if (CacheFS_close(f2) != 0)
            {
                std::cerr << "Error in CacheFS_close while closing legal file" << std::endl;
                return -1;
            }
            if (CacheFS_close(f2) != -1)
            {
                std::cerr << "Error in CacheFS_close while closing illegal file" << std::endl;
                return -1;
            }

            // Read from the first file the first + second block.
            if (CacheFS_pread(f1, buf + 150, 4000, 150) != 4000)
            {
                std::cerr << "Error in CacheFS_pread while legally reading" << std::endl;
                return -1;
            }
            // Read from the illegal file.
            if (CacheFS_pread(f3, buf + 4150, 5000, 0) != -1)
            {
                std::cerr << "Error in CacheFS_pread while illegally reading" << std::endl;
                return -1;
            }
            // Read from the illegal file.
            if (CacheFS_pread(f2, buf + 4150, 5000, 0) != -1)
            {
                std::cerr << "Error in CacheFS_pread while illegally reading" << std::endl;
                return -1;
            }

            // Read from the first file chunk that is bigger then the file itself.
            if (CacheFS_pread(f1, buf + 4150, 7000, 20000) != 5832)
            {
                std::cerr << "Error in CacheFS_pread while reading with larger request" << std::endl;
                return -1;
            }

            // Read from the first file chunk that is bigger then the file itself.
            if (CacheFS_pread(f1, buf + 9982, 150, 50000) != 0)
            {
                std::cerr << "Error in CacheFS_pread while reading with large offset" << std::endl;
                return -1;
            }

            // Read from the first file chunk that is bigger then the file itself.
            if (CacheFS_pread(f1, buf + 9982, 15, 100) != 15)
            {
                std::cerr << "Error in CacheFS_pread while reading with zero count" << std::endl;
                return -1;
            }

            if (CacheFS_pread(f1, buf, 7000, 0) != 7000)
            {
                std::cerr << "Error in CacheFS_pread while legally reading" << std::endl;
                return -1;
            }

            // Read from the first file chunk that is bigger then the file itself.
            if (CacheFS_pread(f1, buf + 7000, 50000, 7000) != 18832)
            {
                std::cerr << "Error in CacheFS_pread while reading with larger request" << std::endl;
                return -1;
            }

            // Close the first file.
            if (CacheFS_close(f1) != 0)
            {
                std::cerr << "Error in CacheFS_close while closing legal file" << std::endl;
                return -1;
            }

            CacheFS_destroy();
//        std::cout << buf << std::endl;
            delete[] buf;
            buf = nullptr;
        }
        catch (std::bad_alloc &exception)
        {
            throw exception;
        }
    }
    return 0;
}