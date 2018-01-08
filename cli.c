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

#include "cli.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CLI_MAX_EXTERNAL_COMMAND     50
#define CLI_MAX_EXTERNAL_MODULE      20
#define CLI_MAX_CHARS_PER_LINE       80
#define CLI_MAX_CMD_CHAR_LINE        30
#define CLI_MAX_STATUS_CHAR_LINE     10
#define CLI_MAX_PARAM                10

#define CLI_BOARD_STRING             "Board"
#define CLI_FIRMWARE_STRING          "Firmware"


// Usefull define for standard command
char* Cli_wrongCmd      = "ERR: Unrecognized command";
char* Cli_wrongParam    = "ERR: Wrong parameters";
char* Cli_doneCmd       = "Command done!";
char* Cli_notConfigMode = "ERR: You are not in configuration mode!";

static char Cli_buffer[LOCCIONI_CLI_BUFFER_SIZE];
static uint8_t Cli_bufferIndex = 0;

static char Cli_params[CLI_MAX_PARAM][LOCCIONI_CLI_BUFFER_SIZE];
static uint8_t Cli_numberOfParams = 0;

static char Cli_statusBuffer[LOCCIONI_CLI_BUFFER_SIZE];

/**
 * This variable is used to store current CLI status: TRUE when configuration mode is
 * selected, FALSE for application mode.
 */
static bool Cli_configMode = FALSE;

static void Cli_functionHelp(void* device, int argc, char argv[][LOCCIONI_CLI_BUFFER_SIZE]);
static void Cli_functionVersion(void* device, int argc, char argv[][LOCCIONI_CLI_BUFFER_SIZE]);
static void Cli_functionStatus(void* device, int argc, char argv[][LOCCIONI_CLI_BUFFER_SIZE]);
static void Cli_networkConfiguration (void* device, int argc, char argv[][LOCCIONI_CLI_BUFFER_SIZE]);
static void Cli_saveFlash (void* device, int argc, char argv[][LOCCIONI_CLI_BUFFER_SIZE]);
static void Cli_reboot (void* device, int argc, char argv[][LOCCIONI_CLI_BUFFER_SIZE]);

typedef struct
{
    char *name;
    char *description;
    void *device;
    void (*cmdFunction)(void* device, int argc, char argv[][LOCCIONI_CLI_BUFFER_SIZE]);
} Cli_Command;

const Cli_Command Cli_commandTable[] =
{
    {"help"      , "Print commands list", 0, Cli_functionHelp},
    {"version"   , "Print actual version of board and firmware", 0, Cli_functionVersion},
    {"status"    , "Print microcontroller status", 0, Cli_functionStatus},
#if LOCCIONI_CLI_ETHERNET == 1
    {"netconfig" , "Set/Get network configurations", 0, Cli_networkConfiguration},
#endif
    {"save"      , "Save parameters into flash memory", 0, Cli_saveFlash},
    {"reboot"    , "Reboot system", 0, Cli_reboot},
};

static Cli_Command Cli_externalCommandTable[CLI_MAX_EXTERNAL_COMMAND];
static uint8_t Cli_externalCommandIndex = 0;

static Cli_Command Cli_externalModuleTable[CLI_MAX_EXTERNAL_MODULE];
static uint8_t Cli_externalModuleIndex = 0;

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

#if defined (LIBOHIBOARD_K64F12)     || \
    defined (LIBOHIBOARD_KV31F12)

    .callbackTx = 0,
    .callbackRx = 0,

#endif
};

