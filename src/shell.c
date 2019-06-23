#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "InputBuffer.h"
#include "Util.h"
#include "Table.h"

#define not !
#define or ||
#define debugstr(str) printf("%s", str)
#define debug(v) printf("%f", v)

int main(int argc, char **argv) {
    InputBuffer_t *input_buffer = new_InputBuffer();
    Command_t *cmd = new_Command();
    State_t *state = new_State();
    Table_t *table_users = NULL;
    Table_like_t *table_likes = NULL;

    table_users = new_Table(argc != 2 ? NULL:argv[1]);
    table_likes = new_like_Table(NULL);
    if (table_users == NULL or table_likes == NULL) return 1;

    int cmd_type;
    for (;;) {
        print_prompt(state);
        read_input(input_buffer);
        cmd_type = parse_input(input_buffer->buffer, cmd);
        if (cmd_type == BUILT_IN_CMD) {
            handle_builtin_cmd(table_users, cmd, state);
        } else if (cmd_type == QUERY_CMD) {
            if (!strncmp(cmd->args[2],"user",4) or !strncmp(cmd->args[3],"user",4)){
                handle_user_query_cmd(table_users, cmd);
            }
            else if (!strncmp(cmd->args[2],"like",4) or !strncmp(cmd->args[3],"like",4)){
                debugstr("QC1");
                handle_like_query_cmd(table_likes, cmd);
            }
        } else if (cmd_type == UNRECOG_CMD) {
            printf("Unrecognized command '%s'.\n", input_buffer->buffer);
        }
        cleanup_Command(cmd);
        clean_InputBuffer(input_buffer);
    }
    return 0;
}
