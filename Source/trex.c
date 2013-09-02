/* see copyright notice in trex.h */
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <setjmp.h>
#include <string.h>
#include "../Include/trex.h"
#include "../Include/util.h"

#ifdef _UNICODE
#define scisprint iswprint
#define scstrlen wcslen
#define scprintf wprintf
#define _SC(x) L(x)
#else
#define scisprint isprint
#define scstrlen strlen
#define scprintf printf
#define _SC(x) (x)
#endif

#ifdef _DEBUG
#include <stdio.h>

static const TRexChar *g_nnames[] =
{
	_SC("NONE"),_SC("OP_GREEDY"),	_SC("OP_OR"),
	_SC("OP_EXPR"),_SC("OP_NOCAPEXPR"),_SC("OP_DOT"),	_SC("OP_CLASS"),
	_SC("OP_CCLASS"),_SC("OP_NCLASS"),_SC("OP_RANGE"),_SC("OP_CHAR"),
	_SC("OP_EOL"),_SC("OP_BOL"),_SC("OP_WB")
};

#endif
#define OP_GREEDY		(MAX_CHAR+1) // * + ? {n}
#define OP_OR			(MAX_CHAR+2)
#define OP_EXPR			(MAX_CHAR+3) //parentesis ()
#define OP_NOCAPEXPR	(MAX_CHAR+4) //parentesis (?:)
#define OP_DOT			(MAX_CHAR+5)
#define OP_CLASS		(MAX_CHAR+6)
#define OP_CCLASS		(MAX_CHAR+7)
#define OP_NCLASS		(MAX_CHAR+8) //negates class the [^
#define OP_RANGE		(MAX_CHAR+9)
#define OP_CHAR			(MAX_CHAR+10)
#define OP_EOL			(MAX_CHAR+11)
#define OP_BOL			(MAX_CHAR+12)
#define OP_WB			(MAX_CHAR+13)

#define TREX_SYMBOL_ANY_CHAR ('.')
#define TREX_SYMBOL_GREEDY_ONE_OR_MORE ('+')
#define TREX_SYMBOL_GREEDY_ZERO_OR_MORE ('*')
#define TREX_SYMBOL_GREEDY_ZERO_OR_ONE ('?')
#define TREX_SYMBOL_BRANCH ('|')
#define TREX_SYMBOL_END_OF_STRING ('$')
#define TREX_SYMBOL_BEGINNING_OF_STRING ('^')
#define TREX_SYMBOL_ESCAPE_CHAR ('\\')


typedef int TRexNodeType;

typedef struct tagTRexNode{
	TRexNodeType type;
	int left;
	int right;
	int next;
}TRexNode;

struct TRex{
	const TRexChar *_eol;
	const TRexChar *_bol;
	const TRexChar *_p;
	int _first;
	int _op;
	TRexNode *_nodes;
	int _nallocated;
	int _nsize;
	int _nsubexpr;
	TRexMatch *_matches;
	int _currsubexp;
	void *_jmpbuf;
	const TRexChar **_error;
};

static int trex_list(TRex *exp);

static int trex_newnode(TRex *exp, TRexNodeType type)
{
	TRexNode n;
	int newid;
	n.type = type;
	n.next = n.right = n.left = -1;
	if(type == OP_EXPR)
		n.right = exp->_nsubexpr++;
	if(exp->_nallocated < (exp->_nsize + 1)) {
		int oldsize = exp->_nallocated;
		exp->_nallocated *= 2;
		exp->_nodes = (TRexNode *)realloc(exp->_nodes, exp->_nallocated * sizeof(TRexNode));
	}
	exp->_nodes[exp->_nsize++] = n;
	newid = exp->_nsize - 1;
	return (int)newid;
}

static void trex_error(TRex *exp,const TRexChar *error)
{
	if(exp->_error) *exp->_error = error;
	longjmp(*((jmp_buf*)exp->_jmpbuf),-1);
}

static void trex_expect(TRex *exp, int n){
	if((*exp->_p) != n) 
		trex_error(exp, _SC("expected paren"));
	exp->_p++;
}

static TRexChar trex_escapechar(TRex *exp)
{
	if(*exp->_p == TREX_SYMBOL_ESCAPE_CHAR){
		exp->_p++;
		switch(*exp->_p) {
		case 'v': exp->_p++; return '\v';
		case 'n': exp->_p++; return '\n';
		case 't': exp->_p++; return '\t';
		case 'r': exp->_p++; return '\r';
		case 'f': exp->_p++; return '\f';
		default: return (*exp->_p++);
		}
	} else if(!scisprint(*exp->_p)) trex_error(exp,_SC("letter expected"));
	return (*exp->_p++);
}

