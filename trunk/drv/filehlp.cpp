#include "pch.h"
#include "main.h"
#include "../inc/accessch.h"

#include "filehlp.h"
#include "security.h"

//! \todo FILE_OPEN_NO_RECALL

#ifndef GUID_ECP_PREFETCH_OPEN
#define GUID_ECP_PREFETCH_OPEN_notdefined
DEFINE_GUID( GUID_ECP_PREFETCH_OPEN, 0xe1777b21, 0x847e, 0x4837, 0xaa, 
    0x45, 0x64, 0x16, 0x1d, 0x28, 0x6, 0x55 );
#endif // GUID_ECP_PREFETCH_OPEN


DriverOperationId
FileOperationSystemToInternal (
    ULONG OperationId
    )
{
    switch ( OperationId )
    {
    case IRP_MJ_CREATE:
        return OP_FILE_CREATE;
    
    case IRP_MJ_CLEANUP:
        return OP_FILE_CLEANUP;

    default:
        __debugbreak();
    }

    return OP_UNKNOWN;
}

//////////////////////////////////////////////////////////////////////////
__checkReturn
BOOLEAN
IsPrefetchEcpPresent (
    __in PFLT_FILTER Filter,
    __in PFLT_CALLBACK_DATA Data
    )
{
#if FLT_MGR_LONGHORN
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    PECP_LIST EcpList = NULL;
    PPREFETCH_OPEN_ECP_CONTEXT PrefetchEcp = NULL;

    // Get the ECP List from the callback data, if present.

    status = FltGetEcpListFromCallbackData( Filter, Data, &EcpList );

    if ( NT_SUCCESS( status ) && EcpList )
    {
        // Check if the prefetch ECP is specified.
        status = FltFindExtraCreateParameter (
            Filter,
            EcpList,
            &GUID_ECP_PREFETCH_OPEN,
            (PVOID*) &PrefetchEcp,
            NULL
            );

        if ( NT_SUCCESS( status ) )
        {
            if ( !FltIsEcpFromUserMode( Filter, PrefetchEcp ) )
            {
                return TRUE;
            }
        }
    }
#else
    UNREFERENCED_PARAMETER( Filter );
    UNREFERENCED_PARAMETER( Data );
#endif // FLT_MGR_LONGHORN

    return FALSE;
}

__checkReturn
NTSTATUS
QueryFileNameInfo (
    __in PFLT_CALLBACK_DATA Data,
    __drv_when(return==0, __deref_out_opt __drv_valueIs(!=0))
    PFLT_FILE_NAME_INFORMATION* FileNameInfo
    )
{
    ULONG QueryNameFlags = FLT_FILE_NAME_NORMALIZED
        | FLT_FILE_NAME_QUERY_ALWAYS_ALLOW_CACHE_LOOKUP;

    NTSTATUS status = FltGetFileNameInformation (
        Data,
        QueryNameFlags,
        FileNameInfo
        );

    if ( !NT_SUCCESS( status ) )
    {
        return status;
    }

    ASSERT( FileNameInfo );

    if ( !FileNameInfo )
    {
        return STATUS_UNSUCCESSFUL;
    }

    status = FltParseFileNameInformation( *FileNameInfo );

    ASSERT( NT_SUCCESS( status ) ); //ignore unsuccessful parse

    return STATUS_SUCCESS;
}

void
ReleaseFileNameInfo (
    __in_opt PFLT_FILE_NAME_INFORMATION* FileNameInfo
    )
{
    ASSERT( FileNameInfo );

    if ( *FileNameInfo )
    {
        FltReleaseFileNameInformation( *FileNameInfo );
        *FileNameInfo = NULL;
    };
}

void
ReleaseContext (
    __deref_out_opt PFLT_CONTEXT* Context
    )
{
    ASSERT( Context );

    if ( !*Context )
    {
        return;
    }

    FltReleaseContext( *Context );
    *Context = NULL;
}

__checkReturn
NTSTATUS
IsDirectoryImp (
    __in PFLT_INSTANCE Instance,
    __in PFILE_OBJECT FileObject,
    __out PBOOLEAN IsDirectory
    )
{
    NTSTATUS status;

    if ( NtBuildNumber >= 6001 )
    {
        status = FltIsDirectory (
            FileObject,
            Instance,
            IsDirectory
            );
    }
    else
    {
        FILE_STANDARD_INFORMATION fsi = {};
        status = FltQueryInformationFile (
            Instance,
            FileObject,
            &fsi,
            sizeof( fsi ),
            FileStandardInformation,
            0
            );

        if ( NT_SUCCESS( status ) )
        {
            *IsDirectory = fsi.Directory;
        }
    }

    return status;
}

