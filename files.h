/****************************************************************************

    Файл files.h

    Заголовочный файл модуля files.c.

    Маткин Илья Александрович               12.03.2009

****************************************************************************/

#ifndef _FILES_H_
#define _FILES_H_

#define FILE_CREATION_TIME       0x00000001
#define FILE_LAST_WRITE_TIME     0x00000002
#define FILE_CHANGE_TIME         0x00000003
#define FILE_LAST_ACCESS_TIME    0x00000004

#include <ntddk.h>


//*************************************************************

// константы для обозначения точки отсчёта изменения позиции
// в функции fseek
typedef enum _SEEK_FILE_POSITION{
    BeginFile = 1,      // с начала файла
    EndFile,            // с конца файла
    CurrentPosition     // с текущей позиции файла
} SEEK_FILE_POSITION;

#define PRINT_TIME(str,tf)  DbgPrint("%s %02d.%02d.%04d  %02d:%02d:%02d",\
        str,tf.Day,tf.Month,tf.Year,tf.Hour,tf.Minute,tf.Second);


//*************************************************************
// объявление функций

HANDLE base_work_file(PWCHAR name,ACCESS_MASK DesiredAccess,ULONG ShareAccess,ULONG CreateDisposition);


#define open_file(name,DesiredAccess,ShareAccess)   \
        base_work_file(name,DesiredAccess,ShareAccess,FILE_OPEN);

#define create_file(name,DesiredAccess,ShareAccess) \
        base_work_file(name,DesiredAccess,ShareAccess,FILE_CREATE);

#define open_always_file(name,DesiredAccess,ShareAccess) \
        base_work_file(name,DesiredAccess,ShareAccess,FILE_OPEN_IF);

#define create_always_file(name,DesiredAccess,ShareAccess) \
        base_work_file(name,DesiredAccess,ShareAccess,FILE_OVERWRITE_IF);

BOOLEAN fget_file_size(IN HANDLE file, OUT PLARGE_INTEGER size);

BOOLEAN fwrite_file(HANDLE file,PVOID buf,ULONG length);

BOOLEAN fread_file(HANDLE file,PVOID buf,ULONG length);

BOOLEAN fcopy_file(HANDLE file_src,HANDLE file_dst);

BOOLEAN fprint_times_file(HANDLE file);

BOOLEAN fseek_file(HANDLE file,SEEK_FILE_POSITION orig,PLARGE_INTEGER pos);

BOOLEAN fset_times_files(HANDLE file,PLARGE_INTEGER create,PLARGE_INTEGER change,PLARGE_INTEGER write,PLARGE_INTEGER access);


BOOLEAN fwrite_file_offset(HANDLE file, PVOID buf, ULONG length);

BOOLEAN fget_time(IN HANDLE file, OUT PTIME_FIELDS time, IN INT action);

HANDLE create_directory(PWSTR path);

#endif  //_FILES_H_