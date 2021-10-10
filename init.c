#include <ntddk.h>
#include <ntstrsafe.h>
#include "ntprint.h"
#include "files.h"
#include "ntstring.h"
#include "ufs.h"

//*************************************************************
// макросы и глобальные переменные

#define DIRECTORY		L"\\??\\C:\\DirDriver"
#define LOGFILE		    L"\\??\\C:\\log.txt"
#define	SYM_LINK_NAME   L"\\Global??\\Driver"
#define DEVICE_NAME     L"\\Device\\DDriver"



#define LOGGING_WRITE   0x00000001
#define LOGGING_READ    0x00000002
#define LOGGING_INFO    0x00000003

UNICODE_STRING  glDeviceName;       // имя устройства
UNICODE_STRING	glSymLinkName;      // имя символьной ссылки



//*************************************************************
// предварительное объявление функций

NTSTATUS DriverEntry(IN PDRIVER_OBJECT pDriverObject, IN PUNICODE_STRING registryPath);
VOID DriverUnload(IN PDRIVER_OBJECT pDriverObject);
NTSTATUS CompleteIrp(PIRP pIrp, NTSTATUS status, ULONG info);
NTSTATUS DispatchCreate(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp);
NTSTATUS DispatchClose(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp);
NTSTATUS DispatchRead(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp);
NTSTATUS DispatchWrite(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp);

NTSTATUS DispatchTest(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp);

NTSTATUS DispatchQueryInformation(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp);
NTSTATUS DispatchSetInformation(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp);
NTSTATUS DispatchQueryVolumeInformation(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp);
NTSTATUS DispatchCleanUp(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp);


void GetSystemTime(PTIME_FIELDS time);
BOOLEAN WriteLogToFile(PWCH filepath, PCHAR file_action, PCHAR buffer, ULONG info, INT action);


//*************************************************************
// описание функций