static void Cli_getCommand (char* name, Cli_Command* cmdFound)
{
    Cli_Command cmd = {name, NULL, NULL};
    uint8_t i;

    for (i = 0; i < CLI_COMMAND_TABLE_SIZED; i++)
    {
        if (strncmp(name, Cli_commandTable[i].name, strlen(Cli_commandTable[i].name)) == 0)
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
        if (strncmp(name, Cli_externalCommandTable[i].name, strlen(Cli_externalCommandTable[i].name)) == 0)
        {
            cmdFound->name        = Cli_externalCommandTable[i].name;
            cmdFound->description = Cli_externalCommandTable[i].description;
            cmdFound->cmdFunction = Cli_externalCommandTable[i].cmdFunction;
            cmdFound->device      = 0;
            return;
        }
    }

    for (i = 0; i < Cli_externalModuleIndex; i++)
    {
        if (strncmp(name, Cli_externalModuleTable[i].name, strlen(Cli_externalModuleTable[i].name)) == 0)
        {
            cmdFound->name        = Cli_externalModuleTable[i].name;
            cmdFound->description = Cli_externalModuleTable[i].description;
            cmdFound->cmdFunction = Cli_externalModuleTable[i].cmdFunction;
            cmdFound->device      = Cli_externalModuleTable[i].device;
            return;
        }
    }
    /* If we don't find any command, return null. */
    cmdFound = 0;
}

static void Cli_prompt (void)
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

static void Cli_functionHelp (void* device, int argc, char argv[][LOCCIONI_CLI_BUFFER_SIZE])
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

    // Print external command
    for (i = 0; i < Cli_externalCommandIndex; i++)
    {
        blank = CLI_MAX_CMD_CHAR_LINE - strlen(Cli_externalCommandTable[i].name);
        Uart_sendString(LOCCIONI_CLI_DEV,Cli_externalCommandTable[i].name);
        for (j=0; j < blank; ++j) Uart_putChar(LOCCIONI_CLI_DEV,' ');
        Uart_putChar(LOCCIONI_CLI_DEV,';');
        Uart_sendStringln(LOCCIONI_CLI_DEV,Cli_externalCommandTable[i].description);
    }

    // Print external module and sub command
    for (i = 0; i < Cli_externalModuleIndex; i++)
    {
        blank = CLI_MAX_CMD_CHAR_LINE - strlen(Cli_externalModuleTable[i].name);
        Uart_sendString(LOCCIONI_CLI_DEV,Cli_externalModuleTable[i].name);
        for (j=0; j < blank; ++j) Uart_putChar(LOCCIONI_CLI_DEV,' ');
        Uart_putChar(LOCCIONI_CLI_DEV,';');
        Uart_sendStringln(LOCCIONI_CLI_DEV,Cli_externalModuleTable[i].description);

        // Print help menu of the module!
        Cli_externalModuleTable[i].cmdFunction(Cli_externalModuleTable[i].device,1,0);
    }
}

static void Cli_functionVersion (void* device, int argc, char argv[][LOCCIONI_CLI_BUFFER_SIZE])
{
    uint8_t i,blank = 0;
    char dateString[26];

    /* Board version */
    blank = CLI_MAX_STATUS_CHAR_LINE - strlen(CLI_BOARD_STRING);
    Uart_sendString(LOCCIONI_CLI_DEV,CLI_BOARD_STRING);
    for (i=0; i < blank; ++i) Uart_putChar(LOCCIONI_CLI_DEV,' ');
    Uart_putChar(LOCCIONI_CLI_DEV,':');
    Uart_putChar(LOCCIONI_CLI_DEV,' ');
    Uart_sendStringln(LOCCIONI_CLI_DEV, PCB_VERSION_STRING);

    /* Firmware version */
    Time_unixtimeToString(FW_TIME_VERSION,dateString);
    blank = CLI_MAX_STATUS_CHAR_LINE - strlen(CLI_FIRMWARE_STRING);
    Uart_sendString(LOCCIONI_CLI_DEV,CLI_FIRMWARE_STRING);
    for (i=0; i < blank; ++i) Uart_putChar(LOCCIONI_CLI_DEV,' ');
    Uart_putChar(LOCCIONI_CLI_DEV,':');
    Uart_putChar(LOCCIONI_CLI_DEV,' ');
    Uart_sendString(LOCCIONI_CLI_DEV, FW_VERSION_STRING);
    Uart_sendString(LOCCIONI_CLI_DEV, " of ");
    Uart_sendStringln(LOCCIONI_CLI_DEV,dateString);
}

