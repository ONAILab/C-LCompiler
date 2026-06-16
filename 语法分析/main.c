#include "treenode.h"
#include "scanner.h"
#include "util.h"
#include "syntax.h" 

int lineno=0;
FILE* source;
FILE* listing;
FILE* syntax_tree_file;
FILE* code;

int EchoSource=0;
int TraceScan=0;

int main()
{
    source=fopen("test.txt","r");
    if (!source) {
        printf("Failed to open test.txt\n");
        return 1;
    }
    listing=stdout;
    TreeNode* syntaxTree = NULL;
    if (lexicalPassed()) {
        syntaxTree=parse();
        fprintf(listing,"\nSyntax tree:\n");
        printTree(syntaxTree);
    } else {
        fprintf(listing,"\n>>> Lexical analysis failed, skipping syntax analysis.\n");
    }
    fclose(source);
    return 0;
}