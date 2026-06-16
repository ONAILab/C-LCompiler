#include "treenode.h"
#include "scanner.h"
#include "util.h"

int lineno=0;
FILE* source;
FILE* listing;
FILE* code;

int EchoSource=0;
int TraceScan=1;

int main() {
    source = fopen("test.txt", "r");
    listing = stdout;
    while (getToken() != ENDFILE);
    return 0;
}