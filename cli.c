/******************************************************************************
 * Copyright (C) 2015-2016 AEA s.r.l. Loccioni Group - Elctronic Design Dept.
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

#define CLI_MAX_EXTERNAL_MODULE      20
#define CLI_MAX_CHARS_PER_LINE       60
#define CLI_MAX_CMD_CHAR_LINE        15
#define CLI_MAX_STATUS_CHAR_LINE     10
#define CLI_MAX_PARAM                10

static char Cli_buffer[LOCCIONI_CLI_BUFFER_SIZE];
static uint8_t Cli_bufferIndex = 0;

static char Cli_params[CLI_MAX_PARAM][LOCCIONI_CLI_BUFFER_SIZE];
static uint8_t Cli_numberOfParams = 0;

static void Cli_functionHelp(void* device, int argc, char argv[][LOCCIONI_CLI_BUFFER_SIZE]);
static void Cli_functionVersion(void* device, int argc, char argv[][LOCCIONI_CLI_BUFFER_SIZE]);
static void Cli_functionStatus(void* device, int argc, char argv[][LOCCIONI_CLI_BUFFER_SIZE]);

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

#if defined (LIBOHIBOARD_KL03Z4)     || \
    defined (LIBOHIBOARD_FRDMKL03Z)  || \
	defined (LIBOHIBOARD_KL15Z4)     || \
	defined (LIBOHIBOARD_KL25Z4)     || \
	defined (LIBOHIBOARD_FRDMKL25Z)
    .oversampling = 16,
#endif
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
    memset(Cli_params, 0, sizeof(Cli_params));
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

static void Cli_functionHelp(void* device, int argc, char argv[][LOCCIONI_CLI_BUFFER_SIZE])
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

        /* Print help menu of the device! */
        Cli_externalCommandTable[i].cmdFunction(Cli_externalCommandTable[i].device,1,0);
    }
}

static void Cli_functionVersion(void* device, int argc, char argv[][LOCCIONI_CLI_BUFFER_SIZE])
{
    uint8_t i,blank = 0;
    char dateString[26];

    /* Board version */
    blank = CLI_MAX_STATUS_CHAR_LINE - strlen("Board");
    Uart_sendString(LOCCIONI_CLI_DEV,"Board");
    for (i=0; i < blank; ++i) Uart_putChar(LOCCIONI_CLI_DEV,' ');
    Uart_putChar(LOCCIONI_CLI_DEV,':');
    Uart_putChar(LOCCIONI_CLI_DEV,' ');
    Uart_sendStringln(LOCCIONI_CLI_DEV, PCB_VERSION_STRING);

    /* Firmware version */
    Time_unixtimeToString(FW_TIME_VERSION,dateString);
    blank = CLI_MAX_STATUS_CHAR_LINE - strlen("Firmware");
    Uart_sendString(LOCCIONI_CLI_DEV,"Firmware");
    for (i=0; i < blank; ++i) Uart_putChar(LOCCIONI_CLI_DEV,' ');
    Uart_putChar(LOCCIONI_CLI_DEV,':');
    Uart_putChar(LOCCIONI_CLI_DEV,' ');
    Uart_sendString(LOCCIONI_CLI_DEV, FW_VERSION_STRING);
    Uart_sendString(LOCCIONI_CLI_DEV, " of ");
    Uart_sendStringln(LOCCIONI_CLI_DEV,dateString);
}

static void Cli_functionStatus(void* device, int argc, char argv[][LOCCIONI_CLI_BUFFER_SIZE])
{
    uint8_t i;

    for (i=0; i<CLI_MAX_CHARS_PER_LINE; ++i) Uart_putChar(LOCCIONI_CLI_DEV,'*');
    Uart_sendString(LOCCIONI_CLI_DEV, "\r\n");
    Uart_sendStringln(LOCCIONI_CLI_DEV, "System Status");
    for (i=0; i<CLI_MAX_CHARS_PER_LINE; ++i) Uart_putChar(LOCCIONI_CLI_DEV,'*');
    Uart_sendString(LOCCIONI_CLI_DEV, "\r\n");

    Cli_functionVersion(0,0,0);
}

