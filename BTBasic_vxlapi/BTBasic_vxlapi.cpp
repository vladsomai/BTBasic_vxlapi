#include "lib.h"
#include "vxlapi.h"

char            g_AppName[] = "BTBasic_vxlapi";                            //!< Application name which is displayed in VHWconf
XLportHandle    g_xlPortHandle = XL_INVALID_PORTHANDLE;                    //!< Global porthandle (we use only one!)
XLaccess        g_xlChannelMask = 0;                                       //!< Global channelmask (includes all founded channels)
XLaccess        g_xlPermissionMask = 0;                                    //!< Global permissionmask (includes all founded channels)
XLulong         g_BaudRate = 500000;                                       //!< Default baudrate
uint32_t        g_rxID = 0;
uint32_t        g_txID = 0;

//Rx thread variables
XLhandle        g_hMsgEvent = nullptr;                                      //!< notification handle for the receive queue
HANDLE          g_hRXThread = nullptr;                                      //!< thread handle (RX)
bool            g_stopRXThread = false;                                     //!< flag to stop the rx thread(only when exiting the dll)

XLstatus CreateRxThread();

//!< Set the received message by the RXThread -> to be returned to the BTBasic.
std::unique_ptr<std::vector<std::string>> receivedBuffer = std::make_unique<std::vector<std::string>>();
extern uint8_t StringToHex(std::string input);//from "utility.cpp"

int8_t InitDriver(uint32_t channelIndexParam, 
                  uint32_t rxID_Param, 
                  uint32_t txID_Param, 
                  uint8_t extendedCANID_Param, 
                  uint32_t baudRateParam )
{
    XLstatus xlStatus = XL_ERROR;

    g_rxID = rxID_Param;
    g_txID = txID_Param;

    //set channel index to desired value based on hardware config.
    const XLuint64 one = 1; 
    g_xlChannelMask = one << channelIndexParam;  //Rule for channelMask = 1 << channelIndex.

    if (extendedCANID_Param)
    {
        g_rxID |= XL_CAN_EXT_MSG_ID;
        g_txID |= XL_CAN_EXT_MSG_ID;
    }

    if (baudRateParam != 0)
        g_BaudRate = baudRateParam;


    //================Driver Init================
    xlStatus = xlOpenDriver();
    if (xlStatus != XL_SUCCESS)
        return DRIVER_COULD_NOT_BE_OPENED;

    g_xlPermissionMask = g_xlChannelMask;

    xlStatus = xlOpenPort(&g_xlPortHandle, g_AppName, g_xlChannelMask, &g_xlPermissionMask, RX_QUEUE_SIZE, XL_INTERFACE_VERSION, XL_BUS_TYPE_CAN);
    if (xlStatus != XL_SUCCESS)
        return PORT_COULD_NOT_BE_OPENED;


    //================Channel Setup================
    //The app must have init access(first app to touch the channel) in order for this function to succeed.
    xlStatus = xlCanSetChannelBitrate(g_xlPortHandle, g_xlChannelMask, g_BaudRate);
    if (xlStatus != XL_SUCCESS)
    {
#if _DEBUG
std::cout << xlGetErrorString(xlStatus) << std::endl;
#endif
    }
    else
    {
       // return CHANNEL_BITRATE_COULD_NOT_BE_SET;
    }
 
    xlStatus = CreateRxThread();
    if (xlStatus != XL_SUCCESS)
        return RX_THREAD_COULD_NOT_BE_CREATED;

    xlStatus = xlActivateChannel(g_xlPortHandle, g_xlChannelMask, XL_BUS_TYPE_CAN, XL_ACTIVATE_RESET_CLOCK);
    if (xlStatus != XL_SUCCESS)
        return CHANNEL_COULD_NOT_BE_ACTIVATED;

    return OK;
}

