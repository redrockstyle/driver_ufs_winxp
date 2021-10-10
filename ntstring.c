#include "ntstring.h"


//
// Получает строку, оканчивающуюся нулём, из UNICODE-строки.
// Память для SZ-строки должна быть освобождена в вызывающей функции.
//
PCHAR GetSZFromUnicodeString(PUNICODE_STRING pUniStr) {

    ANSI_STRING ansiStr;
    char* str;
    USHORT length;
    NTSTATUS status;

    // получаем строку ANSI из UNICODE
    status = RtlUnicodeStringToAnsiString(&ansiStr, pUniStr, TRUE);

    if (!NT_SUCCESS(status)) {
        return NULL;
    }

    length = ansiStr.Length;

    // выделяем память под SZ-строку
    // память должна быть освобождена вызовом ExFreePool (str);
    str = (char*)ExAllocatePool(PagedPool, length + 1);
    if (!str)
        return NULL;

    RtlCopyMemory(str, ansiStr.Buffer, length);
    //strncpy(str,ansistr.Buffer,len);
    str[length] = 0;

    // освобождаем буфер, выделенный под хранение ANSI строки
    RtlFreeAnsiString(&ansiStr);

    return str;
}


//--------------------

//
// Получает UNICODE строку, заканчивающуюся нулём из SZ-строки.
//
PWCH AnsiToUnicode(char* str) {

    ANSI_STRING ansiStr;
    UNICODE_STRING uniStr;
    USHORT length;

    RtlInitAnsiString(&ansiStr, str);
    length = RtlAnsiStringToUnicodeSize(&ansiStr);
    uniStr.Buffer = (PWCH)ExAllocatePool(PagedPool, length);
    uniStr.MaximumLength = length;
    RtlAnsiStringToUnicodeString(&uniStr, &ansiStr, FALSE);

    return uniStr.Buffer;
}


//--------------------


//
// Получает системное врмемя в виде SZ-строки
//
PCHAR GetCurrentTimeString() {

    PCHAR pszTime;
    SIZE_T cchTime = 20;
    LARGE_INTEGER SystemTime;
    LARGE_INTEGER LocalTime;
    TIME_FIELDS  timeFiled;
    NTSTATUS status;

    LPCSTR pszFormat = "%d-%02d-%02d %02d:%02d:%02d";
    pszTime = (char*)ExAllocatePool(PagedPool, cchTime);

    KeQuerySystemTime(&SystemTime);
    ExSystemTimeToLocalTime(&SystemTime, &LocalTime);
    RtlTimeToTimeFields(&LocalTime, &timeFiled);

    status = RtlStringCchPrintfA((NTSTRSAFE_PSTR)pszTime, cchTime, pszFormat,
        timeFiled.Year,
        timeFiled.Month,
        timeFiled.Day,
        timeFiled.Hour,
        timeFiled.Minute,
        timeFiled.Second
    );

    //DbgPrint("STATUS %p", status);
    //DbgPrint("GET TIME SYSTEM %s", pszTime);

    return pszTime;
}