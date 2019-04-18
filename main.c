#include <ctype.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "adthashmap.h"

#define STR_(x) #x
#define STR(x) STR_(x)
#define BITWIDTH_DEFAULT 16
#define BITWIDTH_MAX 32
#define SYMTAB_HASHBKS 1000
#define VARNAME_MAX 16
#define STR_BEGCOMM "//"
#define STR_SET "set"
#define STR_SETDM "dm"
#define STR_SETBW "bw"
#define STR_BIN "bin"
#define STR_DEC "dec"
#define STR_HEX "hex"
#define ERRSTR_SYS_GETLINE "syserror: getline failed"
#define ERRSTR_SYS_MISSINGHELP "syserror: `help.txt` not found"
#define ERRSTR_EVAL_OF "overflowed"
#define ERRSTR_EVAL_UNDEF "undefined"
#define ERRSTR_EVAL_SYNTAX "invalid expression"
#define ERRSTR_EVAL_UHEX "invalid unary operators for hex numbers"
#define ERRSTR_EVAL_VARNAME "variable undefined"
#define ERRSTR_SET_SYNTAX "invalid set option"
#define ERRSTR_SET_DMSYNTAX "invalid display mode (bin|hex|dec)"
#define ERRSTR_SET_BWSYNTAX "invalid bitwidth"
#define ERRSTR_SET_BWILLEGAL "bitwidth must be between 4 and " \
        STR(BITWIDTH_MAX) " and divisible by 4"
#define STR_HELP "help"
#define STR_HELPFILE "help.txt"

enum displaymode {BIN, HEX, DEC};
enum setstt {
    SET_NOERR,
    ERR_INVALID_DM,
    ERR_INVALID_BW,
    ERR_ILLEGAL_BW
};

typedef long long Primary;
typedef unsigned long long UPrimary;
enum token {
    ASSIGN_BOR, ASSIGN_BXOR, ASSIGN_BAND,
    ASSIGN_LSHIFT, ASSIGN_RSHIFT, ASSIGN_URSHIFT,
    ASSIGN, ASSIGN_ADD, ASSIGN_SUBTRACT,
    ASSIGN_MULTIPLY, ASSIGN_DIVIDE, ASSIGN_MODULUS,
    SHIFTLEFT, SHIFTRIGHT, USHIFTRIGHT,
    ADD, SUBTRACT,
    MULTIPLY, DIVIDE, MODULUS,
    UPLUS, UMINUS, BNOT,
};
enum evalstt {
    EVAL_NOERR,
    EVALERR_OF,
    EVALERR_UNDEF,
    EVALERR_SYNTAX,
    EVALERR_UHEX,
    EVALERR_VARNAME
};

int bw = BITWIDTH_DEFAULT;
enum displaymode dm = BIN;

Hashmap *symtab;
char *expr_errors[] = {
    "",
    ERRSTR_EVAL_OF,
    ERRSTR_EVAL_UNDEF,
    ERRSTR_EVAL_SYNTAX,
    ERRSTR_EVAL_UHEX,
    ERRSTR_EVAL_VARNAME
};
char *set_errors[] = {
    "",
    ERRSTR_SET_DMSYNTAX,
    ERRSTR_SET_BWSYNTAX,
    ERRSTR_SET_BWILLEGAL
};

/* ui functions */
void print(Primary);
void printbin(Primary);
void printhex(Primary);
void printdec(Primary);
enum setstt setdm(char **);
enum setstt setbw(char **);

/* recursive descent parsing functions */
enum evalstt assignexpr(char **, Primary *);
enum evalstt borexpr(char **, Primary *);
enum evalstt bxorexpr(char **, Primary *);
enum evalstt bandexpr(char **, Primary *);
enum evalstt shiftexpr(char **, Primary *);
enum evalstt termexpr(char **, Primary *);
enum evalstt factorexpr(char **, Primary *);
enum evalstt unaryexpr(char **, Primary *, int *);
enum evalstt primaryexpr(char **, Primary *, int *);

/* helper functions */
void skip_whitespace(char **);
Primary set_hbits(Primary);
int isoverflowed(Primary);

