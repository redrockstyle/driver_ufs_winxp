#include <ntddk.h>
#include "ntprint.h"
//
// Печать информации об IRP
//
void PrintIRP(PIRP pIrp) {

    KdPrint(("\n"));
    KdPrint(("IRP %p:\n", pIrp));
    //KdPrint"Type %d\n", pIrp->Type));
    //KdPrint"Size %d\n", pIrp->Size));
    //KdPrint"MdlAddress %p\n", pIrp->MdlAddress));
    //KdPrint"Flags %p\n", pIrp->Flags));
    KdPrint(("RequestorMode %d\n", pIrp->RequestorMode));
    KdPrint(("StackCount %d\n", pIrp->StackCount));
    KdPrint(("CurrentLocation %d\n", pIrp->CurrentLocation));
    KdPrint(("UserBuffer %p\n", pIrp->UserBuffer));
    KdPrint(("SystemBuffer %p\n", pIrp->AssociatedIrp.SystemBuffer));
    KdPrint(("\n"));

    return;
}


//--------------------

//
// Печать информации о текущем элементе стека IRP
//
void PrintIRPStack(PIO_STACK_LOCATION pIrpStack) {

    KdPrint(("\n"));
    KdPrint(("IO_STACK_LOCATION %p:\n", pIrpStack));
    KdPrint(("MajorFunction %d\n", pIrpStack->MajorFunction));
    KdPrint(("MinorFunction %d\n", pIrpStack->MinorFunction));
    //KdPrint(("Flags %p\n", pIrpStack->Flags));
    //KdPrint(("Control %p\n", pIrpStack->Control));
    KdPrint(("DeviceObject %p\n", pIrpStack->DeviceObject));
    KdPrint(("FileObject %p\n", pIrpStack->FileObject));
    KdPrint(("\n"));

    return;
}


//--------------------

//
// Печать информации об объекте файла
//
void PrintFileObject(PFILE_OBJECT pFileObject) {

    DbgPrint("\n");
    DbgPrint("FileObject %p:\n", pFileObject);
    //DbgPrint ("Type %d\n", pFileObject->Type);
    //DbgPrint ("Size %d\n", pFileObject->Size);
    DbgPrint("DeviceObject %p\n", pFileObject->DeviceObject);
    //DbgPrint ("FinalStatus %p\n", pFileObject->FinalStatus);
    //DbgPrint ("LockOperation %d\n", pFileObject->LockOperation);
    //DbgPrint ("DeletePending %d\n", pFileObject->DeletePending);
    //DbgPrint ("ReadAccess %d\n", pFileObject->ReadAccess);
    //DbgPrint ("WriteAccess %d\n", pFileObject->WriteAccess);
    //DbgPrint ("DeleteAccess %d\n", pFileObject->DeleteAccess);
    //DbgPrint ("SharedRead %d\n", pFileObject->SharedRead);
    //DbgPrint ("SharedWrite %d\n", pFileObject->SharedWrite);
    //DbgPrint ("SharedDelete %d\n", pFileObject->SharedDelete);
    DbgPrint("Flags %p\n", pFileObject->Flags);

    DbgPrint("FileName %wZ\n", &pFileObject->FileName);

    //DbgPrint ("CurrentByteOffset %p%p\n", *((int*)&pFileObject->CurrentByteOffset.QuadPart), *((int*)&pFileObject->CurrentByteOffset.QuadPart+1));
    DbgPrint("\n");

    return;
}
