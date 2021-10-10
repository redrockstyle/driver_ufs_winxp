#ifndef _NTSTRING_H_
#define _NTSTRING_H_

#include <ntddk.h>
#include <ntstrsafe.h>

PCHAR GetSZFromUnicodeString(PUNICODE_STRING pUniStr);
PWCH AnsiToUnicode(char* str);
PCHAR GetCurrentTimeString();


#endif // !_NTSTRING_H_