static void Cli_functionStatus (void* device, int argc, char argv[][LOCCIONI_CLI_BUFFER_SIZE])
{
    uint8_t i;

    for (i=0; i<CLI_MAX_CHARS_PER_LINE; ++i) Uart_putChar(LOCCIONI_CLI_DEV,'*');
    Uart_sendString(LOCCIONI_CLI_DEV, "\r\n");
    Uart_sendStringln(LOCCIONI_CLI_DEV, "System Status");
    for (i=0; i<CLI_MAX_CHARS_PER_LINE; ++i) Uart_putChar(LOCCIONI_CLI_DEV,'*');
    Uart_sendString(LOCCIONI_CLI_DEV, "\r\n");

    Cli_functionVersion(0,0,0);
}

#if LOCCIONI_CLI_ETHERNET == 1

static uint8_t* Cli_ipAddress      = 0;
static uint8_t* Cli_gatewayAddress = 0;
static uint8_t* Cli_maskAddress    = 0;
static uint8_t* Cli_macAddress     = 0;

void Cli_setNetworkMemoryArray (uint8_t* ip, uint8_t* mask, uint8_t* gw, uint8_t* mac)
{
    Cli_ipAddress = ip;
    Cli_gatewayAddress = gw;
    Cli_maskAddress = mask;
    Cli_macAddress = mac;
}

