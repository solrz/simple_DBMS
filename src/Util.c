#include <stdio.h>
//#include <stdio_ext.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include "Util.h"
#include "Command.h"
#include "Table.h"
#include "SelectState.h"
#define not !
#define or ||
#define debugstr(str) printf("%s", str)
#define debugstrs(str) printf("Util/ %s\n", str)
#define debug(v) printf("%f", v)

///
/// Allocate State_t and initialize some attributes
/// Return: ptr of new State_t
///
State_t* new_State() {
    State_t *state = (State_t*)malloc(sizeof(State_t));
    state->saved_stdout = -1;
    return state;
}

///
/// Print shell prompt
///
void print_prompt(State_t *state) {
    debugstrs("print_prompt");
    if (state->saved_stdout == -1) {
        printf("db > ");
    }
}

///
/// Print the user in the specific format
///
void print_user(User_t *user, SelectArgs_t *sel_args) {
    debugstrs("print_user");
    size_t idx;
    printf("(");
    for (idx = 0; idx < sel_args->fields_len; idx++) {
        if (!strncmp(sel_args->fields[idx], "*", 1)) {
            printf("%d, %s, %s, %d", user->id, user->name, user->email, user->age);
        } else {
            if (idx > 0) printf(", ");

            if (!strncmp(sel_args->fields[idx], "id", 2)) {
                printf("%d", user->id);
            } else if (!strncmp(sel_args->fields[idx], "name", 4)) {
                printf("%s", user->name);
            } else if (!strncmp(sel_args->fields[idx], "email", 5)) {
                printf("%s", user->email);
            } else if (!strncmp(sel_args->fields[idx], "age", 3)) {
                printf("%d", user->age);
            }
        }
    }
    printf(")\n");
}

void print_like(Like_t *like, SelectArgs_t *sel_args) {
    debugstrs("print_like");
    size_t idx;
    printf("(");
    for (idx = 0; idx < sel_args->fields_len; idx++) {
        if (!strncmp(sel_args->fields[idx], "*", 1)) {
            printf("%d, %d", like->id1, like->id2);
        } else {
            if (idx > 0) printf(", ");

            if (!strncmp(sel_args->fields[idx], "id1", 3)) {
                printf("%d", like->id1);
            } else if (!strncmp(sel_args->fields[idx], "id2", 3)) {
                printf("%d", like->id2);
            }
        }
    }
    printf(")\n");
}

///
/// Print the users for given offset and limit restriction
///
void print_users(Table_t *table, int *idxList, size_t idxListLen, Command_t *cmd) {
    debugstrs("print_users");
    size_t idx;
    int limit = cmd->cmd_args.sel_args.limit;
    int offset = cmd->cmd_args.sel_args.offset;

    if (offset == -1) {
        offset = 0;
    }
    debug(table->len);
    if (idxList) {
        for (idx = offset; idx < idxListLen; idx++) {
            if (limit != -1 && (idx - offset) >= limit) {
                break;
            }
            print_user(get_User(table, idxList[idx]), &(cmd->cmd_args.sel_args));
        }
    } else {
        for (idx = offset; idx < table->len; idx++) {
            if (limit != -1 && (idx - offset) >= limit) {
                break;
            }
            print_user(get_User(table, idx), &(cmd->cmd_args.sel_args));
        }
    }
}

void print_likes(Table_like_t *table_like, int *idxList, size_t idxListLen, Command_t *cmd) {
    debugstrs("print_likes");
    size_t idx;
    int limit = cmd->cmd_args.sel_args.limit;
    int offset = cmd->cmd_args.sel_args.offset;

    if (offset == -1) {
        offset = 0;
    }

    if (idxList) {
        for (idx = offset; idx < idxListLen; idx++) {
            if (limit != -1 && (idx - offset) >= limit) {
                break;
            }
            print_like(get_Like(table_like, idxList[idx]), &(cmd->cmd_args.sel_args));
        }
    } else {
        for (idx = offset; idx < table_like->len; idx++) {
            if (limit != -1 && (idx - offset) >= limit) {
                break;
            }
            print_like(get_Like(table_like, idx), &(cmd->cmd_args.sel_args));
        }
    }
}

///
/// This function received an output argument
/// Return: category of the command
///
int parse_input(char *input, Command_t *cmd) {
    debugstrs("parse_input");
    char *token;
    int idx;
    token = strtok(input, " ,\n");
    for (idx = 0; strlen(cmd_list[idx].name) != 0; idx++) {
        if (!strncmp(token, cmd_list[idx].name, cmd_list[idx].len)) {
            cmd->type = cmd_list[idx].type;
        }
    }
    while (token != NULL) {
        add_Arg(cmd, token);
        token = strtok(NULL, " ,\n");
    }
    return cmd->type;
}

