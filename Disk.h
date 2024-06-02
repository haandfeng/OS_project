#pragma once
#include <iostream>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <vector>
#include <string>
#include <stack>
#include <regex>
#include <memory>
#include <mutex>
#include <condition_variable>
#pragma warning(disable:4996)


// measured in "BYTES" 

#define INITIAL_DISK_SIZE  16 * 1024 * 1024 // 磁盘空间 or memery的空间？ size: 16MB

#define INITIAL_BLOCK_SIZE  1 * 1024  // One Block 1KB
#define INITIAL_BLOCK_NUM  1024 // 没用过，直接init 其他block的number了
#define INITIAL_SUPERBLOCK_SIZE  1 * 1024 // suoper block大小
// #define INITIAL_BITMAP_SIZE  2 * 1024
#define INITIAL_INODE_NUMBER  4096 // Inode数量
#define INITIAL_DATA_BLOCK_NUMBER 2040 * 8 // 数据块数量
#define INITLAL_FREEPTR  16541362 // 没用过
#define DISK_PATH "disk.dat"
#define NUM_INDIRECT_ADDRESSES 341 // 1024/3 = 341 一共有341个indirect block

#define INITIAL_INODE_SIZE 128 // inode size 
// measured in "BITS"
// #define BITMAP_RESERVE_BITS 3
// others
// #define MAXIMUM_ABSOLUTE_FILENAME_LENGTH 768
// #define MAXIMUM_FILE_PER_DIRECTORY 128

#define MAXIMUM_FILENAME_LENGTH 62
#define DIRECT_ADDRESS_NUMBER 10 // I-node Direct address 
// measured in "BYTES" 
#define MAXIMUM_FILE_SIZE DIRECT_ADDRESS_NUMBER * INITIAL_BLOCK_SIZE + 1 * NUM_INDIRECT_ADDRESSES * INITIAL_BLOCK_SIZE

int fileSeek(FILE*, long, int, bool=false);
FILE* fileOpen(const char*, const char*, bool=false);
size_t fileRead(void*, size_t, size_t, FILE*, bool=false);
size_t fileWrite(const void*, size_t, size_t, FILE*, bool= false);
int filePutCharacter(int, FILE*);

const char magic_number[] = "xtyshrfs";

// Address size 24bit => 3 byte


class SleepLock {
public:
    SleepLock();  // Constructor
    ~SleepLock(); // Destructor

    void wait();  // Wait for the condition
    void notify();  // Notify one waiting thread
    void notifyAll();  // Notify all waiting threads

private:
    std::mutex mtx; // mtx 用于保护对 ready 变量和 std::condition_variable 的访问。
    std::condition_variable cv; //是一个条件变量，用于线程间的等待和通知机制。条件变量允许一个或多个线程等待某个条件，并由其他线程通知这些等待线程条件已经满足。
    bool ready;
};


class RWLock {
public:
    RWLock();
    ~RWLock();

    void lockRead();
    void unlockRead();
    void lockWrite();
    void unlockWrite();

private:
    std::mutex mtx;
    std::condition_variable cv;
    int readers;
    bool writer;
};

struct Address
{
	// 3 byte
	unsigned char addr[3];

	Address() {
		memset(addr, 0, sizeof addr); // initial address (in the mem) to 0
	}
	Address(int addrInt) {
		from_int(addrInt); // set address from int 
	}
	int to_int() const {
		return (int)addr[0] * 1 + (int)addr[1] * 256 + (int)addr[2] * 256 * 256;  // set address to int  2^8=256
	}
	// Compare to 10
	void from_int(int addrInt) {
		addr[0] = addrInt % 256;
		addrInt /= 256;
		addr[1] = addrInt % 256;
		addrInt /= 256;
		addr[2] = addrInt % 256;
	}
	Address block_addr() {
		return Address((this->to_int() / 1024) << 10);  // this->to_int() / 1024 => nth block    << 10 => set offset to 0
	}
	Address offset() {
		return Address(this->to_int() % 1024);    // get offset in a block
	}
	Address operator+(const Address& a) {
		return Address(this->to_int() + a.to_int());
	}
	Address operator+(const int& a) {
		return Address(this->to_int() + a);
	}
	Address operator-(const Address& a) {
		return Address(this->to_int() - a.to_int());
	}
	Address operator-(const int& a) {
		return Address(this->to_int() - a);
	}
	bool operator==(const Address& a) {
		return this->to_int() == a.to_int();
	}
	bool operator==(const int& a) {
		return this->to_int() == a;
	}
	bool operator!=(const Address& a) {
		return !(*this == a);
	}
	bool operator!=(const int& a) {
		return !(*this == a);
	}
};

class Diskblock {  
	// can get the content from or write the content to a specific disk block
public:
	unsigned char content[INITIAL_BLOCK_SIZE];
	Diskblock() { refreshContent(); }
	void refreshContent();
	Diskblock(int); 
	Diskblock(Address); 
	void load(int,FILE* = NULL, int = INITIAL_BLOCK_SIZE);			// load a block from disk given an address to the content buffer
	void load(Address, FILE* = NULL, int = INITIAL_BLOCK_SIZE);		// load a block from disk given an address to the content buffer
	void write(int, FILE* = NULL, int = INITIAL_BLOCK_SIZE);		// write the content buffer to the specific disk block
	void write(Address, FILE* = NULL, int = INITIAL_BLOCK_SIZE);	// write the content buffer to the specific disk block

};

