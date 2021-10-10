#include <ntddk.h>
#include "files.h"


//*************************************************************
// описание функций

//----------------------------------------

// базова€ функци€ открыти€/создани€ файлов
HANDLE base_work_file(PWCHAR name,ACCESS_MASK DesiredAccess,ULONG ShareAccess,ULONG CreateDisposition){

NTSTATUS status;
OBJECT_ATTRIBUTES oa;
IO_STATUS_BLOCK IostatusBlock;
UNICODE_STRING unistr;
HANDLE file;

    RtlInitUnicodeString(&unistr,name);
	InitializeObjectAttributes(&oa,&unistr,OBJ_CASE_INSENSITIVE + OBJ_KERNEL_HANDLE, NULL, NULL);

    status = ZwCreateFile(	&file,
							DesiredAccess,
							&oa,
							&IostatusBlock,
							0,
							FILE_ATTRIBUTE_NORMAL,
							ShareAccess,
							CreateDisposition,
							FILE_SYNCHRONOUS_IO_NONALERT,
							NULL,0);
    if(!NT_SUCCESS(status))
        return 0;
    else
        return file;
}

//----------------------------------------

// возвращает размер файла по указателю size
BOOLEAN fget_file_size(IN HANDLE file, OUT PLARGE_INTEGER size){

FILE_STANDARD_INFORMATION  fsi;
IO_STATUS_BLOCK   IostatusBlock;
NTSTATUS status;
    
    status =ZwQueryInformationFile( file,
                                    &IostatusBlock,
                                    &fsi,
                                    sizeof(FILE_STANDARD_INFORMATION),
                                    FileStandardInformation
                                   );
    if(!NT_SUCCESS(status))
        return FALSE;

    size->QuadPart = fsi.EndOfFile.QuadPart;

    return TRUE;
}

//----------------------------------------

// записывает данные в файл
BOOLEAN fwrite_file(HANDLE file,PVOID buf,ULONG length){

IO_STATUS_BLOCK   IoStatusBlock;
NTSTATUS status;

    status = ZwWriteFile(file,0,NULL,NULL,&IoStatusBlock,buf,length,NULL,NULL);

    if(!NT_SUCCESS(status))
        return FALSE;
    else
        return TRUE;
}

//----------------------------------------

// читает данные из файла
BOOLEAN fread_file(HANDLE file,PVOID buf,ULONG length){

IO_STATUS_BLOCK   IoStatusBlock;
NTSTATUS status;

    status = ZwReadFile(file,0,NULL,NULL,&IoStatusBlock,buf,length,NULL,NULL);

    if(!NT_SUCCESS(status))
        return FALSE;
    else
        return TRUE;
}

//----------------------------------------

// копирует файл
BOOLEAN fcopy_file(HANDLE file_src,HANDLE file_dst){

LARGE_INTEGER file_size;
PVOID buf;
ULONG ost;
ULONG granularity = 100*1024;

    if(!fget_file_size(file_src,&file_size))
        return FALSE;

    // верхн€€ часть 64-битового числа игнорируетс€
    ost = file_size.LowPart;
    
    // файл копируетс€ част€ми по 100 б
    buf = ExAllocatePool(PagedPool,granularity);
    if(!buf)
        return FALSE;

    while(ost > granularity){

        if(!fread_file(file_src,buf,granularity)){
            ExFreePool(buf);
            return FALSE;
            }
            
        if(!fwrite_file(file_dst,buf,granularity)){
            ExFreePool(buf);
            return FALSE;
            }            
      
        ost -= granularity;
        }

    if(!fread_file(file_src,buf,ost)){
        ExFreePool(buf);
        return FALSE;
        }            
    if(!fwrite_file(file_dst,buf,ost)){
        ExFreePool(buf);
        return FALSE;
        }
    ExFreePool(buf);

    return TRUE;
}

//----------------------------------------

