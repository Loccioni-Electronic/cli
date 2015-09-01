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

#include "cli.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CLI_BUFFER_SIZE              50
#define CLI_MAX_EXTERNAL_MODULE      20
#define CLI_MAX_CHARS_PER_LINE       60
#define CLI_MAX_CMD_CHAR_LINE        15

static char Cli_buffer[CLI_BUFFER_SIZE];
static uint8_t Cli_bufferIndex = 0;

//static void Cli_functionHelp(void* cmd, char* params);
//static void Cli_functionVersion(void* cmd, char* params);
//static void Cli_functionStatus(void* cmd, char* params);

//static void Cli_functionHelp(int argc, char* argv[]);
//static void Cli_functionVersion(int argc, char* argv[]);
//static void Cli_functionStatus(int argc, char* argv[]);

static void Cli_functionHelp(void* device, int argc, char* argv[]);
static void Cli_functionVersion(void* device, int argc, char* argv[]);
static void Cli_functionStatus(void* device, int argc, char* argv[]);

static void Cli_parseParamsString();

const Cli_Command Cli_commandTable[] =
{
    {"help"   , "Print commands list", 0, Cli_functionHelp},
    {"version", "Print actual version of board and firmware", 0, Cli_functionVersion},
    {"status" , "Print microcontroller status", 0, Cli_functionStatus},
};

static Cli_Command Cli_externalCommandTable[CLI_MAX_EXTERNAL_MODULE];
static uint8_t Cli_externalCommandIndex = 0;

#define CLI_COMMAND_TABLE_SIZED         (sizeof Cli_commandTable / sizeof Cli_commandTable[0])

static Uart_Config Cli_uartConfig = {

    .rxPin = LOCCIONI_CLI_RX_PIN,
    .txPin = LOCCIONI_CLI_TX_PIN,

    .clockSource = UART_CLOCKSOURCE_BUS,

    .dataBits = UART_DATABITS_EIGHT,
    .parity = UART_PARITY_NONE,

    .baudrate = LOCCIONI_CLI_BAUDRATE,

    .oversampling = 16,
};


static void Cli_getCommand (char* name, Cli_Command* cmdFound)
{
    Cli_Command cmd = {name, NULL, NULL};
    uint8_t i;

    for (i = 0; i < CLI_COMMAND_TABLE_SIZED; i++)
    {
        if(strncmp(name, Cli_commandTable[i].name, strlen(Cli_commandTable[i].name)) == 0)
        {
            cmdFound->name        = Cli_commandTable[i].name;
            cmdFound->description = Cli_commandTable[i].description;
            cmdFound->cmdFunction = Cli_commandTable[i].cmdFunction;
            cmdFound->device      = Cli_commandTable[i].device;

//            cmdFound->params = Cli_commandTable[i].params;
            return;
        }
    }

    for (i = 0; i < Cli_externalCommandIndex; i++)
    {
        if(strncmp(name, Cli_externalCommandTable[i].name, strlen(Cli_externalCommandTable[i].name)) == 0)
        {
            cmdFound->name        = Cli_externalCommandTable[i].name;
            cmdFound->description = Cli_externalCommandTable[i].description;
            cmdFound->cmdFunction = Cli_externalCommandTable[i].cmdFunction;
            cmdFound->device      = Cli_externalCommandTable[i].device;

//            cmdFound->params = Cli_externalCommandTable[i].params;
            return;
        }
    }
    /* If we don't find any command, return null. */
    cmdFound = 0;
}

static void Cli_prompt(void)
{
    Uart_sendString(LOCCIONI_CLI_DEV, "\r\n$> ");
    memset(Cli_buffer, 0, sizeof(Cli_buffer));
    Cli_bufferIndex = 0;
}


static void Cli_sayHello (void)
{
    uint8_t i = 0;

    Uart_sendString(LOCCIONI_CLI_DEV, "\r\n");
    for (i=0; i<CLI_MAX_CHARS_PER_LINE; ++i) Uart_putChar(LOCCIONI_CLI_DEV,'*');
    Uart_sendString(LOCCIONI_CLI_DEV, "\r\n");
    Uart_sendStringln(LOCCIONI_CLI_DEV, PROJECT_NAME);
    Uart_sendStringln(LOCCIONI_CLI_DEV, PROJECT_COPYRIGTH);
    for (i=0; i<CLI_MAX_CHARS_PER_LINE; ++i) Uart_putChar(LOCCIONI_CLI_DEV,'*');
    Uart_sendString(LOCCIONI_CLI_DEV, "\r\n");
    Cli_functionVersion(0,0,0);
    for (i=0; i<CLI_MAX_CHARS_PER_LINE; ++i) Uart_putChar(LOCCIONI_CLI_DEV,'*');
    Uart_sendString(LOCCIONI_CLI_DEV, "\r\n");
}

