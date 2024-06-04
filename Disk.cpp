#include "Disk.h"
#include <iostream>
#include <cstdio>
#include <iomanip>
#include <sstream>
#include <cmath>

using namespace std;
#pragma warning(disable:4996)

// 读写锁
RWLock::RWLock() : readers(0), writer(false) {}

RWLock::~RWLock() {}

void RWLock::lockRead() {
    std::unique_lock<std::mutex> lock(mtx);
    while (writer) {
        cv.wait(lock);
    }
    ++readers;
}

void RWLock::unlockRead() {
    std::lock_guard<std::mutex> lock(mtx);
    if (--readers == 0) {
        cv.notify_all();
    }
}

void RWLock::lockWrite() {
    std::unique_lock<std::mutex> lock(mtx);
    while (writer||readers > 0) {
        cv.wait(lock);
    }
    writer = true;
}

void RWLock::unlockWrite() {
    std::lock_guard<std::mutex> lock(mtx);
    writer = false;
    cv.notify_all();
}


// 睡眠锁
SleepLock::SleepLock() : ready(true) {}

SleepLock::~SleepLock() {}

void SleepLock::wait() {
    std::unique_lock<std::mutex> lck(mtx);
    while (!ready) {
        cv.wait(lck);
    }
    ready = false;  // Reset ready after waiting
}

void SleepLock::notify() {
    std::lock_guard<std::mutex> lck(mtx);
    ready = true;
    cv.notify_one();
}

void SleepLock::notifyAll() {
    std::lock_guard<std::mutex> lck(mtx);
    ready = true;
    cv.notify_all();
}


// 声明全局对象为外部变量
extern SleepLock globalSleepLock;






int fileSeek(FILE* file, long offSet, int fromWhere, bool error_close_require)
{
    // 调用 fseek 函数移动文件指针
    //int fseek(FILE *stream, long offset, int whence);
    /*	•	*FILE stream：
    •	指向文件的指针，表示要操作的文件流。
    •	long offset：
    •	相对于 whence 参数指定的位置的偏移量。可以是正数、负数或零。
    •	int whence：
    •	定义文件指针的新位置。可以是以下三个宏之一：
    •	SEEK_SET：文件开头。
    •	SEEK_CUR：文件指针的当前位置。
    •	SEEK_END：文件的末尾。

    返回值
    •	成功时，fseek 返回 0。
    •	失败时，fseek 返回非零值，并设置 errno 以指示具体错误。*/
    int r = fseek(file, offSet, fromWhere);
    if (r) { perror("fseek()"); if (error_close_require)fclose(file);}
    return r;
}

FILE* fileOpen(const char* name, const char* mode, bool error_close_require)
{
    FILE* file = fopen(name, mode);
    if (!file) { perror("fopen()"); if (error_close_require)fclose(file);}
    return file;
}

size_t fileRead(void* buffer, size_t elementSize, size_t elementCount, FILE* file, bool error_close_require)
{
    // globalSleepLock.wait();
    size_t r = fread(buffer, elementSize, elementCount, file); // 每次读多少字节，读多少次
    // cout<<"In fread"<<endl;
    // globalSleepLock.notify();
    if (r != elementCount) {
        perror("fread()");
        if (feof(file)) printf("EOF error!\n");
        else printf("Error code: %d\n", ferror(file));
        if (error_close_require)fclose(file);
    }
    return r;
}

size_t fileWrite(const void* buffer, size_t elementSize, size_t elementCount, FILE* file, bool error_close_require)
{
    size_t r = fwrite(buffer, elementSize, elementCount, file);//  写入的内容，写多少字节，多少次
    if (r != elementCount) {
        perror("fread()");
        if (feof(file)) printf("EOF error!\n");
        else printf("Error code: %d\n", ferror(file));
        if (error_close_require)fclose(file);
    }
    return r;
}

int filePutCharacter(int character, FILE *file)
{
    //  ？ 写错了，应该是 int r = fputc(character, file);
    // int r = fputc(0, file);
    int r = fputc(character, file);
    if (r == -1)
    {
        perror("fputc()");
        printf("EOF Error! Code: %d\n", ferror(file));
    }
    return r;
}