__checkReturn
NTSTATUS
FileIsMarkedForDelete  (
    __in PFLT_INSTANCE Instance,
    __in PFILE_OBJECT FileObject,
    __out PBOOLEAN IsMarked
    )
{
    ASSERT( IsMarked );

    FILE_STANDARD_INFORMATION fsi = {};
    NTSTATUS status = FltQueryInformationFile (
        Instance,
        FileObject,
        &fsi,
        sizeof( fsi ),
        FileStandardInformation,
        0
        );

    if ( NT_SUCCESS( status ) )
    {
        *IsMarked = fsi.DeletePending;
    }

    return status;
}

__checkReturn
NTSTATUS
GenerateStreamContext (
    __in PFLT_FILTER Filter,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __drv_when(return==0, __deref_out_opt __drv_valueIs(!=0))
    PSTREAM_CONTEXT* StreamContext
    )
{
    ASSERT( ARGUMENT_PRESENT( Filter ) );
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    PINSTANCE_CONTEXT InstanceContext = NULL;

    if ( !FsRtlSupportsPerStreamContexts( FltObjects->FileObject ) )
    {
        return STATUS_NOT_SUPPORTED;
    }

    status = FltGetStreamContext (
        FltObjects->Instance,
        FltObjects->FileObject,
        (PFLT_CONTEXT*) StreamContext
        );

    if ( NT_SUCCESS( status ) )
    {
        ASSERT( *StreamContext );

        return status;
    }

    status = FltAllocateContext (
        Filter,
        FLT_STREAM_CONTEXT,
        sizeof( STREAM_CONTEXT ),
        NonPagedPool,
        (PFLT_CONTEXT*) StreamContext
        );

    if ( !NT_SUCCESS( status ) )
    {
        return status;
    }

    RtlZeroMemory( *StreamContext, sizeof( STREAM_CONTEXT ) );

    status = FltGetInstanceContext (
        FltObjects->Instance,
        (PFLT_CONTEXT *) &InstanceContext
        );

    // \todo on shadow volumes will be NOT_FOUND
    // ASSERT( NT_SUCCESS( status ) );

    (*StreamContext)->m_InstanceContext = InstanceContext;

    BOOLEAN bIsDirectory;

    status = IsDirectoryImp (
        FltObjects->Instance,
        FltObjects->FileObject,
        &bIsDirectory
        );

    if ( NT_SUCCESS( status ) )
    {
        if ( bIsDirectory )
        {
            InterlockedOr (
                &(*StreamContext)->m_Flags,
                _STREAM_FLAGS_DIRECTORY
                );
        }
    }

    status = FltSetStreamContext (
        FltObjects->Instance,
        FltObjects->FileObject,
        FLT_SET_CONTEXT_REPLACE_IF_EXISTS,
        *StreamContext,
        NULL
        );

    if ( !NT_SUCCESS( status ) )
    {
        ReleaseContext( (PFLT_CONTEXT*) StreamContext );
    }
    else
    {
        ASSERT( *StreamContext );
    }

    return status;
}

__checkReturn
 NTSTATUS
 GetStreamHandleContext (
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out_opt PSTREAMHANDLE_CONTEXT* StreamHandleContext
    )
{
    NTSTATUS status = FltGetStreamHandleContext (
        FltObjects->Instance,
        FltObjects->FileObject,
        (PFLT_CONTEXT*) StreamHandleContext
        );

    return status;
}

__checkReturn
NTSTATUS
GenerateStreamHandleContext (
    __in PFLT_FILTER Filter,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out_opt PSTREAMHANDLE_CONTEXT* StreamHandleContext
    )
{
    NTSTATUS status = GetStreamHandleContext (
        FltObjects,
        StreamHandleContext
        );

    if ( NT_SUCCESS( status ) )
    {
        ASSERT( *StreamHandleContext );

        return status;
    }

    status = FltAllocateContext (
        Filter,
        FLT_STREAMHANDLE_CONTEXT,
        sizeof( STREAMHANDLE_CONTEXT ),
        NonPagedPool,
        (PFLT_CONTEXT*) StreamHandleContext
        );

    if ( !NT_SUCCESS( status ) )
    {
        return status;
    }

    RtlZeroMemory( *StreamHandleContext, sizeof( STREAMHANDLE_CONTEXT ) );

    FltSetStreamHandleContext (
        FltObjects->Instance,
        FltObjects->FileObject,
        FLT_SET_CONTEXT_REPLACE_IF_EXISTS,
        *StreamHandleContext,
        NULL
        );

    if ( !NT_SUCCESS( status ) )
    {
        ReleaseContext( (PFLT_CONTEXT*) StreamHandleContext );
    }
    else
    {
        ASSERT( *StreamHandleContext );
    }

    
    return STATUS_SUCCESS;
}

