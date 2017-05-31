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

    CacheFS_init(100, FBR, 0.3333, 0.5);
    int fd1 = CacheFS_open("TestFile");
    int fd2 = CacheFS_open("TestFile");
    CacheFS_pread(fd1, buf, 150, 4000);
    CacheFS_close(fd1);
    CacheFS_close(fd2);
    CacheFS_destroy();

    std::cout << buf << std::endl;

    delete[] buf;
}