int8_t DisableDriver()
{
    XLstatus xlStatus = XL_ERROR;

    g_stopRXThread = true;
    std::this_thread::sleep_for(std::chrono::milliseconds(300));//give time to rx thread to exit
    
    xlStatus = xlDeactivateChannel(g_xlPortHandle, g_xlChannelMask);
    if (xlStatus != XL_SUCCESS)
        return CHANNEL_COULD_NOT_BE_DEACTIVATED;

    if (g_xlPortHandle != XL_INVALID_PORTHANDLE) 
    {
        xlStatus = xlClosePort(g_xlPortHandle);    
        if (xlStatus != XL_SUCCESS)
            return PORT_COULD_NOT_BE_CLOSED;

        g_xlPortHandle = XL_INVALID_PORTHANDLE;
    }

    xlStatus = xlCloseDriver();
    if (xlStatus != XL_SUCCESS)
        return DRIVER_COULD_NOT_BE_CLOSED;

    return OK;
}

int8_t loadToolbox(const char* toolboxPath)
{
    if (!std::filesystem::exists(toolboxPath))
        return TOOLBOX_FILE_DOES_NOT_EXIST;

    std::ifstream toolboxFile;
    std::streampos end;

    std::unique_ptr<unsigned char[]> spToToolboxBytes;
    std::unique_ptr<XLevent[]> spToEvents;

    toolboxFile.open(toolboxPath, std::ios::binary);

    if (!toolboxFile.is_open())
    {
        return FILE_COULD_NOT_BE_OPENED;
    }

    toolboxFile.seekg(0, std::ios::end);
    end = toolboxFile.tellg();

    const size_t FileSize = static_cast<size_t>(end);

    spToToolboxBytes = std::make_unique<unsigned char[]>(FileSize);

    toolboxFile.seekg(0, std::ios::beg);

    toolboxFile.read(reinterpret_cast<char*>(spToToolboxBytes.get()), FileSize);//cast unsigned char* to char* - does not modify the data
    toolboxFile.close();

    //little endian toolbox (LSB first)
    unsigned char MSB = spToToolboxBytes.get()[1];
    unsigned char LSB = spToToolboxBytes.get()[0];
    unsigned short ToolboxLength = ((MSB << 8) | LSB) + 2;

    size_t EightByteMessages = ToolboxLength / 8; //How many messages to be sent with 8 byte length
    size_t VariableByteMessage = ToolboxLength - EightByteMessages * 8; //Message to be sent with variable length
    size_t messagesSentInChunks = 0;
    size_t ByteIndex = 0;
    const size_t MESSAGECOUNT = 500;

    spToEvents = std::make_unique<XLevent[]>(MESSAGECOUNT);
    memset(spToEvents.get(), 0, sizeof(XLevent) * MESSAGECOUNT);//set all the allocated memory to 0x00 (requirement from Vector)

    /*For speed purpose the toolbox will be loaded in 3 parts
     *Part 1: Messages will be sent in chunks of 500
     *Part 2: Messages will be sent in 8 byte length
     *Part 3: The last message is sent with variable length
     */
    
    XLstatus xlStatus = XL_ERROR;
    //Part 1
    for (size_t i = 0; i < EightByteMessages / MESSAGECOUNT; i++)
    {
        unsigned int  messageCount = MESSAGECOUNT;

        for (size_t messageIndex = 0; messageIndex < MESSAGECOUNT; messageIndex++)
        {
            spToEvents.get()[messageIndex].tag = XL_TRANSMIT_MSG;
            spToEvents.get()[messageIndex].tagData.msg.id = g_txID;
            spToEvents.get()[messageIndex].tagData.msg.dlc = 8;
            spToEvents.get()[messageIndex].tagData.msg.flags = 0;

            for (size_t ji = 0; ji < 8; ji++)
            {
                spToEvents.get()[messageIndex].tagData.msg.data[ji] = spToToolboxBytes[ByteIndex];
                ByteIndex++;
            }
        }

        xlStatus = xlCanTransmit(g_xlPortHandle, g_xlChannelMask, &messageCount, spToEvents.get());
        std::this_thread::sleep_for(std::chrono::milliseconds(300));//mandatory

        messagesSentInChunks++;
    }

    //Part 2
    for (size_t messageIndex = messagesSentInChunks * MESSAGECOUNT; messageIndex < EightByteMessages; messageIndex++)
    {
        unsigned int  messageCount = 1;
        XLevent       xlEvent;

        memset(&xlEvent, 0, sizeof(xlEvent));

        xlEvent.tag = XL_TRANSMIT_MSG;
        xlEvent.tagData.msg.id = g_txID;
        xlEvent.tagData.msg.dlc = 8;
        xlEvent.tagData.msg.flags = 0;

        for (size_t ji = 0; ji < 8; ji++)
        {
            xlEvent.tagData.msg.data[ji] = spToToolboxBytes[ByteIndex];
            ByteIndex++;
        }
        xlStatus = xlCanTransmit(g_xlPortHandle, g_xlChannelMask, &messageCount, &xlEvent);
    }

    //Part 3
    {
        unsigned int  messageCount = 1;
        XLevent       xlEvent;

        memset(&xlEvent, 0, sizeof(xlEvent));

        xlEvent.tag = XL_TRANSMIT_MSG;
        xlEvent.tagData.msg.id = g_txID;
        xlEvent.tagData.msg.dlc = static_cast<unsigned short>(VariableByteMessage);
        xlEvent.tagData.msg.flags = 0;

        for (unsigned short ji = 0; ji < VariableByteMessage; ji++)
        {
            xlEvent.tagData.msg.data[ji] = spToToolboxBytes[ByteIndex];
            ByteIndex++;
        }
        xlStatus = xlCanTransmit(g_xlPortHandle, g_xlChannelMask, &messageCount, &xlEvent);
    }

#if _DEBUG
    std::cout << xlGetErrorString(xlStatus) << std::endl;
#endif

    if (xlStatus == XL_SUCCESS)
        return OK;
    else if (xlStatus == XL_ERR_WRONG_PARAMETER)
        return RX_TX_ID_IS_INVALID;
    else
        return MESSAGE_NOT_SENT;

}

