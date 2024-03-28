/*
 * SPDX-FileCopyrightText: 2024 Izidor Makuc <izidor@makuc.info>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef CLI_INTERNAL_H
#define CLI_INTERNAL_H

#include <inttypes.h>
#include <stdbool.h>

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
	uint32_t logout_time;

#ifdef ENABLE_AUTOMATIC_LOGOFF
	uint32_t logoff_timer_ms;
#endif

#ifdef ENABLE_DIRTY_OUTPUT
	uint32_t timer_output_dirty;
#endif

        void *(*malloc)(size_t size);
        bool (*get_char)(char *);
        void (*send_char)(char c);

	struct cli_user users;
	struct cli_user *current_user;

        struct cli_cmd common_cmd_list;
        size_t input_buff_index;

        char input_end_char;

#ifdef ENABLE_DIRTY_OUTPUT
	char output_dirty;
#endif

        bool special_sequence;
	uint8_t ssb_index;
	char special_sequence_buff[2];


#if defined(ENABLE_HISTORY_V1) || defined(ENABLE_HISTORY_V2)
        char previous_cmd[CLI_COMMAND_BUFF_SIZE];
#endif
        char input_buff[CLI_COMMAND_BUFF_SIZE];
};

#ifdef UNIT_TESTS
void echo_string(struct cli *cli, const char *s);
bool delete_last_echoed_char(struct cli *cli);
struct cli_cmd *cli_search_command(struct cli *cli, 
				   char *cmd_name,
				   bool search_for_unfinished_cmds,
				   uint32_t search_from_index,
				   uint32_t *found_at_index,
				   uint32_t *name_match_cnt);

char *cli_handle_new_character(struct cli *cli, char c, bool hide);

void cli_command_received_handler(struct cli *cli, char *input);
void echo_input_end_sequence(struct cli *cli);
#endif

#endif
