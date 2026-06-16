#include "treenode.h"
#include "scanner.h"
#include "util.h"
#define BUFLEN 256   //����ÿ�ζ�ȡ��һ�д��볤��

typedef enum
{
    START,INASSIGN,INEQ,INLE,INGE,INNEQ,ENTERCOMMENT,LEAVECOMMENT,INCOMMENT,INNUM,INID,DONE
}StateType;

//�����ֽṹ�嶨��͸�ֵ
static struct
{
    char* str;
    TokenType token;
}reservedWords[MAXRESERVED]
={{"else",ELSE},{"if",IF},{"int",INT},{"return",RETURN},{"void",VOID},{"while",WHILE}};


char tokenString[MAXTOKENLEN+1];

static char lineBuf[BUFLEN];   //�洢��ǰ��
static int linepos=0;    //��ǰ���������ַ�λ��
static int bufsize=0;     //��ǰ���ַ�������
static int EOF_Flag =false;

static int getNextChar()    //��ȡ��һ���ַ�
{
    if(linepos>=bufsize)
    {
        lineno++;
        if(fgets(lineBuf,BUFLEN-1,source))
        {
            if(EchoSource)
                fprintf(listing,"%4d: %s",lineno,lineBuf);
            bufsize=strlen(lineBuf);
            linepos=0;
            return lineBuf[linepos++];
        }
        else
        {
            EOF_Flag=true;
            return EOF;
        }
    }
    else
    {
        return lineBuf[linepos++];
    }
}

static void ungetNextChar()     //�����������⣬�����ַ�
{
    if(!EOF_Flag)
        linepos--;
}

static TokenType reservedLookup(char*s)   //��ѯ�Ǻ��Ƿ��Ǳ�����
{
    int i;
    for(i=0;i<MAXRESERVED;i++)
        if(!strcmp(s,reservedWords[i].str))
        return reservedWords[i].token;
    return ID;
}

TokenType getToken()    
{
    int tokenStringIndex=0;
    TokenType currentToken;
    StateType state=START;
    int save;
    while(state!=DONE)       //����˫��case���ʵ��DFA
    {
        int c=getNextChar();
        save=true;
        switch(state)
        {
        case START:
            if(isdigit(c))
                state=INNUM;
            else if(isalpha(c) || c == '_')
                state=INID;
            else if(c=='=')
                state=INASSIGN;
            else if(c=='<')
                state=INLE;
            else if(c=='>')
                state=INGE;
            else if(c=='!')
                state=INNEQ;
            else if((c==' ')||(c=='\t')||(c=='\n'))
                save=false;
            else if(c=='/')
                state=ENTERCOMMENT;
            else
            {
                state=DONE;
                switch(c)
                {
                case EOF:
                    save=false;
                    currentToken=ENDFILE;
                    break;
                case '+':
                    currentToken=PLUS;
                    break;
                case '-':
                    currentToken=MINUS;
                    break;
                case '*':
                    currentToken=TIMES;
                    break;
                case ';':
                    currentToken=SEMI;
                    break;
                case ',':
                    currentToken=COMMA;
                    break;
                case '(':
                    currentToken=LPAREN;
                    break;
                case ')':
                    currentToken=RPAREN;
                    break;
                case '[':
                    currentToken=LMBRACKET;
                    break;
                case ']':
                    currentToken=RMBRACKET;
                    break;
                case '{':
                    currentToken=LBBRACKET;
                    break;
                case '}':
                    currentToken=RBBRACKET;
                    break;
                default:
                    currentToken=ERROR;
                    break;
                }
            }
            break;
        case INASSIGN:
            state=DONE;
            if(c=='=')
            {
                currentToken=EQ;
            }
            else
            {
                ungetNextChar();
                save=false;
                currentToken=ASSIGN;
            }
            break;
        case INLE:
            state=DONE;
            if(c=='=')
            {
                currentToken=LEQ;
            }
            else
            {
                ungetNextChar();
                save=false;
                currentToken=LT;
            }
            break;
        case INGE:
            state=DONE;
            if(c=='=')
            {
                currentToken=GEQ;
            }
            else
            {
                ungetNextChar();
                save=false;
                currentToken=GT;
            }
            break;
        case INNEQ:
            state=DONE;
            if(c=='=')
            {
                currentToken=NEQ;
            }
            else
            {
                ungetNextChar();
                save=false;
                currentToken=ERROR;
            }
            break;
        case ENTERCOMMENT:
            if(c=='*')
            {
                state=INCOMMENT;
                save=false;
                tokenString[0]=' ';
            }
            else if(c==EOF)
            {
                state=DONE;
                currentToken =ENDFILE;
                save=false;
            }
            else
            {
                state=DONE;
                ungetNextChar();
                save=false;
                currentToken=OVER;
            }
            break;
        case INCOMMENT:
            save=false;
            if(c=='*')
            {
                state=LEAVECOMMENT;
            }
            else if(c==EOF)
            {
                state=DONE;
                currentToken=ENDFILE;
            }
            else
            {
                state=INCOMMENT;
            }
            break;
        case LEAVECOMMENT:
            if(c=='*')
            {
                state=LEAVECOMMENT;
            }
            else if(c=='/')
            {
                state=START;
            }
            else if(c==EOF)
            {
                state=DONE;
                currentToken=ENDFILE;
            }
            else
            {
                state=INCOMMENT;
            }
            break;
        case INNUM:
            if(!isdigit(c))
            {
                ungetNextChar();
                save=false;
                state=DONE;
                currentToken=NUM;
            }
            break;
        case INID:
            if(!isalpha(c) && !isdigit(c) && c != '_')
            {
                ungetNextChar();
                save=false;
                state=DONE;
                currentToken=ID;
            }
            break;
        case DONE:
        default:
            fprintf(listing,"scanner bug: state=%d\n",state);
            state=DONE;
            currentToken=ERROR;
            break;


        }//��switch������
        if((save)&&(tokenStringIndex<=MAXTOKENLEN))
        {
            tokenString[tokenStringIndex++]=(char)c;
        }
        if(state==DONE)
        {
            tokenString[tokenStringIndex]='\0';
            if(currentToken==ID)
                currentToken=reservedLookup(tokenString);
        }
    }

    if(TraceScan)
    {
        fprintf(listing,"\t%d ",lineno);
        printToken(currentToken,tokenString);
    }
    return currentToken;
}