static int trex_charclass(TRex *exp,int classid)
{
	int n = trex_newnode(exp,OP_CCLASS);
	exp->_nodes[n].left = classid;
	return n;
}

static int trex_charnode(TRex *exp,TRexBool isclass)
{
	TRexChar t;
	if(*exp->_p == TREX_SYMBOL_ESCAPE_CHAR) {
		exp->_p++;
		switch(*exp->_p) {
			case 'n': exp->_p++; return trex_newnode(exp,'\n');
			case 't': exp->_p++; return trex_newnode(exp,'\t');
			case 'r': exp->_p++; return trex_newnode(exp,'\r');
			case 'f': exp->_p++; return trex_newnode(exp,'\f');
			case 'v': exp->_p++; return trex_newnode(exp,'\v');
			case 'a': case 'A': case 'w': case 'W': case 's': case 'S': 
			case 'd': case 'D': case 'x': case 'X': case 'c': case 'C': 
			case 'p': case 'P': case 'l': case 'u': 
				{
				t = *exp->_p; exp->_p++; 
				return trex_charclass(exp,t);
				}
			case 'b': 
			case 'B':
				if(!isclass) {
					int node = trex_newnode(exp,OP_WB);
					exp->_nodes[node].left = *exp->_p;
					exp->_p++; 
					return node;
				} //else default
			default: 
				t = *exp->_p; exp->_p++; 
				return trex_newnode(exp,t);
		}
	}
	else if(!scisprint(*exp->_p)) {
		
		trex_error(exp,_SC("letter expected"));
	}
	t = *exp->_p; exp->_p++; 
	return trex_newnode(exp,t);
}
static int trex_class(TRex *exp)
{
	int ret = -1;
	int first = -1,chain;
	if(*exp->_p == TREX_SYMBOL_BEGINNING_OF_STRING){
		ret = trex_newnode(exp,OP_NCLASS);
		exp->_p++;
	}else ret = trex_newnode(exp,OP_CLASS);
	
	if(*exp->_p == ']') trex_error(exp,_SC("empty class"));
	chain = ret;
	while(*exp->_p != ']' && exp->_p != exp->_eol) {
		if(*exp->_p == '-' && first != -1){ 
			int r,t;
			if(*exp->_p++ == ']') trex_error(exp,_SC("unfinished range"));
			r = trex_newnode(exp,OP_RANGE);
			if(first>*exp->_p) trex_error(exp,_SC("invalid range"));
			if(exp->_nodes[first].type == OP_CCLASS) trex_error(exp,_SC("cannot use character classes in ranges"));
			exp->_nodes[r].left = exp->_nodes[first].type;
			t = trex_escapechar(exp);
			exp->_nodes[r].right = t;
            exp->_nodes[chain].next = r;
			chain = r;
			first = -1;
		}
		else{
			if(first!=-1){
				int c = first;
				exp->_nodes[chain].next = c;
				chain = c;
				first = trex_charnode(exp,TRex_True);
			}
			else{
				first = trex_charnode(exp,TRex_True);
			}
		}
	}
	if(first!=-1){
		int c = first;
		exp->_nodes[chain].next = c;
		chain = c;
		first = -1;
	}
	/* hack? */
	exp->_nodes[ret].left = exp->_nodes[ret].next;
	exp->_nodes[ret].next = -1;
	return ret;
}

static int trex_parsenumber(TRex *exp)
{
	int ret = *exp->_p-'0';
	int positions = 10;
	exp->_p++;
	while(isdigit(*exp->_p)) {
		ret = ret*10+(*exp->_p++-'0');
		if(positions==1000000000) trex_error(exp,_SC("overflow in numeric constant"));
		positions *= 10;
	};
	return ret;
}