void Disk::parse(char* str)
{
    // •	分割
    char* command = strtok(str, " ");
    if (command == NULL) {
        cout << "no command error" << endl;
        return;
    }

    if (!strcmp(command, "createFile") || !strcmp(command, "mkfile")) {
        /*	•	strtok(NULL, " ") 呼叫表示从上一次 strtok 调用后续的字符串开始继续分割。
            •	第一个参数 NULL 告诉 strtok 从上次找到的分割点接着开始新的搜索。
            •	第二个参数 " " 是一个分隔符集合，这里只包括一个空格。strtok 会使用这个字符来分割字符串。*/
        char* path = strtok(NULL, " ");
        if (path == NULL) {
            cout << "lack of path" << endl;
            return;
        }
        char* size = strtok(NULL, " ");
        if (size == NULL) {
            cout << "lack of size" << endl;
            return;
        }
        unsigned fileSize = atof(size) * 1024;
        if (fileSize == 0) {
            printf("Wrong file size format or file size is 0! Please check again!\n");
            return;
        }
        char* redundant = strtok(NULL, " ");
        if (redundant != NULL) {
            cout << "more arguments than expected" << endl;
            return;
        }
        //TODO: implement creating file

        if (!regex_match(string(path), fileNamePattern))
        {
            printf("Your file name does not meet the specification. "
                   "The file name can only consist of uppercase or lowercase English letters, numbers or underscores\n");
            return;
        }
        // 检查文件名
        vector<string> pathList = stringSplit(string(path), "/");
        for (size_t i = 0; i < pathList.size(); i++)
        {
            if (pathList[i].length() > MAXIMUM_FILENAME_LENGTH - 1) {
                printf("The directory/file name: %s is too long! Maximum length: %d.\n", pathList[i].c_str(), MAXIMUM_FILENAME_LENGTH - 1);
                return;
            }
        }
        int inode_id_ptr = currentInode.inode_id;
        for (size_t i = 0; i < pathList.size(); i++)
        {
            iNode inode_ptr = super.loadInode(inode_id_ptr, diskFile);
            // inode_ptr.lock->lockReader();
            if (!inode_ptr.isDir) {
                printf("%s is a file! You can not create directory under here!\n", getFileName(inode_ptr).c_str());
                return;
            }
            Directory dir = readFileEntriesFromDirectoryFile(inode_ptr);
            // 会返回对应directory文件夹内相同名字的文件inode_id
            short nextInode = dir.findInFileEntries(pathList[i].c_str());
            // 名字存在指向下一个
            if (nextInode != -1) {
                inode_id_ptr = nextInode;
            }
                // 名字不存在，创建对应的文件夹
            else {
                // 不是最后一个就要是dir
                if (i != pathList.size() - 1) {
                    // check free blocks and free inode
                    int parentRequired = parentBlockRequired(inode_ptr);
                    int newRequired = 1;
                    if (!freeBlockCheck(parentRequired + newRequired)) return;
                    if (!freeInodeCheck()) return;

                    int newDirInodeId = allocateResourceForNewDirectory(inode_ptr);
                    inode_id_ptr = createUnderInode(inode_ptr, pathList[i].c_str(), newDirInodeId); // 建立父file下的entry
                }
                    //最后一个是file name
                else {
                    // check free blocks and free inodes for files
                    int parentRequired = parentBlockRequired(inode_ptr);
                    int newRequired = ceil(fileSize / (double)super.BLOCK_SIZE);
                    if (newRequired > DIRECT_ADDRESS_NUMBER) newRequired++;
                    if (!freeBlockCheck(parentRequired + newRequired)) return;
                    if (!freeInodeCheck()) return;

                    int newFileInodeId = allocateResourceForNewFile(inode_ptr, fileSize);
                    inode_id_ptr = createUnderInode(inode_ptr, pathList[i].c_str(), newFileInodeId); // 建立父file下的entry
                }

                if (inode_id_ptr == -1) {
                    printf("Create file failed!\n");
                    return;
                }
                if (i == pathList.size() - 1) {
                    printf("Create file successfully!\n");
                    return;
                }
            }
        }
        printf("File already exists!\n");
        return;


    }
    else if (!strcmp(command, "createDir") || !strcmp(command, "mkdir")) {
        char* path = strtok(NULL, " ");
        if (path == NULL) {
            cout << "lack of path" << endl;
            return;
        }
        char* redundant = strtok(NULL, " ");
        if (redundant != NULL) {
            cout << "more arguments than expected" << endl;
            return;
        }

        // TODO: use new api. check blocks and iNodes needed first.
        //       so don't need to check during allocating.
        if (!regex_match(string(path), fileNamePattern))
        {
            printf("Your file name does not meet the specification. "
                   "The file name can only consist of uppercase or lowercase English letters, numbers or underscores\n");
            return;
        }

        vector<string> pathList = stringSplit(string(path), "/");
        for (size_t i = 0; i < pathList.size(); i++)
        {
            if (pathList[i].length() > MAXIMUM_FILENAME_LENGTH - 1) {
                printf("The directory/file name: %s is too long! Maximum length: %d.\n", pathList[i].c_str(), MAXIMUM_FILENAME_LENGTH - 1);
                return;
            }
        }
        int inode_id_ptr = currentInode.inode_id;
        for (size_t i = 0; i < pathList.size(); i++)
        {
            iNode inode_ptr = super.loadInode(inode_id_ptr, diskFile);
            if (!inode_ptr.isDir) {
                printf("%s is a file! You can not create directory under here!\n", getFileName(inode_ptr).c_str());
                return;
            }
            Directory dir = readFileEntriesFromDirectoryFile(inode_ptr);
            short nextDirectoryInode = dir.findInFileEntries(pathList[i].c_str());
            if (nextDirectoryInode != -1) {
                inode_id_ptr = nextDirectoryInode;
            }
            else {
                // check free blocks and free inode.
                int parentRequired = parentBlockRequired(inode_ptr);
                int newRequired = 1;
                if (!freeBlockCheck(parentRequired + newRequired)) return;
                if (!freeInodeCheck()) return;

                int newDirInodeId = allocateResourceForNewDirectory(inode_ptr);
                inode_id_ptr = createUnderInode(inode_ptr, pathList[i].c_str(), newDirInodeId);
                if (inode_id_ptr == -1) {
                    printf("Create directory failed!\n");
                    return;
                }
                if (i == pathList.size() - 1) {
                    printf("create directory successfully!\n");
                    return;
                }
            }
        }
        printf("directory already exists\n");
        return;


    }
    else if (!strcmp(command, "deleteFile") || !strcmp(command, "rmfile")) {
        char* path = strtok(NULL, " ");
        if (path == NULL) {
            cout << "lack of path" << endl;
            return;
        }
        char* redundant = strtok(NULL, " ");
        if (redundant != NULL) {
            cout << "more arguments than expected" << endl;
            return;
        }
        // 根据路径找到对应的inode id
        short targetFileInodeID = locateInodeFromPath(string(path));
        if (targetFileInodeID == -1) {
            printf("File not found: %s\n", path);
            return;
        }
        iNode fileToBeDelete = super.loadInode(targetFileInodeID, diskFile); // 根据路径找到对应的inode id
        if (fileToBeDelete.isDir) {
            printf("%s is a direcotry!\n",path);
            printf("You can use 'rmdir' command to delete a direcotry!\n");
            return;
        }
        if (deleteFile(fileToBeDelete))
        {
            printf("File deleted successfully: %s\n", path);
        }
        else {
            printf("Failed to delete the file: %s\n", path);
        }

    }
    else if (!strcmp(command, "deleteDir") || !strcmp(command, "rmdir")) {
        char* path = strtok(NULL, " ");
        if (path == NULL) {
            cout << "lack of path" << endl;
            return;
        }
        char* redundant = strtok(NULL, " ");
        if (redundant != NULL) {
            cout << "more arguments than expected" << endl;
            return;
        }
        short targetDirectoryInodeID = locateInodeFromPath(string(path));
        if (targetDirectoryInodeID == -1) {
            printf("Directory not found: %s\n", path);
            return;
        }
        iNode directoryToBeDelete = super.loadInode(targetDirectoryInodeID, diskFile);
        if (!directoryToBeDelete.isDir) {
            printf("%s is a file!\n",path);
            printf("You can use 'rmfile' command to delete a file!\n");
            return;
        }
        if (directoryToBeDelete.inode_id == currentInode.inode_id) {
            printf("You cannot delete current working directory: ");
            printCurrentDirectory("\n");
            return;
        }
        recursiveDeleteDirectory(directoryToBeDelete);
    }
    else if (!strcmp(command, "changeDir") || !strcmp(command, "cd")) {
        char* path = strtok(NULL, " ");
        if (path == NULL) {
            cout << "lack of path" << endl;
            return;
        }
        char* redundant = strtok(NULL, " ");
        if (redundant != NULL) {
            cout << "more arguments than expected" << endl;
            return;
        }
        if (changeDirectory(path)) {  // 改变currentInode
            printf("Directory has changed to %s\n", getFullFilePath(currentInode).c_str());
        }
        else
        {
            printf("Change directory failed!\n");
        }

    }
    else if (!strcmp(command, "dir")||!strcmp(command, "ls")) {
        char* redundant = strtok(NULL, " ");
        if (redundant != NULL) {
            cout << "more arguments than expected" << endl;
            return;
        }
        listDirectory(currentInode);
        printf("Current directory file size: %d\n\n", currentInode.fileSize);
    }
    else if (!strcmp(command, "cp")) {
        char* srcPath = strtok(NULL, " ");
        if (srcPath == NULL) {
            cout << "lack of source path" << endl;
            return;
        }
        char* tgtPath = strtok(NULL, " ");
        if (tgtPath == NULL) {
            cout << "lack of target path" << endl;
            return;
        }
        char* redundant = strtok(NULL, " ");
        if (redundant != NULL) {
            cout << "more arguments than expected" << endl;
            return;
        }
        //TODO: implementing coping file or dir
        vector<string> srcPathList = stringSplit(string(srcPath), "/");
        vector<string> tgtPathList = stringSplit(string(tgtPath), "/");
        int srcId = currentInode.inode_id;

        for (int i = 0; i < srcPathList.size(); i++) {
            iNode inode_ptr = super.loadInode(srcId, diskFile);
            if (!inode_ptr.isDir) {
                printf("%s is a file! Please check your source path again!\n", getFileName(inode_ptr).c_str());
                printf("Copy failed!\n");
                return;
            }
            Directory dir = readFileEntriesFromDirectoryFile(inode_ptr);
            short nextInode = dir.findInFileEntries(srcPathList[i].c_str());
            if (nextInode != -1) {
                srcId = nextInode;
            }
            else {
                printf("source path doesn't exist!\n");
                printf("Copy failed!\n");
                return;
            }
        }
        int tgtId = currentInode.inode_id;
        for (int i = 0; i < tgtPathList.size() - 1; i++) {
            iNode inode_ptr = super.loadInode(tgtId, diskFile);
            if (!inode_ptr.isDir) {
                printf("%s is a file! Please check your target path again!\n", getFileName(inode_ptr).c_str());
                printf("Copy failed!\n");
                return;
            }
            Directory dir = readFileEntriesFromDirectoryFile(inode_ptr);
            short nextInode = dir.findInFileEntries(tgtPathList[i].c_str());
            if (nextInode != -1) {
                tgtId = nextInode;
            }
            else {
                printf("target path doesn't exist!\n");
                return;
            }
        }
        const char* fileName = tgtPathList[tgtPathList.size() - 1].c_str();
        iNode srcInode = super.loadInode(srcId, diskFile);
        iNode tgtInode = super.loadInode(tgtId, diskFile);
        if (!tgtInode.isDir) {
            printf("%s is a file! Please check your target path again!\n", getFileName(tgtInode).c_str());
            printf("Copy failed!\n");
            return;
        }
        // check freeInodes and freeBlocks 计算得到需要用到的block和inode数量
        int blockRequired = blockUsedBy(srcInode),
                parentRequired = parentBlockRequired(tgtInode);
        int iNodeRequired = inodeUsedBy(srcInode);
        printf("blockRequired %d, iNodeRequired %d\n", blockRequired, iNodeRequired);
        if (!freeBlockCheck(blockRequired + parentRequired)) return;
        if (iNodeRequired > super.freeInodeNumber) {
            printf("Not enough free inode left!\n");
            printf("Copy failed!\n");
            return;
        }
        if (copy(srcInode, fileName, tgtInode)) {
            printf("Copy successfully!\n");
        }
        else {
            printf("Copy failed!\n");
        }

    }
    else if (!strcmp(command, "sum")) { //Print disk usage

        char* redundant = strtok(NULL, " ");
        if (redundant != NULL) {
            cout << "more arguments than expected" << endl;
            return;
        }
        printf("Calculating...");
        unsigned spaceForFiles = getDirectorySize(super.loadInode(0, diskFile));
        unsigned spaceUsed = dbm.getDedicateBlock(&super, diskFile) * super.BLOCK_SIZE + spaceForFiles;
        printf("\r");
        printf("Space usage (Bytes): %d/%d (%.4f%%)\n",
               spaceUsed,
               super.TOTAL_SIZE,
               100 * spaceUsed / (double)super.TOTAL_SIZE);
        printf("Inode usage: %d/%d (%.4f%%)\n",
               super.inodeNumber - super.freeInodeNumber,
               super.inodeNumber,
               100 * (super.inodeNumber - super.freeInodeNumber) / (double)super.inodeNumber);
        dbm.printBlockUsage(&super, diskFile);
    }
    else if (!strcmp(command, "cat")) {
        char* path = strtok(NULL, " ");
        if (path == NULL) {
            cout << "lack of path" << endl;
            return;
        }
        char* redundant = strtok(NULL, " ");
        if (redundant != NULL) {
            cout << "more arguments than expected" << endl;
            return;
        }
        if (!regex_match(string(path), fileNamePattern))
        {
            printf("Your file name does not meet the specification. "
                   "The file name can only consist of uppercase or lowercase English letters, numbers or underscores\n");
            return;
        }

        short inode_id_ptr = locateInodeFromPath(path);
        if (inode_id_ptr == -1) return;
        iNode inode_ptr = super.loadInode(inode_id_ptr, diskFile);
        if (inode_ptr.isDir) {
            listDirectory(inode_ptr);
            return;
        }
        printf("Content of %s: \n\n", getFullFilePath(inode_ptr).c_str());
        // 计算block
        int blockRequired = (int)ceil(inode_ptr.fileSize / (double)super.BLOCK_SIZE);
        int directNum = min(blockRequired, DIRECT_ADDRESS_NUMBER);
        int indirectIndexedNum = max(0, blockRequired - directNum);
        int offset = inode_ptr.fileSize % super.BLOCK_SIZE; // bytes in the last block
        if (offset == 0) offset = super.BLOCK_SIZE;

        Diskblock db;
        /*	•	printf 是标准的 C 语言函数，用于输出格式化的文本。
            •	%.1024s 是 printf 函数的格式字符串：
            •	%s 表示期望的输入是一个字符串。
            •	.1024 限定了输出的最大字符数，即使字符串的实际长度超过这个数，也只会输出前 1024 个字符。
            •	(char *)(db.content) 强制类型转换，将 db.content（假设其为某种类型的指针，例如 void * 或其他未指定的类型）转换为 char *。这表明 db.content 指向的内存区域包含或被视为字符数据。*/
        for (int i = 0; i < directNum - 1; i++) {
            db.load(inode_ptr.direct[i], diskFile);
            printf("%.1024s", (char *)(db.content));
        }
        if (indirectIndexedNum > 0) {
            db.load(inode_ptr.direct[directNum - 1], diskFile);
            printf("%.1024s", (char*)(db.content));
            IndirectDiskblock idb;
            idb.load(inode_ptr.indirect, diskFile);
            for (int i = 0; i < indirectIndexedNum - 1; i++) {
                db.load(idb.addrs[i], diskFile);
                printf("%.1024s", (char*)(db.content));
            }
            db.load(idb.addrs[indirectIndexedNum - 1], diskFile, offset);
            printf("%.*s", offset, (char*)(db.content));
        }
        else {
            db.load(inode_ptr.direct[directNum - 1], diskFile, offset);
            /*	•	%.*s 是一个格式指定符，用于输出一个字符串：
                •	%s 指定了期望的输入是一个字符串。
                •	.* 允许在调用 printf 时动态指定字符串的最大输出长度。这里的星号 * 表示长度将从 printf 的参数列表中获取，而非硬编码在格式字符串中。*/
            printf("%.*s", offset, (char*)(db.content));
        }
        printf("\n\n");
        printf("File size: %d Byte(s)\n",inode_ptr.fileSize);
        return;


    }
    else if (!strcmp(command, "cls")) {
        system("clear");
        printWelcomeInfo();
    }
    else if (!strcmp(command, "help")) {
        printHelpInfo();
    }
    else
    {
        printf("Unknown command! Please check again!\n");
    }
}

Disk::Disk()
{
    //这行代码使用正则表达式（regex）定义了一个文件名的匹配模式，用于验证文件名是否仅包含字母（大小写）、数字、下划线（_）、点（.）和斜杠（/）。以下是对这行代码的详细解释：
    fileNamePattern = regex("^([a-z]|[A-Z]|[_/.]|[0-9])*$");
}

Disk::~Disk()
{

}

void Disk::run()
{
    if (!loadDisk())
        exit(1);
    else {

        printf("Load file successful!!\n");

        printf("Block size:%d\n", super.BLOCK_SIZE);
        printf("Block number:%d\n", super.dataBlockNumber);
        printf("Free block number:%d\n", super.freeDataBlockNumber);
        printf("Inode number:%d\n", super.inodeNumber);
        printf("Free inode number:%d\n", super.freeInodeNumber);
        dbm.printBlockUsage(&super,diskFile);
        printf("Loading root inode...");
        if (setCurrentInode(0)) {
            printf("Successful!\n");
            printf("Root inode id:%d\n", currentInode.inode_id);
            printf("Root inode create time:%s\n", currentInode.getCreateTime().c_str());
            printf("Root inode access time:%s\n", currentInode.getAccessTime().c_str());
            printf("Root inode modify time:%s\n", currentInode.getModifiedTime().c_str());
            printf("Root file size:%d\n", currentInode.fileSize);
            printf("Root file start at (direct block addresss):%d\n", currentInode.direct[0].to_int());
            system("clear");
            printWelcomeInfo();
            while (true) {
                printCurrentDirectory();
                printf("#");
                char command[1024];
                scanf("%[^\n]", &command);
                getchar();
                if (!strcmp(command, "exit"))break;
                parse(command);
                memset(command, 0, sizeof command);
                currentInode = super.loadInode(currentInode.inode_id, diskFile);
            }
        }
        else {
            printf("Failed!\n");
            exit(4);
        }
        fclose(diskFile);
    }
}


bool Disk::loadDisk()
{
    FILE* file = fileOpen("disk.dat", "rb+");
    if (file)
    {
        cout << "Disk file found!" << endl;
        char* magic_number_test = new char[sizeof(magic_number)];
        globalSleepLock.wait();
        if (fileSeek(file, 0, SEEK_SET)) return false;
        if (fileRead(magic_number_test, sizeof(char),
                     strlen(magic_number), file) != strlen(magic_number))
            return false;
        globalSleepLock.notify();
        magic_number_test[sizeof(magic_number) - 1] = '\0';
        if (strcmp(magic_number_test, magic_number))
        {
            cout << "Magic number error! Invalid file!!" << endl;
            return false;
        }
        globalSleepLock.wait();
        if (fileSeek(file, sizeof(magic_number) - 1, SEEK_SET))
            return false;
        if (fileRead(&super, sizeof(superBlock), 1, file) != 1)
        {
            fclose(file);
            return false;
        }
        globalSleepLock.notify();
        diskFile = file;
        dbm.freeptr = super.freeptr;
        delete[] magic_number_test;
        return true;
    }
    else
    {
        cout << "Disk file not found! Initial a new file!" << endl;
        file = fileOpen("disk.dat", "w");
        // 设置最后一位为EOF
        globalSleepLock.wait();
        if (fileSeek(file, INITIAL_DISK_SIZE - 1, SEEK_CUR)) return false;
        if (filePutCharacter(0, file)) return false;
        globalSleepLock.notify();
        cout << "Create file successful!" << endl;
        fclose(file);
        file = fileOpen("disk.dat", "rb+");
        globalSleepLock.wait();
        if (fileSeek(file, 0, SEEK_SET)) return false;
        if (fileWrite(magic_number,sizeof(magic_number),1,file) != 1) return false;
        globalSleepLock.notify();
        dbm.initialize(&super, file);
        super.freeptr = dbm.freeptr;
        super.freeDataBlockNumber = dbm.getFreeBlock(file);
        diskFile = file;
        printf("Initializing root directory...\n");
        initializeRootDirectory();

        return true;
    }
}

