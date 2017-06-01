/**
 * @file MyTest.cpp
 * @author Itai Tagar <itagar>
 *
 * @brief Test for CacheFS.
 */


#include <iostream>
#include "CacheFS.h"


int main(int argc, char *argv[])
{
    std::cout << "Starting LRU Test..." << std::endl;
    char *buf = new char[10000];
    CacheFS_init(2, LRU, 0.3333, 0.5);
    int fd1 = CacheFS_open("TheBoyWhoLived");
    int fd2 = CacheFS_open("TheBoyWhoLived");
    CacheFS_pread(fd2, buf, 4096, 0);
    CacheFS_close(fd2);
    CacheFS_pread(fd1, buf + 4096, 0, 4096);
//    CacheFS_pread(fd1, buf + 4150, 2500, 7500);
    CacheFS_close(fd1);
    CacheFS_destroy();
    std::cout << buf << std::endl;
    delete[] buf;
}