#ifndef MANAGESTRING_H
#define MANAGESTRING_H

#include "../includeClass.h"

class ManageString
{
public:
    ManageString();

    static std::vector<std::string> splitString(const std::string& buffer, char del);

    static std::string lstrip(const std::string& buffer);

};

#endif // MANAGESTRING_H
