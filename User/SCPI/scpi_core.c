#include "scpi_core.h"

#include <ctype.h>
#include <string.h>
#include <stdio.h>

/* 命令表由业务层提供 */
extern const SCPI_Entry_t scpi_commands[];
extern const uint32_t scpi_commands_count;

static int scpi_char_icmp(char a, char b)
{
    return (tolower((unsigned char)a) - tolower((unsigned char)b));
}

static int scpi_strnicmp(const char *a, const char *b, uint32_t n)
{
    uint32_t i;
    for (i = 0; i < n; i++)
    {
        char ca = a[i];
        char cb = b[i];
        if (ca == '\0' || cb == '\0')
        {
            return (unsigned char)ca - (unsigned char)cb;
        }
        if (scpi_char_icmp(ca, cb) != 0)
        {
            return scpi_char_icmp(ca, cb);
        }
    }
    return 0;
}

static void scpi_trim_eol(char *s)
{
    size_t n;
    if (s == 0)
        return;
    n = strlen(s);
    while (n > 0)
    {
        char c = s[n - 1];
        if (c == '\r' || c == '\n' || c == ' ' || c == '\t')
        {
            s[n - 1] = '\0';
            n--;
        }
        else
        {
            break;
        }
    }
}

static char *scpi_lskip(char *s)
{
    while (s && (*s == ' ' || *s == '\t'))
    {
        s++;
    }
    return s;
}

void SCPI_Parse(char *input_string)
{
    char *p;
    char *cmd_start;
    char *args;
    uint32_t i;
    size_t cmd_len;

    if (input_string == 0)
    {
        return;
    }

    /* 允许输入携带 CRLF，先裁剪尾部 */
    scpi_trim_eol(input_string);

    p = scpi_lskip(input_string);
    cmd_start = p;
    args = 0;

    /* 找到命令 token 结束位置（空白分隔） */
    while (*p != '\0')
    {
        if (*p == ' ' || *p == '\t')
        {
            *p = '\0';
            p++;
            args = scpi_lskip(p);
            break;
        }
        p++;
    }

    cmd_len = strlen(cmd_start);
    if (cmd_len == 0)
    {
        return;
    }

    for (i = 0; i < scpi_commands_count; i++)
    {
        const char *entry_cmd = scpi_commands[i].command;
        size_t entry_len;
        if (entry_cmd == 0 || scpi_commands[i].callback == 0)
            continue;

        entry_len = strlen(entry_cmd);
        if (entry_len != cmd_len)
            continue;

        if (scpi_strnicmp(cmd_start, entry_cmd, (uint32_t)cmd_len) == 0)
        {
            scpi_commands[i].callback(args);
            return;
        }
    }

    /* 未匹配：业务层可选实现一个 UNKNOWN 处理，这里保持静默 */
}