int main(void)
{
    char *line = NULL;
    size_t linecap;
    symtab = hmap_new(SYMTAB_HASHBKS, VARNAME_MAX, sizeof(Primary));

    for (;;) {
        char *s;
        Primary result;
        enum setstt sstt;
        enum evalstt estt;       

        switch (dm) {
        case BIN: printf("b> "); break;
        case HEX: printf("h> "); break;
        case DEC: printf("d> "); break;
        }
        if (getline(&line, &linecap, stdin) < 0) {
            if (feof(stdin)) {
                printf("^D\n");
                exit(EXIT_SUCCESS);
            }
            printf("%s\n", ERRSTR_SYS_GETLINE);
            continue;
        }
        if (strlen(line) == 1)
            continue;
        s = line;
        skip_whitespace(&s);

        /* handle comments & `help` */
        if (strncmp(s, STR_BEGCOMM, strlen(STR_BEGCOMM)) == 0)
            continue;
        if (strncmp(s, STR_HELP, strlen(STR_HELP)) == 0) {
            FILE *f;

            if ((f = fopen(STR_HELPFILE, "r")) == NULL)
                printf("%s\n", ERRSTR_SYS_MISSINGHELP);
            else
                while (getline(&line, &linecap, f) > 0)
                    printf("%s", line);
            continue;
        }

        /* handle set commands */
        if (strncmp(s, STR_SET, strlen(STR_SET)) == 0) {
            s += strlen(STR_SET);
            skip_whitespace(&s);
            if (strncmp(s, STR_SETDM, strlen(STR_SETDM)) == 0) {
                s += strlen(STR_SETDM);
                if ((sstt = setdm(&s)) != EVAL_NOERR)
                    fprintf(stderr, "%s\n", set_errors[sstt]);
            } else if (strncmp(s, STR_SETBW, strlen(STR_SETBW)) == 0) {
                s += strlen(STR_SETBW);
                if ((sstt = setbw(&s)) != EVAL_NOERR)
                    fprintf(stderr, "%s\n", set_errors[sstt]);
            } else
                fprintf(stderr, "%s\n", ERRSTR_SET_SYNTAX);
            continue;
        }

        /* evaluate */
        if ((estt = assignexpr(&s, &result)) != EVAL_NOERR) {
            fprintf(stderr, "%s\n", expr_errors[estt]);
            continue;
        }
        skip_whitespace(&s);
        if (*s != '\0')
            fprintf(stderr, "%s\n", ERRSTR_EVAL_SYNTAX);
        else
            print(result);
    }
}

void print(Primary x)
{
    if (dm == BIN)
        printbin(x);
    else if (dm == HEX)
        printhex(x);
    else
        printdec(x);
}

void printbin(Primary x)
{
    char buf[BITWIDTH_MAX];
    int i = 0, negative = x < 0 ? 1 : 0;

    if (negative)
        x = -x;
    while (x) {
        buf[i++] = (x % 2) + '0';
        x /= 2;
    }
    if (negative) {
        for (int j = 0; j < i; ++j)
            buf[j] = buf[j] == '0' ? '1' : '0';
        for (int j = 0; j < i; ++j)
            if (buf[j] == '0') {
                buf[j] = '1';
                break;
            } else
                buf[j] = '0';
    }
    for (int j = bw-1; j >= i; --j) {
        printf("%c", negative ? '1' : '0');
        if ((bw-j) % 4 == 0)
            printf(" ");
    }
    for (int j = i-1; j >= 0; --j) {
        printf("%c", buf[j]);
        if ((bw-j) % 4 == 0)
            printf(" ");
    }
    printf("\n");
}

void printhex(Primary x)
{
    UPrimary ux = *(UPrimary *) &x;
    int bitlength = bw, i = 0;
    char buf[BITWIDTH_MAX] = {'0'};

    printf("0x");
    while (bitlength && ux) {
        sprintf(buf+i, "%llx", ux & 0xf);
        ++i;
        ux >>= 4;
        bitlength -= 4;
    }
    for (; i >= 0; --i)
        printf("%c", buf[i]);
    printf("\n");
}

void printdec(Primary x)
{
    printf("%lld\n", x);
}

