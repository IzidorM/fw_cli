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

// this is defined just so the code is more understandable
// in some places :)
#define GET_GUEST_USER(cli) (&cli->users)

STATIC void echo_string(struct cli *cli, const char *s)
{
        for (uint32_t i = 0; s[i] != '\0'; i++)
        {
                cli->send_char(s[i]);
        }
}

STATIC void echo_input_end_sequence(struct cli *cli)
{
	echo_string(cli, "\r\n");
}

STATIC bool delete_last_echoed_char(struct cli *cli)
{
	if (cli->input_buff_index)
	{
		cli->input_buff_index -= 1;
		echo_string(cli, "\b \b");
		return true;
	}
	return false;
}

STATIC bool cli_check_command_name_match(struct cli_cmd *cmd,
					 char *cmd_name,
					 bool match_unfinished_cmds)
{
	size_t cmd_size = strlen(cmd_name);
	if (0 == memcmp(cmd->command_name, 
			cmd_name, cmd_size))
	{
		if (match_unfinished_cmds)
		{
			return true;
		}
		else
		{
			if (0 == cmd->command_name[cmd_size])
			{
				return true;
			}
		}
	}
	return false;
}

STATIC struct cli_cmd *cli_search_command(struct cli *cli, 
					  char *cmd_name,
					  bool match_unfinished_cmds,
					  uint32_t search_after_index,
					  uint32_t *found_at_index,
					  uint32_t *name_match_cnt)
{
	struct cli_cmd *r = NULL;
	uint32_t cmd_counter = 0;

	//put user and common cmd list in an array
        struct cli_cmd *tmp[2] = {
		&cli->common_cmd_list,
		cli->current_user->cmd_list};

	//loop over both cmd lists
	for (uint32_t i = 0; 2 > i; i++)
	{
		for (; NULL != tmp[i]; tmp[i] = tmp[i]->next)
		{
			// skip first N (search_from_index) cmds
			if (search_after_index <= cmd_counter)
			{
				if (cli_check_command_name_match(
					    tmp[i],
					    cmd_name,
					    match_unfinished_cmds))
				{
					r = tmp[i];

					if (found_at_index)
					{
						*found_at_index = 
							cmd_counter;
					}

					if (name_match_cnt)
					{
						*name_match_cnt += 1;
					}
					else
					{
						// goto is the most
						// effective way for 
						// exiting double loop
						goto exit;
					}
				}
			}
			cmd_counter += 1;
		}
	}
exit:
        return r;
}

STATIC void cli_history_save_cmd(struct cli *cli, char *input)
{
#if defined(ENABLE_HISTORY_V1) || defined(ENABLE_HISTORY_V2)
		strncpy(cli->previous_cmd, input, 
			CLI_COMMAND_BUFF_SIZE);
#else
		(void) cli;
		(void) input;
#endif
}

STATIC void cli_history_forget(struct cli *cli)
{
#if defined(ENABLE_HISTORY_V1) || defined(ENABLE_HISTORY_V2)
	cli->previous_cmd[0] = 0;
#else
	(void) cli;
#endif
}

#if defined(ENABLE_HISTORY_V1) || defined(ENABLE_HISTORY_V2)
//STATIC void cli_history_use_last_cmd(struct cli *cli)
STATIC void cli_put_cmd_on_prompt(struct cli *cli, const char *name)
{
	echo_string(cli, name);
	strncpy(cli->input_buff, name,
		CLI_COMMAND_BUFF_SIZE);
	cli->input_buff_index = strlen(
		name);

//	echo_string(cli, cli->previous_cmd);
//	strncpy(cli->input_buff, 
//		cli->previous_cmd,
//		CLI_COMMAND_BUFF_SIZE);
//	cli->input_buff_index = strlen(
//		cli->previous_cmd);

}
#endif

STATIC bool cli_history_handler_input_v1(struct cli *cli)
{
#ifdef ENABLE_HISTORY_V1
	if ((0 == cli->input_buff_index) && (0 != cli->previous_cmd[0]))
	{
		//cli_history_use_last_cmd(cli);
		cli_put_cmd_on_prompt(cli, cli->previous_cmd);
		return true;
	}
	return false;
#else
	(void) cli;
	return false;
#endif
}

