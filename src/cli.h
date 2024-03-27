/*
 * SPDX-FileCopyrightText: 2024 Izidor Makuc <izidor@makuc.info>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef CLI_H
#define CLI_H

#include <inttypes.h>
#include <stdbool.h>

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
	char input_end_char;
	char *prompt_user;
};


char *cli_get_user_input(struct cli *cli, bool hide);

bool cli_add_cmd_common(struct cli *cli, struct cli_cmd_settings cs);
struct cli_user *cli_add_user(struct cli *cli, struct cli_user_settings us);
bool cli_user_add_cmd(struct cli_user *user, struct cli_cmd_settings cs);

struct cli *cli_init(struct cli_settings *s);
uint32_t cli_run(struct cli *cli, uint32_t time_from_last_run_ms);

void cli_report_output_dirty(struct cli *cli);

#endif