void Disk::initializeRootDirectory()
{
    Directory root_dir;
    root_dir.files.push_back(fileEntry(".", 0));
    root_dir.files.push_back(fileEntry("..", 0));
    root_dir.tree->Insert(fileEntry(".", 0));
    root_dir.tree->Insert(fileEntry("..", 0));
    int directoryFileSize = root_dir.files.size() * sizeof(fileEntry);
    Address rootDirectoryFile = allocateNewBlock(diskFile);
    if (rootDirectoryFile == Address(0)) {
        printf("Initialize root directory file failed!\n");
        exit(2);
    }
    for (int i = 0; i < root_dir.files.size(); i++)
    {
        memcpy(db.content + i * sizeof(fileEntry), &root_dir.files[i], sizeof(fileEntry));
    }
    // 复制完了之后还要写回blcok里面，db只是一个buffer
    db.write(rootDirectoryFile);
    Address directAddresses[1] = { rootDirectoryFile };
    int root_inode = super.allocateNewInode(root_dir.files.size() * sizeof(fileEntry), 0, directAddresses, NULL, diskFile);
    if (root_inode >= 0) {
        printf("Root directory initialized successfully!\n");
    }
}

Address Disk::allocateNewBlock(FILE* file)
{
    // 分配了一个block的地址
    Address ad = dbm.alloc(file);
    if (ad == Address(0))return ad;
    super.freeptr = dbm.freeptr;
    super.freeDataBlockNumber = dbm.getFreeBlock(file);
    if (!super.updateSuperBlock(file)) {
        printf("Update super block failed! Reverting...\n");
        dbm.free(ad, file);
        super.freeptr = dbm.freeptr;
        super.freeDataBlockNumber = dbm.getFreeBlock(file);
        return Address(0);
    }
    return ad;
}

bool Disk::freeBlock(Address addr, FILE* file)
{
    dbm.free(addr, file);
    super.freeptr = dbm.freeptr;
    super.freeDataBlockNumber = dbm.getFreeBlock(file);
    if (!super.updateSuperBlock(file)) {
        printf("Update super block failed!\n");
        exit(1);
    }
    return true;
}

bool Disk::setCurrentInode(int inode_id)
{
    //if (fileSeek(diskFile, super.inodeStart + inode_id * super.INODE_SIZE, SEEK_SET)) return false;
    //if (fileRead(&currentInode, sizeof iNode, 1, diskFile) != 1) return false;
    iNode inode = super.loadInode(inode_id);
    if (inode.inode_id != -1) {
        memcpy(&currentInode, &inode, sizeof(iNode));
        return true;
    }
    else return false;
}

Directory Disk::readFileEntriesFromDirectoryFile(iNode inode)
{
    Directory d;
    fileEntry fe;
    unsigned fileSize = inode.fileSize;
    if (fileSize <= DIRECT_ADDRESS_NUMBER * super.BLOCK_SIZE)
    {	// ceil 有问题，改了一下

        // for (int i = 1; i <= fileSize / super.BLOCK_SIZE+1; i++)
        for (int i = 1; i <= ceil((double)fileSize / super.BLOCK_SIZE); i++)
        {
            db.load(inode.direct[i - 1], diskFile);
            // if (i == fileSize / super.BLOCK_SIZE +1) // 最后一个 block
            if (i == (int)ceil((double)fileSize / super.BLOCK_SIZE))
            {
                for (size_t j = 0; j < (fileSize % super.BLOCK_SIZE) / sizeof(fileEntry); j++)
                {
                    memcpy(&fe, db.content + j * sizeof(fileEntry), sizeof(fileEntry));
                    d.files.push_back(fe);
                    d.tree->Insert(fe);
                }
            }
            else {
                for (size_t j = 0; j < super.BLOCK_SIZE / sizeof(fileEntry); j++)
                {
                    memcpy(&fe, db.content + j * sizeof(fileEntry), sizeof(fileEntry));
                    d.files.push_back(fe);
                    d.tree->Insert(fe);
                }
            }
        }
    }
    else {
        for (size_t i = 0; i < DIRECT_ADDRESS_NUMBER; i++)
        {
            db.load(inode.direct[i], diskFile);
            for (size_t j = 0; j < super.BLOCK_SIZE / sizeof(fileEntry); j++)
            {
                memcpy(&fe, db.content + j * sizeof(fileEntry), sizeof(fileEntry));
                d.files.push_back(fe);
                d.tree->Insert(fe);
            }
        }
        IndirectDiskblock idb;
        idb.load(inode.indirect, diskFile);
        unsigned remainingFileSize = fileSize - DIRECT_ADDRESS_NUMBER * super.BLOCK_SIZE;
        unsigned remainingFullBlocks = remainingFileSize / super.BLOCK_SIZE;
        for (size_t i = 0; i < remainingFullBlocks; i++)
        {
            db.load(idb.addrs[i],diskFile);
            for (size_t j = 0; j < super.BLOCK_SIZE / sizeof(fileEntry); j++)
            {
                memcpy(&fe, db.content + j * sizeof(fileEntry), sizeof(fileEntry));
                d.files.push_back(fe);
                d.tree->Insert(fe);
            }
        }
        int lastRemainingEntriesNumber =
                (remainingFileSize % super.BLOCK_SIZE) / sizeof(fileEntry);
        db.load(idb.addrs[remainingFullBlocks],diskFile);
        for (size_t i = 0; i < lastRemainingEntriesNumber; i++)
        {
            memcpy(&fe, db.content + i * sizeof(fileEntry), sizeof(fileEntry));
            d.files.push_back(fe);
            d.tree->Insert(fe);
        }
    }
    // d.tree->Print();
    return d;
}

bool Disk::writeFileEntriesToDirectoryFile(Directory d, iNode inode)
{
    unsigned fileSize = inode.fileSize;
    if (fileSize <= DIRECT_ADDRESS_NUMBER * super.BLOCK_SIZE) // 不需要indirect
    {
        //
        // for (int i = 1; i <= fileSize / super.BLOCK_SIZE+1; i++)
        for (int i = 1; i <= ceil((double)fileSize / super.BLOCK_SIZE); i++)
        {
            //
            // if (i == fileSize / super.BLOCK_SIZE+1)
            if (i == (int)ceil((double)fileSize / super.BLOCK_SIZE)) // 最后一个，不需要写满
            {
                for (int j = (i - 1) * super.BLOCK_SIZE / sizeof(fileEntry); j < d.files.size(); j++)
                {
                    memcpy(db.content + (j % (super.BLOCK_SIZE / sizeof(fileEntry))) * sizeof(fileEntry), &d.files[j], sizeof(fileEntry));
                }
                db.write(inode.direct[i - 1]);
            }
            else
            {
                for (int j = (i - 1) * super.BLOCK_SIZE / sizeof(fileEntry); j < i * super.BLOCK_SIZE / sizeof(fileEntry); j++) // 不是最后一个，需要写满
                {
                    memcpy(db.content + (j % (super.BLOCK_SIZE / sizeof(fileEntry))) * sizeof(fileEntry), &d.files[j], sizeof(fileEntry));
                }
                db.write(inode.direct[i - 1]);
            }
        }
    }
    else { // 需要 indirect
        for (size_t i = 0; i < DIRECT_ADDRESS_NUMBER; i++)
        {
            for (size_t j = 0; j < super.BLOCK_SIZE / sizeof(fileEntry); j++)
            {
                memcpy(db.content + j * sizeof(fileEntry), &d.files[i * (super.BLOCK_SIZE / sizeof(fileEntry)) + j], sizeof(fileEntry));
            }
            db.write(inode.direct[i]);
        }
        IndirectDiskblock idb;
        idb.load(inode.indirect, diskFile);
        unsigned remainingFileSize = fileSize - DIRECT_ADDRESS_NUMBER * super.BLOCK_SIZE;
        unsigned remainingFullBlocks = remainingFileSize / super.BLOCK_SIZE;
        for (size_t i = 0; i < remainingFullBlocks; i++)
        {
            for (size_t j = 0; j < super.BLOCK_SIZE / sizeof(fileEntry); j++)
            {
                memcpy(db.content + j * sizeof(fileEntry), &d.files[DIRECT_ADDRESS_NUMBER * super.BLOCK_SIZE / sizeof(fileEntry) + i * (super.BLOCK_SIZE / sizeof(fileEntry)) + j], sizeof(fileEntry));
            }
            db.write(idb.addrs[i]);
        }
        unsigned lastRemainingEntriesNumber = (remainingFileSize % super.BLOCK_SIZE) / sizeof(fileEntry);
        for (size_t i = 0; i < lastRemainingEntriesNumber; i++)
        {
            memcpy(db.content + i * sizeof(fileEntry), &d.files[(DIRECT_ADDRESS_NUMBER + remainingFullBlocks) * super.BLOCK_SIZE / sizeof(fileEntry) + i], sizeof(fileEntry));
        }
        db.write(idb.addrs[remainingFullBlocks]);
    }
    return true;
}

