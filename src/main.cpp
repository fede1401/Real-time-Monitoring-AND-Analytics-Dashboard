#ifndef MAIN_CPP
#define MAIN_CPP

#include <iostream>
#include <cstdlib>
#include "Server/listener.h"

int main(){
    std::cout << "Listing files:\n";
    system("ls");

    Listener* listener = new Listener("127.0.0.1", 8080, Protocol::TCP);

    listener->receiveData();

    delete listener;
}

#endif