static int trex_element(TRex *exp)
{
	int ret = -1;
	switch(*exp->_p)
	{
	case '(': {
		int expr,newn;
		exp->_p++;


		if(*exp->_p =='?') {
			exp->_p++;
			trex_expect(exp,':');
			expr = trex_newnode(exp,OP_NOCAPEXPR);
		}
		else
			expr = trex_newnode(exp,OP_EXPR);
		newn = trex_list(exp);
		exp->_nodes[expr].left = newn;
		ret = expr;
		trex_expect(exp,')');
			  }
			  break;
	case '[':
		exp->_p++;
		ret = trex_class(exp);
		trex_expect(exp,']');
		break;
	case TREX_SYMBOL_END_OF_STRING: exp->_p++; ret = trex_newnode(exp,OP_EOL);break;
	case TREX_SYMBOL_ANY_CHAR: exp->_p++; ret = trex_newnode(exp,OP_DOT);break;
	default:
		ret = trex_charnode(exp,TRex_False);
		break;
	}

	{
		int op;
		TRexBool isgreedy = TRex_False;
		unsigned short p0 = 0, p1 = 0;
		switch(*exp->_p){
			case TREX_SYMBOL_GREEDY_ZERO_OR_MORE: p0 = 0; p1 = 0xFFFF; exp->_p++; isgreedy = TRex_True; break;
			case TREX_SYMBOL_GREEDY_ONE_OR_MORE: p0 = 1; p1 = 0xFFFF; exp->_p++; isgreedy = TRex_True; break;
			case TREX_SYMBOL_GREEDY_ZERO_OR_ONE: p0 = 0; p1 = 1; exp->_p++; isgreedy = TRex_True; break;
			case '{':
				exp->_p++;
				if(!isdigit(*exp->_p)) trex_error(exp,_SC("number expected"));
				p0 = (unsigned short)trex_parsenumber(exp);
				/*******************************/
				switch(*exp->_p) {
			case '}':
				p1 = p0; exp->_p++;
				break;
			case ',':
				exp->_p++;
				p1 = 0xFFFF;
				if(isdigit(*exp->_p)){
					p1 = (unsigned short)trex_parsenumber(exp);
				}
				trex_expect(exp,'}');
				break;
			default:
				trex_error(exp,_SC(", or } expected"));
		}
		/*******************************/
		isgreedy = TRex_True; 
		break;

		}
		if(isgreedy) {
			int nnode = trex_newnode(exp,OP_GREEDY);
			op = OP_GREEDY;
			exp->_nodes[nnode].left = ret;
			exp->_nodes[nnode].right = ((p0)<<16)|p1;
			ret = nnode;
		}
	}
	if((*exp->_p != TREX_SYMBOL_BRANCH) && (*exp->_p != ')') && (*exp->_p != TREX_SYMBOL_GREEDY_ZERO_OR_MORE) && (*exp->_p != TREX_SYMBOL_GREEDY_ONE_OR_MORE) && (*exp->_p != '\0')) {
		int nnode = trex_element(exp);
		exp->_nodes[ret].next = nnode;
	}

	return ret;
}

static int trex_list(TRex *exp)
{
	int ret=-1,e;
	if(*exp->_p == TREX_SYMBOL_BEGINNING_OF_STRING) {
		exp->_p++;
		ret = trex_newnode(exp,OP_BOL);
	}
	e = trex_element(exp);
	if(ret != -1) {
		exp->_nodes[ret].next = e;
	}
	else ret = e;

	if(*exp->_p == TREX_SYMBOL_BRANCH) {
		int temp,tright;
		exp->_p++;
		temp = trex_newnode(exp,OP_OR);
		exp->_nodes[temp].left = ret;
		tright = trex_list(exp);
		exp->_nodes[temp].right = tright;
		ret = temp;
	}
	return ret;
}

static TRexBool trex_matchcclass(int cclass,TRexChar c)
{
	switch(cclass) {
	case 'a': return isalpha(c)?TRex_True:TRex_False;
	case 'A': return !isalpha(c)?TRex_True:TRex_False;
	case 'w': return (isalnum(c) || c == '_')?TRex_True:TRex_False;
	case 'W': return (!isalnum(c) && c != '_')?TRex_True:TRex_False;
	case 's': return isspace(c)?TRex_True:TRex_False;
	case 'S': return !isspace(c)?TRex_True:TRex_False;
	case 'd': return isdigit(c)?TRex_True:TRex_False;
	case 'D': return !isdigit(c)?TRex_True:TRex_False;
	case 'x': return isxdigit(c)?TRex_True:TRex_False;
	case 'X': return !isxdigit(c)?TRex_True:TRex_False;
	case 'c': return iscntrl(c)?TRex_True:TRex_False;
	case 'C': return !iscntrl(c)?TRex_True:TRex_False;
	case 'p': return ispunct(c)?TRex_True:TRex_False;
	case 'P': return !ispunct(c)?TRex_True:TRex_False;
	case 'l': return islower(c)?TRex_True:TRex_False;
	case 'u': return isupper(c)?TRex_True:TRex_False;
	}
	return TRex_False; /*cannot happen*/
}

