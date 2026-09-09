#include "console.h"

static TaskHandle_t console_task_handle = NULL;
console_cmd_ex_t *command_list = NULL;
size_t command_count = 0;

// Private methods

static void clear_completions(linenoiseCompletions *lc) {
    if (!lc) return;

    for (size_t i = 0; i < lc->len; i++) free(lc->cvec[i]);
    free(lc->cvec);

    lc->cvec = NULL;
    lc->len = 0;  
}

static void get_completions(const char *buf, linenoiseCompletions *lc) {
    esp_console_get_completion(buf, lc);
    if (lc->len > 0) return;

    if (!command_list || !buf || !*buf) return;
    const char *cmd = buf;
    size_t cmd_len = strcspn(buf, " \t");
    if (cmd_len == 0) return;
    
    for (size_t i = 0; i < command_count; i++) {
        console_cmd_ex_t *c = &command_list[i];
        if (!c->arg_completion || !c->cmd.argtable) continue;

        if (strlen(c->cmd.command) == cmd_len && strncmp(cmd, c->cmd.command, cmd_len) == 0) {
            c->arg_completion(buf, lc);
            if (lc->len < 2) return;

            // find common prefix for all arguments
            const char *first = lc->cvec[0];
            size_t prefix_len = strlen(first);

            for (size_t j = 1; j < lc->len; j++) {
                size_t k = 0;
                const char *cur = lc->cvec[j];
                while (k < prefix_len && cur[k] == first[k]) k++;
                prefix_len = k;
                if (prefix_len == 0) {
                    // no common prefix found
                    clear_completions(lc);
                    return;
                }
            }

            char common[prefix_len + 1];
            memcpy(common, first, prefix_len);
            common[prefix_len] = '\0';

            clear_completions(lc);
            linenoiseAddCompletion(lc, common);
            return;
        }
    }
}

static char *inline_hint(const char *buf, int *color, int *bold) {
    *color = 90; // gray (ANSI)
    *bold = 0; // normal (ANSI)

    linenoiseCompletions lc = {0, NULL};
    get_completions(buf, &lc);

    static char hint_buf[128];
    hint_buf[0] = 0;

    if (lc.len == 1) {
        const char *completion = lc.cvec[0];
        cli_split_t line_split = cli_split_last_token(buf);
        cli_split_t completion_split = cli_split_last_token(completion);

        if (completion_split.token_len > line_split.token_len) {
            strncpy(hint_buf, completion_split.token + line_split.token_len, sizeof(hint_buf)-1);
            hint_buf[sizeof(hint_buf)-1] = '\0';
        }
    }

    clear_completions(&lc);
    return hint_buf[0] ? hint_buf : NULL;
}

