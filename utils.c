#include "utils.h"

void read_line(char *buf, int size) { // to take input from user and store it in buf
    if (fgets(buf, size, stdin)) 
    str_strip(buf);
    else buf[0] = '\0';
}


int read_int(const char *prompt) {
    char buf[32]; 
    int v;
    while (1) {
        printf("%s", prompt); 
        fflush(stdout);
        read_line(buf, sizeof(buf));
        if (sscanf(buf, "%d", &v) == 1) return v;
        printf("  [!] Enter an integer.\n");
    }
}

float read_float(const char *prompt) {
    char buf[32]; float v;
    while (1) {
        printf("%s", prompt); 
        fflush(stdout);
        read_line(buf, sizeof(buf));
        if (sscanf(buf, "%f", &v) == 1) return v;
        printf("  [!] Enter a number.\n");
    }
}

void str_strip(char *s) {
    size_t n = strlen(s);
    while (n > 0 && (s[n-1]=='\n'||s[n-1]=='\r'||s[n-1]==' ')) s[--n]='\0';
}

void str_lower(char *s) {
    for (; *s; s++) *s = (char)tolower((unsigned char)*s);
}

int str_contains_ci(const char *hay, const char *needle) {
    char h[512], n[256];
    strncpy(h, hay,    sizeof(h)-1); 
    h[sizeof(h)-1]='\0';
    strncpy(n, needle, sizeof(n)-1); 
    n[sizeof(n)-1]='\0';
    str_lower(h); 
    str_lower(n);
    return strstr(h, n) != NULL;
}

int date_valid(const char *d) {
    if (!d || strlen(d) < 16) return 0;
    for (int i = 0; i < 16; i++) {
        if (i==4||i==7) { if (d[i]!='-') return 0; }
        else if (i==10) { if (d[i]!=' ') return 0; }
        else if (i==13) { if (d[i]!=':') return 0; }
        else { if (!isdigit((unsigned char)d[i])) return 0; }
    }
    return 1;
}

void print_sep(char c, int w) {
    for (int i=0;i<w;i++) putchar(c);
    putchar('\n');
}

void print_header(const char *t) {
    printf("\n"); 
    print_sep('=',62);
    printf("  %s\n",t); 
    print_sep('=',62);
}

void print_section(const char *t) {
    printf("\n"); 
    print_sep('-',52);
    printf("  %s\n",t); 
    print_sep('-',52);
}
