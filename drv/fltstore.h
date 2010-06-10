#ifndef __fltstore_h
#define __fltstore_h

#define FilterId ULONG

#define NumberOfBits 256

typedef enum _OpertorId
{
    fltop_AND           = 0,
} OpertorId;

typedef struct _FltData
{
    ULONG               m_DataSize;
    UCHAR               m_Data[1];
} FltData;

typedef struct _ParamCheckEntry
{
    LIST_ENTRY          m_List;
    OpertorId           m_Operation;
    ULONG               m_NumbersCount;
    PULONG              m_FilterNumbers;
    FltData             m_Data;
} ParamCheckEntry;

typedef struct _FilterEntry
{
    LIST_ENTRY          m_List;
    ULONG               m_FilterId;
    PARAMS_MASK         m_WishMask;
    ULONG               m_RequestTimeout;
    //ULONG               m_AggregationId;
} FilterEntry;

//////////////////////////////////////////////////////////////////////////
class Filters
{

public:
    Filters();
    ~Filters();

    NTSTATUS
    AddRef (
        );

    void
    Release();

    VERDICT
    GetVerdict (
        __in EventData *Event,
        __out PARAMS_MASK *ParamsMask
        );
    
    NTSTATUS
    AddFilter (
        __in_opt ULONG RequestTimeout,
        __in PARAMS_MASK WishMask,
        __in ULONG ParamsCount,
        __in PPARAM_ENTRY Params,
        __out PULONG FilterId
        );

private:
    EX_RUNDOWN_REF      m_Ref;
    EX_PUSH_LOCK        m_AccessLock;

    RTL_BITMAP          m_ActiveFilters;
    ULONG               m_ActiveFiltersBuffer[ NumberOfBits / sizeof(ULONG) ];
    ULONG               m_FilterCount;
    LIST_ENTRY          m_FilterEntryList;
    LIST_ENTRY          m_ParamsCheckList;
};

//////////////////////////////////////////////////////////////////////////

class FiltersTree
{
public:
    static
    VOID
    Initialize (
        );

    static
    VOID
    Destroy (
        );

    static
    VOID
    DeleteAllFilters (
        );

    static LONG GetNextFilterid();

    static RTL_AVL_COMPARE_ROUTINE Compare;
    static RTL_AVL_ALLOCATE_ROUTINE Allocate;
    static RTL_AVL_FREE_ROUTINE Free;
  
    __checkReturn
    static
    Filters*
    GetFiltersBy (
        __in Interceptors Interceptor,
        __in DriverOperationId Operation,
        __in_opt ULONG Minor,
        __in OperationPoint OperationType
        );
    
    __checkReturn
    static
    Filters*
    GetOrCreateFiltersBy (
        __in Interceptors Interceptor,
        __in DriverOperationId Operation,
        __in_opt ULONG Minor,
        __in OperationPoint OperationType
        );
    
private:
    static RTL_AVL_TABLE    m_Tree;
    static EX_PUSH_LOCK     m_AccessLock;
    
    // \todo �������� ������� ��� ���������� ����������
    static LONG             m_FilterIdCounter;

public:
    FiltersTree();
    ~FiltersTree();
};


#endif //__fltstore_h