int Disk::createUnderInode(iNode& parent, const char* name, int newInode)
{
    /*1、只占用直接块，不需要申请新块
      2、只占用直接块，需要申请新块
      3、一开始占用直接块，但加了之后需要申请间接块
      4、占用间接块，不需要申请新块
      5、占用间接块，需要申请新块*/
    // parent.lock->
    if (parent.fileSize == MAXIMUM_FILE_SIZE) {
        printf("This directory has reached its maximum size!\n");
        return -1;
    }
    // bool checkDuplicate = false;
    Directory parent_dir = readFileEntriesFromDirectoryFile(parent);
    // for (size_t i = 0; i < parent_dir.files.size(); i++) // 检查重复
    // {
    //     if (!strcmp(parent_dir.files[i].fileName, name))
    //     {
    //         checkDuplicate = true;
    //         break;
    //     }
    // }

    if (parent_dir.tree->Search(name)>0) {
        printf("File/Directory with same name is exist: %s\nPlease change another name!\n",name);
        return -1;
    }
    unsigned newFileSizeOfCurrentDirectory = parent.fileSize + sizeof(fileEntry);
    if (parent.fileSize < super.BLOCK_SIZE * DIRECT_ADDRESS_NUMBER)
    {
        if (parent.fileSize % super.BLOCK_SIZE != 0) {
            //1、只占用直接块，父目录文件不需要申请新块，需要给新文件夹一个block
            //int block required =0;
            //只是父文件夹的需求量
            //应用新文件(夹)的更改
            if (newInode == -1)return -1;
            //应用父文件夹的更改
            Directory parent_dir = readFileEntriesFromDirectoryFile(parent);

            parent_dir.files.push_back(fileEntry(name, newInode));
            parent_dir.tree->Insert(fileEntry(name, newInode));

            parent.fileSize += sizeof(fileEntry);
            parent.updateModifiedTime();
            super.writeInode(parent, diskFile);
            writeFileEntriesToDirectoryFile(parent_dir, parent);

        }
        else
        {
            //2、只占用直接块，需要申请新块
            //int block required =1;
            //应用新文件(夹)的更改
            if (newInode == -1)return -1;
            //应用父文件夹的更改
            Directory parent_dir = readFileEntriesFromDirectoryFile(parent);
            parent_dir.files.push_back(fileEntry(name, newInode));
            parent_dir.tree->Insert(fileEntry(name, newInode));
            parent.fileSize += sizeof(fileEntry);
            parent.updateModifiedTime();
            Address newBlockForParentDirectory = allocateNewBlock(diskFile);
            parent.direct[parent.fileSize / super.BLOCK_SIZE] = newBlockForParentDirectory;
            super.writeInode(parent, diskFile);
            writeFileEntriesToDirectoryFile(parent_dir, parent);
        }
    }
    else if (parent.fileSize == super.BLOCK_SIZE * DIRECT_ADDRESS_NUMBER)
    {
        //3、一开始占用直接块，但加了之后需要申请间接块
        //int block required =2;
        //应用新文件(夹)的更改
        if (newInode == -1) return -1;
        //应用父文件夹的更改
        Directory parent_dir = readFileEntriesFromDirectoryFile(parent);
        parent_dir.files.push_back(fileEntry(name, newInode));
        parent_dir.tree->Insert(fileEntry(name, newInode));
        parent.fileSize += sizeof(fileEntry);
        parent.updateModifiedTime();
        Address newIndirectAddressBlockForParentDirectory = allocateNewBlock(diskFile);
        Address newIndirectBlockForParentDirectory = allocateNewBlock(diskFile);
        parent.indirect = newIndirectAddressBlockForParentDirectory;
        IndirectDiskblock idb;
        idb.addrs[0] = newIndirectBlockForParentDirectory;
        idb.write(parent.indirect, diskFile); // 更新indirect block的block地址指针
        super.writeInode(parent, diskFile); // 更新inode（indirect）
        writeFileEntriesToDirectoryFile(parent_dir, parent);
    }
    else
    {
        if (parent.fileSize % super.BLOCK_SIZE != 0) {
            //4、占用间接块，不需要申请新块
            //int block required =0;
            //应用新文件(夹)的更改
            if (newInode == -1) return -1;
            //应用父文件夹的更改
            Directory parent_dir = readFileEntriesFromDirectoryFile(parent);
            parent_dir.files.push_back(fileEntry(name, newInode));
            parent_dir.tree->Insert(fileEntry(name, newInode));
            parent.fileSize += sizeof(fileEntry);
            parent.updateModifiedTime();
            super.writeInode(parent, diskFile);
            writeFileEntriesToDirectoryFile(parent_dir, parent);
        }
        else {
            //5、占用间接块，需要申请新块
            //int block required =1.
            //应用新文件(夹)的更改
            if (newInode == -1) return -1;
            //应用父文件夹的更改
            Directory parent_dir = readFileEntriesFromDirectoryFile(parent);
            parent_dir.files.push_back(fileEntry(name, newInode));
            parent_dir.tree->Insert(fileEntry(name, newInode));
            parent.fileSize += sizeof(fileEntry);
            parent.updateModifiedTime();
            Address newIndirectBlockForParentDirectory = allocateNewBlock(diskFile);
            IndirectDiskblock idb;
            idb.load(parent.indirect,diskFile);
            idb.addrs[parent.fileSize / super.BLOCK_SIZE - DIRECT_ADDRESS_NUMBER] = newIndirectBlockForParentDirectory;
            idb.write(parent.indirect, diskFile); // 更新 更新indirect block的地址的block地址指针
            super.writeInode(parent, diskFile);
            writeFileEntriesToDirectoryFile(parent_dir, parent);
        }
    }
    return newInode;
}

short Disk::allocateResourceForNewDirectory(iNode parent)
{
    // return: new inode id for new directory
    Address newBlockForNewDirectory = allocateNewBlock(diskFile);
    Address newBlocks[1] = { newBlockForNewDirectory };
    int newInodeForNewDirectory = super.allocateNewInode(2 * sizeof(fileEntry), parent.inode_id, newBlocks, NULL, diskFile);
    Directory dir;
    dir.files.push_back(fileEntry(".", newInodeForNewDirectory));
    dir.files.push_back(fileEntry("..", parent.inode_id));

    dir.tree->Insert(fileEntry(".", newInodeForNewDirectory));
    dir.tree->Insert(fileEntry("..", parent.inode_id));

    if (!writeFileEntriesToDirectoryFile(dir, super.loadInode(newInodeForNewDirectory)))
    {
        printf("Failed to write file entries to disk for new directory!\n");
        super.freeInode(newInodeForNewDirectory, diskFile);
        freeBlock(newBlockForNewDirectory, diskFile);
        return -1;
    }
    return newInodeForNewDirectory;
}

short Disk::allocateResourceForNewFile(iNode parent, unsigned fileSize)
{
    // return new inode id for new file
    int blockRequired = ceil(fileSize / (double)super.BLOCK_SIZE);
    int directNum = min(blockRequired, DIRECT_ADDRESS_NUMBER);
    int indirectIndexedNum = max(0, blockRequired - directNum);
    bool indirectRequired = blockRequired > DIRECT_ADDRESS_NUMBER;
    //freeBlockCheck(blockRequired + indirectRequired)
    Address* direct = NULL, *indirect = NULL;

    direct = new Address[directNum];
    for (int i = 0; i < directNum; i++) {
        direct[i] = allocateNewBlock(diskFile);
    }
    if (indirectIndexedNum > 0) {
        indirect = new Address();
        *indirect = allocateNewBlock(diskFile);
        IndirectDiskblock idb;
        //TODO 没有文件太大报错处理
        for (int i = 0; i < indirectIndexedNum; i++) {
            idb.addrs[i] = allocateNewBlock(diskFile);
        }
        idb.write(*indirect, diskFile);
    }

    // fill the file with random strings.
    srand(time(NULL));
    for (int i = 0; i < directNum; i++) {
        Diskblock db;
        for (int b = 0; b < super.BLOCK_SIZE; b++) {
            db.content[b] = 'a' + (rand() % 26);
        }
        db.write(direct[i], diskFile);
    }
    if (indirectIndexedNum > 0) {
        IndirectDiskblock idb;
        idb.load(*indirect);
        for (int i = 0; i < indirectIndexedNum; i++) {
            Diskblock db;
            for (int b = 0; b < super.BLOCK_SIZE; b++) {
                db.content[b] = 'a' + (rand() % 26);
            }
            db.write(idb.addrs[i], diskFile);
        }
    }

    int newInode = super.allocateNewInode(fileSize, parent.inode_id, direct, indirect, diskFile, false);

    delete[] direct;
    delete indirect;
    return newInode;
}

int Disk::parentBlockRequired(iNode& parent)
{
    // calculate free blocks needed
    // same structure as createUnderInode
    if (parent.fileSize < super.BLOCK_SIZE * DIRECT_ADDRESS_NUMBER) {
        if (parent.fileSize % super.BLOCK_SIZE != 0)
        {
            return 0;
        }
        else { return 1; }
    }
    else if (parent.fileSize == super.BLOCK_SIZE * DIRECT_ADDRESS_NUMBER) {
        return 2;
    }
    else {
        if (parent.fileSize % super.BLOCK_SIZE != 0)
        {
            return 0;
        }
        else { return 1; }
    }
}

bool Disk::freeBlockCheck(int blockRequired)
{

    if (blockRequired < super.freeDataBlockNumber) {
        return true;
    }
    else
    {
        printf("Free block: %d.\n", super.freeDataBlockNumber);
        printf("No enough blocks to support the operation!\n");
        return false;
    }
}

bool Disk::freeInodeCheck()
{
    if (super.freeInodeNumber == 0) {
        printf("No free inode left");
        return false;
    }
    return true;
}

int Disk::blockUsedBy(iNode& inode_ptr)
{
    // return the block used by the file or dir.
    int ret = 0;
    if (inode_ptr.isDir) {
        Directory dir = readFileEntriesFromDirectoryFile(inode_ptr);
        for (int i = 0; i < dir.files.size(); i++) {
            // other than current and parent inode
            if (!strcmp(dir.files[i].fileName, ".")) continue;
            if (!strcmp(dir.files[i].fileName, "..")) continue;

            short child_inode_id = dir.files[i].inode_id;
            iNode child_inode = super.loadInode(child_inode_id, diskFile);
            ret += blockUsedBy(child_inode);
        }
    }
    int blockUsed = ceil(inode_ptr.fileSize / (double)super.BLOCK_SIZE);
    bool indirectUsed = blockUsed > DIRECT_ADDRESS_NUMBER;

    return ret + blockUsed + indirectUsed;
}

int Disk::inodeUsedBy(iNode& inode_ptr)
{
    int ret = 1;
    // 文件
    if (!inode_ptr.isDir) {
        return ret;
    }
    //如果是文件夹所有的都要被算上
    Directory dir = readFileEntriesFromDirectoryFile(inode_ptr);
    for (int i = 0; i < dir.files.size(); i++) {
        if (!strcmp(dir.files[i].fileName, ".")) continue;
        if (!strcmp(dir.files[i].fileName, "..")) continue;
        iNode child = super.loadInode(dir.files[i].inode_id, diskFile);
        ret += inodeUsedBy(child);
    }
    return ret;
}

bool Disk::copy(iNode& source, const char* name, iNode& target){

    // check duplicates
    bool checkDuplicate = false;
    Directory parent_dir = readFileEntriesFromDirectoryFile(target);
    for (size_t i = 0; i < parent_dir.files.size(); i++)
    {
        if (!strcmp(parent_dir.files[i].fileName, name))
        {
            checkDuplicate = true;
            break;
        }
    }
    if (checkDuplicate) {
        printf("File/Directory with same name is exist: %s\nPlease change another name!\n", name);
        return false;
    }
    // copy file to target
    if (!source.isDir) {
        int newFileId = copyFile(source, target);
        newFileId = createUnderInode(target, name, newFileId); // 添加新的 file entry
        return true;
    }
    // if source is dir, create dir with same name
    //       and copy child files.



    int newDirId = allocateResourceForNewDirectory(target); // create directory in the target directory
    newDirId = createUnderInode(target, name, newDirId);

    iNode newDir = super.loadInode(newDirId, diskFile);
    Directory src = readFileEntriesFromDirectoryFile(source); // read child inode in the source directory.
    for (int i = 0; i < src.files.size(); i++) {
        if (!strcmp(src.files[i].fileName, ".")) continue;
        if (!strcmp(src.files[i].fileName, "..")) continue;
        // copy child files
        iNode child = super.loadInode(src.files[i].inode_id, diskFile);
        copy(child, src.files[i].fileName, newDir); // 递归调用
    }
    return true;
}

short Disk::copyFile(iNode& source, iNode& target)
{
    // allocate disk blocks for new file
    unsigned fileSize = source.fileSize;
    int blockRequired = ceil(fileSize / (double)super.BLOCK_SIZE);
    int directNum = min(blockRequired, DIRECT_ADDRESS_NUMBER);
    int indirectIndexNum = max(0, blockRequired - directNum);

    Address* direct = NULL, * indirect = NULL;

    direct = new Address[directNum];
    for (int i = 0; i < directNum; i++) {
        direct[i] = allocateNewBlock(diskFile);
    }
    if (indirectIndexNum > 0) {
        indirect = new Address();
        *indirect = allocateNewBlock(diskFile);
        IndirectDiskblock idb;
        for (int i = 0; i < indirectIndexNum; i++) {
            idb.addrs[i] = allocateNewBlock(diskFile);
        }
        idb.write(*indirect, diskFile);
    }


    for (int i = 0; i < directNum; i++) {
        Diskblock db;
        db.load(source.direct[i], diskFile);
        db.write(direct[i], diskFile);
    }
    if (indirectIndexNum > 0) {
        IndirectDiskblock idb_src, idb_tgt;
        idb_src.load(source.indirect);
        idb_tgt.load(*indirect);
        for (int i = 0; i < indirectIndexNum; i++) {
            Diskblock db;
            db.load(idb_src.addrs[i], diskFile);
            db.write(idb_tgt.addrs[i], diskFile);
        }
    }
    int newInode =
            super.allocateNewInode(source.fileSize, target.inode_id, direct, indirect, diskFile, false);

    delete[] direct;
    delete indirect;
    return newInode;
}

