#include "treenode.h"
#include "scanner.h"
#include "util.h"
#include "syntax.h"

static TokenType token;
static int Error = 0; // 增加错误标志位，用于Panic-Mode错误恢复

static TreeNode* declaration_list();      // 声明列表：program -> declaration_list
static TreeNode* declaration();           // 声明：declaration -> var-declaration | fun-declaration
static TreeNode* params();                // 参数列表：params -> param (, param)*
static TreeNode* param();                 // 单个参数：param -> type-specifier var
static TreeNode* compound_stmt();         // 复合语句：compound-stmt -> { local-declaration* statement-list* }
static TreeNode* local_declaration();     // 局部声明：local-declaration -> type-specifier var ;
static TreeNode* statement_list();        // 语句列表
static TreeNode* statement();             // 语句：statement -> expression-stmt | compound-stmt | selection-stmt | iteration-stmt | return-stmt
static TreeNode* expression_stmt();       // 表达式语句：expression-stmt -> expression ; | ;
static TreeNode* selection_stmt();        // 选择语句：selection-stmt -> if (expression) statement [else statement]
static TreeNode* iteration_stmt();        // 迭代语句：iteration-stmt -> while (expression) statement
static TreeNode* return_stmt();           // 返回语句：return-stmt -> return [expression] ;
static TreeNode* expression();            // 表达式：expression -> var = expression | simple-expression
static TreeNode* simple_expression(TreeNode* k);  // 简单表达式：simple-expression -> additive-expression (relop additive-expression)*
static TreeNode* var();                   // 变量：var -> ID | ID [expression]
static TreeNode* additive_expression(TreeNode* k); // 加法表达式：additive-expression -> term ((+|-) term)*
static TreeNode* term(TreeNode* k);       // 项：term -> factor ((*|/) factor)*
static TreeNode* factor(TreeNode* k);     // 因子：factor -> (expression) | var | call | NUM
static TreeNode* call(TreeNode* k);       // 函数调用：call -> ID (args)
static TreeNode* args();                  // 实参列表：args -> expression (, expression)*

// Panic-mode 
static void synchronize() {
    Error = 1;
    while (token != ENDFILE) {
        if (token == SEMI) {
            token = getToken();
            return;
        }
        switch (token) {
            case IF: case WHILE: case RETURN: case INT: case VOID:
                return;
            default:
                break;
        }
        token = getToken();
    }
}

static void syntaxError(char* message)
{
    // 避免连续报错机制
    if (!Error) {
        fprintf(listing,"\n>>> ");
        fprintf(listing,"Syntax error at line %d: %s",lineno,message);
        Error = 1;
    }
}

static void match(TokenType expected)
{
    if(token == expected) {
        token = getToken();
        Error = 0; // 匹配成功，恢复正常状态
    }
    else
    {
        syntaxError("unexpected token ->");
        printToken(token, tokenString);
        fprintf(listing,"    ");
    }
}

TreeNode* declaration_list()
{
    TreeNode* t = declaration();
    TreeNode* p = t;
    while(token != INT && token != VOID && token != ENDFILE)
    {
        syntaxError("Invalid declaration starting token");
        synchronize();
        if(token == ENDFILE)
        {
            fprintf(listing,"end of File\n");
            break;
        }
    }
    while(token == INT || token == VOID)
    {
        TreeNode* q = declaration();
        if(q != NULL)
        {
            if(t == NULL)
            {
                t = p = q;
            }
            else
            {
                p->sibling = q;
                p = q;
            }
        }
    }
    match(ENDFILE);
    return t;
}

TreeNode* declaration()
{
    TreeNode* t = NULL;
    TreeNode* p = NULL;
    TreeNode* q = NULL;
    TreeNode* s = NULL;

    if(token == INT)
    {
        p = newNode(IntK);
        match(INT);
    }
    else if(token == VOID)
    {
        p = newNode(VoidK);
        match(VOID);
    }
    else
    {
        syntaxError("declaration expects 'int' or 'void'");
        synchronize();
        return NULL;
    }

    if((p != NULL) && (token == ID))
    {
        q = newNode(IdK);
        match(ID);

        if(token == LPAREN)
        {
            t = newNode(FunK);
            t->child[0] = p;
            t->child[1] = q;
            match(LPAREN);
            t->child[2] = params();
            match(RPAREN);
            t->child[3] = compound_stmt();
        }
        else if(token == LMBRACKET)
        {
            t = newNode(Var_DeclK);
            TreeNode* m = newNode(Array_DeclK);
            match(LMBRACKET);
            match(NUM);
            s = newNode(ConstK);
            m->child[0] = q;
            m->child[1] = s;
            t->child[0] = p;
            t->child[1] = m;
            match(RMBRACKET);
            match(SEMI);
        }
        else if(token == SEMI)
        {
            t = newNode(Var_DeclK);
            t->child[0] = p;
            t->child[1] = q;
            match(SEMI);
        }
        else
        {
            syntaxError("unexpected token after ID in declaration");
            synchronize();
        }
    }
    else
    {
        syntaxError("declaration expects an ID");
        synchronize();
    }
    return t;
}

