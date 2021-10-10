#ifndef _UFS_H
#define _UFS_H


#define RANK_UFS        0x00000000
#define PRINTTIME(str,tf)  DbgPrint("%s %02d.%02d.%04d  %02d:%02d:%02d",\
        str,tf.Day,tf.Month,tf.Year,tf.Hour,tf.Minute,tf.Second);

#include <ntddk.h>



PAGED_LOOKASIDE_LIST glPagedFileList;
PAGED_LOOKASIDE_LIST glPagedDirList;

typedef struct _OpenFileEntry {
    ANSI_STRING name;
    LARGE_INTEGER size;
    PCHAR buffer;
    TIME_FIELDS creationTime;
    TIME_FIELDS changeTime;

    LIST_ENTRY link;
} OpenFileEntry;

typedef struct _OpenDirEntry {
    ANSI_STRING name;
    LARGE_INTEGER size;             // суммарный размер вложенных файлов и вложенных директорий
    TIME_FIELDS creationTime;
    TIME_FIELDS changeTime;

    ULONG drank;                    // степень вложенности (инициализируется RANK_UFS в корне UFS)
    LIST_ENTRY dir;                 // голова списка вложенных директорий
    ULONG frank;
    LIST_ENTRY file;                // голова списка вложенных файлов

    LIST_ENTRY link;                // ссылки на каталоги одного уровня (поля ссылок инициализируются NULL в корне UFS)
} OpenDirEntry;


OpenDirEntry glUniformFileSystem;   // корень файловой системы



void PrintUFS(OpenDirEntry* glUFS);
void PrintFileList(OpenDirEntry* dEntry);

void FreeUFS(OpenDirEntry* glUFS);
OpenFileEntry* IsFoundFileEntry(OpenFileEntry* fEntry, PCHAR filename);
OpenDirEntry* IsFoundDirEntry(OpenDirEntry* glUFS, PCHAR filename);


#endif // !_UFS_H