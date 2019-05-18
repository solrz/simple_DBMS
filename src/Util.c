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
#define and &&
#define or ||
#define bool int
#define true 1
#define false 0
#define debug_state(str,v) printf("%s:%d\n",str,v)
#define debugstr(str) printf("%s", str)
#define debugstrn(str) printf("%s\n", str)
#define debug(v) printf("%d\n", v)
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
    if (state->saved_stdout == -1) {
        printf("db > ");
    }
}

///
/// Print the user in the specific format
///
void print_user(User_t *user, SelectArgs_t *sel_args) {
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
                printf("%u", user->age);
            }
        }
    }
    printf(")\n");
}

///
/// Print the users for given offset and limit restriction
///
void print_users(Table_t *table, int *idxList, size_t idxListLen, Command_t *cmd) {
    size_t idx;
    int limit = cmd->cmd_args.sel_args.limit;
    int offset = cmd->cmd_args.sel_args.offset;

    if (offset == -1) offset = 0;
    // find set clause
    size_t where_clause_start;
    for (size_t i = 0; i < cmd->args_len; ++i) {
        char* current_arg = cmd->args[i];
        if(current_arg){
            if(!strncmp(current_arg,"where",5)){
                where_clause_start = i;
                break;
            }
        }
    }


    WhereArgs_t conditions[100];
    size_t conditions_len = 0;
    bool consitions_isand = false;
    for (size_t i = where_clause_start; i < cmd->args_len; ++i) {
        char* current_arg = cmd->args[i];
        if(current_arg){
            if(current_arg[0] == '=' or current_arg[0] == '>'
            or current_arg[0] == '<' or current_arg[0] == '!'){
                conditions[conditions_len].arg = cmd->args[i-1];
                conditions[conditions_len].cmp_type = cmd->args[i];
                conditions[conditions_len++].value = cmd->args[i+1];
            }
            if(!strncmp(current_arg,"and",3)){
                consitions_isand = true;
            }
        }
    }

    int idxListRemap[table->len];
    int idxListRemap_len = 0;
    for (size_t i = 0; i < table->len; ++i) {
        User_t *user = get_User(table, i);
        bool fit = true;
        for (int j = 0; j < conditions_len; ++j) {
            float buf_num;
            char *buf_str;
            bool buf_isnum = false;
            WhereArgs_t cmp_condition = conditions[j];
            if(!strncmp(cmp_condition.arg, "id",2)) {
                buf_num = user->id;
                buf_isnum = true;
            } else if(!strncmp(cmp_condition.arg, "age",3)) {
                buf_num = user->age;
                buf_isnum = true;
            } else if(!strncmp(cmp_condition.arg, "email",1)) {
                buf_str = user->email;
            } else if(!strncmp(cmp_condition.arg, "name",4)) {
                buf_str = user->name;
            }



            if(buf_isnum){
                if(!strncmp(cmp_condition.cmp_type, "=",1)){
                    fit = buf_num == atoi(cmp_condition.value);
                }else if(!strncmp(cmp_condition.cmp_type, "!=",2)){
                    fit = buf_num != atoi(cmp_condition.value);
                }else if(!strncmp(cmp_condition.cmp_type, ">=",2)){
                    fit = buf_num >= atoi(cmp_condition.value);
                }else if(!strncmp(cmp_condition.cmp_type, "<=",2)){
                    fit = buf_num <= atoi(cmp_condition.value);
                }else if(!strncmp(cmp_condition.cmp_type, ">",1)){
                    fit = buf_num >  atoi(cmp_condition.value);
                }else if(!strncmp(cmp_condition.cmp_type, "<",1)){
                    fit = buf_num <  atoi(cmp_condition.value);
                }
            } else {
                if(!strncmp(cmp_condition.cmp_type, "=",1)){
                    fit = !strcmp(cmp_condition.value, buf_str);
                }else if(!strncmp(cmp_condition.cmp_type, "!=",2)) {
                    fit = strcmp(cmp_condition.value, buf_str);
                }
                ////debugstr(cmp_condition.value);
                ////debugstr(buf_str);
            }
            if(!fit and consitions_isand) break;
            if(fit and !consitions_isand) break;
        }
        if(!fit) continue;
        idxListRemap[idxListRemap_len++] = i;
    }

    if (idxListRemap) {
        for (idx = offset; idx < idxListRemap_len; idx++) {
            if (limit != -1 && (idx - offset) >= limit) break;

            print_user(get_User(table, idxListRemap[idx]), &(cmd->cmd_args.sel_args));
        }
    } else {
        for (idx = offset; idx < table->len; idx++) {
            if (limit != -1 && (idx - offset) >= limit) break;

            print_user(get_User(table, idx), &(cmd->cmd_args.sel_args));
        }
    }
}