int8_t sendData(std::string dataParam)
{
    XLstatus xlStatus = XL_ERROR;
    size_t sizeOfData = dataParam.length();

    size_t sizeOfMessage = sizeOfData / 2; //no. of bytes

    if (!(sizeOfData % 2 == 0 && sizeOfMessage < 100))//check if the telegram contains corect byte length(should be even number and less than 100 bytes)
    {
        return DATA_IS_INVALID;
    }
    size_t EightByteMessages = sizeOfMessage / 8; //number of messages to be sent with 8 byte length
    unsigned short VariableByteMessage = static_cast<unsigned short>(sizeOfMessage - EightByteMessages * 8); //Message to be sent with variable length (last x bytes)

    std::vector<std::string> stringDataBytes;
    for (size_t i = 0; i < sizeOfData; i += 2)
    {
        stringDataBytes.push_back(dataParam.substr(i, 2));
    }

    unsigned char dataBytes[100] = { 0 };//Max allowed bytes to be sent
    size_t ByteIndex = 0;

    //Convert all the bytes from the std::vector to hex-> e.g. "FA" -> 0xFA
    size_t i = 0;
    for (const auto& stringDataByte : stringDataBytes)
    {
        dataBytes[i] = StringToHex(stringDataByte);
        i++;
    }

    /*A CAN message is sent in 2 parts
    *Part 1: Messages will be sent in chunks of 8 bytes
    *Part 2: The last message is sent with variable length
    */

    //Part 1
    for (size_t messageIndex = 0; messageIndex < EightByteMessages; messageIndex++)
    {
        unsigned int  messageCount = 1;
        XLevent       xlEvent;

        memset(&xlEvent, 0, sizeof(xlEvent));

        xlEvent.tag = XL_TRANSMIT_MSG;
        xlEvent.tagData.msg.id = g_txID;
        xlEvent.tagData.msg.dlc = 8;
        xlEvent.tagData.msg.flags = 0;

        for (size_t ji = 0; ji < 8; ji++)
        {
            xlEvent.tagData.msg.data[ji] = dataBytes[ByteIndex];
            ByteIndex++;
        }
        xlStatus = xlCanTransmit(g_xlPortHandle, g_xlChannelMask, &messageCount, &xlEvent);
    }

    //Part 2
    {
        unsigned int  messageCount = 1;
        XLevent       xlEvent;

        memset(&xlEvent, 0, sizeof(xlEvent));

        xlEvent.tag = XL_TRANSMIT_MSG;
        xlEvent.tagData.msg.id = g_txID;
        xlEvent.tagData.msg.dlc = VariableByteMessage;
        xlEvent.tagData.msg.flags = 0;

        for (unsigned short ji = 0; ji < VariableByteMessage; ji++)
        {
            xlEvent.tagData.msg.data[ji] = dataBytes[ByteIndex];
            ByteIndex++;
        }
        xlStatus = xlCanTransmit(g_xlPortHandle, g_xlChannelMask, &messageCount, &xlEvent);
    }

#if _DEBUG
    std::cout <<"SEND DATA FUNCTION REACHED THE END, CURRENT STATUS: "<< xlGetErrorString(xlStatus) << std::endl;
#endif

    if (xlStatus == XL_SUCCESS)
        return OK;
    else if (xlStatus == XL_ERR_WRONG_PARAMETER)
        return RX_TX_ID_IS_INVALID;
    else
        return MESSAGE_NOT_SENT;
}

