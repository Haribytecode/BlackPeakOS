#pragma once
void panic(const char *msg);
void panic_at(const char *msg, const char *file, int line);
#define PANIC(msg) panic_at((msg), __FILE__, __LINE__)