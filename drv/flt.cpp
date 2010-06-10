#include "pch.h"
#include "../inc/accessch.h"
#include "flt.h"
#include "fltstore.h"
#include "excludes.h"

VOID
DeleteAllFilters (
    )
{
    FiltersTree:DeleteAllFilters();
}

__checkReturn
NTSTATUS
FilterEvent (
    __in EventData *Event,
    __inout PVERDICT Verdict,
    __out PARAMS_MASK *ParamsMask
    )
{
    PHANDLE pRequestorProcess;
    ULONG fieldSize;
    
    NTSTATUS status = Event->QueryParameter (
        PARAMETER_REQUESTOR_PROCESS_ID,
        (PVOID*) &pRequestorProcess,
        &fieldSize
        );
    
    if ( !NT_SUCCESS( status ) )
    {
        return STATUS_UNSUCCESSFUL;
    }

    if ( IsInvisibleProcess( *pRequestorProcess ) )
    {
        return STATUS_NOT_SUPPORTED;
    }

    Filters* pFilters = FiltersTree::GetFiltersBy (
        Event->GetInterceptorId(),
        Event->GetOperationId(),
        Event->GetMinor(),
        Event->GetOperationType()
        );
    
    if ( pFilters )
    {
        *Verdict = pFilters->GetVerdict( Event, ParamsMask );

        pFilters->Release();
    }
    else
    {
        *Verdict = VERDICT_NOT_FILTERED;
    }
    
    return STATUS_SUCCESS;
}

__checkReturn
NTSTATUS
FilterProceedChain (
    __in PFILTERS_CHAIN Chain,
    __in ULONG ChainSize
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    
    ASSERT( ARGUMENT_PRESENT( Chain ) );

    Filters* pFilters = NULL;

   __try
   {
        PCHAIN_ENTRY pEntry = Chain->m_Entry;

        for ( ULONG item = 0; item < Chain->m_Count; item++ )
        {
            switch( pEntry->m_Operation )
            {
            case _fltchain_add:
                {
                    Filters* pFilters = FiltersTree::GetOrCreateFiltersBy (
                        pEntry->m_Filter[0].m_Interceptor,
                        pEntry->m_Filter[0].m_FunctionMj,
                        pEntry->m_Filter[0].m_FunctionMi,
                        pEntry->m_Filter[0].m_OperationType
                        );

                    if ( pFilters )
                    {
                        ULONG id;
                        status = pFilters->AddFilter (
                            pEntry->m_Filter->m_RequestTimeout,
                            pEntry->m_Filter->m_WishMask,
                            pEntry->m_Filter->m_ParamsCount,
                            pEntry->m_Filter->m_Params,
                            &id
                            );

                        pFilters->Release();
                        pFilters = NULL;
                    }
                    else
                    {
                        status = STATUS_UNSUCCESSFUL;
                        break;
                    }
                }

                break;
            }
        }
   }
   __finally
   {
       if ( pFilters )
       {
           pFilters->Release();
           pFilters = NULL;
       }
   }

    return status;
}