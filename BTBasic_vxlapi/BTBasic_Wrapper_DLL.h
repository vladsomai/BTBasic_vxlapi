#ifndef BTBASICHEADER
#define BTBASICHEADER

// Definition of enum EXT_DLL_Result
typedef enum class EXT_DLL_Result
{
    EXT_DLL_Result_Error = -1,
    EXT_DLL_Result_OK = 0,
}EXT_DLL_Result;

#ifdef __cplusplus

extern "C" {
#endif

#define DllExport __declspec(dllexport)

    // Export declaration section for function BTBasic_DLL_Call. This function will be
    // called by BT-Basic. 
    DllExport EXT_DLL_Result BTBasic_DLL_Call(char* functionName, char* parameters, char* returnString, int* returnValue);

#ifdef __cplusplus
}
#endif

#endif // !BTBASICHEADER