enum setstt setdm(char **s)
{
    skip_whitespace(s);
    if (strncmp(*s, STR_BIN, strlen(STR_BIN)) == 0)
        dm = BIN;
    else if (strncmp(*s, STR_HEX, strlen(STR_HEX)) == 0)
        dm = HEX;
    else if (strncmp(*s, STR_DEC, strlen(STR_DEC)) == 0)
        dm = DEC;
    else
        return ERR_INVALID_DM;
    return SET_NOERR;
}

enum setstt setbw(char **s)
{
    Primary newbw;

    if (borexpr(s, &newbw) != EVAL_NOERR)
        return ERR_INVALID_BW;
    if (newbw <= 0 || newbw > BITWIDTH_MAX || newbw % 4 != 0)
        return ERR_ILLEGAL_BW;
    bw = newbw;
    return SET_NOERR;
}

enum evalstt assignexpr(char **s, Primary *result)
{
    char varname[VARNAME_MAX] = "", *ss;
    Primary *varptr, varval;
    int i = 0;
    enum evalstt estt;
    enum token tk;

    ss = *s;
    skip_whitespace(s);
    while (isalpha(**s) && i < VARNAME_MAX) {
        varname[i++] = **s;
        ++*s;
    }
    varname[i] = '\0';
    skip_whitespace(s);
    if (strncmp(*s, "=", 1) == 0) { tk = ASSIGN; ++*s; }
    else if (strncmp(*s, "|=", 2) == 0) { tk = ASSIGN_BOR; *s += 2; }
    else if (strncmp(*s, "&=", 2) == 0) { tk = ASSIGN_BAND; *s += 2; }
    else if (strncmp(*s, "^=", 2) == 0) { tk = ASSIGN_BXOR; *s += 2; }
    else if (strncmp(*s, "<<=", 3) == 0) { tk = ASSIGN_LSHIFT; *s += 3; }
    else if (strncmp(*s, ">>=", 3) == 0) { tk = ASSIGN_RSHIFT; *s += 3; }
    else if (strncmp(*s, ">>>=", 4) == 0) { tk = ASSIGN_URSHIFT; *s += 4; }
    else if (strncmp(*s, "+=", 2) == 0) { tk = ASSIGN_ADD; *s += 2; }
    else if (strncmp(*s, "-=", 2) == 0) { tk = ASSIGN_SUBTRACT; *s += 2; }
    else if (strncmp(*s, "*=", 2) == 0) { tk = ASSIGN_MULTIPLY; *s += 2; }
    else if (strncmp(*s, "/=", 2) == 0) { tk = ASSIGN_DIVIDE; *s += 2; }
    else if (strncmp(*s, "%=", 2) == 0) { tk = ASSIGN_MODULUS; *s += 2; }
    else {
        *s = ss;
        return borexpr(s, result);
    }
    if (i == 0)
        return EVALERR_SYNTAX;
    if ((estt = assignexpr(s, result)) != EVAL_NOERR)
        return estt;
    if (tk != ASSIGN) {
        if ((varptr = hmap_get(symtab, varname)) == NULL)
            return EVALERR_VARNAME;
        varval = *(Primary *) varptr;
        switch (tk) {
        case ASSIGN_BOR: *result = varval | *result; break;
        case ASSIGN_BXOR: *result = varval ^ *result; break;
        case ASSIGN_BAND: *result = varval & *result; break;
        case ASSIGN_LSHIFT:
            *result = varval << *result;
            *result = set_hbits(*result);
            break;
        case ASSIGN_RSHIFT: *result = varval >> *result; break;
        case ASSIGN_URSHIFT:
            *result = (varval & ~(-1LL << bw)) >> *result;
            break;
        case ASSIGN_ADD: *result = varval + *result; break;
        case ASSIGN_SUBTRACT: *result = varval - *result; break;
        case ASSIGN_MULTIPLY: *result = varval * *result; break;
        case ASSIGN_DIVIDE:
            if (*result == 0)
                return EVALERR_UNDEF;
            *result = varval / *result;
            break;
        case ASSIGN_MODULUS:
            if (*result == 0)
                return EVALERR_UNDEF;
            *result = varval % *result;
            break;
        default: ;
        }
    }
    if (isoverflowed(*result))
        return EVALERR_OF;
    hmap_add(symtab, varname, result);
    return EVAL_NOERR;
}

