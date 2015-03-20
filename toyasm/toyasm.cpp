#include "toyasm.h"

Instruction instruction[] = {
  {"hlt",	0, 1},
  {"add",	3, 1},
  {"sub",	3, 1},
  {"and",	3, 1},
  {"xor",	3, 1},
  {"shl",	3, 1},
  {"shr",	3, 1},
  {"lda", 2, 2},
  {"ld",	2, 2},
  {"st",	2, 2},
  {"ldi",	2, 1},
  {"sti",	2, 1},
  {"bz",	2, 2},
  {"bp",	2, 2},
  {"jr",	1, 2},
  {"jl",	2, 2}
};

void Toyasm::init()
{
	int i;

	for (i=0; i<255; i++) {
		code[i][0]=NOTUSED;
		code[i][4]=0;
	}

	for (i=0; i<255; i++)
		SymTable[i].cnt=-1;
	SymTblSize = 0;
}

char Toyasm::DecToHex(int i)
{
	if (i<10) return '0'+i;
	switch (i) {
		case 10: return 'A';
		case 11: return 'B';
		case 12: return 'C';
		case 13: return 'D';
		case 14: return 'E';
		case 15: return 'F';
	}
  return '0';
}

void Toyasm::IntToAddress(int address, char *HexAddr)
{
	HexAddr[0] = DecToHex(address/16);
	HexAddr[1] = DecToHex(address%16);
}

void Toyasm::AddLiteralAddress(int address)
{
	literalAddress[literalCnt++]=address;
}

int Toyasm::LookupSymTable(char *label, int define)
{
	int i;

	for (i=0; i<SymTblSize; i++) {
		if (strcasecmp(label, SymTable[i].label)==0) {
			if (define) {
				if (SymTable[i].cnt==0) {
					fprintf(stderr, "Line %d: label %s has been defined\n", lineno, label);
          exit(1);
				} else {
					int j;
					char HexAddr[2];
					IntToAddress(address, HexAddr);
					for (j=0; j<SymTable[i].cnt; j++) {
						int k=SymTable[i].patch[j];
						code[k][2]=HexAddr[0];
						code[k][3]=HexAddr[1];
					}
					SymTable[i].cnt = 0;
					SymTable[i].address=address;
				}
			} else {
				if (SymTable[i].cnt!=0) {
					SymTblNode *p=SymTable+i;
					p->patch[p->cnt] = address;
					p->cnt++;
				}
			}

			return i;
		}
	}

	// not found, creat one
	SymTblNode *p=SymTable+SymTblSize;
	strcpy(p->label, label);
	if (define) {
		p->address = address;
		p->cnt=0;
	} else {
		p->patch[0] = address;
		p->cnt=1;
	}

	return SymTblSize++;
}

void Toyasm::HexData(char *token, char *value)
{
	char number[10];

	if (token[0]!='0' || token[1]!='x') {
		// decimal
		int v=atoi(token);
		if (v>65535 || v<-65535) {
			fprintf(stderr, "Line %d: %s out of range", lineno, token);
			exit(1);
		}

		char buf[10];
		sprintf(buf, "%08X", (short)v);
		strcpy(number, buf+4);
	} else
		strcpy(number, token+2);

	int l=strlen(number);
	if ( l>4 ) {
		fprintf(stderr, "Line %d: hexadecimal number %s out of range\n", lineno, token);
		exit(1);
	}

	int i, j;
	for (i=l-1, j=3; i>=0; i--, j--) {
		value[j]=number[i];
	}
	while (j>=0) {
		value[j]='0';
		j--;
	}
}

int Toyasm::opcode(char *mnemonic)
{
	int i;
	for (i=0; i<16; i++) {
		if (strcasecmp(mnemonic, instruction[i].mnemonic)==0)
			return i;
	}

	return -1;
}

int Toyasm::recogToken(char *token)
{
	int tokentype=-1;

	if ( (tokentype=opcode(token)) >=0 ) return tokentype+INSTRUCTION;

	if (strcasecmp(token, "R0")==0) return 0+REGISTER;
	if (strcasecmp(token, "R1")==0) return 1+REGISTER;
	if (strcasecmp(token, "R2")==0) return 2+REGISTER;
	if (strcasecmp(token, "R3")==0) return 3+REGISTER;
	if (strcasecmp(token, "R4")==0) return 4+REGISTER;
	if (strcasecmp(token, "R5")==0) return 5+REGISTER;
	if (strcasecmp(token, "R6")==0) return 6+REGISTER;
	if (strcasecmp(token, "R7")==0) return 7+REGISTER;
	if (strcasecmp(token, "R8")==0) return 8+REGISTER;
	if (strcasecmp(token, "R9")==0) return 9+REGISTER;
	if (strcasecmp(token, "RA")==0) return 10+REGISTER;
	if (strcasecmp(token, "RB")==0) return 11+REGISTER;
	if (strcasecmp(token, "RC")==0) return 12+REGISTER;
	if (strcasecmp(token, "RD")==0) return 13+REGISTER;
	if (strcasecmp(token, "RE")==0) return 14+REGISTER;
	if (strcasecmp(token, "RF")==0) return 15+REGISTER;

	if (strcasecmp(token, "DW")==0) return DW;
	if (strcasecmp(token, "DUP")==0) return DUP;

	if (token[0]=='-' || token[0]=='+' || isdigit(token[0])!=0) {
		//printf("a number %s\n", token);
		char *endptr=NULL;

		// must be a number
		strtol(token, &endptr, 10);
		if (*endptr=='\0') return DEC;

		strtol(token, &endptr, 16);
		if (*endptr=='\0') return HEX;

		fprintf(stderr, "line %d: %s is not a legal number\n", lineno, token);
		exit(1);
	}

	// it should be a variable
	return LABEL;
}

