#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include "unity.h"

#include "cli_internal.h"
#include "cli.h"

static uint8_t get_char_buff[1024];
static uint32_t get_char_buff_index;

static uint8_t send_char_buff[1024];
static uint32_t send_char_buff_index;

static uint32_t cli_function_01_call_cnt = 0;
static uint32_t cli_function_02_call_cnt = 0;

struct cli *cli_default = NULL;

static bool get_char_test(char *c)
{
	get_char_buff[get_char_buff_index] = *c;
	get_char_buff_index += 1;

	return true;
}

static void send_char_test(char c)
{
	send_char_buff[send_char_buff_index] = c;
	send_char_buff_index += 1;

}

static void cli_function_01(struct cli *cli, char *s)
{
	(void)(cli);
	(void)(s);
	cli_function_01_call_cnt += 1;
}

static void cli_function_02(struct cli *cli, char *s)
{
	(void)(cli);
	(void)(s);
	cli_function_02_call_cnt += 1;
}

struct cli_cmd cli_f01 = {
	.next = NULL,
	.command_name = "f01",
	.command_description = 
	"test function description 01",
	.command_function = cli_function_01,
};

struct cli_cmd cli_f02 = {
	.next = NULL,
	.command_name = "f02",
	.command_description = 
	"test function description 02",
	.command_function = cli_function_02,
};

void setUp(void)
{
	memset(get_char_buff, 0, sizeof(get_char_buff));
	get_char_buff_index = 0;

	memset(send_char_buff, 0, sizeof(send_char_buff));
	send_char_buff_index = 0;

	cli_function_01_call_cnt = 0;
	
	cli_f01.next = NULL;
	cli_f02.next = NULL;

	struct cli_settings s = {
		.my_malloc = malloc,
		.get_char = get_char_test,
		.send_char = send_char_test,
		.input_end_char = '\n',
		.prompt_user = "cli>",
	};

	cli_default = cli_init(&s);
}

void tearDown(void)
{
        
}


void test_cli_init(void)
{
	struct cli_settings s = {
		.my_malloc = malloc,
		.get_char = get_char_test,
		.send_char = send_char_test,
		.input_end_char = '\n',
		.prompt_user = "cli>",
	};
	struct cli *c = cli_init(&s);
	TEST_ASSERT_NOT_NULL(c);
	//free(c);

	struct cli_settings missing_malloc;
	memcpy(&missing_malloc, &s, sizeof(struct cli_settings));
	missing_malloc.my_malloc = NULL;
	TEST_ASSERT_NULL(cli_init(&missing_malloc));

	struct cli_settings missing_get_char;
	memcpy(&missing_get_char, &s, sizeof(struct cli_settings));
	missing_get_char.get_char = NULL;
	TEST_ASSERT_NULL(cli_init(&missing_get_char));

	struct cli_settings missing_send_char;
	memcpy(&missing_send_char, &s, sizeof(struct cli_settings));
	missing_send_char.send_char = NULL;
	TEST_ASSERT_NULL(cli_init(&missing_send_char));

}

void test_cli_echo_string(void)
{
	TEST_ASSERT_NOT_NULL(cli_default);

	const char *test_string = "Test";
	echo_string(cli_default, test_string);

	TEST_ASSERT_EQUAL_size_t(strlen(test_string), 
				 send_char_buff_index);
	TEST_ASSERT_EQUAL_MEMORY(test_string, send_char_buff, 
				 send_char_buff_index);
	
}

void test_echo_input_end_sequence(void)
{
	TEST_ASSERT_NOT_NULL(cli_default);
	echo_input_end_sequence(cli_default);
	TEST_ASSERT_EQUAL_INT8('\r', 
			       send_char_buff[0]);
	TEST_ASSERT_EQUAL_INT8('\n', 
			       send_char_buff[1]);
}

void test_delete_last_char(void)
{
	TEST_ASSERT_NOT_NULL(cli_default);

	cli_handle_new_character(cli_default, 'a', false);
	//echo_string(cli_default, "a");
	TEST_ASSERT_EQUAL_INT8('a', 
			       send_char_buff[0]);
	TEST_ASSERT_EQUAL_INT8(0, 
			       send_char_buff[1]);

	delete_last_echoed_char(cli_default);

	TEST_ASSERT_EQUAL_INT8('a', 
			       send_char_buff[0]);

	TEST_ASSERT_EQUAL_INT8('\b', 
			       send_char_buff[1]);

	TEST_ASSERT_EQUAL_INT8(' ', 
			       send_char_buff[2]);

	TEST_ASSERT_EQUAL_INT8('\b', 
			       send_char_buff[3]);

	TEST_ASSERT_EQUAL_INT8(0, 
			       send_char_buff[4]);
}