static void Cli_parseParams(void)
{
    uint8_t i = 0; /* counter for the buffer */
    uint8_t j = 0; /* counter for each param */
    bool isStringOpen = FALSE; /* to know if double quote is for open or close */
    Cli_numberOfParams = 0;


    for (i = 0; i < (Cli_bufferIndex -2); ++i)
    {
        if ((Cli_buffer[i] != ' ') && (Cli_buffer[i] != '\"'))
        {
            Cli_params[Cli_numberOfParams][j++] = Cli_buffer[i];
        }
        else if ((Cli_buffer[i] == '\"') && (isStringOpen == FALSE))
        {
            isStringOpen = TRUE;
        }
        else if ((Cli_buffer[i] == '\"') && (isStringOpen == TRUE))
        {
            isStringOpen = FALSE;
            Cli_params[Cli_numberOfParams][j] = '\0';
            j = 0; /* Reset param counter */
            Cli_numberOfParams++; /* update number of params */
        }
        else if ((Cli_buffer[i] == ' ') && (isStringOpen == TRUE))
        {
            Cli_params[Cli_numberOfParams][j++] = Cli_buffer[i];
        }
        else if (((Cli_buffer[i] == ' ') && (Cli_buffer[i-1] == ' ')) ||
                 ((Cli_buffer[i] == ' ') && (Cli_buffer[i-1] == '\"')))
        {
            continue;
        }
        else
        {
            Cli_params[Cli_numberOfParams][j] = '\0';
            j = 0; /* Reset param counter */
            Cli_numberOfParams++; /* update number of params */
        }
    }
    Cli_numberOfParams++; /* The last param that can not see! */
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
		    Cli_parseParams();
		    cmd.cmdFunction(cmd.device,Cli_numberOfParams,Cli_params);
		}
		else
		{
	        Uart_sendString(LOCCIONI_CLI_DEV, "Command not found!");
		}

		Cli_bufferIndex = 0;
        Cli_prompt();
	}
	else if(Cli_bufferIndex > LOCCIONI_CLI_BUFFER_SIZE-1)
	{
	    Cli_bufferIndex = 0;
		Cli_prompt();
	}
}

void Cli_init(void)
{
#if defined (LIBOHIBOARD_K12D5)      || \
    defined (LIBOHIBOARD_K64F12)     || \
    defined (LIBOHIBOARD_FRDMK64F)
    Uart_open (LOCCIONI_CLI_DEV, &Cli_uartConfig);
#else
    Uart_open (LOCCIONI_CLI_DEV, 0, &Cli_uartConfig);
#endif

    Cli_sayHello();

    Uart_sendStringln(LOCCIONI_CLI_DEV, "\r\nCLI ready!");

    Cli_bufferIndex = 0;
    Cli_prompt();
}

void Cli_addModule(char* name,
                   char* description,
                   void* device,
                   void (*cmdFunction)(void* device, int argc, char argv[][LOCCIONI_CLI_BUFFER_SIZE]))
{
    if (Cli_externalCommandIndex < CLI_MAX_EXTERNAL_MODULE)
    {
        Cli_externalCommandTable[Cli_externalCommandIndex].name = name;
        Cli_externalCommandTable[Cli_externalCommandIndex].description = description;

        Cli_externalCommandTable[Cli_externalCommandIndex].device = device;
        Cli_externalCommandTable[Cli_externalCommandIndex].cmdFunction = cmdFunction;

        Cli_externalCommandIndex++;
    }
}

void Cli_sendHelpString(char* name, char* description)
{
    uint8_t i;
    uint8_t blank;

    blank = CLI_MAX_CMD_CHAR_LINE - strlen(name) - 2;
    Uart_sendString(LOCCIONI_CLI_DEV,"  "); /* Blank space before command */
    Uart_sendString(LOCCIONI_CLI_DEV,name);
    for (i=0; i < blank; ++i) Uart_putChar(LOCCIONI_CLI_DEV,' ');
    Uart_putChar(LOCCIONI_CLI_DEV,';');
    Uart_sendStringln(LOCCIONI_CLI_DEV,description);
}