void Toyasm::FillData(int address, char *value)
{
	code[address][0]=value[0];
	code[address][1]=value[1];
	code[address][2]=value[2];
	code[address][3]=value[3];
}

void Toyasm::genCode()
{
	int i, j;

	int i_export=0;
	int import=0;
	if (genObject) {
		// count size
		fprintf(ostream, "// size %d\n", address+1);
		// count i_export and import
		for (i=0; i<255; i++) {
			if (SymTable[i].cnt==0) i_export++;
			if (SymTable[i].cnt>0)  import++;
		}
	}

	if (genObject) {
		fprintf(ostream, "// export %d\n", i_export);
		for (i=0; i<255; i++) {
			if (SymTable[i].cnt==0)
				fprintf(ostream, "// %s 0x%02X\n", SymTable[i].label, SymTable[i].address);
		}

		// output address with literal
		fprintf(ostream, "// literal %d ", literalCnt);
		for (i=0; i<literalCnt; i++) {
			fprintf(ostream, "%d ", literalAddress[i]);
		}
		fprintf(ostream, "\n");
	} else {
		for (i=0; i<255; i++) {
			if (SymTable[i].cnt>0) {
				fprintf(stderr, "ERROR: %s is not defined\n", SymTable[i].label);
				exit(1);
			}
#ifdef DEBUG
			if (SymTable[i].cnt==0) {
				fprintf(stderr, "%s: %04X\n", SymTable[i].label, SymTable[i].address);
			}
#endif
		}
	}

	// fix start address
	if (startAddr!=16) {
		code[16][0]='C';
		code[16][1]='0';
		sprintf(code[16]+2, "%02X", startAddr);
	}

	if (genObject) {
		int cnt=0;
		for (i=0; i<255; i++) {
			if (code[i][0]!=NOTUSED) cnt++;
		}
		fprintf(ostream, "// lines %d\n", cnt);
	}

	for (i=0; i<255; i++) {
		if (code[i][0]!=NOTUSED)
			fprintf(ostream, "%02X: %s\n", i, code[i]);
	}

	if (genObject) {
		fprintf(ostream, "// import %d\n", import);
		for (i=0; i<255; i++) {
			if (SymTable[i].cnt>0) {
				fprintf(ostream, "// %s %d ", SymTable[i].label, SymTable[i].cnt);
				for (j=0; j<SymTable[i].cnt; j++) {
					fprintf(ostream, "0x%02X ", SymTable[i].patch[j]);
				}
				fprintf(ostream, "\n");
			}
		}
	}
}

int Toyasm::getIndex(const char* str, const char c)
{
  int i = 0;
  while(*(str+i) != c && *(str+i++) != 0)
  { }

  return i;
}

