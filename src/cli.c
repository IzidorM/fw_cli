/*
 * SPDX-FileCopyrightText: 2024 Izidor Makuc <izidor@makuc.info>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "cli_internal.h"
#include "cli.h"

#ifdef UNIT_TESTS
#define STATIC
#else
#define STATIC static
#endif

STATIC void echo_string(struct cli *cli, const char *s)
{
        for (uint32_t i = 0; s[i] != '\0'; i++)
        {
                cli->send_char(s[i]);
        }

#ifdef ENABLE_DIRTY_OUTPUT
	cli->output_dirty = false;
#endif
}

STATIC void echo_input_end_sequence(struct cli *cli)
{
	echo_string(cli, "\r\n");
}

STATIC void delete_last_echoed_char(struct cli *cli)
{
	if (cli->input_buff_index)
	{
		cli->input_buff_index -= 1;
		echo_string(cli, "\b \b");
	}
}

STATIC struct cli_cmd *cli_search_command(struct cli *cli, 
					  char *cmd_name)
{

        struct cli_cmd *tmp[2] = {&cli->common_cmd_list,
		cli->current_user->cmd_list};

	for (uint32_t i = 0; 2 > i; i++)
	{
		for (; NULL != tmp[i]; tmp[i] = tmp[i]->next)
		{
			if (0 == strcmp(tmp[i]->command_name, cmd_name))
			{
				return tmp[i];
			}
		}
	}
        return NULL;
}

STATIC void cli_command_received_handler(struct cli *cli, char *input)
{
	struct cli_cmd *tmp_command = 
		cli_search_command(cli, input);

	if (tmp_command)
	{
#ifdef ENABLE_HISTORY_V1
		strncpy(cli->previous_cmd, input, 
			CLI_COMMAND_BUFF_SIZE);
#endif
		tmp_command->command_function(cli, input);
	}
}

STATIC char *cli_handle_new_character(struct cli *cli, char c,
	bool hide_echo)
{
	char *ret = NULL;

        if (cli->input_end_char == c)
	{
		cli->input_buff[cli->input_buff_index] = '\0';
#ifdef ENABLE_HISTORY_V1
		if ((0 == cli->input_buff_index) && (0 != cli->previous_cmd[0]))
		{
			echo_string(cli, cli->previous_cmd);
			strncpy(cli->input_buff, 
				cli->previous_cmd,
				CLI_COMMAND_BUFF_SIZE);
			cli->input_buff_index = strlen(
				cli->previous_cmd);

			ret = NULL;
		}
		else
#endif
		{
			ret = cli->input_buff;
			echo_input_end_sequence(cli);
			cli->input_buff_index = 0;
		}

		//cli->special_sequence = false;
		//cli->ssb_index = 0;
        }
        //else if(0x1b == c && (0 == cli->input_buff_index)) //special sequence start
	//{
	//	cli->special_sequence = true;
	//}
	//else if (cli->special_sequence)
	//{
	//	cli->special_sequence_buff[cli->ssb_index] = c;
	//	cli->ssb_index += 1;
	//
	//	if (2 == cli->ssb_index)
	//	{
	//		cli->special_sequence = false;
	//		cli->ssb_index = 0;
	//		if ((0x5b == cli->special_sequence_buff[0])
	//		    && (0x41 == cli->special_sequence_buff[1]))
	//		{
	//			echo_string(cli, cli->input_buff);
	//		}
	//	}
	//}
        else if( '\b' == c) //backspace
        {
		delete_last_echoed_char(cli);
        }
        else if( '\r' == c) //drop character
        {
                
        }
        else if ((cli->input_buff_index + 1) < CLI_COMMAND_BUFF_SIZE)
        {
                cli->input_buff[cli->input_buff_index] = c;
                cli->input_buff_index += 1;

		if (hide_echo)
		{
			cli->send_char('*');
		}
		else
		{
			cli->send_char(c);
		}
        }
	return ret;
}

uint32_t cli_run(struct cli *cli, uint32_t time_from_last_run_ms)
{
	(void) time_from_last_run_ms;

#ifdef ENABLE_DIRTY_OUTPUT
	if (cli->output_dirty)
	{
		cli->timer_output_dirty += time_from_last_run_ms;
	
		if (1000 < cli->timer_output_dirty)
		{
			cli->timer_output_dirty = 0;
			cli->output_dirty = 0;
	
			cli_command_received_handler(cli, "");
			echo_string(cli, cli->current_user->prompt);
		}
	}
#endif

#ifdef ENABLE_AUTOMATIC_LOGOFF
	cli->logoff_timer += time_from_last_run_ms;

	if (30000 < cli->logoff_timer)
	{
		cli->current_user = &cli->users;
		echo_string(cli, cli->current_user->prompt);
		cli->logoff_timer = 0;
#ifdef ENABLE_HISTORY_V1
		cli->previous_cmd[0] = 0;
#endif
	}
#endif

	char c;
	while(cli->get_char(&c))
	{
#ifdef ENABLE_AUTOMATIC_LOGOFF
		cli->logoff_timer = 0;
#endif
		bool hide_echo = false;
		char *input_received = 
			cli_handle_new_character(cli, c, hide_echo);
		if (input_received)
		{
			cli_command_received_handler(cli, 
						     input_received);
			echo_string(cli, cli->current_user->prompt);
		}
	} 

	return 0;
}

bool cli_add_cmd_common(struct cli *cli, struct cli_cmd_settings cs)
{
	if ((NULL == cli))
	{
		return false;
	}

	// tmp cant be NULL by design (there is always at least 
	// help command registered there...
        struct cli_cmd *tmp = &cli->common_cmd_list;
        
        for (; NULL != tmp->next; tmp = tmp->next);

        tmp->next = cli->malloc(sizeof(struct cli_cmd));

	tmp = tmp->next;
	tmp->next = NULL;
	tmp->command_name = cs.command_name;
	tmp->command_description = cs.command_description;
	tmp->command_function = cs.command_function;

	return true;
}

bool cli_user_add_cmd(struct cli_user *user, struct cli_cmd_settings cs)
{
	if ((NULL == user))
	{
		return false;
	}

	struct cli_cmd *tmp = user->cmd_list;
	if (NULL == user->cmd_list)
	{
		user->cmd_list = user->cli->malloc(sizeof(struct cli_cmd));
		tmp = user->cmd_list;
		tmp->next = NULL;
	}
	else
	{
		for (; NULL != tmp->next; tmp = tmp->next);
		tmp->next = user->cli->malloc(sizeof(struct cli_cmd));
		tmp = tmp->next;
	}

	tmp->next = NULL;
	tmp->command_name = cs.command_name;
	tmp->command_description = cs.command_description;
	tmp->command_function = cs.command_function;

	return true;
}

struct cli_user *cli_add_user(struct cli *cli, struct cli_user_settings us)
{
	if ((NULL == cli))
	{
		return NULL;
	}

	//tmp cant be NULL by design, because there is always guest user
        struct cli_user *tmp = &cli->users;
	
        for (; NULL != tmp->next; tmp = tmp->next);

	tmp->next = cli->malloc(sizeof(struct cli_user));
	tmp = tmp->next;

	tmp->next = NULL;
        tmp->cli = cli;
	tmp->name = us.name;
	tmp->password_check = us.password_check;
	tmp->cmd_list = NULL;
	tmp->prompt = us.prompt;

	return tmp;
}

STATIC void help_function(struct cli *cli, char *s)
{
        (void) s;
        struct cli_cmd *tmp[2] = {&cli->common_cmd_list,
		cli->current_user->cmd_list};

	for (uint32_t i = 0; 2 > i; i++)
	{
		for (; NULL != tmp[i]; tmp[i] = tmp[i]->next)
		{
			echo_string(cli, tmp[i]->command_name);
			echo_string(cli, "\r\n");
			if (tmp[i]->command_description)
			{
				echo_string(cli, "\t");
				echo_string(cli, 
					    tmp[i]->command_description);
				echo_string(cli, "\r\n");
			}
		}
	}
}

//// log in as root
STATIC void su(struct cli *cli, char *s)
{
        (void) s;

	echo_string(cli, "user: ");
	char *u = cli_get_user_input(cli, false);

        struct cli_user *tmp = &cli->users;
        for (; NULL != tmp; tmp = tmp->next)
	{
		if (0 == strcmp(tmp->name, u))
		{
			bool select_user = true;
			if (tmp->password_check)
			{
				echo_string(cli, "pass: ");
				if (!tmp->password_check(
					    cli_get_user_input(
						    cli, true)))
				{
					select_user = false;
				}
			}

			if (select_user)
			{
//				echo_string(cli, "selecting user: ");
//				echo_string(cli, tmp->name);
//				echo_string(cli, "\n");
				cli->current_user = tmp;
			}
			return;
		}
	}

	echo_string(cli, "User not found\n");
}

char *cli_get_user_input(struct cli *cli, bool hide)
{
	char c;
	for(;;)
	{
		if (cli->get_char(&c))
		{
			char *input_received = 
				cli_handle_new_character(cli, c, hide);
			if (input_received)
			{
				return input_received;
			}
		}
	} 
	return NULL;
}

struct cli *cli_init(struct cli_settings *s)
{
	if (NULL == s->my_malloc
	    || NULL == s->get_char
	    || NULL == s->send_char)
	{
		return NULL;
	}
	
	struct cli *tmp = s->my_malloc(sizeof(struct cli));

	tmp->malloc = s->my_malloc;
	tmp->get_char = s->get_char;
	tmp->send_char = s->send_char;
	tmp->input_buff_index = 0;
	tmp->common_cmd_list.next = NULL;
	tmp->common_cmd_list.command_name = "help";
	tmp->common_cmd_list.command_description = 
		"print out all the commands";
	tmp->common_cmd_list.command_function = help_function;
	tmp->input_end_char = s->input_end_char;

	tmp->users.next = NULL;
	tmp->users.cli = tmp;
	tmp->users.password_check = NULL;
	tmp->users.cmd_list = NULL;
	tmp->users.prompt = s->prompt_user;
	tmp->users.name = "guest";

	tmp->current_user = &tmp->users;

	// TODO: Check return code of adding SU command
	cli_add_cmd_common(tmp, (struct cli_cmd_settings) 
			   {
				   .command_name = "su",
				   .command_function = su,
				   .command_description = "select user"
			   });

	return tmp;
}


void cli_report_output_dirty(struct cli *cli)
{
	(void) cli;

#ifdef ENABLE_DIRTY_OUTPUT
	cli->output_dirty = true;
	cli->timer_output_dirty = 0;
#endif
}

