/*
This function mimics the BTBasic environment, use it for debugging purposes.
You can toggle .exe or .dll compilation in the project properties.
*/

#include "lib.h"
#include"BTBasic_Wrapper_DLL.h"

extern std::unique_ptr<std::vector<std::string>> receivedBuffer;

int main()
{
	{
		char functionName[100] = "InitDriver";
		char Parameters[100] = "0,000000FF,0,0";
		char returnString[2028] = "";
		int* returnVal = nullptr;

		EXT_DLL_Result result = BTBasic_DLL_Call(functionName, Parameters, returnString, returnVal);
		std::cout <<"Return string: "<< returnString << std::endl;
	}
	system("pause");
	/*
	{
		//get platform key
		char functionName[100] = "sendData";
		char Parameters[100] = "0300FECE33";
		char returnString[2028] = "";
		int* returnVal = nullptr;

		EXT_DLL_Result result = BTBasic_DLL_Call(functionName, Parameters, returnString, returnVal);
		std::cout << "Return string: " << returnString << std::endl;
	}
	*/

	{
		char functionName[100] = "loadToolbox";
		char Parameters[200] = "d:\\_DESKTOP\\_VISUAL_STUDIO\\BTBasic_vxlapi\\Debug\\JCP2_ID_10_43_54_65_66_C3_CF_D0_DD_F1.bin";
		char returnString[2028] = "";
		int* returnVal = nullptr;

		EXT_DLL_Result result = BTBasic_DLL_Call(functionName, Parameters, returnString, returnVal);
		std::cout << "Return string: " << returnString << std::endl;
	}
	/*
	{
		//Enter EOL
		char functionName[100] = "sendData";
		char Parameters[100] = "84000000,BYPASS";
		char returnString[2028] = "";
		int* returnVal = nullptr;

		EXT_DLL_Result result = BTBasic_DLL_Call(functionName, Parameters, returnString, returnVal);
		std::cout << "Return string: " << returnString << std::endl;

	}

		{
		//Enter FHOST
		char functionName[100] = "FHost";
		char Parameters[200] = "d:\\FHostSP_V06.01.01\\FHostSP.exe,d:\\_DESKTOP\\C_HUD_PROJECT_ICT\\A2C14807602_NEW_CHUD\\10445552_FIL_000_AA\\OPBytes_CHUD.prg";
		char returnString[2028] = "";
		int* returnVal = nullptr;

		EXT_DLL_Result result = BTBasic_DLL_Call(functionName, Parameters, returnString, returnVal);
		std::cout << "Return string: " << returnString << std::endl;
	}

	std::cout << "Reset PCB" << std::endl;
	{
		//get kernel version
		char functionName[100] = "sendData";
		char Parameters[100] = "0300FECC31,BYPASS";
		char returnString[2028] = "";
		int* returnVal = nullptr;

		EXT_DLL_Result result = BTBasic_DLL_Call(functionName, Parameters, returnString, returnVal);
		std::cout << "Return string: " << returnString << std::endl;
	}


	{
		//Enter EOL
		char functionName[100] = "getData";
		char Parameters[100] = "";
		char returnString[2028] = "";
		int* returnVal = nullptr;

		EXT_DLL_Result result = BTBasic_DLL_Call(functionName, Parameters, returnString, returnVal);
		std::cout << "Return string: " << returnString << std::endl;
	}


	
	*/

	/*

	{
		//get kernel version
		char functionName[100] = "sendData";
		char Parameters[100] = "0300FECC31";
		char returnString[2028] = "";
		int* returnVal = nullptr;

		EXT_DLL_Result result = BTBasic_DLL_Call(functionName, Parameters, returnString, returnVal);
		std::cout << "Return string: " << returnString << std::endl;
	}

	*/
	{
		char functionName[100] = "DisableDriver";
		char Parameters[100] = "";
		char returnString[2028] = "";
		int* returnVal = nullptr;
		uint16_t a = 0;

		EXT_DLL_Result result = BTBasic_DLL_Call(functionName, Parameters, returnString, returnVal);
		std::cout << "Return string: " << returnString << std::endl;
	}
}
