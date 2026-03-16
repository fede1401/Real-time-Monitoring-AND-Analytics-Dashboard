#include "ManageString.h"

ManageString::ManageString() {}

std::vector<std::string> ManageString::splitString(const std::string& buffer, char del)
{
    std::vector<std::string> subStrings;

    std::stringstream ss(buffer);

    std::string subString;

    // Split string by delimiter
    while (std::getline(ss, subString, del))
    {
        subStrings.push_back(subString);
    }

    return subStrings;
}


std::string ManageString::lstrip(const std::string &buffer)
{
    char* result = new char{} ;

    int j = 0;

    for (int i=0; i<buffer.size(); i++)
    {
        auto char_i = static_cast<uint8_t>(buffer[i]);
        if (char_i != 0x20)
        {
            result[j] = buffer[i];
            j++;
        }

    }

    std::string ret(result);

    delete result;

    return ret;
}
