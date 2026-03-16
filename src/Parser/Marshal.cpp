#include "Marshal.h"

Marshal::Marshal() {}

Request Marshal::unmarshal(std::string &buffer)
{
    Request req;

    int row = 0;
    // std::cout << buffer << std::endl;

    std::stringstream ss(buffer);

    // Temporary object to store the splitted string
    std::string substring;

    // Delimiter
    char del = '\n';

    // Split string by delimiter
    while (std::getline(ss, substring, del))
    {
        row+=1;

        // Split on ' '
        std::stringstream ss_substring(substring);

        std::string sub_substring;

        char subDel = ' ';

        std::vector<std::string> substrings;

        while (std::getline(ss_substring, sub_substring, subDel))
        {
            substrings.push_back(sub_substring);
        }

        if (row==1)
        {
            req.setMethod(substrings[0]);
            req.setPath(substrings[1]);
            req.setVersion(substrings[2]);
        }

        if (row==2)
        {
            req.setHost(substrings[1]);
        }

        if (row==3)
        {
            std::string user_agent = substrings[1]+", "+substrings[3]+", "+substrings[4];
            req.setUserAgent(user_agent);
        }

    }

    return req;
}
