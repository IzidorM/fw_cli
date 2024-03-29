/*
 * SPDX-FileCopyrightText: 2024 Izidor Makuc <izidor@makuc.info>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef CLI_H
#define CLI_H

#include <inttypes.h>
#include <stdbool.h>

// By defining following defines additional features can be enabled
// For more info about the modules read the README.md page on github

//#define ENABLE_USER_MANAGEMENT
// Enable multiple users and su command to switch between them

//#define ENABLE_USER_INPUT_REQUEST
// enable option to request user input in user command function
// this is an alternative to command arguments. Instead of adding
// arguments after command, command can ask for user input interactively

// #define ENABLE_AUTOMATIC_LOGOUT
// Logs out current user after 

//#define ENABLE_ARGUMENT_PARSER
// support for parsing command arguments

// #define ENABLE_HISTORY_V1
// Get last issued command when pressing the enter key on empty prompt

// #define ENABLE_HISTORY_V2
// Get last issued command when pressing up arrow

// #define ENABLE_OS_SUPPORT
// cli will use yield/sleep in blocking functions

// #define ENABLE_AUTOCOMPLETE
// tab will print all available commands starting with the current user
// input. If only one command will match user input, 
// it will be autocompleted

#if defined(ENABLE_USER_MANAGEMENT) && !defined(ENABLE_USER_INPUT_REQUEST)
#define ENABLE_USER_INPUT_REQUEST
#endif

struct cli;
struct cli_user;

struct cli_user_settings {
	char *name;
	bool (*password_check)(char *d);
	char *prompt;
};

struct cli_cmd_settings {
        const char *command_name;
        const char *command_description;
        void (*command_function)(struct cli *cli, 
				 char *command_input_string);
};

struct cli_settings {
        void *(*my_malloc)(size_t size);
        bool (*get_char)(char *);
        void (*send_char)(char c);
#ifdef ENABLE_OS_SUPPORT
	void (*sleep_or_yield)(void);
#endif	
	char input_end_char;
	char *prompt_user;

#ifdef ENABLE_AUTOMATIC_LOGOUT
	uint32_t logout_time_ms;
#endif
};

char *cli_get_user_input(struct cli *cli, bool hide);

bool cli_add_cmd_common(struct cli *cli, struct cli_cmd_settings cs);
struct cli_user *cli_add_user(struct cli *cli, struct cli_user_settings us);
bool cli_user_add_cmd(struct cli_user *user, struct cli_cmd_settings cs);

struct cli *cli_init(struct cli_settings *s);
uint32_t cli_run(struct cli *cli, uint32_t time_from_last_run_ms);

#ifdef ENABLE_ARGUMENT_PARSER
uint32_t cli_argument_parser_get_argc(struct cli *cli);
char *cli_argumument_parser_get_next(struct cli *cli, uint32_t argn);
#endif
#endif
