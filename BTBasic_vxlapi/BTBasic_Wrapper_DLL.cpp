/*
* !!!READ ME!!!!
* 
* DLL MUST BE COMPILED ON X86 ONLY.
* 
* YOU CAN USE GLOBAL VARIABLES - THEY WILL EXIST BETWEEN "dllload" and "dllunload" calls from BTBasic testplan.
* 
* YOU CANNOT DEREFERENCE THE "returnValue" PARAMETER, IT IS SET ONLY BY THE BTBasic HOST PROCESS WITH THE ENUM RETURN TYPE.
* The "returnValue" can be used from MCD only.
*
* DEFAULT "returnValue" FROM "BTBasic_DLL_Call" function is "EXT_DLL_Result::EXT_DLL_Result_Error", 
* RETRUN "EXT_DLL_Result::EXT_DLL_Result_OK" ONLY WHEN YOUR CONDITIONS ARE MET.
* 
* YOU CAN RETURN INFO TO BTBasic TESTPLAN ONLY WITH "returnString", MAXIMUM 2048 BYTES.
* 
* DO NOT CHANGE THE "BTBasic_DLL_Call" FUNCTION SIGNATURE OR REDEFINE / MODIFY THE ENUM "EXT_DLL_Result" FROM THE HEADER.
* 
*/

#include "lib.h"
#include "BTBasic_Wrapper_DLL.h"
#include "vxlapi.h"

extern int8_t InitDriver(uint32_t channelIndexParam,
						 uint32_t rxID_Param,
						 uint32_t txID_Param,
						 uint8_t extendedCANID_Param,
	                     uint32_t baudRateParam);
extern int8_t DisableDriver();

extern int8_t loadToolbox(const char*);
extern int8_t sendData(std::string);
extern int8_t FHost(std::string FHostPath, 
	                std::string PRG_Path);

extern std::unique_ptr<std::vector<std::string>> receivedBuffer;

extern void copyReturnString(const char* input, char* retString);
extern std::vector<std::string> parse_C_style_str(char* input);
extern uint32_t StringToHex32(std::string input);