enum evalstt borexpr(char **s, Primary *result)
{
    Primary left, right;
    enum evalstt estt;

    if ((estt = bxorexpr(s, &left)) != EVAL_NOERR)
        return estt;
    for (;;) {
        skip_whitespace(s);
        if (strncmp(*s, "|", 1) != 0) {
            if (isoverflowed(left))
                return EVALERR_OF;
            *result = left;
            return EVAL_NOERR;
        }
        ++*s;
        if ((estt = bxorexpr(s, &right)) != EVAL_NOERR)
            return estt;
        left |= right;
    }
}

enum evalstt bxorexpr(char **s, Primary *result)
{
    Primary left, right;
    enum evalstt estt;

    if ((estt = bandexpr(s, &left)) != EVAL_NOERR)
        return estt;
    for (;;) {
        skip_whitespace(s);
        if (strncmp(*s, "^", 1) != 0) {
            *result = left;
            return EVAL_NOERR;
        }
        ++*s;
        if ((estt = bandexpr(s, &right)) != EVAL_NOERR)
            return estt;
        left ^= right;
    }
}

enum evalstt bandexpr(char **s, Primary *result)
{
    Primary left, right;
    enum evalstt estt;

    if ((estt = shiftexpr(s, &left)) != EVAL_NOERR)
        return estt;
    for (;;) {
        skip_whitespace(s);
        if (strncmp(*s, "&", 1) != 0) {
            *result = left;
            return EVAL_NOERR;
        }
        ++*s;
        if ((estt = shiftexpr(s, &right)) != EVAL_NOERR)
            return estt;
        left &= right;
    }
}

enum evalstt shiftexpr(char **s, Primary *result)
{
    Primary left, right;
    enum evalstt estt;

    if ((estt = termexpr(s, &left)) != EVAL_NOERR)
        return estt;
    for (;;) {
        enum token tk;

        skip_whitespace(s);
        if (strncmp(*s, "<<", 2) == 0) { tk = SHIFTLEFT; *s += 2; }
        else if (strncmp(*s, ">>>", 3) == 0) { tk = USHIFTRIGHT; *s += 3; }
        else if (strncmp(*s, ">>", 2) == 0) { tk = SHIFTRIGHT; *s += 2; }
        else {
            *result = left;
            return EVAL_NOERR;
        }
        if ((estt = termexpr(s, &right)) != EVAL_NOERR)
            return estt;
        if (right < 0)
            return EVALERR_UNDEF;
        if (tk == SHIFTLEFT) {
            /* Shifting by more than the current bit width returns 0. */
            left <<= right;
            left = set_hbits(left);
        }
        else if (tk == USHIFTRIGHT)
            left = (left & ~(-1LL << bw)) >> right;
        else
            left >>= right;
    }
}

enum evalstt termexpr(char **s, Primary *result)
{
    Primary left, right;
    enum evalstt estt;

    if ((estt = factorexpr(s, &left)) != EVAL_NOERR)
        return estt;
    for (;;) {
        enum token tk;

        skip_whitespace(s);
        if (strncmp(*s, "+", 1) == 0) { tk = ADD; ++*s; }
        else if (strncmp(*s, "-", 1) == 0) { tk = SUBTRACT; ++*s; }
        else {
            *result = left;
            return EVAL_NOERR;
        }
        if ((estt = factorexpr(s, &right)) != EVAL_NOERR)
            return estt;
        if (tk == ADD)
            left += right;
        else
            left -= right;
    }
}

enum evalstt factorexpr(char **s, Primary *result)
{
    Primary left, right;
    int dummy;  /* dummy unused variable */
    enum evalstt estt;

    if ((estt = unaryexpr(s, &left, &dummy)) != EVAL_NOERR)
        return estt;
    for (;;) {
        enum token tk;

        skip_whitespace(s);
        if (strncmp(*s, "*", 1) == 0) { tk = MULTIPLY; ++*s; }
        else if (strncmp(*s, "/", 1) == 0) { tk = DIVIDE; ++*s; }
        else if (strncmp(*s, "%", 1) == 0) { tk = MODULUS; ++*s; }
        else {
            *result = left;
            return EVAL_NOERR;
        }
        if ((estt = unaryexpr(s, &right, &dummy)) != EVAL_NOERR)
            return estt;
        if (tk == MULTIPLY)
            left *= right;
        else {
            if (right == 0)
                return EVALERR_UNDEF;
            if (tk == DIVIDE)
                left /= right;
            else
                left %= right;
        }
    }
}

