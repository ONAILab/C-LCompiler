#ifndef SCANNER_H_INCLUDED
#define SCANNER_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#define MAXTOKENLEN 40
extern char tokenString[MAXTOKENLEN+1];
TokenType getToken();
int lexicalPassed();

#ifdef __cplusplus
}
#endif

#endif // SCANNER_H_INCLUDED
