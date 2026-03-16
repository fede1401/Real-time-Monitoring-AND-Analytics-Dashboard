#ifndef MARSHAL_H
#define MARSHAL_H

#include "../includeClass.h"
#include "../Model/Request.h"

class Marshal
{

public:

    Marshal();

    Request unmarshal(std::string& buffer);
};

#endif // MARSHAL_H
