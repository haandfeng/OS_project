#include "Disk.h"
#include <iostream>
#include <string.h>
#include <vector>
using namespace std;
// 定义一个全局的 SleepLock 对象
SleepLock globalSleepLock;
int main() 
{
	Disk disk;
	disk.run();
}