// отображает информацию о времени создани€, модификации и доступа к файлу
BOOLEAN fprint_times_file(HANDLE file){

FILE_BASIC_INFORMATION fbi;
IO_STATUS_BLOCK   IostatusBlock;
NTSTATUS status;
TIME_FIELDS time;
LARGE_INTEGER loctime;
    
    status =ZwQueryInformationFile( file,
                                    &IostatusBlock,
                                    &fbi,
                                    sizeof(FILE_BASIC_INFORMATION),
                                    FileBasicInformation
                                   );
    if(!NT_SUCCESS(status))
        return FALSE;

    ExSystemTimeToLocalTime(&fbi.CreationTime,&loctime);
    RtlTimeToTimeFields(&loctime,&time);
    PRINT_TIME("Creation time: ",time);
    ExSystemTimeToLocalTime(&fbi.LastWriteTime,&loctime);
    RtlTimeToTimeFields(&loctime,&time);
    PRINT_TIME("Write time: ",time);
    ExSystemTimeToLocalTime(&fbi.ChangeTime,&loctime);
    RtlTimeToTimeFields(&loctime,&time);
    PRINT_TIME("Change time: ",time);
    ExSystemTimeToLocalTime(&fbi.LastAccessTime,&loctime);
    RtlTimeToTimeFields(&loctime,&time);
    PRINT_TIME("Access time: ",time);

    return TRUE;
}

//----------------------------------------

// измен€ет текущую позицию в файле
BOOLEAN fseek_file(HANDLE file,SEEK_FILE_POSITION orig,PLARGE_INTEGER pos){

FILE_POSITION_INFORMATION fpi;
FILE_STANDARD_INFORMATION  fsi;
IO_STATUS_BLOCK   IostatusBlock;
NTSTATUS status;

    switch(orig){
        case BeginFile:
            fpi.CurrentByteOffset.QuadPart = pos->QuadPart;
            status =ZwSetInformationFile(file,&IostatusBlock,&fpi,
                    sizeof(FILE_POSITION_INFORMATION),FilePositionInformation);
            if(!NT_SUCCESS(status))
                return FALSE;
            break;

        case EndFile:
            status =ZwQueryInformationFile(file,&IostatusBlock,&fsi,
                    sizeof(FILE_STANDARD_INFORMATION),FileStandardInformation);
            if(!NT_SUCCESS(status))
                return FALSE;

            if(fsi.EndOfFile.QuadPart < pos->QuadPart)
                fpi.CurrentByteOffset.QuadPart = 0;
            else
                fpi.CurrentByteOffset.QuadPart = fsi.EndOfFile.QuadPart - pos->QuadPart;

            status =ZwSetInformationFile(file,&IostatusBlock,&fpi,
                    sizeof(FILE_POSITION_INFORMATION),FilePositionInformation);
            if(!NT_SUCCESS(status))
                return FALSE;

            pos->QuadPart = fpi.CurrentByteOffset.QuadPart;
            break;

        case CurrentPosition:
            status =ZwQueryInformationFile(file,&IostatusBlock,&fpi,
                    sizeof(FILE_POSITION_INFORMATION),FilePositionInformation);
            if(!NT_SUCCESS(status))
                return FALSE;

            fpi.CurrentByteOffset.QuadPart += pos->QuadPart;

            status =ZwSetInformationFile(file,&IostatusBlock,&fpi,
                    sizeof(FILE_POSITION_INFORMATION),FilePositionInformation);
            if(!NT_SUCCESS(status))
                return FALSE;

            pos->QuadPart = fpi.CurrentByteOffset.QuadPart;
            break;
        default:
            return FALSE;
        }

    return TRUE;
}

//----------------------------------------