TreeNode* params()
{
    TreeNode* t = newNode(ParamsK);
    TreeNode* p = NULL;
    TreeNode* q = NULL;

    if(token == VOID)
    {
        p = newNode(VoidK);
        match(VOID);
        if(token != RPAREN)
        {
            syntaxError("params: unexpected tokens after 'void'");
        }
        if(t != NULL) t->child[0] = p;
    }
    else if(token == INT)
    {
        p = param();
        t->child[0] = p;
        while(token == COMMA)
        {
            match(COMMA);
            q = param();
            if(p != NULL) p->sibling = q;
            p = q;
        }
    }
    else if(token == RPAREN)
    {
        // empty parameter list, e.g. void main()
    }
    else
    {
        syntaxError("params: invalid parameter type");
        synchronize();
    }
    return t;
}

TreeNode* param()
{
    TreeNode* t = NULL;
    TreeNode* p = NULL;
    TreeNode* q = NULL;
    match(INT);
    p = newNode(IntK);
    q = newNode(IdK);
    match(ID);
    if(token == LMBRACKET)
    {
        match(LMBRACKET);
        match(RMBRACKET);
        t = newNode(Array_ParamK);
        t->child[0] = p;
        t->child[1] = q;
    }
    else
    {
        t = newNode(Int_ParamK);
        t->child[0] = p;
        t->child[1] = q;
    }
    return t;
}

TreeNode* compound_stmt()
{
    TreeNode* t = newNode(CompK);
    match(LBBRACKET);
    t->child[0] = local_declaration();
    t->child[1] = statement_list();
    match(RBBRACKET);
    return t;
}


TreeNode* local_declaration()
{
    TreeNode* t = NULL;
    TreeNode* p = NULL;

    while(token == INT || token == VOID)
    {
        TreeNode* q = NULL;
        TreeNode* q1 = NULL;
        TreeNode* q2 = NULL;
        
        if(token == INT) {
            q1 = newNode(IntK);
            match(INT);
        } else if (token == VOID) {
            q1 = newNode(VoidK);
            match(VOID);
        }

        if(token == ID) {
            q2 = newNode(IdK);
            match(ID);
        }

        if(token == LMBRACKET) {   
            match(LMBRACKET);
            TreeNode* q3 = newNode(ConstK);  
            match(NUM);
            match(RMBRACKET);
            
            TreeNode* arrDecl = newNode(Array_DeclK);
            arrDecl->child[0] = q1;
            arrDecl->child[1] = q2;
            arrDecl->child[2] = q3;
            
            q = arrDecl;
            match(SEMI);
        } else if(token == SEMI) {   
            match(SEMI);
            q = newNode(Var_DeclK);
            q->child[0] = q1;
            q->child[1] = q2;
        } else {
            syntaxError("Invalid local declaration syntax");
            synchronize();
        }

        if(q != NULL) {
            if(t == NULL) t = p = q;
            else {
                p->sibling = q;
                p = q;
            }
        }
    }
    return t;
}

TreeNode* statement_list()
{
    TreeNode* t = statement();
    TreeNode* p = t;
    while(token == IF || token == LBBRACKET || token == ID || token == WHILE || token == RETURN || token == SEMI || token == LPAREN || token == NUM)
    {
        TreeNode* q = statement();
        if(q != NULL)
        {
            if(t == NULL) t = p = q;
            else
            {
                p->sibling = q;
                p = q;
            }
        }
    }
    return t;
}

TreeNode* statement()
{
    TreeNode* t = NULL;
    switch(token)
    {
    case IF: t = selection_stmt(); break;
    case WHILE: t = iteration_stmt(); break;
    case RETURN: t = return_stmt(); break;
    case LBBRACKET: t = compound_stmt(); break;
    case ID: case SEMI: case LPAREN: case NUM:
        t = expression_stmt(); break;
    default:
        syntaxError("unexpected token to start a statement");
        synchronize(); 
        break;
    }
    return t;
}

TreeNode* selection_stmt()
{
    TreeNode* t = newNode(Selection_StmtK);
    match(IF);
    match(LPAREN);
    if(t != NULL) t->child[0] = expression();
    match(RPAREN);
    t->child[1] = statement();
    if(token == ELSE)
    {
        match(ELSE);
        if(t != NULL) t->child[2] = statement();
    }
    return t;
}

