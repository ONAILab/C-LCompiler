#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <ctype.h>
#include <unordered_map>
#include <string>
#define MAXSIZE 10240 

using namespace std;

int token;            // 当前标记
char *src;         // 指向源代码字符串指针
int poolsize;         // 默认文本/数据大小
int line = 1;        // 源码行号
char *line_start;    // 源码当前行的起始指针，用于计算列号（第几个字符）

// 哈希表
unordered_map<string, int> keywordMap = {
    {"main", 1}, {"if", 2}, {"then", 3}, {"while", 4}, {"do", 5},
    {"static", 6}, {"default", 7}, {"int", 8}, {"double", 9}, {"struct", 10},
    {"break", 11}, {"else", 12}, {"long", 13}, {"switch", 14}, {"case", 15},
    {"typedef", 16}, {"char", 17}, {"return", 18}, {"const", 19}, {"float", 20},
    {"short", 21}, {"continue", 22}, {"for", 23}, {"void", 24}, {"sizeof", 25}, {"goto", 26}
};

enum TokenKey {
    Key_Var = 39, Key_Num = 40,
    Key_Add = 41, Key_Sub = 42, Key_Mul = 43, Key_Div = 44,
    Key_Inc = 45, Key_Dec = 46, Key_Assign = 47, Key_Ne = 48,
    Key_Le = 49, Key_Shl = 50, Key_Lt = 51, Key_Ge = 52, Key_Shr = 53, Key_Gt = 54,
    Key_Lor = 55, Key_Or = 56, Key_Lan = 58, Key_And = 59,
    Key_Xor = 60, Key_Mod = 61, Key_Brak = 62, Key_Cond = 63,
    Key_Separator = 64, Key_Other = 65, Key_Not = 66, Key_Eq = 70
};

//能够支持的标记符类型
enum TokenType {
    Num = 128, Fun, Sys, Glo, Loc, Id,
    Char, Else, Enum, If, Int, Return, Sizeof, While,
    Assign, Cond, Lor, Lan, Or, Xor, And, Eq, Ne, Lt, Gt, Le, Ge, Shl, Shr, Add, Sub, Mul, Div, Mod, Inc, Dec, Brak,
    Var, Separator, Keyw, Other, Not
};

typedef struct node{
    int key;
    int type;
    int col;
    char value[100];
    struct node *next;
}node;

//用于词法分析，获取下一个标记，它将自动忽略空白字符。
struct node* next(struct node *p){
    int key = 0, type = Other, i, j;
    char value[100];
    int start_col = 0;
    memset(value, 0, sizeof(value));

