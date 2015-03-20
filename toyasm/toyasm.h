#ifndef TOYASM_H
#define TOYASM_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <iostream>

using std::cout;
using std::endl;
using std::string;

#if defined(_WIN32) || defined(_WIN64)
  #define snprintf _snprintf
  #define vsnprintf _vsnprintf
  #define strcasecmp _stricmp
  #define strncasecmp _strnicmp
#endif

//#define DEBUG		1

#define NOTUSED		'-'
#define WHITESPACE	" ,\t\n\r"

// token types
#define REGISTER	0
#define INSTRUCTION	16
#define	DEC		32
#define HEX		48
#define LABEL		64
#define DW		80
#define DUP		96

typedef struct {
  char mnemonic[6];
  int  argc;
  int  format;
} Instruction;

typedef struct {
  char label[128];
  int  address;
  int  cnt;
  int  patch[128];
} SymTblNode;

class Toyasm {
private:
  void init();
  char DecToHex(int i);
  void IntToAddress(int address, char *HexAddr);
  void AddLiteralAddress(int address);
  int LookupSymTable(char *label, int define);
  void HexData(char *token, char *value);
  int opcode(char *mnemonic);
  int recogToken(char *token);
  void FillData(int address, char *value);
  void genCode();
  int getIndex(const char* str, const char c);
  void Reset();

  string sToy;

  SymTblNode SymTable[255];
  int  SymTblSize;

  int   literalAddress[255];
  int   literalCnt;

  FILE *istream;
  FILE *ostream;

  int   lineno;
  int   address;
  int   startAddr;
  int   genObject;

  char code[255][5];
public:
  void create(const char*, bool);

  Toyasm();
  ~Toyasm();
};

#endif
