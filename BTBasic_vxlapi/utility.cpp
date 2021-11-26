#include "lib.h"
/*
* e.g. Input: "FF" => Output: 0xFF
*/
uint8_t StringToHex(std::string input)
{
    uint8_t Byte = 0;

    for (size_t i = 0; i < 2; i++)
    {
        switch (input[i])
        {
        case '0':
            if (i == 0)
                Byte = 0x0 << 4;
            else
                Byte |= 0x0;
            break;

        case '1':
            if (i == 0)
                Byte = 0x1 << 4;
            else
                Byte |= 0x1;
            break;

        case '2':
            if (i == 0)
                Byte = 0x2 << 4;
            else
                Byte |= 0x2;
            break;

        case '3':
            if (i == 0)
                Byte = 0x3 << 4;
            else
                Byte |= 0x3;
            break;

        case '4':
            if (i == 0)
                Byte = 0x4 << 4;
            else
                Byte |= 0x4;
            break;

        case '5':
            if (i == 0)
                Byte = 0x5 << 4;
            else
                Byte |= 0x5;
            break;

        case '6':
            if (i == 0)
                Byte = 0x6 << 4;
            else
                Byte |= 0x6;
            break;

        case '7':
            if (i == 0)
                Byte = 0x7 << 4;
            else
                Byte |= 0x7;
            break;

        case '8':
            if (i == 0)
                Byte = 0x8 << 4;
            else
                Byte |= 0x8;
            break;

        case '9':
            if (i == 0)
                Byte = 0x9 << 4;
            else
                Byte |= 0x9;
            break;

        case 'A':
            if (i == 0)
                Byte = 0xA << 4;
            else
                Byte |= 0xA;
            break;

        case 'B':
            if (i == 0)
                Byte = 0xB << 4;
            else
                Byte |= 0xB;
            break;

        case 'C':
            if (i == 0)
                Byte = 0xC << 4;
            else
                Byte |= 0xC;
            break;

        case 'D':
            if (i == 0)
                Byte = 0xD << 4;
            else
                Byte |= 0xD;
            break;

        case 'E':
            if (i == 0)
                Byte = 0xE << 4;
            else
                Byte |= 0xE;
            break;

        case 'F':
            if (i == 0)
                Byte = 0xF << 4;
            else
                Byte |= 0xF;
            break;

        default:
            throw "Invalid argument received";
        }
    }
    return Byte;
}

void copyReturnString(const char* input, char* returnString)
{
    size_t sizeOfMessage = strlen(input) + 1;
    strcpy_s(returnString, sizeOfMessage, input);
}

std::vector<std::string> parse_C_style_str(char* input)
{
    bool stringIsParsed = false;
    std::vector<std::string> result;
    std::string inputStr = input;

    if (inputStr.empty())
    {
        return result;
    }

    std::string next = inputStr;
    std::string temp;
    size_t indexOfCurrentComma = 0;
    while (!stringIsParsed)
    {
        indexOfCurrentComma = next.find(",");

        if (indexOfCurrentComma == std::string::npos)
        {
            result.push_back(next);
            stringIsParsed = true;
        }
        else
        {
            temp = next.substr(0, indexOfCurrentComma);
            result.push_back(temp);

            next = next.substr(indexOfCurrentComma + 1, inputStr.size());
        }
    }
    return result;
}

/*
* input: "1BFCDA10"
* output: 0x1BFCDA10
*/
uint32_t StringToHex32(std::string input)
{

    /*
    * In case user inserts "F" -> insert "0" before "F" until size is 6 
    * Input will be transformed from "F" to 0x0000000F
    */
    size_t inputLength = input.length();
    std::string zeroChar = "0";
    while (inputLength != 8)
    {
        input.insert(0,zeroChar);
        inputLength++;
    }


    std::vector<std::string> stringDataBytes;
    for (size_t i = 0; i <= 6; i += 2)
    {
        stringDataBytes.push_back(input.substr(i, 2));
    }

    //Convert all the bytes from the std::vector to hex-> e.g. "FA" -> 0xFA
    uint32_t result_MSB = StringToHex(stringDataBytes.at(0))<<24;
    uint32_t result_16_24 = StringToHex(stringDataBytes.at(1))<<16;
    uint32_t result_8_16 = StringToHex(stringDataBytes.at(2))<<8;
    uint32_t resultLSB = StringToHex(stringDataBytes.at(3));

    uint32_t result = result_MSB | result_16_24 | result_8_16 | resultLSB;

    return result;
}