static TRexBool trex_matchclass(TRex* exp,TRexNode *node,TRexChar c)
{
	do {
		switch(node->type) {
			case OP_RANGE:
				if(c >= node->left && c <= node->right) return TRex_True;
				break;
			case OP_CCLASS:
				if(trex_matchcclass(node->left,c)) return TRex_True;
				break;
			default:
				if(c == node->type)return TRex_True;
		}
	} while((node->next != -1) && (node = &exp->_nodes[node->next]));
	return TRex_False;
}

static const TRexChar *trex_matchnode(TRex* exp,TRexNode *node,const TRexChar *str,TRexNode *next)
{
	
	TRexNodeType type = node->type;
	switch(type) {
	case OP_GREEDY: {
		//TRexNode *greedystop = (node->next != -1) ? &exp->_nodes[node->next] : NULL;
		TRexNode *greedystop = NULL;
		int p0 = (node->right >> 16)&0x0000FFFF, p1 = node->right&0x0000FFFF, nmaches = 0;
		const TRexChar *s=str, *good = str;

		if(node->next != -1) {
			greedystop = &exp->_nodes[node->next];
		}
		else {
			greedystop = next;
		}

		while((nmaches == 0xFFFF || nmaches < p1)) {

			const TRexChar *stop;
			if(!(s = trex_matchnode(exp,&exp->_nodes[node->left],s,greedystop)))
				break;
			nmaches++;
			good=s;
			if(greedystop) {
				//checks that 0 matches satisfy the expression(if so skips)
				//if not would always stop(for instance if is a '?')
				if(greedystop->type != OP_GREEDY ||
				(greedystop->type == OP_GREEDY && ((greedystop->right >> 16)&0x0000FFFF) != 0))
				{
					TRexNode *gnext = NULL;
					if(greedystop->next != -1) {
						gnext = &exp->_nodes[greedystop->next];
					}else if(next && next->next != -1){
						gnext = &exp->_nodes[next->next];
					}
					stop = trex_matchnode(exp,greedystop,s,gnext);
					if(stop) {
						//if satisfied stop it
						if(p0 == p1 && p0 == nmaches) break;
						else if(nmaches >= p0 && p1 == 0xFFFF) break;
						else if(nmaches >= p0 && nmaches <= p1) break;
					}
				}
			}
			
			if(s >= exp->_eol)
				break;
		}
		if(p0 == p1 && p0 == nmaches) return good;
		else if(nmaches >= p0 && p1 == 0xFFFF) return good;
		else if(nmaches >= p0 && nmaches <= p1) return good;
		return NULL;
	}
	case OP_OR: {
			const TRexChar *asd = str;
			TRexNode *temp=&exp->_nodes[node->left];
			while( (asd = trex_matchnode(exp,temp,asd,NULL)) ) {
				if(temp->next != -1)
					temp = &exp->_nodes[temp->next];
				else
					return asd;
			}
			asd = str;
			temp = &exp->_nodes[node->right];
			while( (asd = trex_matchnode(exp,temp,asd,NULL)) ) {
				if(temp->next != -1)
					temp = &exp->_nodes[temp->next];
				else
					return asd;
			}
			return NULL;
			break;
	}
	case OP_EXPR:
	case OP_NOCAPEXPR:{
			TRexNode *n = &exp->_nodes[node->left];
			const TRexChar *cur = str;
			int capture = -1;
			if(node->type != OP_NOCAPEXPR && node->right == exp->_currsubexp) {
				capture = exp->_currsubexp;
				exp->_matches[capture].begin = cur;
				exp->_currsubexp++;
			}
			
			do {
				TRexNode *subnext = NULL;
				if(n->next != -1) {
					subnext = &exp->_nodes[n->next];
				}else {
					subnext = next;
				}
				if(!(cur = trex_matchnode(exp,n,cur,subnext))) {
					if(capture != -1){
						exp->_matches[capture].begin = 0;
						exp->_matches[capture].len = 0;
					}
					return NULL;
				}
			} while((n->next != -1) && (n = &exp->_nodes[n->next]));

			if(capture != -1) 
				exp->_matches[capture].len = cur - exp->_matches[capture].begin;
			return cur;
	}				 
	case OP_WB:
		if(str == exp->_bol && !isspace(*str)
		 || (str == exp->_eol && !isspace(*(str-1)))
		 || (!isspace(*str) && isspace(*(str+1)))
		 || (isspace(*str) && !isspace(*(str+1))) ) {
			return (node->left == 'b')?str:NULL;
		}
		return (node->left == 'b')?NULL:str;
	case OP_BOL:
		if(str == exp->_bol) return str;
		return NULL;
	case OP_EOL:
		if(str == exp->_eol) return str;
		return NULL;
	case OP_DOT:{
		*str++;
				}
		return str;
	case OP_NCLASS:
	case OP_CLASS:
		if(trex_matchclass(exp,&exp->_nodes[node->left],*str)?(type == OP_CLASS?TRex_True:TRex_False):(type == OP_NCLASS?TRex_True:TRex_False)) {
			*str++;
			return str;
		}
		return NULL;
	case OP_CCLASS:
		if(trex_matchcclass(node->left,*str)) {
			*str++;
			return str;
		}
		return NULL;
	default: /* char */
		if(*str != node->type) return NULL;
		*str++;
		return str;
	}
	return NULL;
}