static void initialize_console_uart(void) {
    const uart_config_t uart_config = {
        .baud_rate = CONFIG_ESP_CONSOLE_UART_BAUDRATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
    #if SOC_UART_SUPPORT_REF_TICK
        .source_clk = UART_SCLK_REF_TICK,
    #else
        .source_clk = UART_SCLK_XTAL,
    #endif
    };

    ESP_ERROR_CHECK(uart_driver_install(CONFIG_ESP_CONSOLE_UART_NUM, 256, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(CONFIG_ESP_CONSOLE_UART_NUM, &uart_config));
    uart_vfs_dev_use_driver(CONFIG_ESP_CONSOLE_UART_NUM);

    setvbuf(stdin, NULL, _IONBF, 0);
}

static void initialize_console_library(void) {
    esp_console_config_t console_config = {
        .max_cmdline_args = 8,
        .max_cmdline_length = 128
    };
    ESP_ERROR_CHECK(esp_console_init(&console_config));

    linenoiseSetMultiLine(1);
    linenoiseAllowEmpty(false);
    linenoiseHistorySetMaxLen(50);
    linenoiseSetHintsCallback(inline_hint);
    linenoiseSetCompletionCallback(get_completions);
    linenoiseSetMaxLineLen(console_config.max_cmdline_length);\

    if (linenoiseProbe()) linenoiseSetDumbMode(1);
}

static void print_welcome_message() {
    if (linenoiseIsDumbMode()) {
        printf(
            "\n============================================\n"
            "   Welcome to PWrInSpace CLI!\n"
            "   Your terminal does NOT support ANSI escape sequences, so advanced features such as:\n"
            "     - command history,\n"
            "     - tab-based command completion,\n"
            "     - colored and formatted output,\n"
            "   are unavailable.\n"
            "   On Windows, try using Windows Terminal or Putty instead.\n"
            "============================================\n"
        );
    } else {
        printf(
            "\n============================================\n"
            "   Welcome to PWrInSpace CLI!\n"
            "   Type 'help' to see available commands.\n"
            "   Use UP/DOWN arrows to navigate through command history.\n"
            "   Press TAB when typing command to auto-complete.\n"
            "============================================\n"
        );
    }  
}

static void console_task(void *arg) {
    print_welcome_message();

    while (1) {
        char* line = linenoise(PREFIX);
        if (line == NULL) continue;

        if (strlen(line) > 0) { 
            linenoiseHistoryAdd(line);

            int ret_code;
            esp_err_t err = esp_console_run(line, &ret_code);
            if (err == ESP_ERR_NOT_FOUND) {
                printf("Unrecognized command\n");
            } else if (err == ESP_OK && ret_code != ESP_OK) {
                printf("Command returned non-zero error code: 0x%x (%s)\n", ret_code, esp_err_to_name(ret_code));
            } else if (err != ESP_OK) {
                printf("Internal error: %s\n", esp_err_to_name(err));
            }

            linenoiseFree(line);
        }
    }
}

// Public methods

esp_err_t console_init(void) {
    initialize_console_uart();
    initialize_console_library();

    ESP_ERROR_CHECK(esp_console_register_help_command());

    BaseType_t task_ret = xTaskCreate(console_task, "console_task", 4096, NULL, 5, &console_task_handle);
    return (task_ret == pdPASS) ? ESP_OK : ESP_FAIL;
}

esp_console_cmd_t *find_cmd_by_name(const char *name) {
    for (int i = 0; i < command_count; i++) {
        if (strcmp(command_list[i].cmd.command, name) == 0) return &command_list[i].cmd;
    }

    return NULL;
}

esp_err_t print_cmd_usage(const char *name) {
    const esp_console_cmd_t *cmd = find_cmd_by_name(name);

    if (cmd) {
        printf("Usage: %s", cmd->command);
        if (cmd->argtable != NULL) arg_print_syntax(stdout, (void **) cmd->argtable, "\n");

        return ESP_OK;
    }

    return ESP_ERR_NOT_FOUND;
}

cli_split_t cli_split_last_token(const char *line) {
    cli_split_t out = {0};
    if (!line) return out;

    size_t len = strlen(line);
    const char *end = line + len;

    const char *token_end = end;
    while (token_end > line && *(token_end - 1) == ' ')
        token_end--;

    const char *token_start = token_end;
    while (token_start > line && *(token_start - 1) != ' ')
        token_start--;

    out.prefix = line;
    out.prefix_len = token_start - line;
    out.token = token_start;
    out.token_len = token_end - token_start;

    return out;
}

esp_err_t console_register_commands(console_cmd_ex_t *commands, size_t number_of_cmd) {
    if (!commands || number_of_cmd == 0) return ESP_ERR_INVALID_ARG;

    esp_err_t ret = ESP_OK;
    for (int i = 0; i < number_of_cmd; i++) {
        ret = esp_console_cmd_register(&commands[i].cmd);
        if (ret != ESP_OK) {
            return ret;
        }
    }

    command_list = commands;
    command_count = number_of_cmd;
    return ret;
}

esp_err_t console_deinit(void) {
    command_list = NULL;
    command_count = 0;

    if (console_task_handle != NULL) {
        vTaskDelete(console_task_handle);
        console_task_handle = NULL;
    }

    linenoiseSetCompletionCallback(NULL);
    linenoiseSetHintsCallback(NULL);
    return esp_console_deinit();
}