void Toyasm::create(const char* sAsm, bool isO = false)
{
  Reset();

  if(isO)
  {
    genObject = 1;
  }

  int length = getIndex(sAsm, '.') + 1;

  // build hex file string
  int i = 0;
  while(i < length - 1)
  {
    sToy += *(sAsm+i++);
  }
  sToy += ".hex";

	// set up input and output streams
	istream = fopen(sAsm, "r");
	ostream = fopen(sToy.c_str(), "w");

	// initialize
	init();

	// parse and generate code
	char linetext[1024];
	while (!feof(istream)) {
		// read in a line
		if (fgets(linetext, 1024, istream)==NULL) break;
		lineno++;

#ifdef __DEBUG
		fprintf(stderr, "%d: %s\n", lineno, linetext);
#endif
		// parse the line
		int tokentype;
		char *token=strtok(linetext, WHITESPACE);
		if (token==NULL) continue;	// empty line

#ifdef DEBUG
		fprintf(stderr, "%s\n", token);
#endif

		if (token[0]==';') continue;	// comment

		tokentype=recogToken(token);
		address++;
		if (address==16) address++;

		int labelIndex=-1;

		if (tokentype==LABEL) {
			labelIndex = LookupSymTable(token, 1);

			token=strtok(NULL, WHITESPACE);
			if (token==NULL) {
				fprintf(stderr, "Line %d: syntax error\n", lineno);
				exit(1);
			}
			tokentype=recogToken(token);

#ifdef DEBUG
			fprintf(stderr, "%s\n", token);
#endif

			// data definition
			if (tokentype==DUP) {
				token=strtok(NULL, WHITESPACE);
				if (token==NULL) {
					fprintf(stderr, "Line %d: syntax error\n", lineno);
					exit(1);
				}
				tokentype=recogToken(token);

#ifdef DEBUG
				fprintf(stderr, "%s\n", token);
#endif

				if (tokentype!=DEC) {
					fprintf(stderr, "Line %d: DUP requires a decimal number but reads %s\n", lineno, token);
					exit(1);
				}

				int n=atoi(token);
				if (n<0 || n+address>255) {
					fprintf(stderr, "Line %d: DUP out of range\n", lineno);
					exit(1);
				}

				address = address + n-1;

				continue;
			}

			if (tokentype==DW) {
				token=strtok(NULL, WHITESPACE);
				if (token==NULL) {
					fprintf(stderr, "Line %d: syntax error\n", lineno);
					exit(1);
				}
				tokentype=recogToken(token);

				if (tokentype!=HEX && tokentype!=DEC) {
					fprintf(stderr, "Line %d: expect a number, but read %s\n", lineno, token);
					exit(1);
				}

				char value[4];
				HexData(token, value);
				FillData(address, value);

				AddLiteralAddress(address);

				continue;
			}
		}

		// expression
		if ((tokentype>>4) != (INSTRUCTION>>4)) {
			fprintf(stderr, "Line %d: non-recognizable instruction %s\n", lineno, token);
			exit(1);
		}

		if (startAddr<0) {
			if (address<16) {
				address=16;
				// fix label
				if (labelIndex>=0) {
					SymTable[labelIndex].address=address;
				}
			}
			startAddr = address;
		}

		int  opcode=tokentype&0x0F;
		int  argcnt=instruction[opcode].argc;
		char arg[3][128];
		int  argtype[3];
		int  argLabelIndex[3];

#ifdef DEBUG
		fprintf(stderr, "opcode: %d, format: %d\n", opcode, instruction[opcode].format);
#endif
		// read arguments
		int k;
		for (k=0; k<argcnt; k++) {
			token=strtok(NULL, WHITESPACE);
#ifdef DEBUG
			fprintf(stderr, "%d: %s\n", k, token);
#endif
			if (token==NULL || token[0]==';') {
				fprintf(stderr, "Line %d: insufficient number of arguments, %d is required\n", lineno, argcnt);
				exit(1);
			}
			argtype[k]=recogToken(token);
			if (argtype[k]==LABEL) {
				argLabelIndex[k] = LookupSymTable(token, 0);
			}
			strcpy(arg[k], token);
		}

		char *data=code[address];
		data[0]=DecToHex(opcode);
		data[1]=data[2]=data[3]='0';
		if (instruction[opcode].format==1) {
			if (opcode!=0) {
				int i;
				for (i=0; i<argcnt; i++) {
					if ((argtype[i]>>4)!=(REGISTER>>4)) {
						fprintf(stderr, "Line %d: argument must be register but read %s\n", lineno, arg[i]);
						exit(1);
					}
					if (opcode==10 || opcode==11) {
						if (i==0)
							data[1]=DecToHex(argtype[0]&0x0F);
						else {
							data[2]='0';
							data[3]=DecToHex(argtype[1]&0x0F);
						}
					} else
						data[i+1]=DecToHex(argtype[i]&0x0F);
				}
			}
		} else {
			if ((argtype[0]>>4)!=(REGISTER>>4)) {
				fprintf(stderr, "Line %d: argument must be register but read %s\n", lineno, arg[0]);
				exit(1);
			}
			data[1]=DecToHex(argtype[0]&0x0F);

			if (argcnt==2) {
				if (argtype[1]==LABEL) {
#ifdef DEBUG
					fprintf(stderr, "cnt: %d\n", SymTable[argLabelIndex[1]].cnt);
#endif
					if (SymTable[argLabelIndex[1]].cnt==0) {
						// fill in address
						sprintf(data+2, "%02X", SymTable[argLabelIndex[1]].address);
					}
				} else {
					// literal: HEX or DEC
					if (arg[1][0]!='0' || arg[1][1]!='x') {
						int n=atoi(arg[1]);
						if (n<0 || n>255) {
							fprintf(stderr, "Line %d: number out of range\n", lineno);
							exit(1);
						}
						n=n&0x0FF;
						sprintf(arg[1], "0x%02X", n);
//						fprintf(stderr, "Line %d: address must be a lable or hexdecimal, but read %s\n", lineno, arg[1]);
//						exit(1);
					}
					int l=strlen(arg[1]);
					if (l>4 || l<3) {
						fprintf(stderr, "Line %d: hex address %s too long or too short\n", lineno, arg[1]);
						exit(1);
					} else if (l==4) {
						data[2]=arg[1][2];
						data[3]=arg[1][3];
					} else {
						data[2]='0';
						data[3]=arg[1][2];
					}
          AddLiteralAddress(address);
				}
			}
		}
	}

	genCode();

  fclose(istream);
  fclose(ostream);
}

void Toyasm::Reset()
{
  literalCnt = 0;
  lineno = 0;
  address = -1;
  startAddr = -1;
  genObject = 0;
  sToy = "";
}

Toyasm::Toyasm()
{
  Reset();
}

Toyasm::~Toyasm()
{

}
