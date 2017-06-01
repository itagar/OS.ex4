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
    int resInit = CacheFS_init(2, FBR, 0.3333, 0.5);
    if (resInit < 0)
    {
        std::cerr << "Init Error!" << std:: endl;
        exit(1);
    }
    int fd1 = CacheFS_open("TestFile");
    int fd2 = CacheFS_open("TestFile");
    std::cout << "----  FIRST READ  ----" << std::endl;
    CacheFS_pread(fd1, buf, 150, 0);
    std::cout << "----  SECOND READ  ----" << std::endl;
    CacheFS_pread(fd1, buf, 8142, 150);
    std::cout << "----  THIRD READ  ----" << std::endl;
    CacheFS_pread(fd1, buf, 4196, 8200);
    CacheFS_close(fd1);
    CacheFS_close(fd2);
    CacheFS_destroy();
    std::cout << buf << std::endl;
    delete[] buf;
}