DWORD WINAPI RxThread(LPVOID par)
{
    XLstatus xlStatus = XL_ERROR;
    uint32_t msgsrx = RECEIVE_EVENT_SIZE;
    XLevent xlEvent{};
    DWORD code = 0;

    while (true) {
       
        //Wait for the CAN Rx to receive a message
        WaitForSingleObject(g_hMsgEvent, 50);

        xlStatus = XL_SUCCESS;

        while (!xlStatus) {
            msgsrx = RECEIVE_EVENT_SIZE;

            xlStatus = xlReceive(g_xlPortHandle, &msgsrx, &xlEvent);
            if (xlStatus != XL_ERR_QUEUE_IS_EMPTY) {
                if (xlEvent.tagData.msg.id == g_rxID)
                {
                    std::string message = std::string(xlGetEventString(&xlEvent));
                    message = message.substr(message.rfind(',') + 2);
                    message = message.substr(0, message.find(' '));
                    receivedBuffer->push_back(message);
                }
            }
        }

        //stop the rx thread when the flag is set(the flag is set only when closing the driver)
        //the thread must be terminated before unloading the DLL, otherwise host application will crash
        if (g_stopRXThread)
        {
            ExitThread(code);
        }
    }
    return NO_ERROR;
}

XLstatus CreateRxThread()
{
    XLstatus      xlStatus = XL_ERROR;
    DWORD         ThreadId = 0;

    if (g_xlPortHandle != XL_INVALID_PORTHANDLE) {

        // Send an event for each Msg!!!
        xlStatus = xlSetNotification(g_xlPortHandle, &g_hMsgEvent, 1);

        g_hRXThread = CreateThread(0, 0x1000, RxThread, (LPVOID)0, 0, &ThreadId);
    }
    return xlStatus;
}

int8_t FHost(std::string FHostPath, std::string PRG_Path)
{
    std::string temp = FHostPath.substr(0, FHostPath.find_last_of("\\\\"));
    std::string pathToResult = temp + "\\FHostSP_Result.txt";
    std::string command = FHostPath + " /auto /prg=\"" + PRG_Path+"\"";

    if (!std::filesystem::exists(FHostPath.c_str()))
        return FHOST_FILE_DOES_NOT_EXIST;

    if (!std::filesystem::exists(PRG_Path.c_str()))
        return PRG_FILE_DOES_NOT_EXIST;

    //execute the command
    if (system(command.c_str()) == -1)
    {
        return ERROR_EXECUTING_FHOST_COMMAND;
    }

    std::ifstream FHostResultFile;
    FHostResultFile.open(pathToResult, std::ios::in);

    if (!FHostResultFile.is_open())
    {
        return FILE_COULD_NOT_BE_OPENED;
    }

    std::string line;
    std::vector<std::string> resultLines;
    while (std::getline(FHostResultFile, line))
    {
        resultLines.push_back(line);
    }
    FHostResultFile.close();

    std::string lastLine = resultLines.at(resultLines.size() - 1); //get the last line from FHost result file
    if (lastLine.find("SUCCESFULLY") != std::string::npos)
        return OK;
    else
        return FHOST_FINISHED_WITH_ERROR;
}