/* public api */
TRex *trex_compile(const TRexChar *pattern,const TRexChar **error)
{
	TRex *exp = (TRex *)malloc(sizeof(TRex));
	exp->_eol = exp->_bol = NULL;
	exp->_p = pattern;
	exp->_nallocated = (int)scstrlen(pattern) * sizeof(TRexChar);
	exp->_nodes = (TRexNode *)malloc(exp->_nallocated * sizeof(TRexNode));
	exp->_nsize = 0;
	exp->_matches = 0;
	exp->_nsubexpr = 0;
	exp->_first = trex_newnode(exp,OP_EXPR);
	exp->_error = error;
	exp->_jmpbuf = malloc(sizeof(jmp_buf));
	if(setjmp(*((jmp_buf*)exp->_jmpbuf)) == 0) {
		int res = trex_list(exp);
		exp->_nodes[exp->_first].left = res;
		if(*exp->_p!='\0')
			trex_error(exp,_SC("unexpected character"));
#ifdef _DEBUG
		{
			int nsize,i;
			TRexNode *t;
			nsize = exp->_nsize;
			t = &exp->_nodes[0];
			scprintf(_SC("\n"));
			for(i = 0;i < nsize; i++) {
				if(exp->_nodes[i].type>MAX_CHAR)
					scprintf(_SC("[%02d] %10s "),i,g_nnames[exp->_nodes[i].type-MAX_CHAR]);
				else
					scprintf(_SC("[%02d] %10c "),i,exp->_nodes[i].type);
				scprintf(_SC("left %02d right %02d next %02d\n"),exp->_nodes[i].left,exp->_nodes[i].right,exp->_nodes[i].next);
			}
			scprintf(_SC("\n"));
		}
#endif
		exp->_matches = (TRexMatch *) malloc(exp->_nsubexpr * sizeof(TRexMatch));
		memset(exp->_matches,0,exp->_nsubexpr * sizeof(TRexMatch));
	}
	else{
		trex_free(exp);
		return NULL;
	}
	return exp;
}

void trex_free(TRex *exp)
{
	if(exp)	{
		if(exp->_nodes) free(exp->_nodes);
		if(exp->_jmpbuf) free(exp->_jmpbuf);
		if(exp->_matches) free(exp->_matches);
		free(exp);
	}
}

TRexBool trex_match(TRex* exp,const TRexChar* text)
{
	const TRexChar* res = NULL;
	exp->_bol = text;
	exp->_eol = text + scstrlen(text);
	exp->_currsubexp = 0;
	res = trex_matchnode(exp,exp->_nodes,text,NULL);
	if(res == NULL || res != exp->_eol)
		return TRex_False;
	return TRex_True;
}

