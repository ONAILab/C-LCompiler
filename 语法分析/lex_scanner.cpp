#include "treenode.h"
#include "scanner.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <string>
#include <unordered_map>

static std::unordered_map<std::string, int> keywordMap = {
    {"main", 1}, {"if", 2}, {"then", 3}, {"while", 4}, {"do", 5},
    {"static", 6}, {"default", 7}, {"int", 8}, {"double", 9}, {"struct", 10},
    {"break", 11}, {"else", 12}, {"long", 13}, {"switch", 14}, {"case", 15},
    {"typedef", 16}, {"char", 17}, {"return", 18}, {"const", 19}, {"float", 20},
    {"short", 21}, {"continue", 22}, {"for", 23}, {"void", 24}, {"sizeof", 25}, {"goto", 26}
};

enum LexKey {
    Key_Var = 39, Key_Num = 40,
    Key_Add = 41, Key_Sub = 42, Key_Mul = 43, Key_Div = 44,
    Key_Inc = 45, Key_Dec = 46, Key_Assign = 47, Key_Ne = 48,
    Key_Le = 49, Key_Shl = 50, Key_Lt = 51, Key_Ge = 52, Key_Shr = 53, Key_Gt = 54,
    Key_Lor = 55, Key_Or = 56, Key_Lan = 58, Key_And = 59,
    Key_Xor = 60, Key_Mod = 61, Key_Brak = 62, Key_Cond = 63,
    Key_Separator = 64, Key_Other = 65, Key_Not = 66, Key_Eq = 70
};

static char *src = nullptr;
static char *line_start = nullptr;
static char *buffer = nullptr;
static int lex_line = 1;
static int lexicalErrorCount = 0;

char tokenString[MAXTOKENLEN + 1];

static TokenType mapToken(int key, const char *value) {
    if (key >= 1 && key <= 26) {
        if (strcmp(value, "if") == 0) return IF;
        if (strcmp(value, "else") == 0) return ELSE;
        if (strcmp(value, "while") == 0) return WHILE;
        if (strcmp(value, "int") == 0) return INT;
        if (strcmp(value, "return") == 0) return RETURN;
        if (strcmp(value, "void") == 0) return VOID;
        return ID;
    }
    switch (key) {
        case Key_Var: return ID;
        case Key_Num: return NUM;
        case Key_Add: return PLUS;
        case Key_Sub: return MINUS;
        case Key_Mul: return TIMES;
        case Key_Div: return OVER;
        case Key_Eq:  return EQ;
        case Key_Ne:  return NEQ;
        case Key_Lt:  return LT;
        case Key_Le:  return LEQ;
        case Key_Gt:  return GT;
        case Key_Ge:  return GEQ;
        case Key_Assign: return ASSIGN;
        case Key_Separator:
            switch (value[0]) {
                case ';': return SEMI;
                case ',': return COMMA;
                case '(': return LPAREN;
                case ')': return RPAREN;
                case '{': return LBBRACKET;
                case '}': return RBBRACKET;
                case '[': return LMBRACKET;
                case ']': return RMBRACKET;
                default:  return ERROR;
            }
        default: return ERROR;
    }
}

static void initScanner() {
    fseek(source, 0, SEEK_END);
    long size = ftell(source);
    fseek(source, 0, SEEK_SET);
    buffer = (char *)malloc(size + 1);
    if (!buffer) {
        fprintf(listing, "Out of memory for source buffer\n");
        exit(1);
    }
    size_t nread = fread(buffer, 1, size, source);
    buffer[nread] = '\0';
    src = buffer;
    line_start = buffer;
    lex_line = 1;
}

