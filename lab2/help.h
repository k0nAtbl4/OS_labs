// Общие объявления: структура данных в разделяемой памяти
// и вспомогательные функции (ввод/вывод без stdio).

#ifndef HELP_H
#define HELP_H

#include <stddef.h>

#define BUF_SIZE 1024

// данные, которые лежат в общей памяти
struct Shared {
    size_t len_in;      // длина строки в buf_in
    size_t len_mid;     // длина строки в buf_mid
    size_t len_out;     // длина строки в buf_out
    int    done;        // флаг завершения: 1 — работа закончена
    char   buf_in[BUF_SIZE];
    char   buf_mid[BUF_SIZE];
    char   buf_out[BUF_SIZE];
};

#define SHM_SIZE ((size_t)sizeof(struct Shared))

// печать строки в дескриптор (вместо printf)
void print_str(int fd, const char *s);

// печать ошибки в stderr (вместо perror)
void print_err(const char *what);

// чтение строки из fd до '\n' или EOF (вместо fgets).
// Возвращает длину строки, -1 — EOF, -2 — ошибка.
int read_line(int fd, char *buf, size_t cap);

#endif // HELP_H
