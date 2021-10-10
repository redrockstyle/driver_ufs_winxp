#include <ntddk.h>
#include "ufs.h"


void PrintUFS(OpenDirEntry* glUFS) {

    PLIST_ENTRY link;
    OpenDirEntry* dEntry;

    KdPrint(("[outdir]~~~~~~~~~~~~~~~~~~~~~~~~~~~>"));
    KdPrint(("[outdir] Rank D:%d F:%d", glUFS->drank, glUFS->frank));
    KdPrint(("[outdir] Directory name %s", glUFS->name.Buffer));
    KdPrint(("[outdir] Size directory %I64d", glUFS->size.QuadPart));
    PRINTTIME("[outdir] Creation time", glUFS->creationTime);
    PRINTTIME("[outdir] Change time", glUFS->changeTime);

    PrintFileList(glUFS);

    if (glUFS->drank > RANK_UFS) {
        for (link = glUFS->dir.Flink; link != &glUFS->dir; link = link->Flink) {
            dEntry = CONTAINING_RECORD(link, OpenDirEntry, link);
            PrintUFS(dEntry);
        }
    }

    return;
}

void PrintFileList(OpenDirEntry* dEntry) {

    if (dEntry->frank > RANK_UFS) {

        PLIST_ENTRY link;
        OpenFileEntry* fEntry;

        for (link = dEntry->file.Flink; link != &dEntry->file; link = link->Flink) {

            fEntry = CONTAINING_RECORD(link, OpenFileEntry, link);

            KdPrint(("[outfile] Name file %s", fEntry->name.Buffer));
            //KdPrint(("SIZE %I64d", entry->fileSize));
            KdPrint(("[outfile] Size %I64d", fEntry->size.QuadPart));
            KdPrint(("[outfile] Buffer %s", fEntry->buffer));
            PRINTTIME("[outfile] Creation time", fEntry->creationTime);
            PRINTTIME("[outfile] Change time", fEntry->changeTime);
        }
    }

    return;
}

OpenFileEntry* IsFoundFileEntry(OpenFileEntry* fEntry, PCHAR filename) {

    PLIST_ENTRY link;
    OpenFileEntry* entry;

    for (link = fEntry->link.Flink; link != &fEntry->link; link = link->Flink) {
        entry = CONTAINING_RECORD(link, OpenFileEntry, link);
        //KdPrint(("[*] Strlen name found %d %d", strlen(filename), entry->name.Length));
        if (entry->name.Length == strlen(filename)) {
            if (entry->name.Length == RtlCompareMemory(entry->name.Buffer, filename, entry->name.Length)) {
                return entry;
            }
        }
    }

    return NULL;
}

OpenDirEntry* IsFoundDirEntry(OpenDirEntry* glUFS, PCHAR filename) {

    PLIST_ENTRY link;
    OpenDirEntry* entry;

    if (glUFS->drank > RANK_UFS) {
        for (link = glUFS->dir.Flink; link != &glUFS->dir; link = link->Flink) {
            entry = CONTAINING_RECORD(link, OpenDirEntry, link);
            if (entry->name.Length == RtlCompareMemory(entry->name.Buffer, filename, entry->name.Length)) {
                if (filename[entry->name.Length] == '\\') {
                    return IsFoundDirEntry(entry, filename);
                }
            }
        }
    }
    return glUFS;
}

void FreeUFS(OpenDirEntry* glUFS) {

    PLIST_ENTRY pLink;
    OpenFileEntry* fEntry;
    OpenDirEntry* dEntry;
    
    if (glUFS->frank > RANK_UFS) {
        while (!IsListEmpty(&glUFS->file)) {
            pLink = RemoveHeadList(&glUFS->file);
            fEntry = CONTAINING_RECORD(pLink, OpenFileEntry, link);
            ExFreePool(fEntry->buffer);
            RtlFreeAnsiString(&fEntry->name);
            ExFreeToPagedLookasideList(&glPagedFileList, fEntry);
        }
    }
    if (glUFS->drank > RANK_UFS) {
        while (!IsListEmpty(&glUFS->dir)) {
            pLink = RemoveHeadList(&glUFS->dir);
            dEntry = CONTAINING_RECORD(pLink, OpenDirEntry, link);
            if (dEntry->drank > RANK_UFS) {
                FreeUFS(dEntry);
                dEntry->drank--;
            }
            RtlFreeAnsiString(&dEntry->name);
            ExFreeToPagedLookasideList(&glPagedDirList, dEntry);
        }
    }


    return;
}