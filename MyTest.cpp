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
    char *buf = new char[10000];
    CacheFS_init(2, LRU, 0.3333, 0.5);
    int fd1 = CacheFS_open("TheBoyWhoLived");
    int fd2 = CacheFS_open("TheBoyWhoLived");
    CacheFS_pread(fd2, buf, 150, 0);
    CacheFS_close(fd2);
    std::cout << "CLOSE" << std::endl;
    CacheFS_pread(fd1, buf + 150, 4000, 150);
    CacheFS_pread(fd1, buf + 4150, 2500, 7500);
    CacheFS_close(fd1);
    CacheFS_close(fd2);
    CacheFS_destroy();
//    std::cout << buf << std::endl;
    delete[] buf;
}