    while (1) {   // 跳过不识别的字符以及处理空白字符
        start_col = src - line_start + 1; // 记录当前读取字符的首记列号
        token = *src++;
        
        if (token == '\n') {    // 处理换行符
            ++line;
            line_start = src; // 更新新一行的起点
            continue;
        } else if (token == ' ' || token == '\t' || token == '\r') { // 处理空白字符
            continue;
        } else if (token == '#') {   // 处理宏定义
            while (*src != '\0' && *src != '\n') src++;
            continue;
        } else if (isalpha(token) || token == '_') { // 解析标识符和关键字
            i = 0;
            value[i++] = token;
            while (isalnum(*src) || *src == '_') {
                value[i++] = *src++;
            }
            value[i] = '\0';
            
            // 使用哈希表查找关键字，若存在则返回对应的键值，否则默认为变量
            if (keywordMap.count(value)) {
                key = keywordMap[value];
                type = Keyw;
            } else {
                key = Key_Var;
                type = Var;
            }
            break;
        } else if (isdigit(token)) { // 解析数字
            i = 0;
            value[i++] = token;
            while (isalnum(*src) || *src == '_') {
                value[i++] = *src++;
            }
            value[i] = '\0';
            
            // 检查数字序列中是否夹杂字母或下划线，作为非法标识符报错
            int validNum = 1;
            for(int k=0; k<i; k++) {
                if(isalpha(value[k]) || value[k] == '_') { validNum = 0; break; }
            }
            if(!validNum) {
                printf("\n>>>  [词法错误] 第%d行第%d个字符: 非法标识符/数字格式 '%s' <<<\n", line, start_col, value);
                key = Key_Other;
                type = Other;
                break;
            }
            key = Key_Num;
            type = Num;
            break;
        } else if (token == '/') { // 处理注释或除号
            if (*src == '/') { // 单行注释
                while (*src != '\0' && *src != '\n') src++;
                continue;
            } else if (*src == '*') { // 多行注释块 /* ... */
                src++;
                int found = 0;
                while (*src != '\0') {
                    if (*src == '\n') {
                        ++line;
                        line_start = src + 1; // 跨行则更新行起点
                    }
                    if (*src == '*' && *(src + 1) == '/') {
                        src += 2; // 跳出块注释
                        found = 1;
                        break;
                    }
                    src++;
                }
                if (!found) {
                    printf("\n>>>  [词法错误] 第%d行第%d个字符: 多行注释未闭合 <<<\n", line, start_col);
                    key = Key_Other; type = Other;
                    break;
                }
                continue;
            } else {
                key = Key_Div; type = Div; value[0] = token; break;
            }
        } 
        else if (token == '\0') {
            break; // 结束符
        }
        
        // 处理其他运算符和分隔符
        value[0] = token;
        value[1] = '\0';
        
        switch (token) {
            case '+': 
                if (*src == '+') { src++; key = Key_Inc; type = Inc; strcpy(value, "++"); } 
                else { key = Key_Add; type = Add; } 
                break;
            case '-': 
                if (*src == '-') { src++; key = Key_Dec; type = Dec; strcpy(value, "--"); } 
                else { key = Key_Sub; type = Sub; } 
                break;
            case '*': key = Key_Mul; type = Mul; break;
            case '=': 
                if (*src == '=') { src++; key = Key_Eq; type = Eq; strcpy(value, "=="); } 
                else { key = Key_Assign; type = Assign; } 
                break;
            case '!': 
                if (*src == '=') { src++; key = Key_Ne; type = Ne; strcpy(value, "!="); } 
                else { key = Key_Not; type = Not; } 
                break;
            case '<': 
                if (*src == '=') { src++; key = Key_Le; type = Le; strcpy(value, "<="); } 
                else if (*src == '<') { src++; key = Key_Shl; type = Shl; strcpy(value, "<<"); } 
                else { key = Key_Lt; type = Lt; } 
                break;
            case '>': 
                if (*src == '=') { src++; key = Key_Ge; type = Ge; strcpy(value, ">="); } 
                else if (*src == '>') { src++; key = Key_Shr; type = Shr; strcpy(value, ">>"); } 
                else { key = Key_Gt; type = Gt; } 
                break;
            case '|': 
                if (*src == '|') { src++; key = Key_Lor; type = Lor; strcpy(value, "||"); } 
                else { key = Key_Or; type = Or; } 
                break;
            case '&': 
                if (*src == '&') { src++; key = Key_Lan; type = Lan; strcpy(value, "&&"); } 
                else { key = Key_And; type = And; } 
                break;
            case '^': key = Key_Xor; type = Xor; break;
            case '%': key = Key_Mod; type = Mod; break;
            case '[': key = Key_Brak; type = Brak; break;
            case '?': key = Key_Cond; type = Cond; break;
            case '~': case ';': case '{': case '}': case '(': case ')': 
            case ']': case ',': case ':': case '"': case '\\':
                key = Key_Separator; type = Separator; break;
            default:
                printf("\n>>>  [词法错误] 第%d行第%d个字符出现非法字符: '%c' (ASCII: %d) <<<\n", line, start_col, token, token);
                key = Key_Other; type = Other;
                break;
        }
        break; 
    }

    p->key = key;
    p->col = start_col;
    strcpy(p->value, value);
    p->type = type;
    
    node *q = (node *)malloc(sizeof(node));
    q->next = NULL;
    p->next = q;
    return p;
}

//词法分析入口
void program(struct node *p, FILE *fd2){
    printf("词法分析产生的词法二元式为: \n");
    p=next(p);                 
    int tmp=0; 
    while (token > 0){
    	tmp++;
        printf("<%d , %d , %d , %s>    ",line, p->col, p->key, p->value);
        if(tmp%5==0)printf("\n");
        fprintf(fd2,"<%d , %d , %d , %s >\n",line, p->col, p->key, p->value);
        p=p->next;
        next(p);
    }
    printf("\n\n数据已写入output.txt\n\n");
}


int main(void){
    int i;
    FILE *fd1,*fd2;
    char path[50];
    struct node *head;

    head = (node *)malloc(sizeof(node));
    head->next = NULL;

    printf("请输入需要进行词法分析的文件路径： ");
    scanf("%49s", path);

    if ((fd1 = fopen(path, "r")) == NULL){
        printf("could not open(%s)\n", path);
        return -1;
    }
    if ((fd2 = fopen(".\\output.txt", "w")) == NULL){
        printf("could not open(%s)\n", path);
        return -1;
    }

    if (!(src = (char *)malloc(MAXSIZE))){
        printf("could not malloc(%d) for source area\n", poolsize);
        return -1;
    }
    // 读取源文件
    if ((i = fread(src,1,MAXSIZE,fd1)) <= 0){
        printf("read() returned %d\n", i);
        return -1;
    }
    src[i] = 0;  // 结束符
    line_start = src; // 赋予按行追踪的首位标识指针
    fclose(fd1);
    program(head,fd2);
    fclose(fd2);
    return 0;
}
