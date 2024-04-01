# CLI/Shell for embedded system

CLI/Shell is a size-optimized, modular command line interface (CLI) designed for embedded systems. **Fully configured uses less than 1500 bytes of code and less than 200 bytes of RAM.**

## Key Features 

- Written in C
- Small and efficient code base
- Support user management with password protected login
- support autocompletiton
- command argument parsing
- supports history for last command
- supports automatic logout
- Supports a help command
- Simple way to add user commands
- Released under the BSD license

**Size:**
All sizes were measured using GCC 13.2 with -Os optimization for Cortex-M4 target.

Cli core is essential for cli/shell to operate.

|                          | Text | Data |
|---------------------------------|------|------|
| core                            | 545B | 96B  |
| core + all modules                            | 1482 | 136  |

Every module enabled adds following size to core. Modules are independent. The only dependencies between modules are:
- user management needs user input module for password input
- auto logout needs user managment

| module | Text | Data |
|---------------------------------|------|------|
| user input (ui)                 | 74B  | 0    |
| argument parser                 | 110B | 0    |
| user managment + ui             | 352B | 0    |
| user manager + ui + auto logout | 401B | 8B   |
| enter history   (v1)                | 64B  | 32B  |
| arrow history   (v2)                | 132B | 32B  |
| command autocomplete            | 401B | 8B   |


NOTE: A lot of code is shared between modules, thats why when all
modules are enabled the code size is 1482 and not 1727 as you would
expect from adding all text size together. Similar is with ram usage...


## Usage

### Compiling
The whole implementation is in one file `cli.c`. Other than that there are 
two header files `cli.h` and `cli_internal.h`.

The minimum working shell (core) can be extended by defining following defines:

* CLI_COMMAND_BUFF_SIZE defines max size of the cli command name,
  default size is 32bytes

* ENABLE_AUTOCOMPLETE 
  Enables tab autocompletition. It also list all matching commands when tab is pressed.

* ENABLE_USER_MANAGEMENT
  Enables support for multiple users and it also adds su command, which is used to switch
  between users

* ENABLE_USER_INPUT_REQUEST 
  Enables support for asking user for additional input during command execution

* ENABLE_AUTOMATIC_LOGOUT 
  Enables automatic logoff after user predefined time is passed. This needs ENABLE_USER_MANAGEMENT enabled

* ENABLE_ARGUMENT_PARSER
  Enables argument parsing for commands

* ENABLE_HISTORY_V1 
  Pressing enter on the empty command prompt will put on prompt last executed command

* ENABLE_HISTORY_V2 
  Up arrow will bring back last executed command


### Initialization

#### Using Dynamic Memory Allocation

Initialize the CLI by providing your own memory allocation
function. Why and how you should use static runtime memory allocation in
embedded system is described [here](https://github.com/IzidorM/fw_memory_allocator)

```c
#include <stdint.h>
#include "cli.h"

struct cli *example_cli_init()
{
	struct cli_settings cs = {
		.my_malloc = app_malloc,
		.get_char = app_cli_get_char,
		.send_char = app_cli_send_char,
		.input_end_char = '\r',
		.prompt_user = "guest> ",
#ifdef ENABLE_AUTOMATIC_LOGOUT
		.logout_time_ms = 15000,
#endif
	};
	
    return cli_init(&cs);
}
```

### Running

Call the `cli_run(cli, time_from_last_call_ms);` function periodically in a super loop or task in case OS is used.

### Defining and Adding Commands

Define commands and add them to the CLI interface:

```c
#include "cli.h"


static void reboot_cli(struct cli *cli, char *s)
{
        (void) cli;
        (void) s;

        NVIC_SystemReset();
}

static void foo_hello_cli(struct cli *cli, char *s)
{
        (void) cli;
        (void) s;

	dmsg("Hello foo\n");
}

bool user_foo_password_check(char *d)
{
	if (0 == strcmp(d, "lol"))
	{
		return true;
	}
	return false;
}
#endif


void app_cli_commands_add(struct cli *cli)
{
	cli_add_cmd_common(cli, (struct cli_cmd_settings) 
			   {
				   .command_name = "reboot",
				   .command_description = "rebooting the cpu\n",
				   .command_function = reboot_cli,
			   });

#ifdef ENABLE_USER_MANAGEMENT
	struct cli_user *foo = cli_add_user(
		cli, (struct cli_user_settings)
		{
			.name= "foo",
			.password_check = user_foo_password_check,
			.prompt = "foo> ",
			
		});

	ASSERT(foo);

	cli_user_add_cmd(foo, (struct cli_cmd_settings) 
			   {
				   .command_name = "hello",
				   .command_description = "say hello to user\n",
				   .command_function = foo_hello_cli,
			   });
#endif

}



```

## Unit Tests

Unit tests are available in the `unit_test` folder. Before running them, update the path to Unity in the Makefile. The tests should execute smoothly on any Linux system with GCC installed.