//
// Функция инициализации драйвера
//
NTSTATUS DriverEntry(
    IN PDRIVER_OBJECT pDriverObject,   // указатель на объект драйвера
    IN PUNICODE_STRING registryPath    // запись в реестре, соответствующая драйверу
) {
    PDEVICE_OBJECT  pDeviceObject;              // указатель на объект устройство
    NTSTATUS        status = STATUS_SUCCESS;    // статус завершения функции
    UNICODE_STRING unStr;

        // Регистрация рабочих процедур драйвера.
    pDriverObject->MajorFunction[IRP_MJ_CREATE                  ] = DispatchCreate;
    pDriverObject->MajorFunction[IRP_MJ_CLOSE                   ] = DispatchClose;
    pDriverObject->MajorFunction[IRP_MJ_READ                    ] = DispatchRead;
    pDriverObject->MajorFunction[IRP_MJ_WRITE                   ] = DispatchWrite;
    pDriverObject->MajorFunction[IRP_MJ_QUERY_INFORMATION       ] = DispatchQueryInformation;
    pDriverObject->MajorFunction[IRP_MJ_SET_INFORMATION         ] = DispatchSetInformation;


    //pDriverObject->MajorFunction[IRP_MJ_CREATE_NAMED_PIPE       ] = DispatchTest;
    //pDriverObject->MajorFunction[IRP_MJ_QUERY_EA                ] = DispatchTest;
    //pDriverObject->MajorFunction[IRP_MJ_SET_EA                  ] = DispatchTest;
    //pDriverObject->MajorFunction[IRP_MJ_FLUSH_BUFFERS           ] = DispatchTest;
    pDriverObject->MajorFunction[IRP_MJ_QUERY_VOLUME_INFORMATION] = DispatchQueryVolumeInformation; //0x0a
    //pDriverObject->MajorFunction[IRP_MJ_SET_VOLUME_INFORMATION  ] = DispatchTest;
    //pDriverObject->MajorFunction[IRP_MJ_DIRECTORY_CONTROL       ] = DispatchTest;
    //pDriverObject->MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL     ] = DispatchTest;
    //pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL          ] = DispatchTest;
    //pDriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL ] = DispatchTest;
    //pDriverObject->MajorFunction[IRP_MJ_SHUTDOWN                ] = DispatchTest;
    //pDriverObject->MajorFunction[IRP_MJ_LOCK_CONTROL            ] = DispatchTest; 
    pDriverObject->MajorFunction[IRP_MJ_CLEANUP                 ] = DispatchCleanUp; //12
    //pDriverObject->MajorFunction[IRP_MJ_CREATE_MAILSLOT         ] = DispatchTest;
    //pDriverObject->MajorFunction[IRP_MJ_QUERY_SECURITY          ] = DispatchTest;
    //pDriverObject->MajorFunction[IRP_MJ_SET_SECURITY            ] = DispatchTest;
    //pDriverObject->MajorFunction[IRP_MJ_POWER                   ] = DispatchTest;
    //pDriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL          ] = DispatchTest;
    //pDriverObject->MajorFunction[IRP_MJ_DEVICE_CHANGE           ] = DispatchTest; //18
    //pDriverObject->MajorFunction[IRP_MJ_QUERY_QUOTA             ] = DispatchTest;
    //pDriverObject->MajorFunction[IRP_MJ_SET_QUOTA               ] = DispatchTest;
    //pDriverObject->MajorFunction[IRP_MJ_PNP                     ] = DispatchTest;
    // всего процедур в массиве IRP_MJ_MAXIMUM_FUNCTION

    pDriverObject->DriverUnload = DriverUnload;

    KdPrint(("[*] Load driver %wZ\n", &pDriverObject->DriverName));
    KdPrint(("[*] Registry path %wZ\n", registryPath));

    RtlInitUnicodeString(&glDeviceName, DEVICE_NAME);
    RtlInitUnicodeString(&glSymLinkName, SYM_LINK_NAME);

    // создание устройства
    status = IoCreateDevice(pDriverObject,         // указатель на объект драйвера
                            0,                     // размер области дополнительной памяти устройства
                            &glDeviceName,         // имя устройства
                            FILE_DEVICE_UNKNOWN,   // идентификатор типа устройства
                            0,                     // дополнительная информация об устройстве
                            FALSE,                 // используется системой(для драйверов должно быть FALSE)
                            &pDeviceObject);       // адрес для сохранения указателя на объект устройства
    if (!NT_SUCCESS(status)) {
        return status;
    }

    // буферизованный ввод/вывод для операций чтения/записи
    pDeviceObject->Flags |= DO_BUFFERED_IO;

    KdPrint(("[*] Create device %wZ\n", &glDeviceName));

    // создание символьной ссылки
    status = IoCreateSymbolicLink(&glSymLinkName, &glDeviceName);
    if (!NT_SUCCESS(status)) {
        IoDeleteDevice(pDeviceObject);
        return status;
    }

    KdPrint(("[*] Create symbolic link %wZ\n", &glSymLinkName));


    // создаем резервный список
    ExInitializePagedLookasideList(&glPagedFileList, NULL, NULL, 0, sizeof(OpenFileEntry), ' LFO', 0);
    ExInitializePagedLookasideList(&glPagedDirList, NULL, NULL, 0, sizeof(OpenDirEntry), ' LFO', 0);

    InitializeListHead(&glUniformFileSystem.dir);
    InitializeListHead(&glUniformFileSystem.file);
    RtlInitUnicodeString(&unStr, DIRECTORY);
    RtlUnicodeStringToAnsiString(&glUniformFileSystem.name, &unStr, TRUE);
    glUniformFileSystem.drank = RANK_UFS;
    glUniformFileSystem.frank = RANK_UFS;
    glUniformFileSystem.link.Blink = NULL;
    glUniformFileSystem.link.Flink = NULL;
    GetSystemTime(&glUniformFileSystem.creationTime);
    GetSystemTime(&glUniformFileSystem.changeTime);

    KdPrint(("[*] Init UFS %s", glUniformFileSystem.name.Buffer));

    return status;
}


//--------------------

//
// Функция, вызываемая при выгрузке драйвера.
//
VOID DriverUnload(IN PDRIVER_OBJECT pDriverObject) {

    // удаление символьной ссылки и объекта устройства
    IoDeleteSymbolicLink(&glSymLinkName);
    IoDeleteDevice(pDriverObject->DeviceObject);

    // удаляем элементы списка файлов
    FreeUFS(&glUniformFileSystem);

    // удаляем резервный список
    ExDeletePagedLookasideList(&glPagedFileList);
    ExDeletePagedLookasideList(&glPagedDirList);

    KdPrint(("[*] Driver unload\n"));

    return;
}