STATIC void cli_command_received_handler(struct cli *cli, char *input)
{
	struct cli_cmd *tmp_command = 
		cli_search_command(cli, input, false, 0, NULL, NULL);

	if (tmp_command)
	{
		cli_history_save_cmd(cli, input);
		tmp_command->command_function(cli, input);
	}
}

STATIC bool cli_special_sequence_handler(struct cli *cli, char c)
{
	bool r = false;
        //if(0x1b == c && (0 == cli->input_buff_index)) //special sequence start
	if(0x1b == c)
	{
		cli->special_sequence = true;
		r = true;
	}
	else if (cli->special_sequence)
	{
		//cli->special_sequence_buff[cli->ssb_index] = c;
		cli->ssb_index += 1;
		r = true;
		if (2 == cli->ssb_index)
		{
			cli->special_sequence = false;
			cli->ssb_index = 0;

			while (delete_last_echoed_char(cli));

			// up arrow
			if (//(0x5b == cli->special_sequence_buff[0]) &&
				(0x41 == c))
			{
#ifdef ENABLE_HISTORY_V2
				cli_put_cmd_on_prompt(cli, 
						      cli->previous_cmd);
#endif
			}
			// down arrow 
//			else if ((0x5b == cli->special_sequence_buff[0])
//				 && (0x42 == cli->special_sequence_buff[1]))
//			{
//			        while (delete_last_echoed_char(cli));
//			}
		}
	}

	return r;
}

STATIC void cli_autocomplete(struct cli *cli)
{
	cli->input_buff[cli->input_buff_index] = 0;
	uint32_t cmd_found_at_index = 0;
	uint32_t search_from_index = 0;
	uint32_t cmd_match_cnt = 0;
	
	struct cli_cmd *cmd = cli_search_command(
		cli, 
		cli->input_buff,
		true, 
		search_from_index, 
		NULL,
		&cmd_match_cnt);
	
	if (1 < cmd_match_cnt)
	{
		cli->send_char('\n');
		for(;;)
		{
			cmd = cli_search_command(
				cli, 
				cli->input_buff,
				true, cmd_found_at_index, 
				&cmd_found_at_index, 
				NULL);
			if (cmd)
			{
				cmd_found_at_index += 1;
				echo_string(cli, cmd->command_name);
				cli->send_char('\n');
			}
			else
			{
				break;
			}
		}

		echo_input_end_sequence(cli);
		echo_string(cli, cli->current_user->prompt);
		cli->input_buff[cli->input_buff_index] = 0;
		echo_string(cli, cli->input_buff);
	}
	else
	{
		cmd = cli_search_command(
			cli, 
			cli->input_buff,
			true, search_from_index, 
			NULL,
			NULL);
		if (cmd)
		{
			while (delete_last_echoed_char(cli));
			cli_put_cmd_on_prompt(cli, cmd->command_name);

//			echo_string(cli, cmd->command_name);
//			strncpy(cli->input_buff, cmd->command_name,
//				CLI_COMMAND_BUFF_SIZE);
//			cli->input_buff_index = strlen(cmd->command_name);

		}
	}
}

