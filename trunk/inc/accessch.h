#ifndef __accesscheck_h
#define __accesscheck_h

#define ACCESSCH_PORT_NAME          L"\\AccessCheckPort"
#define ACCESSCH_MAX_CONNECTIONS    1

#define DRV_EVENT_CONTENT_SIZE      0x1000

#define _STREAM_FLAGS_DIRECTORY     0x00000001

typedef enum Interceptors
{
    FILE_MINIFILTER                 = 0,
    VOLUME_MINIFILTER               = 1,
};

typedef enum DriverOperationId
{
    OP_UNKNOWN                      = -1,
    OP_VOLUME_ATTACH                = 1,
    OP_FILE_CREATE                  = 2,
    OP_FILE_CLEANUP                 = 3,
};

typedef enum OperationPoint
{
    PreProcessing   = 0x001,
    PostProcessing  = 0x002,
};

typedef enum Parameters
{
    // bit position
    PARAMETER_RESERVED0             = 0,
    PARAMETER_RESERVED1             = 1,
    PARAMETER_FILE_NAME             = 2,
    PARAMETER_VOLUME_NAME           = 3,
    PARAMETER_REQUESTOR_PROCESS_ID  = 4,
    PARAMETER_CURRENT_THREAD_ID     = 5,
    PARAMETER_LUID                  = 6,
    PARAMETER_SID                   = 7,
    PARAMETER_DESIRED_ACCESS        = 8,
    PARAMETER_CREATE_OPTIONS        = 9,
    PARAMETER_CREATE_MODE           = 10,
    PARAMETER_RESULT_STATUS         = 11,
    PARAMETER_RESULT_INFORMATION    = 12,
    PARAMETER_DEVICE_TYPE           = 13,
    PARAMETER_FILESYSTEM_TYPE       = 14,
    PARAMETER_BUS_TYPE              = 15,
    PARAMETER_DEVICE_ID             = 16,

    //
    PARAMETER_MAXIMUM               = 63
} *PParameters;

#define Id2Bit( _id ) ( 1 << _id )

#define PARAMS_MASK __int64
#define _PARAMS_COUNT ( sizeof( PARAMS_MASK ) * 8 )

typedef ULONG VERDICT, *PVERDICT;
#define VERDICT_NOT_FILTERED        0x0000
#define VERDICT_ALLOW               0x0001
#define VERDICT_DENY                0x0002
#define VERDICT_ASK                 0x0004

#include <pshpack8.h>

// send message communication  
typedef struct _REPLY_RESULT
{
    ULONG               m_Flags;
} REPLY_RESULT, *PREPLY_RESULT;

typedef struct _EVENT_PARAMETER
{
    Parameters          m_Id;
    ULONG               m_Size;
    UCHAR               m_Data[1];
} EVENT_PARAMETER, *PEVENT_PARAMETER;

typedef struct _MESSAGE_DATA
{
    ULONG               m_EventId;
    Interceptors        m_Interceptor;
    DriverOperationId   m_OperationId;
    ULONG               m_FuncionMi;
    OperationPoint      m_OperationType;
    ULONG               m_ParametersCount;
    EVENT_PARAMETER     m_Parameters[1];
} MESSAGE_DATA, *PMESSAGE_DATA;

// notify structures
typedef enum _NOTIFY_ID
{
    // common commands
    ntfcom_Connect       = 0x0001,
    
    // object's commands
    ntfcom_PrepareIO     = 0x0100 // result struct
} NOTIFY_ID;

typedef struct _NOTIFY_COMMAND
{
    ULONG               m_Reserved;
    NOTIFY_ID           m_Command;
    ULONG               m_EventId;
} NOTIFY_COMMAND, *PNOTIFY_COMMAND;

// ntfcom_PrepareIO
typedef struct _NC_IOPREPARE
{
    HANDLE              m_Section;
    LARGE_INTEGER       m_IoSize;
} NC_IOPREPARE, *PNC_IOPREPARE;

// filters structures
// �������� ��������� ��� ������ � ��������� � �������, ������� ���������
// ����������� ������� ��������� � r3

#define FilterChain     PVOID

typedef enum FltOperation
{
    _fltop_equ      = 0x0000,
    _fltop_and      = 0x0001,
};

typedef struct _FILTER_PARAMETER
{
    ULONG               m_Size;
    UCHAR               m_Data[1];
} FILTER_PARAMETER, *PFILTER_PARAMETER;

typedef struct _PARAM_ENTRY
{
    Parameters          m_Id;
    ULONG               m_Count;
    FltOperation        m_Operation;
    FILTER_PARAMETER    m_FltData;
}PARAM_ENTRY, *PPARAM_ENTRY;

typedef struct _FILTER
{
    Interceptors        m_Interceptor;
    ULONG               m_FunctionMj;
    ULONG               m_FunctionMi;
    OperationPoint    m_OperationType;
    ULONG               m_RequestTimeout;       //msec
    ULONG               m_ParamsCount;
    PARAM_ENTRY         m_Params[1];
} FILTER,*PFILTER;

// end filters structures

#include <poppack.h>

#endif // __accesscheck_h 