//--------------------

//
// Функция обработки запроса на открытие устройства драйвера
//
NTSTATUS DispatchCreate(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp) {

    NTSTATUS status = STATUS_SUCCESS;   // статус завершения операции ввода/вывода
    PIO_STACK_LOCATION IrpStack;        // указатель на текущий элемент стека IRP-пакета
    ULONG info = 0;                     // количество возвращённых байт

    IrpStack = IoGetCurrentIrpStackLocation(pIrp);

    KdPrint(("~~~~~~~~~~~~~>Open  device<~~~~~~~~~~~~~~\n"));

    return CompleteIrp(pIrp, status, info); // Завершение IRP
}


//--------------------

//
// Функция обработки запроса на закрытие устройства драйвера
//
NTSTATUS DispatchClose(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp) {

    NTSTATUS status = STATUS_SUCCESS;
    PIO_STACK_LOCATION IrpStack;
    ULONG info = 0;

    IrpStack = IoGetCurrentIrpStackLocation(pIrp);

    KdPrint(("~~~~~~~~~~~~~>Close device<~~~~~~~~~~~~~~\n"));

    return CompleteIrp(pIrp, status, info);
}


//--------------------

//
// Функция обработки запроса на чтение
//
NTSTATUS DispatchRead(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp) {

    NTSTATUS status = STATUS_SUCCESS;
    PLIST_ENTRY link;
    PIO_STACK_LOCATION pIrpStack;
    PWCH filename;
    UNICODE_STRING unicodeDir;
    UNICODE_STRING filePath;
    LARGE_INTEGER fileSize;
    TIME_FIELDS time;
    HANDLE fileRead;
    USHORT size;
    ULONG info = 0;
    ULONG i;
    ULONG ost;
    char* outputBuffer;

    pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
    // В стеке IRP доступны следующие параметры
    // pIrpStack->Parameters.Read.Length         // размер записываемых данных
    // pIrpStack->Parameters.Read.Key
    // pIrpStack->Parameters.Read.ByteOffset

    KdPrint(("~~~~~~~>Read %d bytes to offset %d<~~~~~~~\n", pIrpStack->Parameters.Read.Length, *((int*)&pIrpStack->Parameters.Read.ByteOffset)));
    PrintIRP(pIrp);
    PrintIRPStack(pIrpStack);
    //PrintListFiles(&glOpenList);
    PrintUFS(&glUniformFileSystem);

    // определяем расположение буфера для выходных данных
    if (pDeviceObject->Flags & DO_BUFFERED_IO) {
        // если для устройства определён буферизованный ввод/вывод,
        // то записываем данные в системный буфер
        outputBuffer = (char*)pIrp->AssociatedIrp.SystemBuffer;
    }
    else {
        // иначе непосредственно в пользовательский буфер
        outputBuffer = (char*)pIrp->UserBuffer;
    }
    KdPrint(("[*] Output buffer: %08X\n", outputBuffer));

    filename = pIrpStack->FileObject->FileName.Buffer;
    RtlInitUnicodeString(&unicodeDir, DIRECTORY);

    size = unicodeDir.Length + sizeof(filename) + sizeof(DIRECTORY);
    filePath.Buffer = ExAllocatePool(PagedPool, size);
    filePath.MaximumLength = size;

    if (!filePath.Buffer) {
        KdPrint(("[!] Couldn't allocate memory for device name string"));
        return CompleteIrp(pIrp, STATUS_INSUFFICIENT_RESOURCES, info);
    }
    RtlCopyUnicodeString(&filePath, &unicodeDir);
    RtlAppendUnicodeToString(&filePath, filename);
    //DbgPrint("STATUS : %08X\n", status);

    fileRead = open_file(filePath.Buffer, GENERIC_READ, 0);
    if (!fileRead)
        KdPrint(("[!] Error open file for read %ws", filePath.Buffer));
    else {
        fget_file_size(fileRead, &fileSize);
        ost = fileSize.LowPart;

        if (pIrpStack->Parameters.Read.Length > ost) {
            info = ost;
        }
        else {
            info = pIrpStack->Parameters.Read.Length;
        }

        fread_file(fileRead, (PVOID)outputBuffer, info);
        ZwClose(fileRead);

        //запись действия в log
        WriteLogToFile(LOGFILE, GetSZFromUnicodeString(&filePath), outputBuffer, info, LOGGING_READ);
    }
    KdPrint(("[*] Readed file %ls", filePath.Buffer));
    ExFreePool(filePath.Buffer);

    return CompleteIrp(pIrp, status, info);
}