class IndirectDiskblock {
public:
	Address addrs[NUM_INDIRECT_ADDRESSES];  // addresses loaded from an indirect disk block
	void load(Address,FILE* =NULL);			//load addresses given an indirect block address (address is indirect block address)
	void write(Address,FILE* =NULL);		//write the pointers to the specific block(address is block address)
};

class iNode
{
public:
	unsigned fileSize;
	time_t inode_create_time;
	time_t inode_access_time;
	time_t inode_modify_time;
	// 锁
	// RWLock* lock;  // 使用原始指针管理读写锁
    // void initLock(); // 初始化锁
    // void deleteLock(); // 删除锁
	bool isDir;

	int parent;
	int inode_id;

	Address direct[DIRECT_ADDRESS_NUMBER];
	Address indirect;

	iNode(unsigned fileSize, int parent, int inode_id, bool isDir = true);
    iNode();
    ~iNode();
    // 允许复制和移动
    iNode(const iNode& other);
    iNode& operator=(const iNode& other);
    iNode(iNode&& other) noexcept;
    iNode& operator=(iNode&& other) noexcept;

	void updateCreateTime();
	void updateModifiedTime();
	void updateAccessTime();
	std::string getCreateTime();
	std::string getModifiedTime();
	std::string getAccessTime();
};

class superBlock
{
public:
	superBlock();
	
	unsigned inodeNumber;				    // amount of inodes
	unsigned freeInodeNumber;				// amount of free inodes
	unsigned dataBlockNumber;				// amount of blocks
	unsigned freeDataBlockNumber;	        // amount of free blocks
	
	char inodeMap[INITIAL_INODE_NUMBER / 8];

	unsigned BLOCK_SIZE;
	unsigned INODE_SIZE;
	unsigned SUPERBLOCK_SIZE;
	unsigned TOTAL_SIZE;
	
	//int free_block_SP;					// free block stack pointer
	//int *free_block;					// free block stack

	unsigned superBlockStart;
	//unsigned inodeBitmapStart;
	//unsigned blockBitmapStart;
	unsigned inodeStart;
	unsigned blockStart;
    bool updateSuperBlock(FILE* = NULL);
	Address freeptr;
	
	int allocateNewInode(unsigned, int, Address[], Address*, FILE* = NULL, bool=true);
	bool freeInode(int, FILE* = NULL);
	iNode loadInode(short, FILE* = NULL);
	bool writeInode(iNode, FILE* = NULL);
};


class DiskblockManager {

public:
	Address freeptr;								// freeptr = super->blockStart;
	void initialize(superBlock*,FILE* =NULL);		// initialize when the disk is created
	Address alloc(FILE* =NULL);						// allocate a free block and return the free block address
	void free(Address addr,FILE* =NULL);			// recycle the unused block and push it to the stack
	void printBlockUsage(superBlock*, FILE* =NULL);	// print disk block usage
	int getFreeBlock(FILE*);						// get free block number
	int getFreeBlock(int);							// get free block number from linked list block number
	int getLinkedListBlock(FILE*);					// get number of blocks used for address linked list
	int getDedicateBlock(superBlock*,FILE*);		// get dedicated block number
};

struct fileEntry {
	char fileName[MAXIMUM_FILENAME_LENGTH];
	short inode_id;
	fileEntry() {};
	fileEntry(const char* , short);
};

struct Directory {
	std::vector<fileEntry> files;
	short findInFileEntries(const char*);
};


class Disk
{
public:
	Disk();
	~Disk();
	FILE* diskFile;
	
	
	void run();
	bool loadDisk();
	void initializeRootDirectory();
	Address allocateNewBlock(FILE* =NULL);
	bool freeBlock(Address addr, FILE* = NULL);
	bool setCurrentInode(int inode_id);

	void parse(char* str);
	// directory 操作 
	Directory readFileEntriesFromDirectoryFile(iNode);
	bool writeFileEntriesToDirectoryFile(Directory, iNode);
	int createUnderInode(iNode&, const char*, int);
	// 文件和directory操作
	short allocateResourceForNewDirectory(iNode);
	short allocateResourceForNewFile(iNode, unsigned);
	bool deleteFile(iNode);

	int parentBlockRequired(iNode&);
	bool freeBlockCheck(int);
	bool freeInodeCheck();
	int blockUsedBy(iNode&);
	int inodeUsedBy(iNode&);
	bool copy(iNode&, const char*, iNode&); // copy directory or file
	short copyFile(iNode&, iNode&);

	void listDirectory(iNode);
	void printCurrentDirectory(const char* ="\0");
	std::string getFullFilePath(iNode, const char* = "\0");
	std::string getFileName(iNode);
	bool changeDirectory(const char*);
	std::vector<std::string> stringSplit(const std::string&, const std::string&);
	short locateInodeFromPath(std::string);
	void recursiveDeleteDirectory(iNode);
	
	unsigned getDirectorySize(iNode);
	void printWelcomeInfo();
	void printHelpInfo();
private:
	superBlock super;
	DiskblockManager dbm;
	iNode currentInode;
	Diskblock db;
	std::regex fileNamePattern;
};