TRexBool trex_searchrange(TRex* exp,const TRexChar* text_begin,const TRexChar* text_end,const TRexChar** out_begin, const TRexChar** out_end)
{
	const TRexChar *cur = NULL;
	int node = exp->_first;
	if(text_begin >= text_end) return TRex_False;
	exp->_bol = text_begin;
	exp->_eol = text_end;
	do {
		cur = text_begin;
		while(node != -1) {
			exp->_currsubexp = 0;
			cur = trex_matchnode(exp,&exp->_nodes[node],cur,NULL);
			if(!cur)
				break;
			node = exp->_nodes[node].next;
		}
		*text_begin++;
	} while(cur == NULL && text_begin != text_end);

	if(cur == NULL)
		return TRex_False;

	--text_begin;

	if(out_begin) *out_begin = text_begin;
	if(out_end) *out_end = cur;
	return TRex_True;
}

TRexBool trex_search(TRex* exp,const TRexChar* text, const TRexChar** out_begin, const TRexChar** out_end)
{
	return trex_searchrange(exp,text,text + scstrlen(text),out_begin,out_end);
}

int trex_getsubexpcount(TRex* exp)
{
	return exp->_nsubexpr;
}

TRexBool trex_getsubexp(TRex* exp, int n, TRexMatch *subexp)
{
	if( n<0 || n >= exp->_nsubexpr) return TRex_False;
	*subexp = exp->_matches[n];
	return TRex_True;
}

void rx_getnext(TRex* x, int i, char** val)
{
	TRexMatch match;
	trex_getsubexp(x,i,&match);
	CHECK(NULL!=val);
	*val = (char*)calloc(match.len+1,1);
	CHECK(NULL!=*val);
	memcpy(*val,match.begin,match.len);
}

#ifdef _CUTEST

void check2strings(CuTest* tc, TRexChar* text, TRexChar* format, 
		TRexChar* expect1, TRexChar* expect2)
{
	TRexChar *begin, *end, *error;
	TRex* x = trex_compile(format,&error);
	while (trex_search(x,text,&begin,&end)) {
		char* string1, *string2;
		int n = trex_getsubexpcount(x);	
		CuAssert(tc,"Expect 2 matches",3==n);
		rx_getnext(x,1,&string1);
		rx_getnext(x,2,&string2);
		CuAssert(tc,"Unexpected string",0==strcmp(string1,expect1));
		CuAssert(tc,"Unexpected string",0==strcmp(string2,expect2));
		free(string1); free(string2);
		text = end;
	}		
	trex_free(x);
}

void testCuTest1(CuTest* tc)
{
	check2strings(tc, _TREXC("name=conan % the barbarian; son: of thor"),
			_TREXC("name=([\\c\\w\\s\\p]+); son:([\\c\\w\\s\\p]+)"),
			_TREXC("conan % the barbarian"),
			_TREXC(" of thor"));
}