//--------------------

//
// Функция обработки запроса на запись
//
NTSTATUS DispatchWrite(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp) {

    NTSTATUS status = STATUS_SUCCESS;
    PIO_STACK_LOCATION pIrpStack;
    ULONG info = 0;
    USHORT size;
    char* inputBuffer;
    UNICODE_STRING unicodeDir;
    UNICODE_STRING filePath;
    PCHAR foundname;
    PWCH filename;
    HANDLE fileWrite;

    OpenDirEntry* dHeadEntry;
    OpenFileEntry* fEntryTmp;
    PCHAR filePathSZ;
    OpenFileEntry* fEntry = NULL;

    pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
    // pIrpStack->Parameters.Write.Length
    // pIrpStack->Parameters.Write.Key
    // pIrpStack->Parameters.Write.ByteOffset

    KdPrint(("~~~~~~~>Write %d bytes to offset %d<~~~~~~~\n", pIrpStack->Parameters.Write.Length, *((int*)&pIrpStack->Parameters.Write.ByteOffset)));
    PrintIRP(pIrp);
    PrintIRPStack(pIrpStack);

    // определяем расположение буфера с входными данными
    if (pDeviceObject->Flags & DO_BUFFERED_IO) {
        // если для устройства определён буферизованный ввод/вывод,
        // то записываем данные в системный буфер
        inputBuffer = (char*)pIrp->AssociatedIrp.SystemBuffer;
    }
    else {
        // иначе непосредственно в пользовательский буфер
        inputBuffer = (char*)pIrp->UserBuffer;
    }
    // \filename
    KdPrint(("[*] File name: %wZ\n", &pIrpStack->FileObject->FileName));
    KdPrint(("[*] Input buffer: %p\n", inputBuffer));
    KdPrint(("[*] String buffer: %s\n", inputBuffer));
    
    // filename > \*
    if (pIrpStack->FileObject->FileName.Length > 2) {
        filename = pIrpStack->FileObject->FileName.Buffer;
        RtlInitUnicodeString(&unicodeDir, DIRECTORY);

        size = unicodeDir.Length + sizeof(filename) + sizeof(DIRECTORY);
        filePath.Buffer = ExAllocatePool(PagedPool, size);
        filePath.MaximumLength = size;

        if (!filePath.Buffer) {
            KdPrint(("[!] Couldn't allocate memory for device name string"));
            return CompleteIrp(pIrp, STATUS_INSUFFICIENT_RESOURCES, info);
        }
        RtlCopyUnicodeString(&filePath, &unicodeDir);
        RtlAppendUnicodeToString(&filePath, filename);

        //fileWrite = open_always_file(filePath.Buffer, GENERIC_WRITE, 0);
        //if (!fileWrite) {
        fileWrite = create_always_file(filePath.Buffer, GENERIC_WRITE, 0);
        if (!fileWrite) {
            KdPrint(("[!] Error create file for write %ws", filePath.Buffer));
            ExFreePool(filePath.Buffer);
            return CompleteIrp(pIrp, STATUS_HANDLE_NO_LONGER_VALID, info);
        }
        //}
        // записчь буфура в файл
        if (!fwrite_file(fileWrite, inputBuffer, pIrpStack->Parameters.Write.Length)) {
            KdPrint(("[!] Error write file %ws", filePath.Buffer));
        }
        else {
            KdPrint(("[*] Successful write to file %ws", filePath.Buffer));
        }

        filePathSZ = GetSZFromUnicodeString(&pIrpStack->FileObject->FileName);
        dHeadEntry = IsFoundDirEntry(&glUniformFileSystem, filePathSZ);

        if (dHeadEntry->frank == RANK_UFS) {
            InitializeListHead(&dHeadEntry->file);
        }
        else {
            fEntryTmp = CONTAINING_RECORD(&dHeadEntry->file, OpenFileEntry, link);
            fEntry = IsFoundFileEntry(fEntryTmp, filePathSZ);
        }
        if (fEntry == NULL) {
            fEntry = (OpenFileEntry*)ExAllocateFromPagedLookasideList(&glPagedFileList);
            InsertTailList(&dHeadEntry->file, &fEntry->link);
            RtlUnicodeStringToAnsiString(&fEntry->name, &pIrpStack->FileObject->FileName, TRUE);
            fget_time(fileWrite, &fEntry->creationTime, FILE_CREATION_TIME);

            dHeadEntry->frank++;
        }
        else {
            dHeadEntry->size = RtlLargeIntegerSubtract(dHeadEntry->size, fEntry->size);
            ExFreePool(fEntry->buffer);
        }


        fEntry->buffer = ExAllocatePool(PagedPool, strlen(inputBuffer));
        RtlCopyMemory(fEntry->buffer, inputBuffer, strlen(inputBuffer));
        //strcpy(entry->buffer, inputBuffer);
        fget_time(fileWrite, &fEntry->changeTime, FILE_CHANGE_TIME);
        fget_file_size(fileWrite, &fEntry->size);
        dHeadEntry->size = RtlLargeIntegerAdd(fEntry->size, dHeadEntry->size);

        ZwClose(fileWrite);

        // запись действия в log
        info = pIrpStack->Parameters.Read.Length;
        WriteLogToFile(LOGFILE, GetSZFromUnicodeString(&filePath), inputBuffer, info, LOGGING_WRITE);


        KdPrint(("[*] File %s contained in container dir %s", fEntry->name.Buffer, dHeadEntry->name.Buffer));

        ExFreePool(filePath.Buffer);
        ExFreePool(filePathSZ);
    }
    else {
        KdPrint(("[!] Error filename\n"));
    }

    

    return CompleteIrp(pIrp, status, info);
}