void test_cli_add_cmd(void)
{
	TEST_ASSERT_NOT_NULL(cli_default);

	cli_add_cmd_common(cli_default, (struct cli_cmd_settings) 
			   {
				   .command_name = "f01",
				   .command_description = 
				   "test function description 01",
				   .command_function = cli_function_01,
			   });

	//cli_add_cmd_common(cli_default, &cli_f01);

	//TEST_ASSERT_EQUAL_PTR(&cli_f01, cli_default->common_cmd_list.next);
	TEST_ASSERT_NULL(cli_default->common_cmd_list.next->next->next);

	//cli_add_cmd_common(cli_default, &cli_f02);
	cli_add_cmd_common(cli_default, (struct cli_cmd_settings) 
			   {
				   .command_name = "f02",
				   .command_description = 
				   "test function description 02",
				   .command_function = cli_function_02,
			   });

	//TEST_ASSERT_EQUAL_PTR(&cli_f02, 
	//		      cli_default->common_cmd_list.next->next);
	TEST_ASSERT_NULL(cli_default->common_cmd_list.next->next->next->next);
}

void test_cli_search_command(void)
{

	TEST_ASSERT_NOT_NULL(cli_default);
	cli_add_cmd_common(cli_default, (struct cli_cmd_settings) 
			   {
				   .command_name = "f01",
				   .command_description = 
				   "test function description 01",
				   .command_function = cli_function_01,
			   });
//	cli_add_cmd_common(cli_default, &cli_f01);

	struct cli_cmd *t1 = cli_search_command(cli_default, 
						"f01",
						strlen("f01"),
						false, false);
	//TEST_ASSERT_EQUAL_PTR(&cli_f01, t1);

	t1 = cli_search_command(cli_default, "f02", strlen("f02"),
				false, false);
	TEST_ASSERT_NULL(t1);

//	cli_add_cmd_common(cli_default, &cli_f02);
	cli_add_cmd_common(cli_default, (struct cli_cmd_settings) 
			   {
				   .command_name = "f02",
				   .command_description = 
				   "test function description 02",
				   .command_function = cli_function_02,
			   });

	t1 = cli_search_command(cli_default, "f02", strlen("f02"),
				false, false);
	//TEST_ASSERT_EQUAL_PTR(&cli_f02, t1);
}

void test_cli_command_received_handler(void)
{
	TEST_ASSERT_NOT_NULL(cli_default);
//	cli_add_cmd_common(cli_default, &cli_f01);
//	cli_add_cmd_common(cli_default, &cli_f02);
	cli_add_cmd_common(cli_default, (struct cli_cmd_settings) 
			   {
				   .command_name = "f01",
				   .command_description = 
				   "test function description 01",
				   .command_function = cli_function_01,
			   });

	cli_add_cmd_common(cli_default, (struct cli_cmd_settings) 
			   {
				   .command_name = "f02",
				   .command_description = 
				   "test function description 02",
				   .command_function = cli_function_02,
			   });

	TEST_ASSERT_EQUAL_INT32(0, cli_function_01_call_cnt);
	TEST_ASSERT_EQUAL_INT32(0, cli_function_02_call_cnt);
	cli_command_received_handler(cli_default, "f01");
	TEST_ASSERT_EQUAL_INT32(1, cli_function_01_call_cnt);
	TEST_ASSERT_EQUAL_INT32(0, cli_function_02_call_cnt);
}

void test_cli_handle_input(void)
{
	TEST_ASSERT_NOT_NULL(cli_default);

	char *test_input_01 = "test";
	char *tmp = NULL;
	for (uint32_t i = 0; strlen(test_input_01) > i; i++)
	{
		tmp = cli_handle_new_character(
			cli_default, test_input_01[i], false);
		TEST_ASSERT_NULL(tmp);
		TEST_ASSERT_EQUAL_INT8(test_input_01[i], 
				       send_char_buff[i]);

	}

	tmp = cli_handle_new_character(cli_default, 
				       cli_default->input_end_char,
				       false);

	TEST_ASSERT_NOT_NULL(tmp);
	TEST_ASSERT_EQUAL_STRING(test_input_01, tmp);
	// TODO: test prompt echo

	// TODO: test delete character
}