void testCuTest2(CuTest* tc) 
{
	const char* content = "fc3e651929681ad9060754894fafb201CodigoBusca=1999&EventoId=2&Id=13428&LoteId=2&Ingresso="\
		"PISTA%201&Lote=1%BA%20LOTE&Sexo=F&Tipo=M&Limitado=0&ValorV=100.00&ValorP=100.00&Esgotado=0&Lugar=0&ChaveImg=&QtdImg=0&" \
		"ChaveModelo=34bb4453&TaxaAdm=0.00&VendaFechada=1&Cupom=0&Vinculo=N&Multiplo=0|CodigoBusca=1995&EventoId=2&Id=1&" \
		"LoteId=2&Ingresso=PISTA%205&Lote=1%BA%20LOTE&Sexo=U&Tipo=I&Limitado=0&ValorV=1.00&ValorP=1.00&Esgotado=0&Lugar=0&"\
		"ChaveImg=&QtdImg=0&ChaveModelo=884922ec&TaxaAdm=0.00&VendaFechada=1&Cupom=0&Vinculo=N&Multiplo=0|CodigoBusca=16580&"\
		"EventoId=1658&Id=13514&LoteId=3927&Ingresso=INGRESSO%20FEMININO%20-%201%BA%20LOTE&Lote=1%B0%20LOTE&Sexo=F&Tipo=M&"\
		"Limitado=0&ValorV=15.00&ValorP=&Esgotado=0&Lugar=0&ChaveImg=&QtdImg=0&ChaveModelo=24eccf7f&TaxaAdm=0.00&VendaFechada=1&"\
		"Cupom=0&Vinculo=N&Multiplo=0|CodigoBusca=16581&EventoId=1658&Id=13515&LoteId=3927&" \
		"Ingresso=INGRESSO%20MASCULINO%20-%201%BA%20LOTE&Lote=1%B0%20LOTE&Sexo=M&Tipo=M&Limitado=0&"\
		"ValorV=20.00&ValorP=&Esgotado=0&Lugar=0&ChaveImg=&QtdImg=0&ChaveModelo=ac5d7d71&TaxaAdm=0.00&"\
		"VendaFechada=1&Cupom=0&Vinculo=N&Multiplo=0|CodigoBusca=16588&EventoId=1658&Id=13523&LoteId=3927&"\
		"Ingresso=INGRESSO%20%2A%20CORTESIA%20%2A&Lote=1%B0%20LOTE&Sexo=U&Tipo=M&Limitado=0&ValorV=0.00&"\
		"ValorP=&Esgotado=0&Lugar=0&ChaveImg=&QtdImg=0&ChaveModelo=1a4ad3cc&TaxaAdm=0.00&VendaFechada=1&"\
		"Cupom=0&Vinculo=N&Multiplo=0|CodigoBusca=16588&EventoId=1658&Id=13523&LoteId=3928&Ingresso="\
		"INGRESSO%20%2A%20CORTESIA%20%2A&Lote=2%BA%20LOTE&Sexo=U&Tipo=M&Limitado=0&ValorV=0.00&ValorP="\
		"&Esgotado=0&Lugar=0&ChaveImg=&QtdImg=0&ChaveModelo=3a4d1cca&TaxaAdm=0.00&VendaFechada=1&Cupom=0&"\
		"Vinculo=N&Multiplo=0|CodigoBusca=16588&EventoId=1658&Id=13523&LoteId=3929&Ingresso="\
		"INGRESSO%20%2A%20CORTESIA%20%2A&Lote=3%BA%20LOTE&Sexo=U&Tipo=M&Limitado=0&ValorV=0.00&ValorP=&"\
		"Esgotado=0&Lugar=0&ChaveImg=&QtdImg=0&ChaveModelo=ca34cad1&TaxaAdm=0.00&VendaFechada=1&Cupom=0&Vinculo=N&Multiplo=0";
	const TRexChar *begin, *end, *error;
	TRex* x;	
	int n,i=0;
	struct data {
		char* eventoid;
		char* id;
		char* loteid;
		char* ingresso;
		char* lote;
		char* sexo;
		char* tipo;
		char* limitado;
		char* valorv;
		char* valorp;
		char* esgotado;
		char* lugar;
		char* chaveImg;
		char* qtdImg;
		char* chaveModelo;
		char* taxaAdm;
		char* vendaFechada;
		char* cupom;
		char* vinculo;
		char* multiplo;
	} datatab[] = {
		"2","13428","2","PISTA%201","1%BA%20LOTE","F","M","0","100.00","100.00","0","0","0","0","34bb4453","0.00","1","0","N","0",
		"2","1","2","PISTA%205","1%BA%20LOTE","U","I","0","1.00","1.00","0","0","0","0","884922ec","0.0","1","0","N","0",
		"1658","13514","3927","INGRESSO%20FEMININO%20-%201%BA%20LOTE","1%B0%20LOTE","F","M","0","15.00","0.00","0","0","0","0","24eccf7f","0.00","1","0","N","0",
		"1658","13515","3927","INGRESSO%20MASCULINO%20-%201%BA%20LOTE","1%B0%20LOTE","M","M","0","20.00","0.00","0","0","0","0","ac5d7d71","0.00","1","0","N","0",
		"1658","13523","3927","INGRESSO%20%2A%20CORTESIA%20%2A","1%B0%20LOTE","U","M","0","0.00","0.00","0","0","0","0","1a4ad3cc","0.00","1","0","N","0",
		"1658","13523","3928","INGRESSO%20%2A%20CORTESIA%20%2A","2%BA%20LOTE","U","M","0","0.00","0.00","0","0","0","0","3a4d1cca","0.00","1","0","N","0",
		"1658","13523","3929","INGRESSO%20%2A%20CORTESIA%20%2A","3%BA%20LOTE","U","M","0","0.00","0.00","0","0","0","0","ca34cad1","0.00","1","0","N","0",

	};
	// workaround for a regexp bug..
	char* newcontent, *tmp, *tmp2;
	tmp2 = strReplace(content,"ValorP=&","ValorP=0.00&");
	newcontent = strReplace(tmp2,"ChaveImg=&","ChaveImg=0.00&");
	if (NULL!=tmp2) free(tmp2);
	tmp = newcontent;
	CHECK(NULL!=(x=trex_compile(_TREXC("EventoId=(\\d+)&Id=(\\d+)&LoteId=(\\d+)&Ingresso=([\\c\\w\\s\\p]+)&" \
		"Lote=([\\c\\w\\s\\p]+)&Sexo=(\\w)&Tipo=(\\w)&Limitado=(\\d)&ValorV=([\\d\\p]+)&ValorP=([\\d\\p]+)&"\
		"Esgotado=(\\d)&Lugar=(\\d+)&ChaveImg=(\\d+)&QtdImg=(\\d+)&ChaveModelo=([\\c\\w\\s\\p]+)&"\
		"TaxaAdm=([\\d\\p]+)&VendaFechada=(\\d)&Cupom=(\\d)&Vinculo=(\\w)&Multiplo=(\\d)"),&error)));
	while (trex_search(x,_TREXC(tmp),&begin,&end)) {
		n = trex_getsubexpcount(x);	
		if (n==21) {
			char *eventoid, *id, *loteid, *ingresso, *lote, *sexo, *tipo, *limitado, *valorv, *valorp, *esgotado;					
			char* lugar, *chaveImg, *qtdImg, *chaveModelo, *taxaAdm, *vendaFechada, *cupom, *vinculo, *multiplo;
			rx_getnext(x, 1, &eventoid);
			rx_getnext(x, 2, &id);
			rx_getnext(x, 3, &loteid);
			rx_getnext(x, 4, &ingresso);
			rx_getnext(x, 5, &lote);
			rx_getnext(x, 6, &sexo);
			rx_getnext(x, 7, &tipo);
			rx_getnext(x, 8, &limitado);
			rx_getnext(x, 9, &valorv);
			rx_getnext(x, 10, &valorp);
			rx_getnext(x, 11, &esgotado);
			rx_getnext(x, 12, &lugar);
			rx_getnext(x, 13, &chaveImg);
			rx_getnext(x, 14, &qtdImg);
			rx_getnext(x, 15, &chaveModelo);
			rx_getnext(x, 16, &taxaAdm);
			rx_getnext(x, 17, &vendaFechada);
			rx_getnext(x, 18, &cupom);
			rx_getnext(x, 19, &vinculo);
			rx_getnext(x, 20, &multiplo);
			CuAssertTrue(tc,0==strcmp(datatab[i].eventoid,eventoid));
			CuAssertTrue(tc,0==strcmp(datatab[i].id,id));
			CuAssertTrue(tc,0==strcmp(datatab[i].loteid,loteid));
			CuAssertTrue(tc,0==strcmp(datatab[i].ingresso,ingresso));
			CuAssertTrue(tc,0==strcmp(datatab[i].lote,lote));
			CuAssertTrue(tc,0==strcmp(datatab[i].sexo,sexo));
			CuAssertTrue(tc,0==strcmp(datatab[i].tipo,tipo));
			CuAssertTrue(tc,0==strcmp(datatab[i].limitado,limitado));
			CuAssertTrue(tc,0==strcmp(datatab[i].eventoid,eventoid));
			CuAssertTrue(tc,0==strcmp(datatab[i].valorv,valorv));
			CuAssertTrue(tc,0==strcmp(datatab[i].valorp,valorp));
			CuAssertTrue(tc,0==strcmp(datatab[i].esgotado,esgotado));
			CuAssertTrue(tc,0==strcmp(datatab[i].lugar,lugar));
			CuAssertTrue(tc,0==strcmp(datatab[i].chaveImg,chaveImg));
			CuAssertTrue(tc,0==strcmp(datatab[i].taxaAdm,taxaAdm));
			CuAssertTrue(tc,0==strcmp(datatab[i].vendaFechada,vendaFechada));
			CuAssertTrue(tc,0==strcmp(datatab[i].cupom,cupom));
			CuAssertTrue(tc,0==strcmp(datatab[i].vinculo,vinculo));
			CuAssertTrue(tc,0==strcmp(datatab[i].multiplo,multiplo));
			i++;
		}
		tmp = end;
	}
	if (NULL!=newcontent) 
		free(newcontent);
}

CuSuite* CuGetTRexSuite(void)
{
	CuSuite* suite = CuSuiteNew();
	SUITE_ADD_TEST(suite, testCuTest1);
	SUITE_ADD_TEST(suite, testCuTest2);
	return suite;
}

#endif