//--------------------

//
// Функция обработки запроса информации о файле
//
NTSTATUS DispatchQueryInformation(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp) {

    NTSTATUS status = STATUS_SUCCESS;
    PIO_STACK_LOCATION pIrpStack;
    ULONG info = 0;
    FILE_BASIC_INFORMATION* fbi;
    FILE_STANDARD_INFORMATION* fsi;

    
    pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
    // pIrpStack->Parameters.QueryFile.Length
    // pIrpStack->Parameters.QueryFile.FileInformationClass

    KdPrint(("~~~~~~>Query information device  %d<~~~~~~\n", pIrpStack->Parameters.QueryFile.FileInformationClass));
    PrintIRP(pIrp);
    PrintIRPStack(pIrpStack);
    PrintFileObject(pIrpStack->FileObject);

    // в зависимости от типа запроса заполняем
    // буфер как соответствующую структуру
    switch (pIrpStack->Parameters.QueryFile.FileInformationClass) {

    case FileBasicInformation:
        // информация о времени изменения, создания, последнего доступа, последней записи
        info = sizeof(FILE_BASIC_INFORMATION);
        fbi = (FILE_BASIC_INFORMATION*)pIrp->AssociatedIrp.SystemBuffer;
        fbi->ChangeTime.QuadPart = 0;
        fbi->CreationTime.QuadPart = 0;
        fbi->LastAccessTime.QuadPart = 0;
        fbi->LastWriteTime.QuadPart = 0;
        fbi->FileAttributes = 0;
        break;

    case FileStandardInformation:
        info = sizeof(FILE_STANDARD_INFORMATION);
        fsi = (FILE_STANDARD_INFORMATION*)pIrp->AssociatedIrp.SystemBuffer;
        fsi->AllocationSize.QuadPart = 0;
        fsi->EndOfFile.QuadPart = 100;      // размер файла
        fsi->NumberOfLinks = 1;             // количество жёстких ссылок
        fsi->Directory = FALSE;
        fsi->DeletePending = FALSE;
        break;

    case FileNameInformation:

        ;
        //if (pDeviceObject->Flags & DO_BUFFERED_IO) {
        //    inputBuffer = (char*)pIrp->AssociatedIrp.SystemBuffer;
        //    KdPrint(("ONE SELECT"));
        //}
        //else {
        //    KdPrint(("TWO SELECT"));
        //    inputBuffer = (char*)pIrp->UserBuffer;
        //}

        //for (link = glOpenFiles.Flink; link != &glOpenFiles; link = link->Flink) {
        //    OpenFileEntry* entry = CONTAINING_RECORD(link, OpenFileEntry, link);
        //    if (info + entry->fileName.Length + 1 <= pIrpStack->Parameters.Read.Length) {
        //        RtlCopyMemory(inputBuffer + info, entry->fileName.Buffer, entry->fileName.Length);
        //        info += entry->fileName.Length;
        //        inputBuffer[info++] = '\n';
        //    }
        //    else {
        //        break;
        //    }
        //}

    }

    return CompleteIrp(pIrp, status, info);
}