///
/// Handle built-in commands
/// Return: command type
///
void handle_builtin_cmd(Table_t *table, Command_t *cmd, State_t *state) {
    debugstrs("handle_builtin_cmd");
    if (!strncmp(cmd->args[0], ".exit", 5)) {
        archive_table(table);
        exit(0);
    } else if (!strncmp(cmd->args[0], ".output", 7)) {
        if (cmd->args_len == 2) {
            if (!strncmp(cmd->args[1], "stdout", 6)) {
                close(1);
                dup2(state->saved_stdout, 1);
                state->saved_stdout = -1;
            } else if (state->saved_stdout == -1) {
                int fd = creat(cmd->args[1], 0644);
                state->saved_stdout = dup(1);
                if (dup2(fd, 1) == -1) {
                    state->saved_stdout = -1;
                }
                fpurge(stdout); //This is used to clear the stdout buffer
            }
        }
    } else if (!strncmp(cmd->args[0], ".load", 5)) {
        if (cmd->args_len == 2) {
            load_table(table, cmd->args[1]);
        }
    } else if (!strncmp(cmd->args[0], ".help", 5)) {
        print_help_msg();
    }
}

///
/// Handle query type commands
/// Return: command type
///
///
/// Handle query type commands
/// Return: command type
///
int handle_user_query_cmd(Table_t *table_user, Command_t *cmd) {
    debugstrs("handle_user_query_cmd");
    if (!strncmp(cmd->args[0], "insert", 6)) {
        handle_user_insert_cmd(table_user, cmd);
        return INSERT_CMD;
    } else if (!strncmp(cmd->args[0], "select", 6)) {
        handle_user_select_cmd(table_user, cmd);
        return SELECT_CMD;
    } else {
        return UNRECOG_CMD;
    }
}

int handle_user_insert_cmd(Table_t *table_user, Command_t *cmd) {
    debugstrs("handle_user_insert_cmd");
    int ret = 0;
    User_t *user = command_to_User(cmd);
    if (user) {
        ret = add_User(table_user, user);
        if (ret > 0) {
            cmd->type = INSERT_CMD;
        }
    }
    return ret;
}

int handle_user_select_cmd(Table_t *table_user, Command_t *cmd) {
    debugstrs("handle_user_select_cmd");
    cmd->type = SELECT_CMD;
    user_field_state_handler(cmd, 1);

    print_users(table_user, NULL, 0, cmd);
    return table_user->len;
}

///
/// Handle query type commands
/// Return: command type
///
int handle_like_query_cmd(Table_like_t *table_like, Command_t *cmd) {
    debugstrs("handle_like_query_cmd");
    if (!strncmp(cmd->args[0], "insert", 6)) {
        handle_like_insert_cmd(table_like, cmd);
        return INSERT_CMD;
    } else if (!strncmp(cmd->args[0], "select", 6)) {
        handle_like_select_cmd(table_like, cmd);
        return SELECT_CMD;
    } else {
        return UNRECOG_CMD;
    }
}

int handle_like_insert_cmd(Table_like_t *table_like, Command_t *cmd) {
    debugstrs("handle_like_insert_cmd");
    int ret = 0;
    Like_t *like = command_to_Like(cmd);
    if (like) {
        ret = add_Like(table_like, like);
        if (ret > 0) {
            cmd->type = INSERT_CMD;
        }
    }
    return ret;
}

int handle_like_select_cmd(Table_like_t *table_like, Command_t *cmd) {
    debugstrs("handle_like_select_cmd");
    debugstr("QC1");
    cmd->type = SELECT_CMD;
    like_field_state_handler(cmd, 1);

    debugstr("QC2");
    print_likes(table_like, NULL, 0, cmd);
    return table_like->len;
}



///
/// Show the help messages
///
void print_help_msg() {
    const char msg[] = "# Supported Commands\n"
    "\n"
    "## Built-in Commands\n"
    "\n"
    "  * .exit\n"
    "\tThis cmd archives the table, if the db file is specified, then exit.\n"
    "\n"
    "  * .output\n"
    "\tThis cmd change the output strategy, default is stdout.\n"
    "\n"
    "\tUsage:\n"
    "\t    .output (<file>|stdout)\n\n"
    "\tThe results will be redirected to <file> if specified, otherwise they will display to stdout.\n"
    "\n"
    "  * .load\n"
    "\tThis command loads records stored in <DB file>.\n"
    "\n"
    "\t*** Warning: This command will overwrite the records already stored in current table. ***\n"
    "\n"
    "\tUsage:\n"
    "\t    .load <DB file>\n\n"
    "\n"
    "  * .help\n"
    "\tThis cmd displays the help messages.\n"
    "\n"
    "## Query Commands\n"
    "\n"
    "  * insert\n"
    "\tThis cmd inserts one user record into table.\n"
    "\n"
    "\tUsage:\n"
    "\t    insert <id> <name> <email> <age>\n"
    "\n"
    "\t** Notice: The <name> & <email> are string without any whitespace character, and maximum length of them is 255. **\n"
    "\n"
    "  * select\n"
    "\tThis cmd will display all user records in the table.\n"
    "\n";
    printf("%s", msg);
}