void Disk::listDirectory(iNode directory_inode)
{
    Directory dir = readFileEntriesFromDirectoryFile(directory_inode);
    printf("\nFile(s) and directory(s) of %s :\n",getFullFilePath(directory_inode).c_str());
    printf("\nFile Name\tFile Size\tFile Type\tCreate Time\t\t\tModified Time\t\t\tInode ID\n");
    iNode in;
    string fileName;
    const char* c = "asdfasdfasdfas";
    for (size_t i = 0; i < dir.files.size(); i++)
    {
        fileName = string(dir.files[i].fileName);
        in = super.loadInode(dir.files[i].inode_id);
        for (size_t j = 0; j < (int)ceil((double)fileName.size() / 14); j++)
        {
            if (j == 0)
            {
                printf("%s%s%d B\t\t%s\t\t%s\t%s\t%d\n",
                       fileName.substr(0,14).c_str(),
                       (fileName.size() >= 8 ? "\t" : "\t\t"),
                       in.fileSize, (in.isDir?"Dir":"File"),
                       in.getCreateTime().c_str(),
                       in.getModifiedTime().c_str(),
                       in.inode_id);
            }
            else {
                printf("%s\n", fileName.substr(j * 14, 14).c_str());
            }
        }
    }
    printf("\n");
}

void Disk::printCurrentDirectory(const char* end)
{
    printf("%s", getFullFilePath(currentInode, end).c_str());
}

string Disk::getFullFilePath(iNode inode, const char* end)
{
    string lastChar = (inode.isDir ? "/" : "");
    string result = "/";
    stack<string> directories;
    while (inode.inode_id != 0)
    {
        directories.push(getFileName(inode));
        inode = super.loadInode(inode.parent, diskFile);
    }

    while (!directories.empty()) {
        result += (directories.top() + (directories.size() == 1 ? lastChar : "/"));
        directories.pop();
    }
    result += (" " + string(end));
    return result;
}



string Disk::getFileName(iNode inode)
{
    if (inode.inode_id == 0) {
        return string("");
    }
    iNode parent_inode = super.loadInode(inode.parent, diskFile);
    Directory parent_dir = readFileEntriesFromDirectoryFile(parent_inode);
    for (size_t i = 0; i < parent_dir.files.size(); i++)
    {
        if (parent_dir.files[i].inode_id == inode.inode_id)
            return string(parent_dir.files[i].fileName);
    }
    printf("No such file or directory!\n");
    exit(5);

}

bool Disk::changeDirectory(const char* destination)
{
    short destination_id = locateInodeFromPath(destination);
    if (destination_id == -1) {
        printf("Directory not found: %s\n", destination);
        return false;
    }
    iNode destination_inode = super.loadInode(destination_id,diskFile);
    if (!destination_inode.isDir) {
        printf("%s is a file! Not a directory!\n", destination);
        return false;
    }
    currentInode = destination_inode;
    return true;
}

vector<string> Disk::stringSplit(const string& str, const string& pattern)
{
    regex re(pattern);
    vector<string> splitResult = vector<string> {
            sregex_token_iterator(str.begin(), str.end(), re, -1),sregex_token_iterator()
    };
    //remove all empty string
    vector<string>::iterator it = splitResult.begin();
    while (it != splitResult.end())
    {
        if ((*it) == "") it = splitResult.erase(it);
        else it++;
    }
    return splitResult;
}

short Disk::locateInodeFromPath(std::string path)
{
    vector<string> pathList = stringSplit(path, "/");
    short inode_id_ptr = currentInode.inode_id;
    for (size_t i = 0; i < pathList.size(); i++)
    {
        iNode inode_ptr = super.loadInode(inode_id_ptr, diskFile); // 从现在的inode开始
        if (!inode_ptr.isDir) {
            printf("%s is a file! Please check your path again!\n",
                   getFileName(inode_ptr).c_str());
            return -1;
        }
        Directory dir = readFileEntriesFromDirectoryFile(inode_ptr);//打开现在inode的entry，开始匹配
        short nextDirectoryInode = dir.findInFileEntries(pathList[i].c_str());
        if (nextDirectoryInode != -1) {
            inode_id_ptr = nextDirectoryInode;
        }
        else {
            printf("File/Directory not found: %s\n", pathList[i].c_str());
            return -1;
        }
    }
    return inode_id_ptr;
}

void Disk::recursiveDeleteDirectory(iNode inode)
{
    if (inode.fileSize == 2 * sizeof(fileEntry)) { // 只有两个初始directory
        string filePath = getFullFilePath(inode);
        printf("Directory deleted %s: %s\n",
               (deleteFile(inode) ? "successful" : "failed"), filePath.c_str());
        return;
    }
    Directory dir = readFileEntriesFromDirectoryFile(inode);
    for (size_t i = 0; i < dir.files.size(); i++)
    {
        if (!strcmp(dir.files[i].fileName, ".") || !strcmp(dir.files[i].fileName, ".."))
            continue;
        iNode current_inode_ptr = super.loadInode(dir.files[i].inode_id, diskFile);
        if (current_inode_ptr.isDir) {
            recursiveDeleteDirectory(current_inode_ptr);
        }
        else {
            string filePath = getFullFilePath(current_inode_ptr);
            printf("File deleted %s: %s\n",
                   (deleteFile(current_inode_ptr) ? "successful" : "failed"), filePath.c_str());
        }
    }
    string filePath = getFullFilePath(inode);
    printf("Directory deleted %s: %s\n",
           (deleteFile(inode) ? "successful" : "failed"), filePath.c_str()); // 删干净自己目录下的所有东西，再把自己删了
    return;
}

bool Disk::deleteFile(iNode inode)  // 删掉file，需要做的操作有 free掉inode 对应的block， free 掉inode， free 掉 fileEntry
{
    int blockCount = (int)ceil((double)inode.fileSize / super.BLOCK_SIZE);
    if (blockCount <= DIRECT_ADDRESS_NUMBER) {
        for (size_t i = 0; i < blockCount; i++)
        {
            freeBlock(inode.direct[i], diskFile);
        }
    }
    else
    {
        for (size_t i = 0; i < DIRECT_ADDRESS_NUMBER; i++)
        {
            freeBlock(inode.direct[i], diskFile);
        }
        IndirectDiskblock idb;
        idb.load(inode.indirect);
        int remainDataBlock = blockCount - DIRECT_ADDRESS_NUMBER;
        for (size_t i = 0; i < remainDataBlock; i++)
        {
            freeBlock(idb.addrs[i]);
        }
        freeBlock(inode.indirect);
    }
    super.freeInode(inode.inode_id, diskFile);
    iNode parent_inode = super.loadInode(inode.parent, diskFile);
    Directory parent_dir = readFileEntriesFromDirectoryFile(parent_inode);
    for (size_t i = 0; i < parent_dir.files.size(); i++)
    {
        if (parent_dir.files[i].inode_id == inode.inode_id)
        {
            parent_dir.files.erase(parent_dir.files.begin() + i);
        }
    }
    int parent_block_count = (int)ceil((double)parent_inode.fileSize / super.BLOCK_SIZE);
    // 边界，如果刚好那个entry在
    if (parent_block_count <= DIRECT_ADDRESS_NUMBER)
    {
        if (parent_inode.fileSize % super.BLOCK_SIZE == sizeof(fileEntry)) {
            freeBlock(parent_inode.direct[parent_block_count - 1], diskFile);
        }
    }
    else {
        IndirectDiskblock idb;
        idb.load(parent_inode.indirect, diskFile);
        if (parent_inode.fileSize == DIRECT_ADDRESS_NUMBER * super.BLOCK_SIZE + sizeof(fileEntry)) {
            freeBlock(idb.addrs[parent_block_count - 1 - DIRECT_ADDRESS_NUMBER], diskFile);
            freeBlock(parent_inode.indirect, diskFile);
            parent_inode.indirect = Address(0);
        }
        else if (parent_inode.fileSize % super.BLOCK_SIZE == sizeof(fileEntry)) {
            freeBlock(idb.addrs[parent_block_count - 1 - DIRECT_ADDRESS_NUMBER], diskFile);
            idb.addrs[parent_block_count - 1 - DIRECT_ADDRESS_NUMBER] = Address(0);
        }
    }
    parent_inode.fileSize -= sizeof(fileEntry);
    if (!super.writeInode(parent_inode, diskFile)) { printf("Failed to update parent inode (id: %d)!\n", parent_inode.inode_id); return false; }
    if (!writeFileEntriesToDirectoryFile(parent_dir, parent_inode)) { printf("Failed to update parent directory file (inode id: %d)!\n", parent_inode.inode_id); return false; }
    return true;
}

unsigned Disk::getDirectorySize(iNode current)
{
    if ((current.isDir && current.fileSize == 2 * sizeof(fileEntry)) || !current.isDir)
        return current.fileSize;
    Directory dir = readFileEntriesFromDirectoryFile(current);
    unsigned sum = current.fileSize;
    for (size_t i = 0; i < dir.files.size(); i++)
    {
        iNode child = super.loadInode(dir.files[i].inode_id, diskFile);
        if (child.inode_id == current.parent || child.inode_id == current.inode_id) continue;
        sum += getDirectorySize(child);
    }
    return sum;
}

void Disk::printWelcomeInfo()
{

    printf("#################################################\n");
    printf("#    Index-node-based File Management System    #\n");
    printf("#                  Version 1.0                  #\n");
    printf("#                                               #\n");
    printf("#                 Developed by:                 #\n");
    printf("#         Tianyi Xiang    Haorui  Song          #\n");
    printf("#         201836020389    201830581404          #\n");
    printf("#################################################\n");
    printf("#   School of Computer Science and Engineering  #\n");
    printf("#      South China University of Technology     #\n");
    printf("#################################################\n");
    printf("\nYou can input 'help' to get command instructions!\n");
    printf("\n");
}

void Disk::printHelpInfo()
{
    printf("Create a file: createFile/mkfile fileName fileSize(in KB) \neg. mkfile /mydir/hello.txt 10\n\n");
    printf("Delete a file: deleteFile/rmfile filename \neg. rmfile /mydir/hello.txt\n\n");
    printf("Create a directory: createDir/mkdir dirName\neg. mkdir /dir1/sub1\n\n");
    printf("Delete a directory: deleteDir/rmdir dirName\neg. rmdir /dir1/sub1\n\n");
    printf("Change current working directory: changeDir/cd dirName\neg. cd dir2\n\n");
    printf("List directory: dir\n\n");
    printf("Copy file: cp sourceFile targetFile\neg. cp /dir1/sub1/hello.txt /dir2/sub2/hello.txt\n\n");
    printf("Display the usage of storage space: sum\n\n");
    printf("Print out file contents: cat fileName\neg. cat /dir1/file1\n\n");
}

iNode superBlock::loadInode(short id,FILE* file)
{
    bool specify_FILE_object = file != NULL;
    if (!specify_FILE_object) file = fileOpen(DISK_PATH, "rb+");
    iNode inode;
    globalSleepLock.wait();
    if (fileSeek(file, inodeStart + id * INODE_SIZE, SEEK_SET)) return iNode(0, -1, -1);
    if (fileRead(&inode, sizeof (iNode), 1, file) != 1) return iNode(0, -1, -1);
    globalSleepLock.notify();
    if (!specify_FILE_object) fclose(file);
    return inode;
}

bool superBlock::writeInode(iNode inode, FILE* file)
{
    bool specify_FILE_object = file != NULL;
    if (!specify_FILE_object) file = fileOpen(DISK_PATH, "rb+");
    globalSleepLock.wait();
    if (fileSeek(file, inodeStart + inode.inode_id * INODE_SIZE, SEEK_SET)) return false;
    globalSleepLock.notify();
    if (fileWrite(&inode, sizeof (iNode), 1, file) != 1)return false;
    if (!specify_FILE_object) fclose(file);
    return true;
}