//--------------------

//
// Функция обработки запроса установки информации о файле
//
NTSTATUS DispatchSetInformation(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp) {

    NTSTATUS status = STATUS_SUCCESS;
    PIO_STACK_LOCATION pIrpStack;
    ULONG info = 0;

    pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
    // pIrpStack->Parameters.SetFile.Length
    // pIrpStack->Parameters.SetFile.FileInformationClass
    // pIrpStack->Parameters.SetFile.FileObject

#if DBG
    DbgPrint(".....Set information %d file %p.....\n", pIrpStack->Parameters.SetFile.FileInformationClass, pIrpStack->Parameters.SetFile.FileObject);
    PrintIRP(pIrp);
    PrintIRPStack(pIrpStack);
    PrintFileObject(pIrpStack->FileObject);
#endif

    return CompleteIrp(pIrp, status, info);
}


//--------------------

//
// Функция завершения обработки IRP
//
NTSTATUS CompleteIrp(PIRP pIrp, NTSTATUS status, ULONG info) {

    pIrp->IoStatus.Status = status;             // статус завершении операции
    pIrp->IoStatus.Information = info;          // количество возращаемой информации
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);  // завершение операции ввода-вывода
    return status;
}


//--------------------


//--------------------

//
// Печать о событии в log-файл
//
BOOLEAN WriteLogToFile(PWCH logfilepath, PCHAR filepath_action, PCHAR buffer, ULONG info, INT action) {

    PCHAR timestr, chinfo;
    UNICODE_STRING unicode_info;
    HANDLE logfile;
    SIZE_T sizeChInfo = 8;
    chinfo = (char*)ExAllocatePool(PagedPool, sizeChInfo);

    logfile = open_file(logfilepath, GENERIC_WRITE, 0);
    if (!logfile) {
        logfile = create_file(logfilepath, GENERIC_WRITE, 0);
        if (!logfile) {
            DbgPrint("Error logfile");
            return FALSE;
        }
    }
    timestr = GetCurrentTimeString();
    //DbgPrint("TIME SYSTEM %s", timestr);

    if (action == LOGGING_INFO) {
        fwrite_file_offset(logfile, "info\n", sizeof("info\n") - 1);

        //
        // ...
        //
    }
    else {
        RtlStringCchPrintfA((NTSTRSAFE_PSTR)chinfo, sizeChInfo, "%d", info);
        //filepath_action = GetSZFromUnicodeString(&filepath_action);

        if (action == LOGGING_READ) fwrite_file_offset(logfile, "read:{", sizeof("read:{") - 1);
        else if (action == LOGGING_WRITE) fwrite_file_offset(logfile, "write:{", sizeof("write:{") - 1);

        fwrite_file_offset(logfile, filepath_action, strlen(filepath_action));
        fwrite_file_offset(logfile, "} buffer:{", sizeof("} buffer:{") - 1);
        fwrite_file_offset(logfile, buffer, info);

        ExFreePool(filepath_action);
    }
    fwrite_file_offset(logfile, "} bytes:{", sizeof("} bytes:{") - 1);
    fwrite_file_offset(logfile, chinfo, strlen(chinfo));
    fwrite_file_offset(logfile, "} time:{", sizeof("} time:{") - 1);
    fwrite_file_offset(logfile, timestr, strlen(timestr));
    fwrite_file_offset(logfile, "}\n", sizeof("}\n") - 1);

    ExFreePool(chinfo);
    ExFreePool(timestr);
    ZwClose(logfile);

    return TRUE;
}

