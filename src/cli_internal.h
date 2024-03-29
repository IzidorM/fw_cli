/*
 * SPDX-FileCopyrightText: 2024 Izidor Makuc <izidor@makuc.info>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef CLI_INTERNAL_H
#define CLI_INTERNAL_H

#include <inttypes.h>
#include <stdbool.h>

#include "cli.h"

#ifdef UNIT_TESTS
#define STATIC
#else
#define STATIC static
#endif

#ifndef CLI_COMMAND_BUFF_SIZE
#define CLI_COMMAND_BUFF_SIZE 32
#endif

struct cli;
struct cli_user {
	struct cli_user *next;	
	struct cli *cli;
	char *name;
	bool (*password_check)(char *d);
        struct cli_cmd *cmd_list;
	char *prompt;
};

struct cli_cmd {
        struct cli_cmd *next;
        const char *command_name;
        const char *command_description;
        void (*command_function)(struct cli *cli, 
				 char *command_input_string);
};

struct cli {

#ifdef ENABLE_AUTOMATIC_LOGOUT
	uint32_t logout_time_ms;
	uint32_t logout_timer_ms;
#endif

        void *(*malloc)(size_t size);
        bool (*get_char)(char *);
        void (*send_char)(char c);
#ifdef ENABLE_OS_SUPPORT
	void (*sleep_or_yield)(void);
#endif	
	struct cli_user users;
	struct cli_user *current_user;

        struct cli_cmd common_cmd_list;
        size_t input_buff_index;

        char input_end_char;

        bool special_sequence;
	uint8_t ssb_index;
	//char special_sequence_buff[2];

#ifdef ENABLE_ARGUMENT_PARSER
	uint8_t argc;
#endif

#if defined(ENABLE_HISTORY_V1) || defined(ENABLE_HISTORY_V2)
        char previous_cmd[CLI_COMMAND_BUFF_SIZE];
#endif
        char input_buff[CLI_COMMAND_BUFF_SIZE];
};

#ifdef ENABLE_AUTOMATIC_LOGOUT

STATIC void cli_logout_handler(struct cli *cli, 
			       uint32_t time_from_last_run_ms);
STATIC void cli_reset_logout_timer(struct cli *cli);

#endif //automatic logout

#ifdef ENABLE_ARGUMENT_PARSER
STATIC void cli_argumument_parser_reset(struct cli *cli);
char *cli_argumument_parser_get_next(struct cli *cli, uint32_t argn);
#endif

#if defined(ENABLE_HISTORY_V1) || defined(ENABLE_HISTORY_V2)
STATIC void cli_put_cmd_on_prompt(struct cli *cli, const char *name);
STATIC void cli_history_save_cmd(struct cli *cli, char *input);
#endif // history common

#if (defined(ENABLE_HISTORY_V1)	      \
     || defined(ENABLE_HISTORY_V2))	\
	&& defined(ENABLE_USER_MANAGEMENT)
STATIC void cli_history_forget(struct cli *cli);
#endif


#if defined(ENABLE_HISTORY_V1)
STATIC bool cli_history_handler_input_v1(struct cli *cli);
#endif // history v1
#if defined(ENABLE_HISTORY_V2)
STATIC bool cli_special_sequence_handler(struct cli *cli, char c);
#endif // history v2

#ifdef ENABLE_USER_MANAGEMENT
bool cli_user_add_cmd(struct cli_user *user, 
		      struct cli_cmd_settings cs);
struct cli_user *cli_add_user(struct cli *cli, 
			      struct cli_user_settings us);
STATIC void su_cmd(struct cli *cli, char *s);
STATIC void cli_change_current_user(struct cli *cli, 
				    struct cli_user *user);
#endif // user management

#ifdef ENABLE_AUTOCOMPLETE
STATIC void cli_autocomplete(struct cli *cli);
#endif


#ifdef UNIT_TESTS
void echo_string(struct cli *cli, const char *s);
bool delete_last_echoed_char(struct cli *cli);
struct cli_cmd *cli_search_command(struct cli *cli, 
				   char *cmd_name,
				   bool match_unfinished_cmds,
				   uint32_t search_after_index,
				   uint32_t *found_at_index,
				   uint32_t *name_match_cnt);


char *cli_handle_new_character(struct cli *cli, char c, bool hide);

void cli_command_received_handler(struct cli *cli, char *input);
void echo_input_end_sequence(struct cli *cli);
#endif

#endif