superBlock::superBlock()
{
    inodeNumber = INITIAL_INODE_NUMBER;
    freeInodeNumber = INITIAL_INODE_NUMBER;
    dataBlockNumber = INITIAL_DATA_BLOCK_NUMBER;
    freeDataBlockNumber = INITIAL_DATA_BLOCK_NUMBER;
    BLOCK_SIZE = INITIAL_BLOCK_SIZE;
    INODE_SIZE = INITIAL_INODE_SIZE;
    SUPERBLOCK_SIZE = INITIAL_BLOCK_SIZE;
    superBlockStart = sizeof(magic_number) - 1;
    TOTAL_SIZE = INITIAL_DISK_SIZE;
    inodeStart = INITIAL_SUPERBLOCK_SIZE;
    blockStart = inodeStart + inodeNumber * INITIAL_INODE_SIZE;
    memset(inodeMap, 0, sizeof(inodeMap));
}

int superBlock::allocateNewInode(unsigned fileSize, int parent, Address direct[], Address* indirect, FILE* file, bool isDir)
{
    bool specify_FILE_object = file != NULL;

    if (freeInodeNumber == 0) {
        printf("No free index-node can be allocated!\n");
        return -1;
    }
    if (!specify_FILE_object) file = fileOpen(DISK_PATH, "rb+");
    int arrayIndex;
    int offset;
    bool found = false;
    for (size_t i = 0; i < inodeNumber; i++)
    {
        arrayIndex = i / 8;
        offset = i % 8;
        if ((inodeMap[arrayIndex] >> (7 - offset)) % 2 == 0) {
            found = true;
            break;
        }
    }
    if (!found) {
        printf("No free index-node can be allocated!\n");
        return -1;
    }
    inodeMap[arrayIndex] |= (1 << (7 - offset));
    iNode inode(fileSize, parent, arrayIndex * 8 + offset, isDir);  // iNode::iNode(unsigned fileSize, int parent, int inode_id, bool isDir)
    if (fileSize <= DIRECT_ADDRESS_NUMBER * BLOCK_SIZE)
    {
        // linux编译报错，改一下
        for (size_t i = 1; i <= ceil((double)fileSize / BLOCK_SIZE); i++)
        {
            inode.direct[i - 1] = direct[i - 1];
        }
        // for (size_t i = 1; i <= fileSize / BLOCK_SIZE + 1; i++)
        // {
        // 	inode.direct[i - 1] = direct[i - 1];
        // }
    }
    else {
        for (size_t i = 1; i <= DIRECT_ADDRESS_NUMBER; i++)
        {
            inode.direct[i - 1] = direct[i - 1];
        }
        inode.indirect = *indirect;
    }

    if (!specify_FILE_object) file = fileOpen(DISK_PATH, "rb+");
    globalSleepLock.wait();
    if (fileSeek(file, inodeStart + inode.inode_id * INODE_SIZE, SEEK_SET)) return -2;
    if (fileWrite(&inode, sizeof (iNode), 1, file) != 1) return -2;
    globalSleepLock.notify();
    if (!specify_FILE_object) { fclose(file); file = NULL; }
    freeInodeNumber--;
    if (!updateSuperBlock(file)) {
        printf("Update super block failed!\n");
        return -3;
    }
    return arrayIndex * 8 + offset;
}

bool superBlock::freeInode(int inodeid, FILE* file)
{
    int arrayIndex = inodeid / 8;
    int offset = inodeid % 8;
    inodeMap[arrayIndex] &= (~(1 << (7 - offset)));
    freeInodeNumber++;
    return updateSuperBlock(file);
}

bool superBlock::updateSuperBlock(FILE* file)
{
    bool specify_FILE_object = file != NULL;
    if (!specify_FILE_object) file = fileOpen(DISK_PATH, "rb+");
    globalSleepLock.wait();
    if (fileSeek(file, superBlockStart, SEEK_SET)) return false;
    if (fileWrite(this, sizeof (superBlock), 1, file) != 1) return false;
    globalSleepLock.notify();
    if (!specify_FILE_object) { fclose(file); }
    return true;
}


void Diskblock::refreshContent()
{
    memset(content, 0, sizeof content);
}

Diskblock::Diskblock(int addrInt)
{
    load(addrInt);
}

Diskblock::Diskblock(Address addr)
{
    load(addr);
}

void Diskblock::load(int addrInt, FILE* file, int numByte)
{
    refreshContent();
    bool specify_FILE_object = file != NULL;
    if(!specify_FILE_object) file = fileOpen(DISK_PATH, "rb+");
    globalSleepLock.wait();
    fileSeek(file, addrInt, SEEK_SET);
    fileRead(content, numByte, 1, file);
    globalSleepLock.notify();
    if (!specify_FILE_object) fclose(file);
}

void Diskblock::load(Address addr, FILE* file, int numByte)
{
    int addrInt = addr.to_int();
    load(addrInt, file);
}

void Diskblock::write(int addrInt, FILE* file, int numByte)
{
    bool specify_FILE_object = file != NULL;
    if (!specify_FILE_object) file = fileOpen(DISK_PATH, "rb+");
    globalSleepLock.wait();
    fileSeek(file, addrInt, SEEK_SET);
    fileWrite(content, numByte, 1, file);
    globalSleepLock.notify();
    if (!specify_FILE_object) fclose(file);
    refreshContent();
}

void Diskblock::write(Address addr, FILE* file, int numByte)
{

    int addrInt = addr.to_int();
    write(addrInt, file);
}

void IndirectDiskblock::load(Address a,FILE* file)
{
    bool specify_FILE_object = file != NULL;
    int addrInt = a.to_int();
    if(!specify_FILE_object) file = fileOpen(DISK_PATH, "rb+");
    memset(addrs, 0, sizeof(addrs));
    globalSleepLock.wait();
    fileSeek(file, addrInt, SEEK_SET);
    fileRead(addrs, 3, NUM_INDIRECT_ADDRESSES, file);
    globalSleepLock.notify();
    if (!specify_FILE_object) fclose(file);
}

void IndirectDiskblock::write(Address blockAddr,FILE* file)  // still untested
{
    bool specify_FILE_object = file != NULL;
    int addrInt = blockAddr.to_int();
    if (!specify_FILE_object) file = fileOpen(DISK_PATH, "rb+");
    globalSleepLock.wait();
    fileSeek(file, addrInt, SEEK_SET);
    fileWrite(addrs, sizeof addrs, 1, file);
    globalSleepLock.notify();
    if (!specify_FILE_object) fclose(file);
}



// // 初始化锁
// void iNode::initLock() {
//     lock = new RWLock();
// }

// // 删除锁
// void iNode::deleteLock() {
//     delete lock;
// }

iNode::iNode(unsigned fileSize, int parent, int inode_id, bool isDir)
        : fileSize(fileSize), parent(parent), inode_id(inode_id), isDir(isDir) {
    inode_create_time = std::time(nullptr);
    inode_access_time = inode_create_time;
    inode_modify_time = inode_create_time;
    //initLock();
}

iNode::iNode() : iNode(0, -1, -1, true) {}

iNode::~iNode() {
    //deleteLock();
}

iNode::iNode(const iNode& other)
        : fileSize(other.fileSize), inode_create_time(other.inode_create_time),
          inode_access_time(other.inode_access_time), inode_modify_time(other.inode_modify_time),
          isDir(other.isDir), parent(other.parent), inode_id(other.inode_id) {
    std::copy(std::begin(other.direct), std::end(other.direct), std::begin(direct));
    indirect = other.indirect;
    //initLock();  // 为每个副本创建一个新的锁
}

iNode& iNode::operator=(const iNode& other) {
    if (this != &other) {
        fileSize = other.fileSize;
        inode_create_time = other.inode_create_time;
        inode_access_time = other.inode_access_time;
        inode_modify_time = other.inode_modify_time;
        isDir = other.isDir;
        parent = other.parent;
        inode_id = other.inode_id;
        std::copy(std::begin(other.direct), std::end(other.direct), std::begin(direct));
        indirect = other.indirect;
        //deleteLock();  // 删除旧的锁
        //initLock();  // 为每个副本创建一个新的锁
    }
    return *this;
}
iNode::iNode(iNode&& other) noexcept
        : fileSize(other.fileSize), inode_create_time(other.inode_create_time),
          inode_access_time(other.inode_access_time), inode_modify_time(other.inode_modify_time),
          isDir(other.isDir), parent(other.parent), inode_id(other.inode_id){
    std::copy(std::begin(other.direct), std::end(other.direct), std::begin(direct));
    indirect = other.indirect;
    //other.lock = nullptr;  // 防止原对象析构时删除锁
}

iNode& iNode::operator=(iNode&& other) noexcept {
    if (this != &other) {
        fileSize = other.fileSize;
        inode_create_time = other.inode_create_time;
        inode_access_time = other.inode_access_time;
        inode_modify_time = other.inode_modify_time;
        isDir = other.isDir;
        parent = other.parent;
        inode_id = other.inode_id;
        std::copy(std::begin(other.direct), std::end(other.direct), std::begin(direct));
        indirect = other.indirect;
        //deleteLock();  // 删除旧的锁
        //lock = other.lock;  // 移动锁指针
        //other.lock = nullptr;  // 防止原对象析构时删除锁
    }
    return *this;
}

void iNode::updateCreateTime()
{
    time(&inode_create_time);
}

void iNode::updateModifiedTime()
{
    time(&inode_access_time);
}

void iNode::updateAccessTime()
{
    time(&inode_modify_time);
}

string iNode::getCreateTime()
{
    string time_str = ctime((time_t const*)&(inode_create_time));
    time_str = time_str.substr(0, time_str.size() - 1);
    return time_str;
}

string iNode::getModifiedTime()
{
    string time_str = ctime((time_t const*)&(inode_modify_time));
    time_str = time_str.substr(0, time_str.size() - 1);
    return time_str;
}

string iNode::getAccessTime()
{
    string time_str = ctime((time_t const*)&(inode_access_time));
    time_str = time_str.substr(0, time_str.size() - 1);
    return time_str;
}




void DiskblockManager::initialize(superBlock* super, FILE* file)
{

    freeptr = super->blockStart;
    IndirectDiskblock iblock;
    iblock.load(Address(super->blockStart),file);
    iblock.addrs[0] = freeptr;
    iblock.write(Address(super->blockStart),file);
    for (int i = super->blockStart / super->BLOCK_SIZE + 1; i < super->TOTAL_SIZE / super->BLOCK_SIZE; i++) {
        Address freeAddr = i * super->BLOCK_SIZE;
        free(freeAddr,file);		//空闲块地址，初始化
    }
}

Address DiskblockManager::alloc(FILE* file)
{
    // load the current linked-list block
    IndirectDiskblock iblock;
    Address blockAddr = freeptr.block_addr();
    iblock.load(blockAddr,file);
    //到头了
    if (freeptr.offset().to_int() == 0) {
        Address preBlockAddr = iblock.addrs[0];
        // no free block
        if (preBlockAddr == blockAddr) {  // 自己指向自己 说明没有了
            printf("out of disk memory!\n");
            return Address(0);
        }
        memset(iblock.addrs, 0, sizeof iblock.addrs);
        iblock.write(blockAddr, file);
        iblock.load(preBlockAddr, file);
        freeptr = preBlockAddr + (NUM_INDIRECT_ADDRESSES - 1) * 3;  //先前block的指向最后一个
        Address freeAddr = iblock.addrs[freeptr.offset().to_int() / 3];
        iblock.addrs[freeptr.offset().to_int() / 3].from_int(0); //分配了的地址置0
        freeptr = freeptr - 3; // 向上移动
        iblock.write(preBlockAddr, file); // 重新写回去

        return freeAddr;
    }
    else {
        Address freeAddr = iblock.addrs[freeptr.offset().to_int() / 3];
        iblock.addrs[freeptr.offset().to_int() / 3].from_int(0);
        freeptr = freeptr - 3;
        iblock.write(blockAddr, file);
        return freeAddr;
    }
}

