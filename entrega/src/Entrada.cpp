#include "Entrada.h"
#include "FifoLectura.h"
#include "FifoEscritura.h"
#include "defines.h"
#include "Logger.h"
#include "SIG_Trap.h"
#include "SignalHandler.h"
#include <time.h>
#include <fstream>

Entrada::Entrada(std::string nom, int cap, pid_t c) : nombre(nom), capacidad(cap), cola_pid(c)
{
    //ctor
}

Entrada::~Entrada()
{
    //dtor
}

void Entrada::_run(){

    // trap de SIGINT
    SIG_Trap sigint_handler(SIGINT);
    SIG_Trap sigalrm_handler(SIGALRM);
    SignalHandler::getInstance()->registrarHandler(SIGINT, &sigint_handler);
    SignalHandler::getInstance()->registrarHandler(SIGALRM, &sigalrm_handler);

    Logger *l = Logger::getInstance();

    Logger::log("ENTRADA", "Creo canal de lectura a cola", DEBUG);
    FifoLectura canal_escuchar_de_cola ( nombre + C_COLA_A_ENTRADA );

    Logger::log("ENTRADA", "Creo canal de escritura a salida", DEBUG);
    FifoEscritura canal_a_salida(nombre + C_SALIDA);

    Logger::log("ENTRADA", "Creo canal para cobrar a personas", DEBUG);
    FifoLectura canal_cobrar_a_persona(nombre + C_COBRAR_A_PERSONA);

    while(sigint_handler.signalWasReceived() == 0){
        pid_t pid_leido;

        // solo arranco si estoy lleno de gente o se acaba el timer
        while(sigint_handler.signalWasReceived() == 0 && personas.size() < capacidad && sigalrm_handler.signalWasReceived() == 0){
            kill(cola_pid, SIGPASARAENTRADA);
            canal_escuchar_de_cola.abrir();
            ssize_t bytes_leidos = canal_escuchar_de_cola.leer(static_cast<void*>(&pid_leido),sizeof(pid_leido));
            if(bytes_leidos > 0){

                /// TODO: le cobro a la persona
                kill(pid_leido, SIGPAGAR);
                canal_cobrar_a_persona.abrir();
                int monto;
                bytes_leidos = canal_cobrar_a_persona.leer(static_cast<void*>(&monto),sizeof(monto));
                std::string intermedio = "Cobro " + std::to_string(monto) + " a " + std::to_string(pid_leido);
                Logger::log("ENTRADA", intermedio, DEBUG);
                // ver si bytes_leidos > 0
                /// TODO: actualizar caja
                canal_cobrar_a_persona.cerrar();
                // la meto al juego
                personas.push_back(pid_leido);
                intermedio = "Entra al juego: " + std::to_string(pid_leido);
                Logger::log("ENTRADA", intermedio, DEBUG);
            }
            canal_escuchar_de_cola.cerrar();

            // desde que llega el primero empiezo el timer
            if(personas.size() == 1){
                alarm(5);
            }
        }

        /// TODO: o me llene de gente (o paso el tiempo ESTO FALTA), va a empezar el juego
         if(!sigint_handler.signalWasReceived()){
            if(sigalrm_handler.signalWasReceived()){
                Logger::log("ENTRADA", "Se acabo el tiempo de espera", DEBUG);
                sigalrm_handler.reset();
            } else if(personas.size() == capacidad){
                Logger::log("ENTRADA", "Se lleno el juego", DEBUG);
            }
            alarm(0); // apago la alarma
            std::string intermedio = "Arranca el juego con: ";
            for(int i = 0; i < personas.size(); ++i){
                intermedio += std::to_string(personas[i]) + " ";
            }
            Logger::log("ENTRADA", intermedio, DEBUG);

            // simulo la duracion del juego
            sleep(10);

            // notificar a la salida con un pipe que se va toda la gente
            Logger::log("ENTRADA", "Salen todos del juego", DEBUG);
            canal_a_salida.abrir();
            for(int i = 0; i < personas.size(); ++i){
                pid_t sale = personas[i];
                canal_a_salida.escribir(static_cast<void*>(&sale), sizeof(sale));
            }
            canal_a_salida.cerrar();
            personas.clear();
        }
    }

    Logger::log("ENTRADA", "Se cierra la entrada", DEBUG);
    canal_a_salida.cerrar();
    canal_escuchar_de_cola.cerrar();
    canal_escuchar_de_cola.eliminar();
    canal_cobrar_a_persona.cerrar();
    canal_cobrar_a_persona.eliminar();
    //SignalHandler::getInstance()->destruir();
}
