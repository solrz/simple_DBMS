#include <string.h>
#include <stdlib.h>
#include "Command.h"
#include "SelectState.h"

#define not !
#define or ||
#define debugstr(str) printf("%s", str)
#define debugstrs(str) printf("SelectState/%s\n", str)
#define debug(v) printf("%f", v)

void field_state_handler(Command_t *cmd, size_t arg_idx) {
    cmd->cmd_args.sel_args.fields = NULL;
    cmd->cmd_args.sel_args.fields_len = 0;
    cmd->cmd_args.sel_args.limit = -1;
    cmd->cmd_args.sel_args.offset = -1;
    debug(cmd->args_len);
    while (arg_idx < cmd->args_len) {
        if (!strncmp(cmd->args[arg_idx], "*", 1) or
            !strncmp(cmd->args[arg_idx], "id", 2) or
            !strncmp(cmd->args[arg_idx], "name", 4) or
            !strncmp(cmd->args[arg_idx], "email", 5) or
            !strncmp(cmd->args[arg_idx], "age", 3)) {

            add_select_field(cmd, cmd->args[arg_idx]);

        } else if (!strncmp(cmd->args[arg_idx], "from", 4)) {
            table_state_handler(cmd, arg_idx + 1);
            return;
        } else if (!strncmp(cmd->args[arg_idx], "avg(", 4) or
                   !strncmp(cmd->args[arg_idx], "sum(", 4) or
                   !strncmp(cmd->args[arg_idx], "count(", 6)) {
            add_select_field(cmd, cmd->args[arg_idx]);
        } else {
            cmd->type = UNRECOG_CMD;
            return;
        }
        arg_idx += 1;
    }
    cmd->type = UNRECOG_CMD;
    return;
}

void user_field_state_handler(Command_t *cmd, size_t arg_idx) {
    debugstrs("user_field_state_handler");
    cmd->cmd_args.sel_args.fields = NULL;
    cmd->cmd_args.sel_args.fields_len = 0;
    cmd->cmd_args.sel_args.limit = -1;
    cmd->cmd_args.sel_args.offset = -1;
    while (arg_idx < cmd->args_len) {
        if (!strncmp(cmd->args[arg_idx], "*", 1) or
            !strncmp(cmd->args[arg_idx], "id", 2) or
            !strncmp(cmd->args[arg_idx], "name", 4) or
            !strncmp(cmd->args[arg_idx], "email", 5) or
            !strncmp(cmd->args[arg_idx], "age", 3) or
            !strncmp(cmd->args[arg_idx], "avg(", 4) or
            !strncmp(cmd->args[arg_idx], "sum(", 4) or
            !strncmp(cmd->args[arg_idx], "count(", 6)) {

            add_select_field(cmd, cmd->args[arg_idx]);

        } else if (!strncmp(cmd->args[arg_idx], "from", 4) or
                   !strncmp(cmd->args[arg_idx], "user", 4) or
                   !strncmp(cmd->args[arg_idx], "like", 4)) {


        } else if (!strncmp(cmd->args[arg_idx], "offset", 6)) {
            offset_state_handler(cmd, arg_idx + 1);
            return;

        } else if (!strncmp(cmd->args[arg_idx], "limit", 5)) {
            limit_state_handler(cmd, arg_idx + 1);
            return;
        } else {
            cmd->type = UNRECOG_CMD;
            return;
        }
        debugstrs("user_field_state_handler_rec");
        arg_idx += 1;
    }
    cmd->type = UNRECOG_CMD;
    return;
}

void like_field_state_handler(Command_t *cmd, size_t arg_idx) {
    cmd->cmd_args.sel_args.fields = NULL;
    cmd->cmd_args.sel_args.fields_len = 0;
    cmd->cmd_args.sel_args.limit = -1;
    cmd->cmd_args.sel_args.offset = -1;
    while (arg_idx < cmd->args_len) {
        debugstr(cmd->args[arg_idx]);
        if (!strncmp(cmd->args[arg_idx], "*", 1) or
            !strncmp(cmd->args[arg_idx], "id1", 3) or
            !strncmp(cmd->args[arg_idx], "id2", 3)) {

            add_select_field(cmd, cmd->args[arg_idx]);

        } else if (!strncmp(cmd->args[arg_idx], "from", 4) or
                   !strncmp(cmd->args[arg_idx], "user", 4) or
                   !strncmp(cmd->args[arg_idx], "like", 4) or
                   !strncmp(cmd->args[arg_idx], "into", 4)) {
            continue;

        } else if (!strncmp(cmd->args[arg_idx], "avg(", 4) or
                   !strncmp(cmd->args[arg_idx], "sum(", 4) or
                   !strncmp(cmd->args[arg_idx], "count(", 6)) {
            add_select_field(cmd, cmd->args[arg_idx]);
        } else if (!strncmp(cmd->args[arg_idx], "offset", 6)) {
            offset_state_handler(cmd, arg_idx + 1);
            return;

        } else if (!strncmp(cmd->args[arg_idx], "limit", 5)) {
            limit_state_handler(cmd, arg_idx + 1);
            return;
        } else {
            cmd->type = UNRECOG_CMD;
            return;
        }
        arg_idx += 1;
    }
    cmd->type = UNRECOG_CMD;
    return;
}

void table_state_handler(Command_t *cmd, size_t arg_idx) {
    if (arg_idx < cmd->args_len
        && !strncmp(cmd->args[arg_idx], "user", 5)
        && !strncmp(cmd->args[arg_idx], "like", 5)) {

        arg_idx++;
        if (arg_idx == cmd->args_len) {
            return;
        } else if (!strncmp(cmd->args[arg_idx], "offset", 6)) {
            offset_state_handler(cmd, arg_idx + 1);
            return;
        } else if (!strncmp(cmd->args[arg_idx], "limit", 5)) {
            limit_state_handler(cmd, arg_idx + 1);
            return;
        }
    }
    cmd->type = UNRECOG_CMD;
    return;
}


void offset_state_handler(Command_t *cmd, size_t arg_idx) {
    if (arg_idx < cmd->args_len) {
        cmd->cmd_args.sel_args.offset = atoi(cmd->args[arg_idx]);

        arg_idx++;

        if (arg_idx == cmd->args_len) {
            return;
        } else if (arg_idx < cmd->args_len
                   && !strncmp(cmd->args[arg_idx], "limit", 5)) {

            limit_state_handler(cmd, arg_idx + 1);
            return;
        }
    }
    cmd->type = UNRECOG_CMD;
    return;
}

void limit_state_handler(Command_t *cmd, size_t arg_idx) {
    if (arg_idx < cmd->args_len) {
        cmd->cmd_args.sel_args.limit = atoi(cmd->args[arg_idx]);

        arg_idx++;

        if (arg_idx == cmd->args_len) {
            return;
        }
    }
    cmd->type = UNRECOG_CMD;
    return;
}