DllExport EXT_DLL_Result BTBasic_DLL_Call(char* functionName, char* parameters,
	char* returnString, int* returnValue)
{
	const char* messageToReturn = nullptr;
	size_t sizeOfMessage = 0;

	std::vector<std::string> params = parse_C_style_str(parameters);

	//Handle each function here
	if (strcmp(functionName, "loadToolbox") == 0)
	{
		receivedBuffer->clear();
		int8_t toolboxResult = GENERAL_ERROR;
		std::string toolboxFilePath;
		std::string receivedData;

		try
		{
			toolboxFilePath = params.at(0);
		}
		catch (const std::out_of_range& toolboxErr)
		{
			std::out_of_range temp = toolboxErr;
			copyReturnString("ERROR: Path is not specified!", returnString);
			return EXT_DLL_Result::EXT_DLL_Result_Error;
		}

		toolboxResult = loadToolbox(toolboxFilePath.c_str());
		std::this_thread::sleep_for(std::chrono::milliseconds(300));

		switch (toolboxResult)
		{
		case OK:
			try
			{
				receivedData = receivedBuffer->at(0);
			}
			catch (const std::out_of_range& BufferEmpty)
			{
				std::out_of_range temp = BufferEmpty;
				copyReturnString("ERROR: Toolbox data was sent successfully but no message received from ECU!", returnString);
				return EXT_DLL_Result::EXT_DLL_Result_Error;
			}
			copyReturnString(receivedData.c_str(), returnString);
			return EXT_DLL_Result::EXT_DLL_Result_OK;
		case FILE_COULD_NOT_BE_OPENED:
			copyReturnString("ERROR: Toolbox file could not be opened, check whether is not in use!", returnString);
			return EXT_DLL_Result::EXT_DLL_Result_Error;
		case RX_TX_ID_IS_INVALID:
			copyReturnString("ERROR: rxID or txID are invalid, check whether you specify the ext id flag!", returnString);
			return EXT_DLL_Result::EXT_DLL_Result_Error;
		case TOOLBOX_FILE_DOES_NOT_EXIST:
			copyReturnString("ERROR: Toolbox file does not exist in the specified path!", returnString);
			return EXT_DLL_Result::EXT_DLL_Result_Error;
		default:
			copyReturnString("ERROR: Message not sent, please check whether you configured the driver correctly."
				, returnString);
			return EXT_DLL_Result::EXT_DLL_Result_Error;
		}
	}
	else if (strcmp(functionName, "getData") == 0)
	{
		std::string buffer{};

		if (receivedBuffer.get()->size() != 0)//verify that vector is not empty
		{
			for (const auto& message : *receivedBuffer.get())
			{
				buffer += message;
			}

			copyReturnString(buffer.c_str(), returnString);
			return EXT_DLL_Result::EXT_DLL_Result_OK;
		}

		copyReturnString("ERROR: No message received!", returnString);
		return EXT_DLL_Result::EXT_DLL_Result_Error;
	}
	else if (strcmp(functionName, "sendData") == 0)
	{
		receivedBuffer->clear();

		int8_t messageResult = GENERAL_ERROR;
		std::string DataToBeSent{};
		std::string buffer{};
		std::string bypassReceive{};

		try
		{
			DataToBeSent = params.at(0);
		}
		catch (const std::out_of_range& sendDataErr)
		{
			std::out_of_range temp = sendDataErr;
			copyReturnString("ERROR: Data to be sent is not specified!", returnString);
			return EXT_DLL_Result::EXT_DLL_Result_Error;
		}
		try
		{
			bypassReceive = params.at(1);
		}
		catch (const std::out_of_range& BypassMode)
		{
			std::out_of_range temp = BypassMode;
#if _DEBUG
			std::cout << "BYPASS parameter not specified." << std::endl;
			std::cout << "Error catched: " << BypassMode.what() << std::endl;
#endif
		}

		messageResult = sendData(DataToBeSent);

		if (messageResult == OK && bypassReceive == "BYPASS")
		{
			copyReturnString("Returned to testplan, BYPASS MODE", returnString);
			return EXT_DLL_Result::EXT_DLL_Result_OK;
		}

		//in normal mode function returns the received buffer
		if (messageResult == OK)
		{
			if (receivedBuffer.get()->size() != 0)//verify that vector is not empty
			{
				for (const auto& message : *receivedBuffer.get())
				{
					buffer += message;
				}

				copyReturnString(buffer.c_str(), returnString);
				return EXT_DLL_Result::EXT_DLL_Result_OK;
			}
			else
			{
				copyReturnString("ERROR: No message returned.", returnString);
				return EXT_DLL_Result::EXT_DLL_Result_Error;
			}
		}
		else if (messageResult == MESSAGE_NOT_SENT)
		{
			copyReturnString("ERROR: Message could not be sent.", returnString);
			return EXT_DLL_Result::EXT_DLL_Result_Error;
		}
		else if (messageResult == RX_TX_ID_IS_INVALID)
		{
			copyReturnString("ERROR: rxID or txID are invalid, check whether you specify the ext id flag!", returnString);
			return EXT_DLL_Result::EXT_DLL_Result_Error;
		}
		else if (messageResult == DATA_IS_INVALID)
		{
			copyReturnString("ERROR: Message is not in the right format, check documentation.", returnString);
			return EXT_DLL_Result::EXT_DLL_Result_Error;
		}

		copyReturnString("ERROR: Something went wrong!", returnString);
		return EXT_DLL_Result::EXT_DLL_Result_Error;
	}
	else if (strcmp(functionName, "FHost") == 0)
	{
		int8_t FHostResult = GENERAL_ERROR;
		std::string FHostPath{};
		std::string PRGPath{};
		try
		{
			FHostPath = params.at(0);
			PRGPath = params.at(1);
		}
		catch (const std::out_of_range& fhostParam)
		{
#if _DEBUG
			std::cout << "Error: " << fhostParam.what() << std::endl;
#endif
			std::out_of_range temp = fhostParam;

			copyReturnString("ERROR: Parameters are invalid!", returnString);
			return EXT_DLL_Result::EXT_DLL_Result_Error;
		}

		FHostResult = FHost(FHostPath, PRGPath);

		switch (FHostResult)
		{
		case OK:
			copyReturnString("FHost executed successfully!", returnString);
			return EXT_DLL_Result::EXT_DLL_Result_OK;

		case FHOST_FILE_DOES_NOT_EXIST:
			copyReturnString("ERROR: FHost.exe not found, verify the specified path!", returnString);
			return EXT_DLL_Result::EXT_DLL_Result_Error;

		case PRG_FILE_DOES_NOT_EXIST:
			copyReturnString("ERROR: PRG file not found, verify the specified path!", returnString);
			return EXT_DLL_Result::EXT_DLL_Result_Error;

		case ERROR_EXECUTING_FHOST_COMMAND:
			copyReturnString("ERROR: FHOST Command could not be executed!", returnString);
			return EXT_DLL_Result::EXT_DLL_Result_Error;

		case FILE_COULD_NOT_BE_OPENED:
			copyReturnString("ERROR: Result file could not be opened!", returnString);
			return EXT_DLL_Result::EXT_DLL_Result_Error;

		case FHOST_FINISHED_WITH_ERROR:
			copyReturnString("ERROR: FHost finished with error, please check the result file under the FHost folder!", returnString);
			return EXT_DLL_Result::EXT_DLL_Result_Error;

		default:
			copyReturnString("ERROR: Something went wrong.", returnString);
			return EXT_DLL_Result::EXT_DLL_Result_Error;
		}

		copyReturnString("ERROR: Something went wrong.", returnString);
		return EXT_DLL_Result::EXT_DLL_Result_Error;
	}
	else if (strcmp(functionName, "InitDriver") == 0)
	{
		int8_t result = GENERAL_ERROR;

		std::string channelIndexString{};
		uint32_t channelIndex = 0;

		std::string rxIDString{};
		uint32_t rxID = 0;

		std::string txIDString{};
		uint32_t txID = 0;

		std::string  extendedCANIDString{};
		uint8_t  extendedCANID = 0;

		std::string baudRateString = "";
		std::uint32_t baudRate = 0;

		try
		{
			channelIndexString = params.at(0);
			rxIDString = params.at(1);
			txIDString = params.at(2);
			extendedCANIDString = params.at(3);

			if (rxIDString.length() > 8 || txIDString.length() > 8)
			{
				throw std::out_of_range("rxID or txID is too large, consider using 32bit value");
			}
		}
		catch (const std::out_of_range& vectorOutOfRange)
		{
#if _DEBUG
			std::cout << "Parameters are not vaild." << std::endl;
			std::cout << "Error: " << vectorOutOfRange.what() << std::endl;
#endif
			std::out_of_range temp = vectorOutOfRange;

			copyReturnString("ERROR: Driver could not be initialized, parameters are invalid!", returnString);
			return EXT_DLL_Result::EXT_DLL_Result_Error;
		}

		//in case baud rate is specified 
		try
		{
			baudRateString = params.at(4);
		}
		catch (const std::out_of_range& BaudRateErr)
		{
#if _DEBUG
			std::cout << "Setting default baud rate to 500k" << std::endl;
			std::cout << "Error: " << BaudRateErr.what() << std::endl;
#endif
			std::out_of_range temp = BaudRateErr;
		}

		//Convert parameters from string to int
		try
		{
			channelIndex = std::stoi(channelIndexString);
			extendedCANID = std::stoi(extendedCANIDString);
			if (!baudRateString.empty())
				baudRate = std::stoi(baudRateString);
		}
		catch (const std::invalid_argument& ia) {
			std::invalid_argument temp = ia;
			copyReturnString("ERROR: Driver could not be initialized, you introduced an invalid channel index, extended CANID or baud rate! Consider using numbers.", returnString);
			return EXT_DLL_Result::EXT_DLL_Result_Error;
		}

		try
		{
			rxID = StringToHex32(rxIDString);
			txID = StringToHex32(txIDString);
		}
		catch (const char* exp) 
		{
			const char* temp = exp;
			copyReturnString("ERROR: Driver could not be initialized, you introduced an invalid rxID or txID! Consider using numbers and chars from 0 to F.", returnString);
			return EXT_DLL_Result::EXT_DLL_Result_Error;
		}

		result = InitDriver(channelIndex, rxID, txID, extendedCANID, baudRate);


		//You can add different return messages based on the result with a switch *not all are implemented*
		switch (result)
		{
		case OK:
			copyReturnString("Driver initialized!", returnString);
			return EXT_DLL_Result::EXT_DLL_Result_OK;

		case CHANNEL_BITRATE_COULD_NOT_BE_SET:
			copyReturnString("ERROR: Driver could not be initialized, channel bitrate could not be set, make sure the channel is not in use!", returnString);
			return EXT_DLL_Result::EXT_DLL_Result_Error;

		case PORT_COULD_NOT_BE_OPENED:
			copyReturnString("ERROR: Driver could not be initialized, verify the channel index!", returnString);
			return EXT_DLL_Result::EXT_DLL_Result_Error;

		default:
			copyReturnString("ERROR: Driver could not be initialized, parameters are invalid!", returnString);
			return EXT_DLL_Result::EXT_DLL_Result_Error;
		}

	}
	else if (strcmp(functionName, "DisableDriver") == 0)
	{ 
	   int8_t result = DisableDriver();

	   switch (result)
	   {
	   case OK:
		   copyReturnString("Driver disabled!", returnString);
		   return EXT_DLL_Result::EXT_DLL_Result_OK;

	   case CHANNEL_COULD_NOT_BE_DEACTIVATED:
		   copyReturnString("ERROR: Driver could not be disabled, channel is invalid!", returnString);
		   return EXT_DLL_Result::EXT_DLL_Result_Error;

	   case PORT_COULD_NOT_BE_CLOSED:
		   copyReturnString("ERROR: Driver could not be disabled, port is invalid!", returnString);
		   return EXT_DLL_Result::EXT_DLL_Result_Error;

	   default:
		   copyReturnString("ERROR: Driver could not be disabled, default!", returnString);
		   return EXT_DLL_Result::EXT_DLL_Result_Error;
	   }
	}
	else
	{
		copyReturnString("Function does not exist!", returnString);
		return EXT_DLL_Result::EXT_DLL_Result_Error;
	}

	/*
	* !!!DO NOT REMOVE!!!
	* Function should not reach this state
	* Secure the return of the function in case programmer messes with the if block above
	*/
	copyReturnString("Default return state reached, please chack the wrapper DLL return paths!", returnString);
	return EXT_DLL_Result::EXT_DLL_Result_Error;
}