void DiskblockManager::free(Address freeAddr, FILE* file)  // addr is the freed or after used block address
{
    // load the current linked-list block
    IndirectDiskblock iblock;
    Address blockAddr = freeptr.block_addr();
    iblock.load(blockAddr,file);

    // if free pointer not points to the last address in the block
    if (freeptr.offset().to_int() != (NUM_INDIRECT_ADDRESSES - 1) * 3) {
        freeptr = freeptr + 3;
        iblock.addrs[freeptr.offset().to_int() / 3] = freeAddr;
        iblock.write(blockAddr,file);
    }
    else {
        Address newNodeAddr = iblock.addrs[NUM_INDIRECT_ADDRESSES - 1];  // get the last free-address in the block.
        cout << "new linked-list block " << newNodeAddr.to_int() << endl;
        iblock.load(newNodeAddr,file);
        memset(iblock.addrs, 0, sizeof iblock.addrs);
        iblock.addrs[0] = blockAddr;
        iblock.addrs[1] = freeAddr;
        freeptr.from_int(newNodeAddr.to_int() + 3);
        iblock.write(newNodeAddr,file);
    }
}

void DiskblockManager::printBlockUsage(superBlock* super,FILE* file)
{
    int linkedListBlock = getLinkedListBlock(file);
    int freeBlock = getFreeBlock(linkedListBlock);
    printf("Block usage: %d/%d (%.4f%%)\n",
           super->TOTAL_SIZE / super->BLOCK_SIZE - freeBlock,
           super->TOTAL_SIZE / super->BLOCK_SIZE,
           100 * (super->TOTAL_SIZE / super->BLOCK_SIZE - freeBlock) /
           (double)(super->TOTAL_SIZE / super->BLOCK_SIZE));
    cout << "blocks for linked list: " << linkedListBlock << endl;
    cout << "blocks used for data: " <<
         (super->TOTAL_SIZE - super->SUPERBLOCK_SIZE - super->INODE_SIZE * super->inodeNumber) / super->BLOCK_SIZE
         - freeBlock - linkedListBlock << endl;
    cout << "blocks used for Inodes: " << (super->INODE_SIZE * super->inodeNumber) / super->BLOCK_SIZE << endl;
    cout << "blocks used for superblock: " << super->SUPERBLOCK_SIZE / super->BLOCK_SIZE << endl;
}
int DiskblockManager::getFreeBlock(int linkedListBlock)
{
    return (linkedListBlock - 1) * 339 + freeptr.offset().to_int() / 3;   // 339=341-2
}
int DiskblockManager::getFreeBlock(FILE* file)
{
    return (getLinkedListBlock(file) - 1) * 339 + freeptr.offset().to_int() / 3;
}



int DiskblockManager::getLinkedListBlock(FILE* file)
{
    int linkedListBlock = 0;
    Address ptr = freeptr.block_addr();
    IndirectDiskblock iblock;
    iblock.load(ptr, file);
    do
    {
        linkedListBlock += 1;
        ptr = iblock.addrs[0];
        iblock.load(ptr, file);
    } while (ptr != iblock.addrs[0]);
    linkedListBlock += 1;
    return linkedListBlock;
}

int DiskblockManager::getDedicateBlock(superBlock* super, FILE* file)
{
    return getLinkedListBlock(file) + (super->INODE_SIZE * super->inodeNumber) / super->BLOCK_SIZE + super->SUPERBLOCK_SIZE / super->BLOCK_SIZE;
}


Node::~Node() {}
//结点创建对象
Node::Node()
{
    isLeaf = true;
    count = 0;
    Parent = NULL;
}
//叶子结点创建对象
Leaf_Node::Leaf_Node()
{
    isLeaf = true;
    Pre_Node = NULL;
    Next_Node = NULL;
}
//中间结点创建对象
Inter_Node::Inter_Node()
{
    isLeaf = false;
    for (int i = 0; i < M * 2 + 1; i++)
        Child[i] = NULL;
}
//Bplus创建对象
Bplus::Bplus()
{
    Root = NULL;
}

//结点查找兄弟结点
Node* Node::GetBrother(int& flag)
{
    if (NULL == Parent)
        return NULL;

    Node* p = NULL;
    for (int i = 0; i <= Parent->count; i++)
    {
        if (Parent->Child[i] == this)
        {
            if (i == Parent->count)
            {
                p = Parent->Child[i - 1];//左兄弟 flag=1
                flag = 1;
            }
            else
            {
                p = Parent->Child[i + 1];//右兄弟 flag=2
                flag = 2;
            }
        }
    }
    return p;
}

//结点输出
void Node::Print()
{
    for (int i = 0; i < count; i++)
    {
        cout << "(" << key[i]<<"  iNode id : "<<key[i].inode_id<< ")  ";
        if (i >= count - 1)
            cout << " | ";
    }
}









Inter_Node::~Inter_Node()
{
    for (int i = 0; i < M * 2 + 1; i++)
        Child[i] = NULL;
}
bool Inter_Node::Merge(Inter_Node* p)
{
    key[count] = p->Child[0]->key[0];
    count++;
    Child[count] = p->Child[0];
    for (int i = 0; i < p->count; i++)
    {
        key[count] = p->key[i];
        count++;
        Child[count] = p->Child[i + 1];
    }
    return true;
}

//中间结点插入
bool Inter_Node::Insert(fileEntry value, Node* New)
{
    int i = 0;
    for (; (i < count) && (value > key[i]); i++)//i指向key要插入的位置
    {
    }
    for (int j = count; j > i; j--)//挪动倒地方
        key[j] = key[j - 1];
    for (int j = count + 1; j > i + 1; j--)//父亲key值改变，孩子移动；
        Child[j] = Child[j - 1];
    key[i] = value;//关键字传到父亲节点
    Child[i + 1] = New;//newnode放到该放的位置
    New->Parent = this;
    count++;
    return true;
}

//中间结点分裂
fileEntry Inter_Node::Split(Inter_Node* p, fileEntry k)
{
    int i = 0, j = 0;
    if ((k > this->key[M - 1]) && (k < this->key[M]))//分裂的地方在中间
    {
        for (i = M; i < M * 2; i++, j++)
            p->key[j] = this->key[i];//拷贝后面值进brother
        j = 1;
        for (i = M + 1; i <= M * 2; i++, j++)
        {
            this->Child[i]->Parent = p;//孩子跟着往后移动
            p->Child[j] = this->Child[i];
        }
        this->count = M;//关键子数量各位一半
        p->count = M;
        return k;
    }
    int pos = k < this->key[M - 1] ? (M - 1) : M;//看k大小和中间-1比较，定位在前面还是在后面节点
    k = this->key[pos];//pos为分裂点,定位为前还是后分裂点,最后肯定为中间值
    j = 0;
    for (i = pos + 1; i < M * 2; i++, j++)//前节点考后节点,从插入的位置分，插入以后的放到新节点
        p->key[j] = this->key[i];
    j = 0;
    for (i = pos + 1; i <= M * 2; i++, j++)//将孩子送给兄弟
    {
        this->Child[i]->Parent = p;
        p->Child[j] = this->Child[i];
    }
    this->count = pos;
    p->count = M * 2 - pos - 1;
    return k;
}

//中间结点删除
bool Inter_Node::Delete(fileEntry k)
{
    int i = 0;
    for (; (k >= key[i]) && (i < count); i++)
    {
    }
    for (int j = i - 1; j < count - 1; j++)
        key[j] = key[j + 1];
    // k = i;
    int m = i;
    for (; m < count; m++)
    {
        Child[m] = Child[m + 1];
    }
    Child[m] = NULL;
    count--;
    return true;
}

//中间结点
bool Inter_Node::Slib(Inter_Node* p)
{
    int i, j;
    if (p->key[0] < this->key[0])
    {
        for (i = count; i > 0; i--)
            key[i] = key[i - 1];
        for (j = count + 1; j > 0; j--)
            Child[j] = Child[j - 1];
        key[0] = Child[0]->key[0];
        Child[0] = p->Child[p->count];
    }
    else
    {
        key[count] = p->Child[0]->key[0];
        Child[count + 1] = p->Child[0];
        for (i = 1; i < p->count - 1; i++)
            p->key[i - 1] = p->key[i];
        for (j = 0; j < p->count - 1; j++)
            p->Child[j] = p->Child[j + 1];
    }
    this->count++;
    p->count--;
    return true;
}














Leaf_Node::~Leaf_Node() {}
fileEntry Leaf_Node::Split(Leaf_Node* p)
{
    int j = 0;
    for (int i = M; i < M * 2; i++, j++)//把值copy到新节点
        p->key[j] = this->key[i];//this为old node
    this->count = this->count - j;
    p->count = j;
    return p->key[0];
}

//叶子结点删除
bool Leaf_Node::Delete(fileEntry value)
{
    bool found = false;
    int i = 0;
    for (; i < count; i++)
    {
        if (value == key[i])
        {
            found = true;
            break;
        }
    }
    if (false == found)
        return false;
    int j = i;
    for (; j < count - 1; j++)
        key[j] = key[j + 1];
    key[j] = "";
    count--;
    return true;
}

//叶子结点的插入
bool Leaf_Node::Insert(fileEntry value)
{
    int i = 0;
    for (; (value > key[i]) && (i < count); i++)//按顺序
    {
    }
    for (int j = count; j > i; j--)//移动，找到应该插入的关键字位置
        key[j] = key[j - 1];
    key[i] = value;//插入关键字
    count++;
    return true;
}

//叶结点查找
Leaf_Node* Bplus::Find(fileEntry data)//data为关键字
{
    int i = 0;
    Node* p = Root; //?????bplus的跟
    Inter_Node* q;  //?м???
    while (NULL != p)
    {
        if (p->isLeaf) //????????????
            break;
        for (i = 0; i < p->count; i++) //??????p??key?????p不是叶子，循环至count的节点
        {
            if (data < p->key[i])
                break;
        }
        q = (Inter_Node*)p;
        p = q->Child[i];
    }

    return (Leaf_Node*)p;//把根return,如果跟为空,第一个插入函数生成的节点即为根
}

//叶子结点,合并叶子结点
bool Leaf_Node::Merge(Leaf_Node* p)
{
    if (this->count + p->count > M * 2)//如果加在一起格满说明不需要合并
        return false;
    for (int i = 0; i < p->count; i++)//否则将oldnode的关键字都插入到bro里
        this->Insert(p->key[i]);
    return true;
}













Bplus::~Bplus() {}
//B+树添加结点
bool Bplus::Add_Node(Inter_Node* p, fileEntry k, Node* New_Node)
{
    if (NULL == p || p->isLeaf)
        return false;
    if (p->count < M * 2)//父亲不满
        return p->Insert(k, New_Node);
    Inter_Node* Brother = new Inter_Node;
    //叶子节点满，父节点也满分裂情况
    fileEntry NewKey = p->Split(Brother, k);//NewKey为需要提取并插入到root节点的值

    //确定需要插入的关键字，是插入到分裂节点的哪个位置
    if (p->count < Brother->count)
    {
        p->Insert(k, New_Node);
    }
    else if (p->count > Brother->count)
    {
        Brother->Insert(k, New_Node);
    }
    else
    {
        Brother->Child[0] = New_Node;
        New_Node->Parent = Brother;
    }
    Inter_Node* parent = (Inter_Node*)(p->Parent);
    if (NULL == parent)
    {
        parent = new Inter_Node();
        parent->Child[0] = p;
        parent->key[0] = NewKey;//newkey为分裂传回，为插入的中间值
        parent->Child[1] = Brother;
        p->Parent = parent;
        Brother->Parent = parent;
        parent->count = 1;
        Root = parent;
        return true;
    }
    return Add_Node(parent, NewKey, Brother);
}

