/******************************************************************************
 * Copyright (C) 2015 AEA s.r.l. Loccioni Group - Elctronic Design Dept.
 *
 * Authors:
 *  Marco Giammarini <m.giammarini@loccioni.com>
 *  Alessio Paolucci <a.paolucci89@gmail.com>
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

#ifndef __LOCCIONI_CLI_H
#define __LOCCIONI_CLI_H

/**
 * In this file you must define the UART device information:
 *     #define LOCCIONI_CLI_DEV        UARTx
 *     #define LOCCIONI_CLI_RX_PIN     UART_PINS_PTxx
 *     #define LOCCIONI_CLI_TX_PIN     UART_PINS_PTxx
 *     #define LOCCIONI_CLI_BAUDRATE   115200
 */
#include "board.h"

#define LOCCIONI_CLI_LIBRARY_VERSION     "1.0"
#define LOCCIONI_CLI_LIBRARY_VERSION_M   1
#define LOCCIONI_CLI_LIBRARY_VERSION_m   0
#define LOCCIONI_CLI_LIBRARY_TIME        0

/* Public define */
#define LOCCIONI_CLI_BUFFER_SIZE         50


typedef struct
{
    char *name;
    char *description;
    void *device;
    void (*cmdFunction)(void* device, int argc, char argv[][LOCCIONI_CLI_BUFFER_SIZE]);
} Cli_Command;

void Cli_init(void);
void Cli_check(void);

void Cli_addModule(char* name,
                   char* description,
                   void* device,
                   void (*cmdFunction)(void* device, int argc, char argv[][LOCCIONI_CLI_BUFFER_SIZE]));

void Cli_sendHelpString(char* name, char* description);
void Cli_sendStatusString(char* name, char* value);

#endif /* __CLI_LOCCIONI_H */