///
/// This function received an output argument
/// Return: category of the command
///
int parse_input(char *input, Command_t *cmd) {
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
                //__fpurge(stdout); //This is used to clear the stdout buffer
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
int handle_query_cmd(Table_t *table, Command_t *cmd) {
    if (!strncmp(cmd->args[0], "insert", 6)) {
        handle_insert_cmd(table, cmd);
        return INSERT_CMD;
    } else if (!strncmp(cmd->args[0], "select", 6)) {
        handle_select_cmd(table, cmd);
        return SELECT_CMD;
    } else if (!strncmp(cmd->args[0], "update", 6)) {
        handle_update_cmd(table, cmd);
        return SELECT_CMD;
    } else if (!strncmp(cmd->args[0], "delete", 6)) {
        handle_delete_cmd(table, cmd);
        return SELECT_CMD;
    } else {
        return UNRECOG_CMD;
    }
}

int handle_update_cmd(Table_t *table, Command_t *cmd) {
    // find set + where clause
    size_t set_clause_start=-1;
    size_t where_clause_start=-1;
    for (size_t i = 0; i < cmd->args_len; ++i) {
        char* current_arg = cmd->args[i];
        if(current_arg){
            if(!strncmp(current_arg,"set",3)){
                set_clause_start = i;
            } else if(!strncmp(current_arg,"where",5)){
                where_clause_start = i;
            }
        }
    }
    //debugstr("set@");
    //debug(set_clause_start);
    //debugstr("where@");
    //debug(where_clause_start);

    // find field to update
    WhereArgs_t update;
    if (set_clause_start != -1){
        update.arg = cmd->args[++set_clause_start];
        update.cmp_type = cmd->args[++set_clause_start];
        update.value = cmd->args[++set_clause_start];
    }

    // find field to fit
    WhereArgs_t conditions[100];
    size_t conditions_len = 0;
    bool consitions_isand = false;
    for (size_t i = where_clause_start; i < cmd->args_len; ++i) {
        char* current_arg = cmd->args[i];
        if(current_arg){
            if(current_arg[0] == '=' or current_arg[0] == '>'
               or current_arg[0] == '<' or current_arg[0] == '!'){
                conditions[conditions_len].arg = cmd->args[i-1];
                conditions[conditions_len].cmp_type = cmd->args[i];
                conditions[conditions_len++].value = cmd->args[i+1];
            }
            if(!strncmp(current_arg,"and",3)){
                consitions_isand = true;
            }
        }
    }

    // find user fitting
    int idxListRemap[table->len];
    int idxListRemap_len = 0;
    for (size_t i = 0; i < table->len; ++i) {
        User_t *user = get_User(table, i);
        bool fit = true;
        for (int j = 0; j < conditions_len; ++j) {
            float buf_num;
            char *buf_str;
            bool buf_isnum = false;
            WhereArgs_t cmp_condition = conditions[j];
            if(!strncmp(cmp_condition.arg, "id",2)) {
                buf_num = user->id;
                buf_isnum = true;
            } else if(!strncmp(cmp_condition.arg, "age",3)) {
                buf_num = user->age;
                buf_isnum = true;
            } else if(!strncmp(cmp_condition.arg, "email",1)) {
                buf_str = user->email;
            } else if(!strncmp(cmp_condition.arg, "name",4)) {
                buf_str = user->name;
            }



            if(buf_isnum){
                if(!strncmp(cmp_condition.cmp_type, "=",1)){
                    fit = buf_num == atoi(cmp_condition.value);
                }else if(!strncmp(cmp_condition.cmp_type, "!=",2)){
                    fit = buf_num != atoi(cmp_condition.value);
                }else if(!strncmp(cmp_condition.cmp_type, ">=",2)){
                    fit = buf_num >= atoi(cmp_condition.value);
                }else if(!strncmp(cmp_condition.cmp_type, "<=",2)){
                    fit = buf_num <= atoi(cmp_condition.value);
                }else if(!strncmp(cmp_condition.cmp_type, ">",1)){
                    fit = buf_num >  atoi(cmp_condition.value);
                }else if(!strncmp(cmp_condition.cmp_type, "<",1)){
                    fit = buf_num <  atoi(cmp_condition.value);
                }
            } else {
                if(!strncmp(cmp_condition.cmp_type, "=",1)){
                    fit = !strcmp(cmp_condition.value, buf_str);
                }else if(!strncmp(cmp_condition.cmp_type, "!=",2)) {
                    fit = strcmp(cmp_condition.value, buf_str);
                }
                ////debugstr(cmp_condition.value);
                ////debugstr(buf_str);
            }
            if(!fit and consitions_isand) break;
            if(fit and !consitions_isand) break;
        }
        if(!fit) continue;
        idxListRemap[idxListRemap_len++] = i;
    }

    // do update
    User_t* user;
    size_t i = 0;
    while(i < idxListRemap_len){
        user = get_User(table,idxListRemap[i++]);
        if(!strncmp(update.arg,"age",3)){
            user->age = atoi(update.value);
        } else if(!strncmp(update.arg,"id",2)){
            user->id = atoi(update.value);
        } else if(!strncmp(update.arg,"email",5)){
            strncpy(user->email, update.value, MAX_USER_EMAIL);
        } else if(!strncmp(update.arg,"name",4)){
            strncpy(user->email, update.value, MAX_USER_NAME);        }
    }
}
int handle_delete_cmd(Table_t *table, Command_t *cmd) {
    // find where clause
    size_t where_clause_start=-1;
    for (size_t i = 0; i < cmd->args_len; ++i) {
        char* current_arg = cmd->args[i];
        if(current_arg){
            if(!strncmp(current_arg,"where",5)){
                where_clause_start = i;
            }
        }
    }
    //debugstr("where@");
    //debug(where_clause_start);

    // find field to update


    // find field to fit
    WhereArgs_t conditions[100];
    size_t conditions_len = 0;
    bool consitions_isand = false;
    for (size_t i = where_clause_start; i < cmd->args_len; ++i) {
        char* current_arg = cmd->args[i];
        if(current_arg){
            if(current_arg[0] == '=' or current_arg[0] == '>'
               or current_arg[0] == '<' or current_arg[0] == '!'){
                conditions[conditions_len].arg = cmd->args[i-1];
                conditions[conditions_len].cmp_type = cmd->args[i];
                conditions[conditions_len++].value = cmd->args[i+1];
            }
            if(!strncmp(current_arg,"and",3)){
                consitions_isand = true;
            }
        }
    }

    // find user fitting
    int idxListRemap[table->len];
    int idxListRemap_len = 0;
    for (size_t i = 0; i < table->len; ++i) {
        User_t *user = get_User(table, i);
        bool fit = true;
        for (int j = 0; j < conditions_len; ++j) {
            float buf_num;
            char *buf_str;
            bool buf_isnum = false;
            WhereArgs_t cmp_condition = conditions[j];
            if(!strncmp(cmp_condition.arg, "id",2)) {
                buf_num = user->id;
                buf_isnum = true;
            } else if(!strncmp(cmp_condition.arg, "age",3)) {
                buf_num = user->age;
                buf_isnum = true;
            } else if(!strncmp(cmp_condition.arg, "email",1)) {
                buf_str = user->email;
            } else if(!strncmp(cmp_condition.arg, "name",4)) {
                buf_str = user->name;
            }



            if(buf_isnum){
                if(!strncmp(cmp_condition.cmp_type, "=",1)){
                    fit = buf_num == atoi(cmp_condition.value);
                }else if(!strncmp(cmp_condition.cmp_type, "!=",2)){
                    fit = buf_num != atoi(cmp_condition.value);
                }else if(!strncmp(cmp_condition.cmp_type, ">=",2)){
                    fit = buf_num >= atoi(cmp_condition.value);
                }else if(!strncmp(cmp_condition.cmp_type, "<=",2)){
                    fit = buf_num <= atoi(cmp_condition.value);
                }else if(!strncmp(cmp_condition.cmp_type, ">",1)){
                    fit = buf_num >  atoi(cmp_condition.value);
                }else if(!strncmp(cmp_condition.cmp_type, "<",1)){
                    fit = buf_num <  atoi(cmp_condition.value);
                }
            } else {
                if(!strncmp(cmp_condition.cmp_type, "=",1)){
                    fit = !strcmp(cmp_condition.value, buf_str);
                }else if(!strncmp(cmp_condition.cmp_type, "!=",2)) {
                    fit = strcmp(cmp_condition.value, buf_str);
                }
                ////debugstr(cmp_condition.value);
                ////debugstr(buf_str);
            }
            if(!fit and consitions_isand) break;
            if(fit and !consitions_isand) break;
        }
        if(!fit) continue;
        idxListRemap[idxListRemap_len++] = i;
    }

    // do update
    User_t* user;
    int i = idxListRemap_len-1;
    while(i >= 0){
        //debug_state("index",i);
        user = get_User(table, idxListRemap[i]);
        //debug_state("id",user->id);
        remove_User(table,idxListRemap[i--]);
    }
}
///
/// The return value is the number of rows insert into table
/// If the insert operation success, then change the input arg
/// `cmd->type` to INSERT_CMD
///
int handle_insert_cmd(Table_t *table, Command_t *cmd) {
    int ret = 0;
    User_t *user = command_to_User(cmd);
    if (user) {
        ret = add_User(table, user);
        if (ret > 0) {
            cmd->type = INSERT_CMD;
        }
    }
    return ret;
}

///
/// The return value is the number of rows select from table
/// If the select operation success, then change the input arg
/// `cmd->type` to SELECT_CMD
///
int handle_select_cmd(Table_t *table, Command_t *cmd) {
    cmd->type = SELECT_CMD;
    field_state_handler(cmd, 1);
    if (!strncmp(cmd->cmd_args.sel_args.fields[0], "avg(", 4) or
        !strncmp(cmd->cmd_args.sel_args.fields[0], "sum(", 4) or
        !strncmp(cmd->cmd_args.sel_args.fields[0], "count(", 6)){
        print_aggre(table, cmd);
    }else{
        print_users(table, NULL, 0, cmd);
    }
    return table->len;
}

int print_aggre(Table_t *table, Command_t *cmd) {
    float aggre_value=0.0f;
    int aggre_type=0;
    if(!strncmp(cmd->cmd_args.sel_args.fields[0], "avg(", 4)){
        aggre_type=1;
    } else if(!strncmp(cmd->cmd_args.sel_args.fields[0], "sum(", 4)){
        aggre_type=2;
    } else if(!strncmp(cmd->cmd_args.sel_args.fields[0], "count(", 6)){
        aggre_type=3;
    }
    size_t idx;
    int limit = cmd->cmd_args.sel_args.limit;
    int offset = cmd->cmd_args.sel_args.offset;

    if (offset == -1) offset = 0;
    WhereArgs_t conditions[100];
    size_t conditions_len = 0;
    bool consitions_isand = false;
    for (size_t i = 0; i < cmd->args_len; ++i) {
        char* current_arg = cmd->args[i];
        if(current_arg){
            if(current_arg[0] == '=' or current_arg[0] == '>'
               or current_arg[0] == '<' or current_arg[0] == '!'){
                conditions[conditions_len].arg = cmd->args[i-1];
                conditions[conditions_len].cmp_type = cmd->args[i];
                conditions[conditions_len++].value = cmd->args[i+1];
            }
            if(!strncmp(current_arg,"and",3)){
                consitions_isand = true;
            }
        }
    }

    int idxListRemap[table->len];
    int idxListRemap_len = 0;
    for (size_t i = 0; i < table->len; ++i) {
        User_t *user = get_User(table, i);
        bool fit = true;
        for (int j = 0; j < conditions_len; ++j) {
            float buf_num;
            char *buf_str;
            bool buf_isnum = false;
            WhereArgs_t cmp_condition = conditions[j];
            if(!strncmp(cmp_condition.arg, "id",2)) {
                buf_num = user->id;
                buf_isnum = true;
            } else if(!strncmp(cmp_condition.arg, "age",3)) {
                buf_num = user->age;
                buf_isnum = true;
            } else if(!strncmp(cmp_condition.arg, "email",1)) {
                buf_str = user->email;
            } else if(!strncmp(cmp_condition.arg, "name",4)) {
                buf_str = user->name;
            }



            if(buf_isnum){
                if(!strncmp(cmp_condition.cmp_type, "=",1)){
                    fit = buf_num == atoi(cmp_condition.value);
                }else if(!strncmp(cmp_condition.cmp_type, "!=",2)){
                    fit = buf_num != atoi(cmp_condition.value);
                }else if(!strncmp(cmp_condition.cmp_type, ">=",2)){
                    fit = buf_num >= atoi(cmp_condition.value);
                }else if(!strncmp(cmp_condition.cmp_type, "<=",2)){
                    fit = buf_num <= atoi(cmp_condition.value);
                }else if(!strncmp(cmp_condition.cmp_type, ">",1)){
                    fit = buf_num >  atoi(cmp_condition.value);
                }else if(!strncmp(cmp_condition.cmp_type, "<",1)){
                    fit = buf_num <  atoi(cmp_condition.value);
                }
            } else {
                if(!strncmp(cmp_condition.cmp_type, "=",1)){
                    fit = !strcmp(cmp_condition.value, buf_str);
                }else if(!strncmp(cmp_condition.cmp_type, "!=",2)) {
                    fit = strcmp(cmp_condition.value, buf_str);
                }
                ////debugstr(cmp_condition.value);
                ////debugstr(buf_str);
            }
            if(!fit and consitions_isand) break;
            if(fit and !consitions_isand) break;
        }
        if(!fit) continue;
        idxListRemap[idxListRemap_len++] = i;
    }
    for (idx = 0; idx < idxListRemap_len; idx++) {
        User_t *user = get_User(table, idx);
        if(aggre_type==1 or aggre_type==2){
            aggre_value = aggre_value + (float)(user->age);
        }else if (aggre_type==3){
            aggre_value = aggre_value + 1.0;
        }
        ////debug(user->age);
        ////debugstr("\n");
        ////debug(aggre_value);
        ////debugstr("\n");
    }
    if(aggre_type==1){
        aggre_value = aggre_value / idxListRemap_len;
        printf("(%.3llf)\n",aggre_value);
    }
    if(aggre_type==2 or aggre_type==3){
        int cache = (int)aggre_value;
        printf("(%d)\n",cache);
    }
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

