#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

void  read_line(char *buf, int size);
int   read_int(const char *prompt);
float read_float(const char *prompt);
void  str_strip(char *s);
void  str_lower(char *s);
int   str_contains_ci(const char *hay, const char *needle);
int   date_valid(const char *d);
void  print_sep(char c, int w);
void  print_header(const char *t);
void  print_section(const char *t);

#endif
