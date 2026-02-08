#include "json_simple.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

/**
 * @brief Procura a ocorrência de uma chave no formato "key" dentro do JSON.
 * @param json Texto JSON.
 * @param key  Nome da chave (sem aspas).
 * @return Ponteiro para a ocorrência da chave ou NULL.
 */
static const char *find_key(const char *json, const char *key)
{
    static char pat[64];
    snprintf(pat, sizeof(pat), "\"%s\"", key);
    return strstr(json, pat);
}

bool json_get_string(const char *json, const char *key, char *out, size_t outsz)
{
    const char *p = find_key(json, key);
    if (!p) return false;

    p = strchr(p, ':');
    if (!p) return false;
    p++;

    while (*p && isspace((unsigned char)*p)) p++;
    if (*p != '"') return false;
    p++;

    size_t i = 0;
    while (*p && *p != '"' && i + 1 < outsz) {
        out[i++] = *p++;
    }
    out[i] = '\0';
    return (i > 0);
}

bool json_get_int(const char *json, const char *key, int *out)
{
    const char *p = find_key(json, key);
    if (!p) return false;

    p = strchr(p, ':');
    if (!p) return false;
    p++;

    while (*p && (isspace((unsigned char)*p) || *p == '"')) p++;

    int sign = 1;
    if (*p == '-') { sign = -1; p++; }

    if (!isdigit((unsigned char)*p)) return false;

    int v = 0;
    while (*p && isdigit((unsigned char)*p)) {
        v = v * 10 + (*p - '0');
        p++;
    }
    *out = v * sign;
    return true;
}