//B+树查找data
short Bplus::Search(fileEntry data)
{
    int i = 0;
    // sPath = "查找路径为: ";
    Node* p = Root;
    if (NULL == p)
        return 0;
    Inter_Node* q;
    while (NULL != p)
    {
        if (p->isLeaf)
            break;
        for (i = 0; (i < p->count) && (data >= p->key[i]); i++)
        {
        }
        int k = i > 0 ? i - 1 : i;
        // sPath += p->key[k].fileName;
        // sPath += "-->";
        q = (Inter_Node*)p;
        p = q->Child[i];
    }
    if (NULL == p)
        return 0;
    // sPath += p->key[0].fileName;
    // bool found = false;
    for (i = 0; i < p->count; i++)
    {
        if (data == p->key[i]){
            // found = true;
            return p->key[i].inode_id;
        }

        //sPath += to_string(p->key[i]);
        //sPath += "-->";
    }
    // if (found)
    // {
    //     // sPath += " OK";
    // }
    // else
    // {
    //     // sPath += " FAIL";
    // }
    // return found;
    return 0;
}

short Bplus::Search(const char * name)
{
    fileEntry data(name,1);
    int i = 0;
    // sPath = "查找路径为: ";
    Node* p = Root;
    if (NULL == p)
        return 0;
    Inter_Node* q;
    while (NULL != p)
    {
        if (p->isLeaf)
            break;
        for (i = 0; (i < p->count) && (data >= p->key[i]); i++)
        {
        }
        int k = i > 0 ? i - 1 : i;
        // sPath += p->key[k].fileName;
        // sPath += "-->";
        // cout<<sPath<<endl;
        q = (Inter_Node*)p;
        p = q->Child[i];
    }
    if (NULL == p)
        return 0;
    // sPath += p->key[0].fileName;
    bool found = false;
    for (i = 0; i < p->count; i++)
    {
        if (data == p->key[i]){
            return p->key[i].inode_id;
            // found = true;
        }

        //sPath += to_string(p->key[i]);
        //sPath += "-->";
    }
    // if (found)
    // {
    //     // sPath += " OK";
    // }
    // else
    // {
    //     // sPath += " FAIL";
    // }
    return 0;
}


//B+树的插入
bool Bplus::Insert(fileEntry data) //data为插入的关键字
{

    // string a;
    if (Search(data)>0)//查找要插入的值
        return false;

    Leaf_Node* Old_Node = Find(data);//找到需要插入的叶子节点定义为oldnode

    if (NULL == Old_Node)
    {
        Old_Node = new Leaf_Node;//树为空
        Root = Old_Node;
    }

    if (Old_Node->count < M * 2) {//有空间插入，直接插进去并返回
        return Old_Node->Insert(data);

    }

    Leaf_Node* New_Node = new Leaf_Node;//即将分裂

    fileEntry k = Old_Node->Split(New_Node);//k为新节点第一个关键子

    Leaf_Node* OldNext = Old_Node->Next_Node;
    Old_Node->Next_Node = New_Node;//相邻叶子节点相连
    New_Node->Next_Node = OldNext;
    New_Node->Pre_Node = Old_Node;

    if (NULL != OldNext)
        OldNext->Pre_Node = New_Node;

    if (data < k)//小于newnode key[0]，插前面，否则插后面
    {
        Old_Node->Insert(data);
    }
    else
    {
        New_Node->Insert(data);
    }
    Inter_Node* parent = (Inter_Node*)(Old_Node->Parent);

    if (NULL == parent)//初始化parent，若没有父结点，新建一个
    {
        Inter_Node* New_Root = new Inter_Node;
        New_Root->Child[0] = Old_Node;
        New_Root->key[0] = k;
        New_Root->Child[1] = New_Node;
        Old_Node->Parent = New_Root;
        New_Node->Parent = New_Root;
        New_Root->count = 1;
        Root = New_Root;
        return true;
    }

    return Add_Node(parent, k, New_Node);//向父亲里插值或者分裂父亲建立新的节点
}

//B+树的删除
bool Bplus::Delete(fileEntry data)
{
    Leaf_Node* Old_Node = Find(data); //查找数据
    if (NULL == Old_Node)//树为空
        return false;
    if (false == Old_Node->Delete(data)) //删除
        return false;
    Inter_Node* parent = (Inter_Node*)(Old_Node->Parent);
    if (NULL == parent)
    {
        if (0 == Old_Node->count)//将整棵树删掉，没父亲没key
        {
            delete Old_Node;
            Root = NULL;
        }
        return true;
    }
    if (Old_Node->count >= M)
    {
        for (int i = 0; (i < parent->count) && (data >= parent->key[i]); i++)
        {
            if (parent->key[i] == data)//如果要删除的key比父亲索引他的值要大就直接删除，如果相等，就给父亲一个新索引
                parent->key[i] = Old_Node->key[0];
        }
        return true;
    }
    //不满足规格，要合并或借值
    int flag = 1;
    Leaf_Node* Brother = (Leaf_Node*)(Old_Node->GetBrother(flag));
    fileEntry NewData;
    if (Brother->count > M)//借值
    {
        if (1 == flag)//左兄弟
        {
            NewData = Brother->key[Brother->count - 1];//要被借走的数据
        }
        else//右兄弟
        {
            NewData = Brother->key[0];
        }
        Old_Node->Insert(NewData);
        Brother->Delete(NewData);
        //替换parent中的key值
        if (1 == flag)
        {
            for (int i = 0; i <= parent->count; i++)//向左兄弟借值
            {
                if (parent->Child[i] == Old_Node && i > 0)
                    parent->key[i - 1] = Old_Node->key[0];
            }
        }
        else
        {
            for (int i = 0; i <= parent->count; i++)//向右兄弟借值
            {
                if (parent->Child[i] == Old_Node && i > 0)
                    parent->key[i - 1] = Old_Node->key[0];
                if (parent->Child[i] == Brother && i > 1)
                    parent->key[i - 1] = Brother->key[0];
            }
        }
        return true;
    }
    fileEntry NewKey;
    if (1 == flag)//无法借值，合并
    {
        Brother->Merge(Old_Node);
        NewKey = Old_Node->key[0];//标记要删除的父亲里的key
        Leaf_Node* OldNext = Old_Node->Next_Node;//接入后面兄弟
        Brother->Next_Node = OldNext;
        if (NULL != OldNext)
            OldNext->Pre_Node = Brother;
        delete Old_Node;
    }
    else
    {
        Old_Node->Merge(Brother);
        NewKey = Brother->key[0];
        Leaf_Node* OldNext = Brother->Next_Node;
        Old_Node->Next_Node = OldNext;
        if (NULL != OldNext)
            OldNext->Pre_Node = Old_Node;
        delete Brother;
    }
    return Remove_Node(parent, NewKey);//移除parent或者移除parent中关键字；
}

//Bplus 移除结点
bool Bplus::Remove_Node(Inter_Node* p, fileEntry k)
{
    if (false == p->Delete(k))
    {
        return false;
    }
    Inter_Node* parent = (Inter_Node*)(p->Parent);
    if (NULL == parent)
    {
        if (0 == p->count)
        {
            Root = p->Child[0];
            delete p;
        }
        return true;
    }
    if (p->count >= M)//父亲不合并
    {
        //删掉parent中的关键字
        for (int i = 0; (i < parent->count) && (k >= parent->key[i]); i++)
        {
            if (parent->key[i] == k)//看父亲的parent里有没有要删除的关键字，有就更新索引
                parent->key[i] = p->key[0];
        }
        return true;
    }
    //父亲合并
    int flag = 1;
    Inter_Node* Brother = (Inter_Node*)(p->GetBrother(flag));
    if (Brother->count > M)//父亲借值
    {
        p->Slib(Brother);
        if (1 == flag)
        {
            for (int i = 0; i <= parent->count; i++)
            {
                if (parent->Child[i] == p && i > 0)
                    parent->key[i - 1] = p->key[0];
            }
        }
        else
        {
            for (int i = 0; i <= parent->count; i++)
            {
                if (parent->Child[i] == p && i > 0)
                    parent->key[i - 1] = p->key[0];
                if (parent->Child[i] == Brother && i > 0)
                    parent->key[i - 1] = Brother->key[0];
            }
        }
        return true;
    }
    //兄弟借值
    fileEntry NewKey;
    if (1 == flag)
    {
        Brother->Merge(p);
        NewKey = p->key[0];
        delete p;
    }
    else
    {
        p->Merge(Brother);
        NewKey = Brother->key[0];
        delete Brother;
    }
    return Remove_Node(parent, NewKey);
}

//Bplus输出
void Bplus::Print()
{
    Node* p = Root;
    if (NULL == p)
        return;
    Inter_Node* a;
    int H = 0;
    queue<Node*> q;
    queue<int> h;
    q.push(p);
    h.push(1);
    while (!q.empty())
    {
        p = q.front();
        if (H != h.front())
        {
            cout << endl;
            cout << H << endl;
            H = h.front();
        }
        q.pop();
        h.pop();
        p->Print();
        if (NULL != p && !p->isLeaf)
        {
            a = (Inter_Node*)p;
            for (int i = 0; i <= p->count; i++)
            {
                q.push(a->Child[i]);
                h.push(H + 1);
            }
        }
    }
}













fileEntry::fileEntry() {
    std::memset(fileName, 0, MAXIMUM_FILENAME_LENGTH);
    inode_id = 0;
}

// 带参数的构造函数
fileEntry::fileEntry(const char* name, short id) {
    std::strncpy(fileName, name, MAXIMUM_FILENAME_LENGTH - 1);
    fileName[MAXIMUM_FILENAME_LENGTH - 1] = '\0'; // 确保字符串以null结尾
    inode_id = id;
}

// 小于运算符
bool fileEntry::operator<(const fileEntry& other) const {
    return std::strcmp(fileName, other.fileName) < 0;
}

// 等于运算符
bool fileEntry::operator==(const fileEntry& other) const {
    return std::strcmp(fileName, other.fileName) == 0;
}

// 大于运算符
bool fileEntry::operator>(const fileEntry& other) const {
    return std::strcmp(fileName, other.fileName) > 0;
}

// 大于等于运算符
bool fileEntry::operator>=(const fileEntry& other) const {
    return std::strcmp(fileName, other.fileName) > 0||std::strcmp(fileName, other.fileName) == 0;
}
// 赋值函数
void fileEntry::operator=(const char* name) {
    std::strncpy(fileName, name, MAXIMUM_FILENAME_LENGTH - 1);
    fileName[MAXIMUM_FILENAME_LENGTH - 1] = '\0'; // 确保字符串以null结尾
}
void fileEntry::operator=(const fileEntry& other)  {
    std::strncpy(fileName, other.fileName, MAXIMUM_FILENAME_LENGTH - 1);
    fileName[MAXIMUM_FILENAME_LENGTH - 1] = '\0'; // 确保字符串以null结尾
    inode_id=other.inode_id;
}


std::ostream& operator<<(std::ostream& os, const fileEntry& fe) {
    os << fe.fileName;
    return os;
}







Directory::Directory() {
    this->tree = new Bplus();
}
Directory::~Directory(){
    delete tree;
}
// short Directory::findInFileEntries(const char* name){
//     for (size_t i = 0; i < files.size(); i++)
//     {
//         if (!strcmp(files[i].fileName, name))
//             return files[i].inode_id;
//     }
//     return -1;
// }
void Directory::printFileTree(){
    tree->Print();
}

short Directory::findInFileEntries(const char* fileName) {
    return tree->Search(fileName);
}





LRUCache::LRUCache(int capacity) : capacity(capacity) {}

Directory LRUCache::get(short inode_id) {
    auto it = cache.find(inode_id);
    if (it == cache.end()) {
        return Directory(); // Not found
    }
    touch(it);
    return it->second.first;
}

void LRUCache::put(short inode_id, Directory tree) {
    auto it = cache.find(inode_id);
    if (it != cache.end()) {
        touch(it);
    } else {
        if (cache.size() == capacity) {
            cache.erase(used.front());
            used.pop_front();
        }
        used.push_back(inode_id);
    }
    cache[inode_id] = {tree, std::prev(used.end())};
}

void LRUCache::touch(std::unordered_map<short, std::pair<Directory, ListIt>>::iterator it) {
    short inode_id = it->first;
    used.erase(it->second.second);
    used.push_back(inode_id);
    it->second.second = std::prev(used.end());
}