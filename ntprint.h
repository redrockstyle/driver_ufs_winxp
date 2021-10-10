#ifndef _NTPRINT_H
#define _NTPRINT_H

#include <ntddk.h>

void PrintIRP(PIRP pIrp);
void PrintIRPStack(PIO_STACK_LOCATION IrpStack);
void PrintFileObject(PFILE_OBJECT pFileObject);

#endif // !_NTPRINT_H