static bool scanOneToken(int &key, char *value, int &start_col) {
    int c;

    while (true) {
        start_col = src - line_start + 1;
        c = *src++;

        if (c == '\n') {
            lex_line++;
            line_start = src;
            continue;
        }
        if (c == ' ' || c == '\t' || c == '\r') {
            continue;
        }
        if (c == '#') {
            while (*src != '\0' && *src != '\n') src++;
            continue;
        }

        if (isalpha(c) || c == '_') {
            int i = 0;
            value[i++] = c;
            while (isalnum(*src) || *src == '_')
                value[i++] = *src++;
            value[i] = '\0';
            if (keywordMap.count(value))
                key = keywordMap[value];
            else
                key = Key_Var;
            return true;
        }

        if (isdigit(c)) {
            int i = 0;
            value[i++] = c;
            while (isalnum(*src) || *src == '_')
                value[i++] = *src++;
            value[i] = '\0';
            for (int k = 0; k < i; k++) {
                if (isalpha(value[k]) || value[k] == '_') {
                    fprintf(listing, "\n>>> [Lexical Error] line %d col %d: invalid identifier/number '%s' <<<\n",
                            lex_line, start_col, value);
                    lexicalErrorCount++;
                    key = Key_Other;
                    return true;
                }
            }
            key = Key_Num;
            return true;
        }

        if (c == '/') {
            if (*src == '/') {
                while (*src != '\0' && *src != '\n') src++;
                continue;
            }
            if (*src == '*') {
                src++;
                int found = 0;
                while (*src != '\0') {
                    if (*src == '\n') {
                        lex_line++;
                        line_start = src + 1;
                    }
                    if (*src == '*' && *(src + 1) == '/') {
                        src += 2;
                        found = 1;
                        break;
                    }
                    src++;
                }
                if (!found) {
                    fprintf(listing, "\n>>> [Lexical Error] line %d col %d: unclosed block comment <<<\n",
                            lex_line, start_col);
                    lexicalErrorCount++;
                    key = Key_Other;
                    return true;
                }
                continue;
            }
            key = Key_Div;
            value[0] = c;
            return true;
        }

        if (c == '\0') {
            return false;
        }

        value[0] = c;
        switch (c) {
            case '+':
                if (*src == '+') { src++; key = Key_Inc; strcpy(value, "++"); }
                else { key = Key_Add; }
                break;
            case '-':
                if (*src == '-') { src++; key = Key_Dec; strcpy(value, "--"); }
                else { key = Key_Sub; }
                break;
            case '*': key = Key_Mul; break;
            case '=':
                if (*src == '=') { src++; key = Key_Eq; strcpy(value, "=="); }
                else { key = Key_Assign; }
                break;
            case '!':
                if (*src == '=') { src++; key = Key_Ne; strcpy(value, "!="); }
                else { key = Key_Not; }
                break;
            case '<':
                if (*src == '=') { src++; key = Key_Le; strcpy(value, "<="); }
                else if (*src == '<') { src++; key = Key_Shl; strcpy(value, "<<"); }
                else { key = Key_Lt; }
                break;
            case '>':
                if (*src == '=') { src++; key = Key_Ge; strcpy(value, ">="); }
                else if (*src == '>') { src++; key = Key_Shr; strcpy(value, ">>"); }
                else { key = Key_Gt; }
                break;
            case '|':
                if (*src == '|') { src++; key = Key_Lor; strcpy(value, "||"); }
                else { key = Key_Or; }
                break;
            case '&':
                if (*src == '&') { src++; key = Key_Lan; strcpy(value, "&&"); }
                else { key = Key_And; }
                break;
            case '^': key = Key_Xor; break;
            case '%': key = Key_Mod; break;
            case '[': key = Key_Brak; break;
            case '?': key = Key_Cond; break;
            case '~': case ';': case '{': case '}': case '(': case ')':
            case ']': case ',': case ':': case '"': case '\\':
                key = Key_Separator; break;
            default:
                fprintf(listing, "\n>>> [Lexical Error] line %d col %d: illegal character '%c' (ASCII: %d) <<<\n",
                        lex_line, start_col, c, c);
                lexicalErrorCount++;
                key = Key_Other;
                break;
        }
        return true;
    }
}

static bool preChecked = false;
static bool hasError = false;

static void initLexical();

extern "C" int lexicalPassed() {
    initLexical();
    return !hasError;
}

static void initLexical() {
    if (preChecked) return;
    preChecked = true;

    initScanner();

    // Phase 1: full lexical pre-scan
    fprintf(listing, "--- Lexical Analysis Phase ---\n");
    char *save_src = src;
    char *save_line_start = line_start;
    int save_lex_line = lex_line;

    int key; char value[100]; int start_col;
    while (scanOneToken(key, value, start_col)) {
        if (key == 0 && value[0] == '\0') break;
        lineno = lex_line;
    }

    if (lexicalErrorCount > 0) {
        hasError = true;
        fprintf(listing, "\n>>> Lexical analysis found %d error(s), syntax analysis aborted.\n\n", lexicalErrorCount);
        return;
    }

    // Phase 2: reset for syntax analysis
    fprintf(listing, "--- Lexical Analysis Passed, starting Syntax Analysis ---\n");
    src = save_src;
    line_start = save_line_start;
    lex_line = save_lex_line;
}

extern "C" TokenType getToken() {
    initLexical();

    if (hasError) {
        return ENDFILE;
    }

    int key; char value[100]; int start_col;
    if (!scanOneToken(key, value, start_col)) {
        return ENDFILE;
    }

    lineno = lex_line;
    strncpy(tokenString, value, MAXTOKENLEN);
    tokenString[MAXTOKENLEN] = '\0';
    return mapToken(key, value);
}