// устанавливает времена создани€, изменени€, записи и доступа к файлу
BOOLEAN fset_times_files(HANDLE file,PLARGE_INTEGER create,PLARGE_INTEGER change,PLARGE_INTEGER write,PLARGE_INTEGER access){

FILE_BASIC_INFORMATION fbi;
IO_STATUS_BLOCK   IostatusBlock;
NTSTATUS status;
TIME_FIELDS time;
LARGE_INTEGER loctime;
    
    status =ZwQueryInformationFile( file,
                                    &IostatusBlock,
                                    &fbi,
                                    sizeof(FILE_BASIC_INFORMATION),
                                    FileBasicInformation
                                   );
    if(!NT_SUCCESS(status))
        return FALSE;

    if(create)
        fbi.CreationTime.QuadPart = create->QuadPart;
    if(change)
        fbi.ChangeTime.QuadPart = change->QuadPart;
    if(write)
        fbi.LastWriteTime.QuadPart = write->QuadPart;
    if(access)
        fbi.LastAccessTime.QuadPart = access->QuadPart;

    status =ZwSetInformationFile( file,
                                    &IostatusBlock,
                                    &fbi,
                                    sizeof(FILE_BASIC_INFORMATION),
                                    FileBasicInformation
                                   );
    if(!NT_SUCCESS(status))
        return FALSE;

    return TRUE;
}


// записывает данные в файл
BOOLEAN fwrite_file_offset(HANDLE file, PVOID buf, ULONG length) {

    IO_STATUS_BLOCK   IoStatusBlock;
    NTSTATUS status;
    PLARGE_INTEGER byte_offset;
    LARGE_INTEGER file_size;

    fget_file_size(file, &file_size);
    //DbgPrint("OFFSET %d", file_size);

    status = ZwWriteFile(file, 0, NULL, NULL, &IoStatusBlock, buf, length, &file_size, NULL);

    if (!NT_SUCCESS(status))
        return FALSE;
    else
        return TRUE;
}

//----------------------------------------

// 
BOOLEAN fget_time(IN HANDLE file, OUT PTIME_FIELDS time, IN INT action) {

FILE_BASIC_INFORMATION fbi;
IO_STATUS_BLOCK IostatusBlock;
NTSTATUS status;
LARGE_INTEGER loctime;
TIME_FIELDS times;


    status = ZwQueryInformationFile(file,
                                    &IostatusBlock,
                                    &fbi,
                                    sizeof(FILE_BASIC_INFORMATION),
                                    FileBasicInformation
    );
    if (!NT_SUCCESS(status))
        return FALSE;

    switch (action) {
    case FILE_CREATION_TIME:
        ExSystemTimeToLocalTime(&fbi.CreationTime, &loctime);
        break;
    case FILE_LAST_WRITE_TIME:
        ExSystemTimeToLocalTime(&fbi.LastWriteTime, &loctime);
        break;
    case FILE_CHANGE_TIME:
        ExSystemTimeToLocalTime(&fbi.ChangeTime, &loctime);
        break;
    case FILE_LAST_ACCESS_TIME:
        ExSystemTimeToLocalTime(&fbi.LastAccessTime, &loctime);
        break;
    default:
        return FALSE;
    }
    RtlTimeToTimeFields(&loctime, time);
    //PRINT_TIME("TIME_F_GET_TIME", (*time));

    return TRUE;
}

//----------------------------------------


HANDLE create_directory(PWSTR path) {
    IO_STATUS_BLOCK iosb;
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS Status;
    HANDLE hDir = NULL;
    FILE_OBJECT* FileObj;
    UNICODE_STRING dirName;

    RtlInitUnicodeString(&dirName, path);
    //KdPrint(("[*] Directory '%wZ'", &dirName));

    InitializeObjectAttributes( &ObjectAttributes,
                                &dirName,
                                (OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE),
                                NULL,
                                NULL
    );


    Status = ZwCreateFile ( &hDir,
                            GENERIC_READ,
                            &ObjectAttributes,
                            &iosb,
                            0,
                            FILE_ATTRIBUTE_NORMAL | SYNCHRONIZE,
                            FILE_SHARE_WRITE | FILE_SHARE_READ,
                            FILE_CREATE,
                            FILE_DIRECTORY_FILE,
                            NULL,
                            0
    );

    //if (hDir) {
    //    ZwClose(hDir);
    //}
    //if (!NT_SUCCESS(Status)) {
    //    return FALSE;
    //}
    //return TRUE;

    return hDir;
}