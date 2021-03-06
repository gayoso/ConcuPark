#ifndef COLA_H
#define COLA_H

#include "Runnable.h"
#include <queue>
#include <unistd.h>
#include <string>

class Cola : public Runnable
{
    public:
        Cola(std::string nombre);
        virtual ~Cola();
    protected:

        virtual void _run();

    private:

        std::queue<pid_t> personas;
        std::string nombre;
};

#endif // COLA_H