enum evalstt unaryexpr(char **s, Primary *result, int *ishex)
{
    enum evalstt estt;

    for (;;) {
        enum token tk;

        skip_whitespace(s);
        if (strncmp(*s, "+", 1) == 0) { tk = UPLUS; ++*s; }
        else if (strncmp(*s, "-", 1) == 0) { tk = UMINUS; ++*s; }
        else if (strncmp(*s, "~", 1) == 0) { tk = BNOT; ++*s; }
        else {
            if ((estt = primaryexpr(s, result, ishex)) != EVAL_NOERR)
                return estt;
            return EVAL_NOERR;
        }
        if (tk == UPLUS) {
            if ((estt = unaryexpr(s, result, ishex)) != EVAL_NOERR)
                return estt;
            if (*ishex)
                return EVALERR_UHEX;
            return EVAL_NOERR;
        } else if (tk == UMINUS) {
            if ((estt = unaryexpr(s, result, ishex)) != EVAL_NOERR)
                return estt;
            if (*ishex)
                return EVALERR_UHEX;
            *result = -*result;
            return EVAL_NOERR;
        } else {
            if ((estt = unaryexpr(s, result, ishex)) != EVAL_NOERR)
                return estt;
            *result = ~*result;
            return EVAL_NOERR;
        }
    }
}

enum evalstt primaryexpr(char **s, Primary *result, int *ishex)
{
    char lastxdigit, varname[VARNAME_MAX] = "";
    Primary *varval;
    int noprimary = 1, bitlength = 0, isvar = 0, i = 0;
    enum evalstt estt;

    skip_whitespace(s);
    *ishex = 0;
    if (**s == '(') {
        ++*s;
        if ((estt = assignexpr(s, result)) != EVAL_NOERR)
            return estt;
        skip_whitespace(s);
        if (**s != ')')
            return EVALERR_SYNTAX;
        ++*s;
        return EVAL_NOERR;
    }

    /* handle hex */
    if (strncmp(*s, "0x", 2) == 0 || strncmp(*s, "0X", 2) == 0) {
        *ishex = 1;
        *s += 2;
        if (isdigit(**s))
            lastxdigit = **s;
        else
            lastxdigit = tolower(**s);
    }
    *result = 0;
    while (*ishex && isxdigit(**s)) {
        noprimary = 0;
        if (bitlength == bw)
            return EVALERR_OF;
        if (isdigit(**s))
            *result = *result*16 + (**s - '0');
        else
            *result = *result*16 + (10 + **s - 'a');
        bitlength += 4;
        ++*s;
    }
    if (bitlength == bw && (lastxdigit == '8' || lastxdigit =='9' ||
                lastxdigit >= 'a'))
        *result -= 2*pow(2, bw-1);
    
    /* handle decimal */
    while (!*ishex && isdigit(**s)) {
        noprimary = 0;
        *result = *result*10 + (**s - '0');
        ++*s;
    }
    
    /* handle variable */
    while (isalpha(**s) && i < VARNAME_MAX) {
        noprimary = 0;
        isvar = 1;
        varname[i++] = **s;
        ++*s;
    }
    if (isvar) {
        varname[i] = '\0';
        if ((varval = hmap_get(symtab, varname)) == NULL)
            return EVALERR_VARNAME;
        else
            *result = *(Primary *) varval;
    }

    if (noprimary)
        return EVALERR_SYNTAX;
    return EVAL_NOERR;
}

int isoverflowed(Primary x)
{
    if (x < 0)
        return x < -pow(2, bw-1);
    return x > pow(2, bw-1)-1;
}

Primary set_hbits(Primary x)
{
    if (x & (1 << (bw-1)))
        return x |= (-1LL << bw);
    else
        return x &= ~(-1LL << bw);
}

void skip_whitespace(char **s)
{
    while (isspace(**s))
        ++*s;
}