static void Cli_networkConfiguration (void* device, int argc, char argv[][LOCCIONI_CLI_BUFFER_SIZE])
{
    uint8_t tmp[6];

    if (argc == 1)
    {
        if (Cli_configMode)
        {
            Cli_sendHelpString("ip|gw|mask x.x.x.x","Set ip|gw|mask address");
            Cli_sendHelpString("mac y:y:y:y:y:y","Set mac address");
        }
        Cli_sendHelpString("show","Show network configuration");
        return;
    }

    if ((argc == 2) && (strcmp(argv[1], "show") == 0))
    {
        sprintf(Cli_statusBuffer,
                "%d.%d.%d.%d",
                Cli_ipAddress[0],
                Cli_ipAddress[1],
                Cli_ipAddress[2],
                Cli_ipAddress[3]);
        Cli_sendStatusString("ip",Cli_statusBuffer,0);

        sprintf(Cli_statusBuffer,
                "%d.%d.%d.%d",
                Cli_gatewayAddress[0],
                Cli_gatewayAddress[1],
                Cli_gatewayAddress[2],
                Cli_gatewayAddress[3]);
        Cli_sendStatusString("gateway",Cli_statusBuffer,0);

        sprintf(Cli_statusBuffer,
                "%d.%d.%d.%d",
                Cli_maskAddress[0],
                Cli_maskAddress[1],
                Cli_maskAddress[2],
                Cli_maskAddress[3]);
        Cli_sendStatusString("mask",Cli_statusBuffer,0);

        sprintf(Cli_statusBuffer,
                "%02X:%02X:%02X:%02X:%02X:%02X",
                Cli_macAddress[0],
                Cli_macAddress[1],
                Cli_macAddress[2],
                Cli_macAddress[3],
                Cli_macAddress[4],
                Cli_macAddress[5]);
        Cli_sendStatusString("mac",Cli_statusBuffer,0);
        return;
    }

    // The network can be configured only in debug mode
    if ((argc == 3) && (Cli_configMode))
    {
        if (strcmp(argv[1], "ip") == 0)
        {
            if (Utility_isValidIp4Address(argv[2]))
            {
                sscanf(argv[2],
                       "%d.%d.%d.%d",
                       &tmp[0],
                       &tmp[1],
                       &tmp[2],
                       &tmp[3]);

                for (uint8_t i = 0; i < 4; ++i) Cli_ipAddress[i] = tmp[i];

                LOCCIONI_CLI_DONECMD();
                return;
            }
            else
            {
                LOCCIONI_CLI_WRONGPARAM();
                return;
            }
        }

        if (strcmp(argv[1], "gw") == 0)
        {
            if (Utility_isValidIp4Address(argv[2]))
            {
                sscanf(argv[2],
                       "%d.%d.%d.%d",
                       &tmp[0],
                       &tmp[1],
                       &tmp[2],
                       &tmp[3]);

                for (uint8_t i = 0; i < 4; ++i) Cli_gatewayAddress[i] = tmp[i];

                LOCCIONI_CLI_DONECMD();
                return;
            }
            else
            {
                LOCCIONI_CLI_WRONGPARAM();
                return;
            }
        }

        if (strcmp(argv[1], "mask") == 0)
        {
            if (Utility_isValidIp4Address(argv[2]))
            {
                sscanf(argv[2],
                       "%d.%d.%d.%d",
                       &tmp[0],
                       &tmp[1],
                       &tmp[2],
                       &tmp[3]);

                for (uint8_t i = 0; i < 4; ++i) Cli_maskAddress[i] = tmp[i];

                LOCCIONI_CLI_DONECMD();
                return;
            }
            else
            {
                LOCCIONI_CLI_WRONGPARAM();
                return;
            }
        }

        if (strcmp(argv[1], "mac") == 0)
        {
            if (Utility_isValidMacAddress(argv[2]))
            {
                sscanf(argv[2],
                       "%d:%d:%d:%d:%d:%d",
                       &tmp[0],
                       &tmp[1],
                       &tmp[2],
                       &tmp[3],
                       &tmp[4],
                       &tmp[5]);

                for (uint8_t i = 0; i < 6; ++i) Cli_macAddress[i] = tmp[i];

                LOCCIONI_CLI_DONECMD();
                return;
            }
            else
            {
                LOCCIONI_CLI_WRONGPARAM();
                return;
            }
        }
    }

    if (!Cli_configMode)
    {
        Uart_sendString(LOCCIONI_CLI_DEV,Cli_notConfigMode);
        return;
    }

    LOCCIONI_CLI_WRONGCMD();
}
#endif

static System_Errors (*Cli_saveCallbackFunction)(void) = 0;

void Cli_saveCallback (System_Errors (*saveCallback)(void))
{
    Cli_saveCallbackFunction = saveCallback;
}

static void Cli_saveFlash (void* device, int argc, char argv[][LOCCIONI_CLI_BUFFER_SIZE])
{
    if (argc != 1)
          return;

    if (!Cli_configMode)
    {
        Uart_sendString(LOCCIONI_CLI_DEV,Cli_notConfigMode);
        return;
    }

    Cli_sendString("Saving parameters...");
    Cli_saveCallbackFunction();
    Cli_sendString("Reboot necessary!");
}

static void Cli_reboot (void* device, int argc, char argv[][LOCCIONI_CLI_BUFFER_SIZE])
{
    if (argc != 1)
          return;

    if (!Cli_configMode)
    {
        Uart_sendString(LOCCIONI_CLI_DEV,Cli_notConfigMode);
        return;
    }

    Cli_sendString("Reboot...\r\n");
    NVIC_SystemReset();
}

static void Cli_parseParams (void)
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