//--------------------


NTSTATUS DispatchCleanUp(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp) {

    NTSTATUS status = STATUS_SUCCESS;
    PIO_STACK_LOCATION pIrpStack;
    UNICODE_STRING unicodeDir;
    UNICODE_STRING dirPath;
    PCHAR dirPathSZ;
    OpenDirEntry* dHeadEntry;
    OpenDirEntry* dEntry;
    HANDLE hDir;
    PWCH filename;
    USHORT size;
    OpenDirEntry* tmpEntry;

    //PrintIRP(pIrp);
    //PrintIRPStack(&pIrpStack);
    pIrpStack = IoGetCurrentIrpStackLocation(pIrp);

    //KdPrint(("[*] DispatchCleanUp"));
    KdPrint(("[*] File name: %wZ\n", &pIrpStack->FileObject->FileName));

    if (pIrpStack->FileObject->FileName.Length > 2) {

        filename = pIrpStack->FileObject->FileName.Buffer;
        RtlInitUnicodeString(&unicodeDir, DIRECTORY);

        size = unicodeDir.Length + sizeof(filename) + sizeof(DIRECTORY);
        dirPath.Buffer = ExAllocatePool(PagedPool, size);
        dirPath.MaximumLength = size;

        if (!dirPath.Buffer) {
            KdPrint(("[!] Couldn't allocate memory for device name string"));
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        RtlCopyUnicodeString(&dirPath, &unicodeDir);
        RtlAppendUnicodeToString(&dirPath, filename);

        hDir = create_directory(dirPath.Buffer);
        if (!hDir) {
            KdPrint(("[!] Error create directory %ws", dirPath.Buffer));
        }
        else {

            dirPathSZ = GetSZFromUnicodeString(&pIrpStack->FileObject->FileName);
            dHeadEntry = IsFoundDirEntry(&glUniformFileSystem, dirPathSZ);
            if (dHeadEntry->drank == RANK_UFS) {
                InitializeListHead(&dHeadEntry->dir);
            }
            dEntry = (OpenDirEntry*)ExAllocateFromPagedLookasideList(&glPagedDirList);
            InsertTailList(&dHeadEntry->dir, &dEntry->link);
            RtlUnicodeStringToAnsiString(&dEntry->name, &pIrpStack->FileObject->FileName, TRUE);
            fget_time(hDir, &dEntry->creationTime, FILE_CREATION_TIME);
            fget_time(hDir, &dEntry->changeTime, FILE_CHANGE_TIME);
            dEntry->drank = RANK_UFS;
            dEntry->frank = RANK_UFS;
            dEntry->size.QuadPart = 0;
            dHeadEntry->drank++;

            //tmpEntry = CONTAINING_RECORD(dHeadEntry->dir.Flink, OpenDirEntry, link);
            //KdPrint(("[*] Contained name dir %s in container dir %s", tmpEntry->name.Buffer, dHeadEntry->name.Buffer));

            KdPrint(("[*] Successful create directory %ws", dirPath.Buffer));
        }

        ExFreePool(dirPath.Buffer);
        ZwClose(hDir);
    }
    else {
        KdPrint(("[!] Error name directory"));
    }


    PrintUFS(&glUniformFileSystem);
    return status;
}

//--------------------



NTSTATUS DispatchTest(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp) {

    PIO_STACK_LOCATION pIrpStack;

    pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
    KdPrint(("***********SUCCESS*************"));
    PrintIRP(pIrp);
    PrintIRPStack(pIrpStack);
    KdPrint(("***********SUCCESS*************"));
    return STATUS_SUCCESS;
}

void GetSystemTime(PTIME_FIELDS time) {
    LARGE_INTEGER SystemTime;
    LARGE_INTEGER LocalTime;

    KeQuerySystemTime(&SystemTime);
    ExSystemTimeToLocalTime(&SystemTime, &LocalTime);
    RtlTimeToTimeFields(&LocalTime, time);

    return;
}

NTSTATUS DispatchQueryVolumeInformation(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp) {

    NTSTATUS status = STATUS_SUCCESS;

    KdPrint(("[*] DispatchQueryVolumeInformation"));



    return status;
}