//static void Cli_functionHelp(void* cmd, char* params)
static void Cli_functionHelp(void* device, int argc, char* argv[])
{
    uint8_t i,j;
    uint8_t blank = 0;

    Cli_sayHello();

    for (i = 0; i < CLI_COMMAND_TABLE_SIZED; i++)
    {
        blank = CLI_MAX_CMD_CHAR_LINE - strlen(Cli_commandTable[i].name);
        Uart_sendString(LOCCIONI_CLI_DEV,Cli_commandTable[i].name);
        for (j=0; j < blank; ++j) Uart_putChar(LOCCIONI_CLI_DEV,' ');
        Uart_putChar(LOCCIONI_CLI_DEV,';');
        Uart_sendStringln(LOCCIONI_CLI_DEV,Cli_commandTable[i].description);
    }

    for (i = 0; i < Cli_externalCommandIndex; i++)
    {
        blank = CLI_MAX_CMD_CHAR_LINE - strlen(Cli_externalCommandTable[i].name);
        Uart_sendString(LOCCIONI_CLI_DEV,Cli_externalCommandTable[i].name);
        for (j=0; j < blank; ++j) Uart_putChar(LOCCIONI_CLI_DEV,' ');
        Uart_putChar(LOCCIONI_CLI_DEV,';');
        Uart_sendStringln(LOCCIONI_CLI_DEV,Cli_externalCommandTable[i].description);
    }
}

//static void Cli_functionVersion(void* cmd, char* params)
static void Cli_functionVersion(void* device, int argc, char* argv[])
{
    Uart_sendString(LOCCIONI_CLI_DEV,   "Board Version    : ");
    Uart_sendStringln(LOCCIONI_CLI_DEV, PCB_VERSION_STRING);
    Uart_sendString(LOCCIONI_CLI_DEV,   "Firmware Version : ");
    Uart_sendStringln(LOCCIONI_CLI_DEV, FW_VERSION_STRING);
//    Uart_sendString(LOCCIONI_CLI_DEV,   "Firmware Date    : ");
//    Uart_sendStringln(LOCCIONI_CLI_DEV, FW_TIME_VERSION);
}

//static void Cli_functionStatus(void* cmd, char* params)
static void Cli_functionStatus(void* device, int argc, char* argv[])
{

/* TODO */
}

void Cli_check(void)
{
    Cli_Command cmd = {NULL, NULL, NULL, NULL};
    char c;

    if (Uart_isCharPresent(LOCCIONI_CLI_DEV))
    {
    	Uart_getChar(LOCCIONI_CLI_DEV, &c);
    	Cli_buffer[Cli_bufferIndex++] = c;
    }

	if ((Cli_bufferIndex != 0) &&
	    (Cli_buffer[Cli_bufferIndex-2] == '\r') && (Cli_buffer[Cli_bufferIndex-1] == '\n'))
    {
		/* No message, only enter command! */
		if (Cli_bufferIndex == 2)
		{
			Cli_bufferIndex = 0;
	        Cli_prompt();
	        return;
		}

		Uart_sendString(LOCCIONI_CLI_DEV, "\r\n");
		Cli_getCommand(Cli_buffer,&cmd);

		if (cmd.name != NULL)
		{
		    //          cmd.cmdFunction(&cmd, Cli_buffer + strlen(cmd.name));
		    cmd.cmdFunction(cmd.device,0,0);
		}
		else
		{
	        Uart_sendString(LOCCIONI_CLI_DEV, "Command not found!");
		}

		Cli_bufferIndex = 0;
        Cli_prompt();
	}
	else if(Cli_bufferIndex > CLI_BUFFER_SIZE-1)
	{
	    Cli_bufferIndex = 0;
		Cli_prompt();
	}
}

void Cli_init(void)
{
    Uart_open (LOCCIONI_CLI_DEV, 0, &Cli_uartConfig);

    Cli_sayHello();

    Uart_sendStringln(LOCCIONI_CLI_DEV, "\r\nCLI ready!");

    Cli_bufferIndex = 0;
    Cli_prompt();
}

void Cli_addModule(char* name,
                   char* description,
                   void* device,
                   void (*cmdFunction)(void* device, int argc, char* argv[]))
//void Cli_addModule (char* name, char* description, void (*cmdFunction)(void *cmd, char *cmdLine))
{
    if (Cli_externalCommandIndex < CLI_MAX_EXTERNAL_MODULE)
    {
        Cli_externalCommandTable[Cli_externalCommandIndex].name = name;
        Cli_externalCommandTable[Cli_externalCommandIndex].description = description;

        Cli_externalCommandTable[Cli_externalCommandIndex].device = device;
        Cli_externalCommandTable[Cli_externalCommandIndex].cmdFunction = cmdFunction;
//        Cli_externalCommandTable[Cli_externalCommandIndex].params = "";

        Cli_externalCommandIndex++;
    }
}