void Cli_check (void)
{
    Cli_Command cmd = {NULL, NULL, NULL, NULL};
    char c;

    if (Uart_isCharPresent(LOCCIONI_CLI_DEV))
    {
    	Uart_getChar(LOCCIONI_CLI_DEV, &c);
    	// When buffer is grather then 0, delete one char
    	if ((c == '\b') && (Cli_bufferIndex > 0))
    	{
    	    Cli_bufferIndex--;
    	    return;
    	}
    	// When no chars into buffer, return to main function
    	else if ((c == '\b') && (Cli_bufferIndex == 0))
    	{
    	    return;
    	}

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

void Cli_init (void)
{
    Uart_open (LOCCIONI_CLI_DEV, &Cli_uartConfig);

    Cli_sayHello();

    Uart_sendStringln(LOCCIONI_CLI_DEV, "\r\nCLI ready!");

    Cli_bufferIndex = 0;
    Cli_prompt();
}

void Cli_setConfigMode (bool config)
{
    Cli_configMode = config;
}

bool Cli_isConfigMode (void)
{
    return Cli_configMode;
}

void Cli_addModule (char* name,
                    char* description,
                    void* device,
                    void (*cmdFunction)(void* device, int argc, char argv[][LOCCIONI_CLI_BUFFER_SIZE]))
{
    if (Cli_externalModuleIndex < CLI_MAX_EXTERNAL_MODULE)
    {
        Cli_externalModuleTable[Cli_externalModuleIndex].name = name;
        Cli_externalModuleTable[Cli_externalModuleIndex].description = description;

        Cli_externalModuleTable[Cli_externalModuleIndex].device = device;
        Cli_externalModuleTable[Cli_externalModuleIndex].cmdFunction = cmdFunction;

        Cli_externalModuleIndex++;
    }
}

void Cli_addCommand (char* name,
                     char* description,
                     void (*cmdFunction)(void* device, int argc, char argv[][LOCCIONI_CLI_BUFFER_SIZE]))
{
    if (Cli_externalCommandIndex < CLI_MAX_EXTERNAL_COMMAND)
    {
        Cli_externalCommandTable[Cli_externalCommandIndex].name = name;
        Cli_externalCommandTable[Cli_externalCommandIndex].description = description;

        Cli_externalCommandTable[Cli_externalCommandIndex].device = 0;
        Cli_externalCommandTable[Cli_externalCommandIndex].cmdFunction = cmdFunction;

        Cli_externalCommandIndex++;
    }
}

void Cli_sendHelpString (char* name, char* description)
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

void Cli_sendStatusString (char* name, char* value, char* other)
{
    uint8_t i;
    uint8_t blank;

    blank = CLI_MAX_CMD_CHAR_LINE - strlen(name);
    Uart_sendString(LOCCIONI_CLI_DEV,name);
    for (i=0; i < blank; ++i) Uart_putChar(LOCCIONI_CLI_DEV,' ');
    Uart_putChar(LOCCIONI_CLI_DEV,':');
    Uart_putChar(LOCCIONI_CLI_DEV,' ');

    if (other)
    {
        Uart_sendString(LOCCIONI_CLI_DEV,value);
        Uart_putChar(LOCCIONI_CLI_DEV,' ');
        Uart_sendStringln(LOCCIONI_CLI_DEV,other);
    }
    else
    {
        Uart_sendStringln(LOCCIONI_CLI_DEV,value);
    }
}

void Cli_sendString (char* text)
{
    Uart_sendStringln(LOCCIONI_CLI_DEV,text);
}

void Cli_sendMessage (char* who, char* message, Cli_MessageType type)
{
    switch (type)
    {
    case CLI_MESSAGETYPE_INFO:
        Uart_sendString(LOCCIONI_CLI_DEV,"INFO: ");
        break;
    case CLI_MESSAGETYPE_WARNING:
        Uart_sendString(LOCCIONI_CLI_DEV,"WARNING: ");
        break;
    case CLI_MESSAGETYPE_ERROR:
        Uart_sendString(LOCCIONI_CLI_DEV,"ERROR: ");
        break;
    }

    Uart_sendString(LOCCIONI_CLI_DEV,who);
    Uart_sendString(LOCCIONI_CLI_DEV,"> ");
    Uart_sendStringln(LOCCIONI_CLI_DEV,message);
}