//#include "debug_io.h"
STATIC char *cli_handle_new_character(struct cli *cli, char c,
				      bool hide_echo)
{
//	dmsg("\n %c 0x%x\n", c, c);
	char *ret = NULL;

        if (cli->input_end_char == c)
	{
		cli->input_buff[cli->input_buff_index] = '\0';

		if (cli_history_handler_input_v1(cli))
		{
			ret = NULL;
		}
		else
		{
			ret = cli->input_buff;
			echo_input_end_sequence(cli);
			cli->input_buff_index = 0;
		}

		cli->special_sequence = false;
		cli->ssb_index = 0;
        }
	else if (cli_special_sequence_handler(cli, c))
	{

	}
        else if( '\b' == c || 0x7f == c) //backspace
        {
		delete_last_echoed_char(cli);
        }
        else if( '\t' == c) //tab
        {
		cli_autocomplete(cli);
        }
        else if( '\r' == c) //drop character
        {
                //TODO: Maybe there is a need for droping more than
		// one character?
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

#ifdef ENABLE_AUTOMATIC_LOGOFF
#include "cli_automatic_loggoff.c"
#endif

uint32_t cli_run(struct cli *cli, uint32_t time_from_last_run_ms)
{
	(void) time_from_last_run_ms;

#ifdef ENABLE_AUTOMATIC_LOGOFF
	cli_logoff_handler(cli, time_from_last_run_ms);
#endif

	char c;
	while(cli->get_char(&c))
	{
#ifdef ENABLE_AUTOMATIC_LOGOFF
		cli_reset_logoff_timer(cli);
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

STATIC struct cli_cmd *cli_make_new_cmd(struct cli *cli, 
			     struct cli_cmd_settings cs)
{
	struct cli_cmd *tmp = cli->malloc(sizeof(struct cli_cmd));
	tmp->next = NULL;
	tmp->command_name = cs.command_name;
	tmp->command_description = cs.command_description;
	tmp->command_function = cs.command_function;
	return tmp;
}

STATIC void cli_add_cmd_to_list(struct cli_cmd *list,
				       struct cli_cmd *new)	       
{
        struct cli_cmd *tmp = list;
        for (; NULL != tmp->next; tmp = tmp->next);
        tmp->next = new;
}

bool cli_add_cmd_common(struct cli *cli, struct cli_cmd_settings cs)
{
	if ((NULL == cli))
	{
		return false;
	}

	struct cli_cmd *new = cli_make_new_cmd(cli, cs);

	cli_add_cmd_to_list(&cli->common_cmd_list, new);

	return true;
}

bool cli_user_add_cmd(struct cli_user *user, struct cli_cmd_settings cs)
{
	if ((NULL == user))
	{
		return false;
	}

	struct cli_cmd *new = cli_make_new_cmd(user->cli, cs);

	if (NULL == user->cmd_list)
	{
		// first cmd added to users list
		user->cmd_list = new;
	}
	else
	{
		cli_add_cmd_to_list(user->cmd_list, new);
	}

	return true;
}

struct cli_user *cli_add_user(struct cli *cli, 
			      struct cli_user_settings us)
{
	if ((NULL == cli))
	{
		return NULL;
	}

	//tmp cant be NULL by design, because there is always 
	// guest user added when cli is initialized
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

STATIC void help_cmd(struct cli *cli, char *s)
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
				echo_string(
					cli, 
					tmp[i]->command_description);
				echo_string(cli, "\r\n");
			}
		}
	}
}

STATIC void su_cmd(struct cli *cli, char *s)
{
        (void) s;

	echo_string(cli, "user: ");
	char *u = cli_get_user_input(cli, false);

        struct cli_user *tmp = &cli->users;
        for (; NULL != tmp; tmp = tmp->next)
	{
		if (0 == strcmp(tmp->name, u))
		{
			if (tmp->password_check)
			{
				echo_string(cli, "pass: ");
				if (tmp->password_check(
					    cli_get_user_input(
						    cli, true)))
				{
					cli->current_user = tmp;
				}
			}
			else
			{
				cli->current_user = tmp;
			}

			return;
		}
	}
}

char *cli_get_user_input(struct cli *cli, bool hide)
{
	// TODO: Do we need timeout here?
	// TODO: Add yield for use in OS
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
	tmp->common_cmd_list.command_function = help_cmd;
	tmp->input_end_char = s->input_end_char;

	tmp->users.next = NULL;
	tmp->users.cli = tmp;
	tmp->users.password_check = NULL;
	tmp->users.cmd_list = NULL;
	tmp->users.prompt = s->prompt_user;
	tmp->users.name = "guest";
	tmp->current_user = &tmp->users;
	tmp->logout_time = s->logout_time;

	// TODO: Check return code of adding SU command
	cli_add_cmd_common(tmp, (struct cli_cmd_settings) 
			   {
				   .command_name = "su",
				   .command_function = su_cmd,
				   .command_description = "select user"
			   });
	return tmp;
}



