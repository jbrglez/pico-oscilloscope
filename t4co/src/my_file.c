#ifndef MY_FILE_C
#define MY_FILE_C

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "my_types.h"


typedef struct {
    u64 contents_size;
    void *contents;
} file_contents_t;



void FreeFileMemory(void *Memory)
{
    free(Memory);
}


file_contents_t ReadEntireFile(char *FileName)
{
    file_contents_t Result = {};
    int FileHandle = open(FileName, O_RDONLY);
    if (FileHandle == -1) {
        return Result;
    }

    struct stat FileStatus;
    if (fstat(FileHandle, &FileStatus) == -1) {
        close(FileHandle);
        return Result;
    }
    Result.contents_size = (u64)FileStatus.st_size;

    Result.contents = malloc(Result.contents_size);
    if (!Result.contents) {
        Result.contents_size = 0;
        close(FileHandle);
        return Result;
    }

    u32 BytesToRead = Result.contents_size;
    u8 *NextByteLocation = (u8 *)Result.contents;
    while (BytesToRead) {
        i32 BytesRead = read(FileHandle, NextByteLocation, BytesToRead);
        if (BytesRead == -1) {
            free(Result.contents);
            Result.contents = 0;
            Result.contents_size = 0;
            close(FileHandle);
            return Result;
        }
        BytesToRead -= BytesRead;
        NextByteLocation += BytesRead;
    }
    close(FileHandle);

    return Result;
}


b32 WriteEntireFile(char *FileName, void *Memory, u64 MemorySize)
{
    int FileHandle = open(FileName, O_TRUNC | O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (FileHandle == -1) return 0;

    u32 BytesToWrite = MemorySize;
    u8 *NextByteLocation = (u8 *)Memory;
    while(BytesToWrite) {
        i32 BytesWritten = write(FileHandle, NextByteLocation, BytesToWrite);
        if (BytesWritten == -1) {
            close(FileHandle);
            return 0;
        }
        BytesToWrite -= BytesWritten;
        NextByteLocation += BytesWritten;
    }
    close(FileHandle);
    return 1;
}


#endif
