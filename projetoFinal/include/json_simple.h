#ifndef JSON_SIMPLE_H
#define JSON_SIMPLE_H

/**
 * @file json_simple.h
 * @brief Parser JSON simples (sem biblioteca externa).
 *
 * Observação: este parser é propositalmente simples e assume payloads pequenos e bem-formados.
 * Ele procura por "chaves" e extrai string/int. Não valida JSON completo.
 */

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Extrai o valor de uma chave JSON do tipo string.
 * @param json     Texto JSON.
 * @param key      Nome da chave (sem aspas).
 * @param out      Buffer de saída.
 * @param outsz    Tamanho do buffer de saída.
 * @return true se encontrou e extraiu; false caso contrário.
 */
bool json_get_string(const char *json, const char *key, char *out, size_t outsz);

/**
 * @brief Extrai o valor de uma chave JSON do tipo inteiro.
 * @param json     Texto JSON.
 * @param key      Nome da chave (sem aspas).
 * @param out      Ponteiro para receber o inteiro.
 * @return true se encontrou e extraiu; false caso contrário.
 */
bool json_get_int(const char *json, const char *key, int *out);

#ifdef __cplusplus
}
#endif

#endif // JSON_SIMPLE_H