TreeNode* expression_stmt()
{
    TreeNode* t = NULL;
    if(token == SEMI)
    {
        match(SEMI);
    }
    else
    {
        t = expression();
        match(SEMI);
    }
    return t;
}

TreeNode* iteration_stmt()
{
    TreeNode* t = newNode(Iteration_StmtK);
    match(WHILE);
    match(LPAREN);
    if(t != NULL) t->child[0] = expression();
    match(RPAREN);
    if(t != NULL) t->child[1] = statement();
    return t;
}

TreeNode* return_stmt()
{
    TreeNode* t = newNode(Return_StmtK);
    match(RETURN);
    if(token == SEMI)
    {
        match(SEMI);
        return t;
    }
    else
    {
        if(t != NULL) t->child[0] = expression();
    }
    match(SEMI);
    return t;
}

TreeNode* expression()
{
    TreeNode* t = var();
    if(t == NULL) 
    {
        t = simple_expression(t);
    }
    else 
    {
        TreeNode* p = NULL;
        if(token == ASSIGN) 
        {
            p = newNode(AssignK);
            match(ASSIGN);
            p->child[0] = t;
            p->child[1] = expression();
            return p;
        }
        else 
        {
            t = simple_expression(t);
        }
    }
    return t;
}

TreeNode* simple_expression(TreeNode* k)
{
    TreeNode* t = additive_expression(k);
    if(token == EQ || token == GT || token == GEQ || token == LT || token == LEQ || token == NEQ)
    {
        TreeNode* q = newNode(OpK);
        if(q != NULL)
        {
            q->attr.op = token;
            q->child[0] = t;
            t = q;
            match(token);
            t->child[1] = additive_expression(NULL);
        }
    }
    return t;
}

TreeNode* additive_expression(TreeNode* k)
{
    TreeNode* t = term(k);
    while(token == PLUS || token == MINUS)
    {
        TreeNode* q = newNode(OpK);
        q->attr.op = token;
        q->child[0] = t;
        match(token);
        q->child[1] = term(NULL);
        t = q;
    }
    return t;
}

TreeNode* term(TreeNode* k)
{
    TreeNode* t = factor(k);
    while(token == TIMES || token == OVER)
    {
        TreeNode* q = newNode(OpK);
        q->attr.op = token;
        q->child[0] = t;
        match(token);
        q->child[1] = factor(NULL);
        t = q;
    }
    return t;
}

TreeNode* factor(TreeNode* k)
{
    TreeNode* t = NULL;
    TreeNode* p = NULL;
    if(k != NULL)
    {
        if(token == LPAREN && k->nodekind != Array_ElemK) 
        {
            t = call(k);
        }
        else 
        {
            t = k;
        }
    }
    else
    {
        switch(token)
        {
        case LPAREN:
            match(LPAREN);
            t = expression();
            match(RPAREN);
            break;
        case ID:
            p = var();
            if(token == LPAREN && p->nodekind != Array_ElemK)
            {
                t = call(p);
            }
            else
            {
                t = p;
            }
            break;
        case NUM:
            t = newNode(ConstK);
            match(NUM);
            break;
        default:
            syntaxError("invalid factor");
            synchronize(); // 【改进】
            break;
        }
    }
    return t;
}

TreeNode* var()
{
    TreeNode* t = NULL;
    TreeNode* p = NULL;
    TreeNode* q = NULL;
    if(token == ID)
    {
        p = newNode(IdK);
        match(ID);
        if(token == LMBRACKET)
        {
            match(LMBRACKET);
            q = expression();
            match(RMBRACKET);
            t = newNode(Array_ElemK);
            t->child[0] = p;
            t->child[1] = q;
        }
        else
        {
            t = p;
        }
    }
    return t;
}

TreeNode* call(TreeNode* k)
{
    TreeNode* t = newNode(CallK);
    if(k != NULL) t->child[0] = k;
    match(LPAREN);
    if(token == RPAREN)
    {
        match(RPAREN);
    }
    else 
    {
        t->child[1] = args();
        match(RPAREN);
    }
    return t;
}

TreeNode* args()
{
    TreeNode* t = newNode(ArgsK);
    TreeNode* s = NULL;
    TreeNode* p = NULL;
    if(token != RPAREN)
    {
        s = expression();
        p = s;
        while(token == COMMA)
        {
            TreeNode* q;
            match(COMMA);
            q = expression();
            if(q != NULL)
            {
                if(s == NULL) s = p = q;
                else
                {
                    p->sibling = q;
                    p = q;
                }
            }
        }
    }
    if(s != NULL) t->child[0] = s;
    return t;
}

TreeNode* parse()
{
    TreeNode* t;
    token = getToken();
    t = declaration_list();
    if(token != ENDFILE)
        syntaxError("Code ends before file\n");
    return t;
}
