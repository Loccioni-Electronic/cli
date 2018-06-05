/******************************************************************************
 * Copyright (C) 2015-2018 AEA s.r.l. Loccioni Group - Elctronic Design Dept.
 *
 * Authors:
 *  Marco Giammarini <m.giammarini@loccioni.com>
 *  Alessio Paolucci <a.paolucci89@gmail.com>
 *  Matteo Piersantelli
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 ******************************************************************************/

/******************************************************************************
 * @mainpage CLI interface based on @a libohiboard
 *
 * @section changelog ChangeLog
 *
 * @li v1.5.0 of 2018/01/xx - Added addCommand function, added network
 * configuration, added save and reboot command, added sendMessage function.
 *
 * @section library External Library
 *
 * The library use the following external library
 * @li libohiboard https://github.com/ohilab/libohiboard a C framework for
 * NXP Kinetis microcontroller
 *
 *
 ******************************************************************************/

#ifndef __LOCCIONI_CLI_H
#define __LOCCIONI_CLI_H

/**
 * In this file you must define the UART device information:
 *     #define LOCCIONI_CLI_DEV        UARTx
 *     #define LOCCIONI_CLI_RX_PIN     UART_PINS_PTxx
 *     #define LOCCIONI_CLI_TX_PIN     UART_PINS_PTxx
 *     #define LOCCIONI_CLI_BAUDRATE   115200
 *
 *     #define LOCCIONI_CLI_ETHERNET   1/0
 */
#ifndef __NO_BOARD_H
#include "board.h"
#endif

#define LOCCIONI_CLI_LIBRARY_VERSION     "1.4.0"
#define LOCCIONI_CLI_LIBRARY_VERSION_M   1
#define LOCCIONI_CLI_LIBRARY_VERSION_m   4
#define LOCCIONI_CLI_LIBRARY_VERSION_bug 0
#define LOCCIONI_CLI_LIBRARY_TIME        1497865040

/* Public define */
#ifndef LOCCIONI_CLI_BUFFER_SIZE
#define LOCCIONI_CLI_BUFFER_SIZE         50
#endif

extern char* Cli_wrongCmd;
extern char* Cli_wrongParam;
extern char* Cli_doneCmd;

#define LOCCIONI_CLI_WRONGCMD()           Cli_sendString(Cli_wrongCmd)
#define LOCCIONI_CLI_WRONGPARAM()         Cli_sendString(Cli_wrongParam)
#define LOCCIONI_CLI_DONECMD()            Cli_sendString(Cli_doneCmd)

void Cli_init (void);
void Cli_check (void);

void Cli_addModule (char* name,
                    char* description,
                    void* device,
                    void (*cmdFunction)(void* device, int argc, char argv[][LOCCIONI_CLI_BUFFER_SIZE]));

void Cli_addCommand (char* name,
                     char* description,
                     void (*cmdFunction)(void* device, int argc, char argv[][LOCCIONI_CLI_BUFFER_SIZE]));

void Cli_sendHelpString (char* name, char* description);
void Cli_sendStatusString (char* name, char* value, char* other);
void Cli_sendString (char* text);

typedef enum
{
    CLI_MESSAGETYPE_INFO,
    CLI_MESSAGETYPE_WARNING,
    CLI_MESSAGETYPE_ERROR,
} Cli_MessageType;

/**
 * @param who String that contain the name of message sender
 * @param message String that contains the message
 * @param type The type of the message: INFO, WARNING or ERROR
 */
void Cli_sendMessage (char* who, char* message, Cli_MessageType type);

/**
 *
 * @param saveCallback User callback to save data into memory
 */
void Cli_saveCallback (System_Errors (*saveCallback)(void));

#if LOCCIONI_CLI_ETHERNET == 1
void Cli_setNetworkMemoryArray (uint8_t* ip, uint8_t* mask, uint8_t* gw, uint8_t* mac);
#endif

void Cli_setConfigMode (bool config);
bool Cli_isConfigMode (void);

#endif /* __CLI_LOCCIONI_H */
