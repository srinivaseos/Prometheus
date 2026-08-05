#include <sys/types.h>
#include <sys/stat.h>

#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <fts.h>
#include <fnmatch.h>
#include <bsd/stdlib.h>


#include "hpack.h"


typedef enum {
	JSMN_UNDEFINED = 0,
	JSMN_OBJECT = 1,
	JSMN_ARRAY = 2,
	JSMN_STRING = 3,
	JSMN_PRIMITIVE = 4
} jsmntype_t;


enum jsmnerr {
	/* Not enough tokens were provided */
	JSMN_ERROR_NOMEM = -1,
	/* Invalid character inside JSON string */
	JSMN_ERROR_INVAL = -2,
	/* The string is not a full JSON packet, more bytes expected */
	JSMN_ERROR_PART = -3
};


typedef struct {
	jsmntype_t type;
	int start;
	int end;
	int size;
#ifdef JSMN_PARENT_LINKS
	int parent;
#endif
} jsmntok_t;


typedef struct {
	unsigned int pos; /* offset in the JSON string */
	unsigned int toknext; /* next token to allocate */
	int toksuper; /* superior token node, e.g parent object or array */
} jsmn_parser;


struct	jsmnn 
{
	struct parse	*p; /* parser object */
	union 
	{
		char *str; /* JSMN_PRIMITIVE, JSMN_STRING */
		struct jsmnp *obj; /* JSMN_OBJECT */
		struct jsmnn **array; /* JSMN_ARRAY */
	} d;
	size_t		 fields; /* entries in "d" */
	jsmntype_t	 type; /* type of node */
};

struct	jsmnp {
	struct jsmnn	*lhs; /* left of colon */
	struct jsmnn	*rhs; /* right of colon */
};


/* JSON parsing routines */
struct jsmnn	*json_parse(const char *, size_t);
void		 json_free(struct jsmnn *);
struct jsmnn	*json_getarrayobj(struct jsmnn *);
struct jsmnn	*json_getarray(struct jsmnn *, const char *);
struct jsmnn	*json_getobj(struct jsmnn *, const char *);
char		*json_getstr(struct jsmnn *, const char *);





void jsmn_init( jsmn_parser * parser);
int  jsmn_parse( jsmn_parser * parser, const char * js, size_t len, jsmntok_t * tokens, unsigned int num_tokens);


static jsmntok_t *jsmn_alloc_token( jsmn_parser * parser, jsmntok_t * tokens, size_t num_tokens) 
{
	jsmntok_t *tok;
	if (parser->toknext >= num_tokens) 
	{
		return NULL;
	}
	tok = &tokens[parser->toknext++];
	tok->start = tok->end = -1;
	tok->size = 0;
#ifdef JSMN_PARENT_LINKS
	tok->parent = -1;
#endif
	return tok;
}


static void jsmn_fill_token(jsmntok_t *token, jsmntype_t type, int start, int end) 
{
	token->type = type;
	token->start = start;
	token->end = end;
	token->size = 0;
}


static int jsmn_parse_primitive( jsmn_parser * parser, const char * js, size_t len, jsmntok_t *tokens, size_t num_tokens) 
{
	jsmntok_t *token;
	int start;

	start = parser->pos;

	for (; parser->pos < len && js[parser->pos] != '\0'; parser->pos++) 
	{
		switch (js[parser->pos]) {
#ifndef JSMN_STRICT
			/* In strict mode primitive must be followed by "," or "}" or "]" */
			case ':':
#endif
			case '\t' : case '\r' : case '\n' : case ' ' :
			case ','  : case ']'  : case '}' :
				goto found;
		}
		if (js[parser->pos] < 32 || js[parser->pos] >= 127) {
			parser->pos = start;
			return JSMN_ERROR_INVAL;
		}
	}
#ifdef JSMN_STRICT
	/* In strict mode primitive must be followed by a comma/object/array */
	parser->pos = start;
	return JSMN_ERROR_PART;
#endif

found:
	if (tokens == NULL) {
		parser->pos--;
		return 0;
	}
	token = jsmn_alloc_token(parser, tokens, num_tokens);
	if (token == NULL) {
		parser->pos = start;
		return JSMN_ERROR_NOMEM;
	}
	jsmn_fill_token(token, JSMN_PRIMITIVE, start, parser->pos);
#ifdef JSMN_PARENT_LINKS
	token->parent = parser->toksuper;
#endif
	parser->pos--;
	return 0;
}


static int jsmn_parse_string( jsmn_parser * parser, const char * js, size_t len, jsmntok_t *tokens, size_t num_tokens) 
{
	jsmntok_t *token;

	int start = parser->pos;

	parser->pos++;

	/* Skip starting quote */
	for (; parser->pos < len && js[parser->pos] != '\0'; parser->pos++) {
		char c = js[parser->pos];

		/* Quote: end of string */
		if (c == '\"') {
			if (tokens == NULL) {
				return 0;
			}
			token = jsmn_alloc_token(parser, tokens, num_tokens);
			if (token == NULL) {
				parser->pos = start;
				return JSMN_ERROR_NOMEM;
			}
			jsmn_fill_token(token, JSMN_STRING, start+1, parser->pos);
#ifdef JSMN_PARENT_LINKS
			token->parent = parser->toksuper;
#endif
			return 0;
		}

		/* Backslash: Quoted symbol expected */
		if (c == '\\' && parser->pos + 1 < len) {
			int i;
			parser->pos++;
			switch (js[parser->pos]) {
				/* Allowed escaped symbols */
				case '\"': case '/' : case '\\' : case 'b' :
				case 'f' : case 'r' : case 'n'  : case 't' :
					break;
				/* Allows escaped symbol \uXXXX */
				case 'u':
					parser->pos++;
					for(i = 0; i < 4 && parser->pos < len && js[parser->pos] != '\0'; i++) {
						/* If it isn't a hex character we have an error */
						if(!((js[parser->pos] >= 48 && js[parser->pos] <= 57) || /* 0-9 */
									(js[parser->pos] >= 65 && js[parser->pos] <= 70) || /* A-F */
									(js[parser->pos] >= 97 && js[parser->pos] <= 102))) { /* a-f */
							parser->pos = start;
							return JSMN_ERROR_INVAL;
						}
						parser->pos++;
					}
					parser->pos--;
					break;
				/* Unexpected symbol */
				default:
					parser->pos = start;
					return JSMN_ERROR_INVAL;
			}
		}
	}
	parser->pos = start;
	return JSMN_ERROR_PART;
}



int jsmn_parse(jsmn_parser *parser, const char *js, size_t len,
		jsmntok_t *tokens, unsigned int num_tokens) {
	int r;
	int i;
	jsmntok_t *token;
	int count = parser->toknext;

	for (; parser->pos < len && js[parser->pos] != '\0'; parser->pos++) {
		char c;
		jsmntype_t type;

		c = js[parser->pos];
		switch (c) {
			case '{': case '[':
				count++;
				if (tokens == NULL) {
					break;
				}
				token = jsmn_alloc_token(parser, tokens, num_tokens);
				if (token == NULL)
					return JSMN_ERROR_NOMEM;
				if (parser->toksuper != -1) {
					tokens[parser->toksuper].size++;
#ifdef JSMN_PARENT_LINKS
					token->parent = parser->toksuper;
#endif
				}
				token->type = (c == '{' ? JSMN_OBJECT : JSMN_ARRAY);
				token->start = parser->pos;
				parser->toksuper = parser->toknext - 1;
				break;
			case '}': case ']':
				if (tokens == NULL)
					break;
				type = (c == '}' ? JSMN_OBJECT : JSMN_ARRAY);
#ifdef JSMN_PARENT_LINKS
				if (parser->toknext < 1) {
					return JSMN_ERROR_INVAL;
				}
				token = &tokens[parser->toknext - 1];
				for (;;) {
					if (token->start != -1 && token->end == -1) {
						if (token->type != type) {
							return JSMN_ERROR_INVAL;
						}
						token->end = parser->pos + 1;
						parser->toksuper = token->parent;
						break;
					}
					if (token->parent == -1) {
						break;
					}
					token = &tokens[token->parent];
				}
#else
				for (i = parser->toknext - 1; i >= 0; i--) {
					token = &tokens[i];
					if (token->start != -1 && token->end == -1) {
						if (token->type != type) {
							return JSMN_ERROR_INVAL;
						}
						parser->toksuper = -1;
						token->end = parser->pos + 1;
						break;
					}
				}
				/* Error if unmatched closing bracket */
				if (i == -1) return JSMN_ERROR_INVAL;
				for (; i >= 0; i--) {
					token = &tokens[i];
					if (token->start != -1 && token->end == -1) {
						parser->toksuper = i;
						break;
					}
				}
#endif
				break;
			case '\"':
				r = jsmn_parse_string(parser, js, len, tokens, num_tokens);
				if (r < 0) return r;
				count++;
				if (parser->toksuper != -1 && tokens != NULL)
					tokens[parser->toksuper].size++;
				break;
			case '\t' : case '\r' : case '\n' : case ' ':
				break;
			case ':':
				parser->toksuper = parser->toknext - 1;
				break;
			case ',':
				if (tokens != NULL && parser->toksuper != -1 &&
						tokens[parser->toksuper].type != JSMN_ARRAY &&
						tokens[parser->toksuper].type != JSMN_OBJECT) {
#ifdef JSMN_PARENT_LINKS
					parser->toksuper = tokens[parser->toksuper].parent;
#else
					for (i = parser->toknext - 1; i >= 0; i--) {
						if (tokens[i].type == JSMN_ARRAY || tokens[i].type == JSMN_OBJECT) {
							if (tokens[i].start != -1 && tokens[i].end == -1) {
								parser->toksuper = i;
								break;
							}
						}
					}
#endif
				}
				break;
#ifdef JSMN_STRICT
			/* In strict mode primitives are: numbers and booleans */
			case '-': case '0': case '1' : case '2': case '3' : case '4':
			case '5': case '6': case '7' : case '8': case '9':
			case 't': case 'f': case 'n' :
				/* And they must not be keys of the object */
				if (tokens != NULL && parser->toksuper != -1) {
					jsmntok_t *t = &tokens[parser->toksuper];
					if (t->type == JSMN_OBJECT ||
							(t->type == JSMN_STRING && t->size != 0)) {
						return JSMN_ERROR_INVAL;
					}
				}
#else
			/* In non-strict mode every unquoted value is a primitive */
			default:
#endif
				r = jsmn_parse_primitive(parser, js, len, tokens, num_tokens);
				if (r < 0) return r;
				count++;
				if (parser->toksuper != -1 && tokens != NULL)
					tokens[parser->toksuper].size++;
				break;

#ifdef JSMN_STRICT
			/* Unexpected char in strict mode */
			default:
				return JSMN_ERROR_INVAL;
#endif
		}
	}

	if (tokens != NULL) {
		for (i = parser->toknext - 1; i >= 0; i--) {
			/* Unmatched opened object or array */
			if (tokens[i].start != -1 && tokens[i].end == -1) {
				return JSMN_ERROR_PART;
			}
		}
	}

	return count;
}


/**
 * Creates a new parser based over a given  buffer with an array of tokens
 * available.
 */
void jsmn_init(jsmn_parser *parser) 
{
	parser->pos = 0;
	parser->toknext = 0;
	parser->toksuper = -1;
}


////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////



struct	parse 
{
	struct jsmnn	*nodes; /* all nodes */
	size_t		 cur; /* current number */
	size_t		 max; /* nodes in "nodes" */
};




static ssize_t
build(struct parse *parse, struct jsmnn **np,
    jsmntok_t *t, const char *js, size_t sz)
{
	size_t		 i, j;
	struct jsmnn	*n;
	ssize_t		 tmp;

	if (sz == 0)
		return 0;

	//assert(parse->cur < parse->max);
	n = *np = &parse->nodes[parse->cur++];
	n->p = parse;
	n->type = t->type;

	switch (t->type) {
	case JSMN_STRING:
		/* FALLTHROUGH */
	case JSMN_PRIMITIVE:
		n->fields = 1;
		n->d.str = strndup
			(js + t->start,
			 t->end - t->start);
		if (n->d.str == NULL)
			break;
		return 1;
	case JSMN_OBJECT:
		n->fields = t->size;
		n->d.obj = calloc(n->fields,
			sizeof(struct jsmnp));
		if (n->d.obj == NULL)
			break;
		for (i = j = 0; i < (size_t)t->size; i++) {
			tmp = build(parse,
				&n->d.obj[i].lhs,
				t + 1 + j, js, sz - j);
			if (tmp < 0)
				break;
			j += tmp;
			tmp = build(parse,
				&n->d.obj[i].rhs,
				t + 1 + j, js, sz - j);
			if (tmp < 0)
				break;
			j += tmp;
		}
		if (i < (size_t)t->size)
			break;
		return j + 1;
	case JSMN_ARRAY:
		n->fields = t->size;
		n->d.array = calloc(n->fields,
			sizeof(struct jsmnn *));
		if (n->d.array == NULL)
			break;
		for (i = j = 0; i < (size_t)t->size; i++) {
			tmp = build(parse,
				&n->d.array[i],
				t + 1 + j, js, sz - j);
			if (tmp < 0)
				break;
			j += tmp;
		}
		if (i < (size_t)t->size)
			break;
		return j + 1;
	default:
		break;
	}

	return -1;
}

/*
 * Fully free up a parse sequence.
 * This handles all nodes sequentially, not recursively.
 */
static void
jsmnparse_free(struct parse *p)
{
	size_t	 i;

	if (p == NULL)
		return;
	for (i = 0; i < p->max; i++) {
		struct jsmnn	*n = &p->nodes[i];
		switch (n->type) {
		case JSMN_ARRAY:
			free(n->d.array);
			break;
		case JSMN_OBJECT:
			free(n->d.obj);
			break;
		case JSMN_PRIMITIVE:
			free(n->d.str);
			break;
		case JSMN_STRING:
			free(n->d.str);
			break;
		case JSMN_UNDEFINED:
			break;
		}
	}
	free(p->nodes);
	free(p);
}

/*
 * Allocate a tree representation of "t".
 * This returns NULL on allocation failure or when sz is zero, in which
 * case all resources allocated along the way are freed already.
 */
static struct jsmnn *
jsmntree_alloc(jsmntok_t *t, const char *js, size_t sz)
{
	struct jsmnn	*first;
	struct parse	*p;

	if (sz == 0)
		return NULL;

	p = calloc(1, sizeof(struct parse));
	if (p == NULL)
		return NULL;

	p->max = sz;
	p->nodes = calloc(p->max, sizeof(struct jsmnn));
	if (p->nodes == NULL) {
		free(p);
		return NULL;
	}

	if (build(p, &first, t, js, sz) < 0) {
		jsmnparse_free(p);
		first = NULL;
	}

	return first;
}

/*
 * Call through to free parse contents.
 */
void
json_free(struct jsmnn *first)
{

	if (first != NULL)
		jsmnparse_free(first->p);
}

/*
 * Just check that the array object is in fact an object.
 */
struct jsmnn *
json_getarrayobj(struct jsmnn *n)
{

	return n->type != JSMN_OBJECT ? NULL : n;
}

/*
 * Extract an array from the returned JSON object, making sure that it's
 * the correct type.
 * Returns NULL on failure.
 */
struct jsmnn *
json_getarray(struct jsmnn *n, const char *name)
{
	size_t		 i;

	if (n->type != JSMN_OBJECT)
		return NULL;
	for (i = 0; i < n->fields; i++) {
		if (n->d.obj[i].lhs->type != JSMN_STRING &&
		    n->d.obj[i].lhs->type != JSMN_PRIMITIVE)
			continue;
		else if (strcmp(name, n->d.obj[i].lhs->d.str))
			continue;
		break;
	}
	if (i == n->fields)
		return NULL;
	if (n->d.obj[i].rhs->type != JSMN_ARRAY)
		return NULL;
	return n->d.obj[i].rhs;
}

/*
 * Extract subtree from the returned JSON object, making sure that it's
 * the correct type.
 * Returns NULL on failure.
 */
struct jsmnn *
json_getobj(struct jsmnn *n, const char *name)
{
	size_t		 i;

	if (n->type != JSMN_OBJECT)
		return NULL;
	for (i = 0; i < n->fields; i++) {
		if (n->d.obj[i].lhs->type != JSMN_STRING &&
		    n->d.obj[i].lhs->type != JSMN_PRIMITIVE)
			continue;
		else if (strcmp(name, n->d.obj[i].lhs->d.str))
			continue;
		break;
	}
	if (i == n->fields)
		return NULL;
	if (n->d.obj[i].rhs->type != JSMN_OBJECT)
		return NULL;
	return n->d.obj[i].rhs;
}

/*
 * Extract a single string from the returned JSON object, making sure
 * that it's the correct type.
 * Returns NULL on failure.
 */
char *
json_getstr(struct jsmnn *n, const char *name)
{
	size_t		 i;
	char		*cp;

	if (n->type != JSMN_OBJECT)
		return NULL;
	for (i = 0; i < n->fields; i++) {
		if (n->d.obj[i].lhs->type != JSMN_STRING &&
		    n->d.obj[i].lhs->type != JSMN_PRIMITIVE)
			continue;
		else if (strcmp(name, n->d.obj[i].lhs->d.str))
			continue;
		break;
	}
	if (i == n->fields)
		return NULL;
	if (n->d.obj[i].rhs->type != JSMN_STRING &&
	    n->d.obj[i].rhs->type != JSMN_PRIMITIVE)
		return NULL;

	cp = strdup(n->d.obj[i].rhs->d.str);
	if (cp == NULL)
		perror("strdup\n");
	return cp;
}

/*
 * Parse an HTTP response body from a buffer of size "sz".
 * Returns an opaque pointer on success, otherwise NULL on error.
 */
struct jsmnn *
json_parse(const char *buf, size_t sz)
{
	struct jsmnn	*n;
	jsmn_parser	 p;
	jsmntok_t	*tok;
	int		 r;
	size_t		 tokcount;

	jsmn_init(&p);
	tokcount = 32768;

	/* Do this until we don't need any more tokens. */
again:
	tok = calloc(tokcount, sizeof(jsmntok_t));
	if (tok == NULL) {
		perror("calloc");
		return NULL;
	}

	/* Actually try to parse the JSON into the tokens. */

	r = jsmn_parse(&p, buf, sz, tok, tokcount);
	if (r < 0 && r == JSMN_ERROR_NOMEM) {
		tokcount *= 2;
		free(tok);
		goto again;
	} else if (r < 0) {
		printf("jsmn_parse: %d\n", r);
		free(tok);
		return NULL;
	}

	/* Now parse the tokens into a tree. */

	n = jsmntree_alloc(tok, buf, r);
	free(tok);
	return n;
}



int	 verbose = 0;
int	 encode = 0;

static void h_log(int level, const char *fmt, ...)
{
	va_list	ap;

	//if (verbose < level)
	//	return;

	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
}


static const char * json_uascii_decode( char *str)
{
	char		*p, *q;
	char		 hex[5];
	unsigned long	 x;

	hex[4] = '\0';
	p = q = str;

	while (*p != '\0') {
		if (*p != '\\') {
			*q = *p;
			p++;
			q++;
			continue;
		}
		switch (*(p + 1)) {
		case 'u':
			/* Encoding character is followed by four hex chars */
			if (!(isxdigit((unsigned char)p[2]) &&
			    isxdigit((unsigned char)p[3]) &&
			    isxdigit((unsigned char)p[4]) &&
			    isxdigit((unsigned char)p[5])))
				return (NULL);

			hex[0] = p[2];
			hex[1] = p[3];
			hex[2] = p[4];
			hex[3] = p[5];

			/* We don't support non-ASCII chars */
			if ((x = strtoul(hex, NULL, 16)) > 0x7f)
				return (NULL);
			*q = (char)x;
			p += 5;
			break;
		case '"':
			*q = '"';
			p++;
			break;
		default:
			*q = *p;
			break;
		}
		p++;
		q++;
	}
	*q = '\0';

	return (str);
}



static int hpack_headerblock_cmp( struct hpack_headerblock * a, struct hpack_headerblock * b)
{
	struct hpack_header	*ha, *hb;

	for (ha = TAILQ_FIRST(a), hb = TAILQ_FIRST(b);
	    !(ha == NULL && hb == NULL);
	    ha = TAILQ_NEXT(ha, hdr_entry),
	    hb = TAILQ_NEXT(hb, hdr_entry)) {
#define ONE_NULL(_a, _b)	(		\
	((_a) != NULL && (_b) == NULL) ||	\
	((_a) == NULL && (_b) != NULL)		\
)
		if (ONE_NULL(ha, hb) ||
		    ONE_NULL(ha->hdr_name, hb->hdr_name) ||
		    ONE_NULL(ha->hdr_value, hb->hdr_value))
			return (-1);
#undef ONE_NULL
		if (ha->hdr_name != NULL &&
		    strcmp(ha->hdr_name, hb->hdr_name) != 0)
			return (-2);
		if (ha->hdr_value != NULL &&
		    strcmp(ha->hdr_value, hb->hdr_value) != 0)
			return (-3);
	}

	return (0);
}




static int hpack_headerblock_print(const char *prefix, struct hpack_headerblock *hdrs)
{
	struct hpack_header	*hdr;

	if (hdrs == NULL)
		return (0);
	if (TAILQ_EMPTY(hdrs)) {
		h_log(2, "%s: empty headers\n", prefix);
		return (-1);
	}

	TAILQ_FOREACH(hdr, hdrs, hdr_entry) {
		if (hdr->hdr_name == NULL || hdr->hdr_value == NULL) {
			if (prefix != NULL)
				h_log(2, "%s invalid header: %s: %s\n", prefix,
				    hdr->hdr_name == NULL ?
				    "(null)" : hdr->hdr_name,
				    hdr->hdr_value == NULL ?
				    "(null)" : hdr->hdr_value);
			return (-1);
		}
		if (prefix != NULL)
			h_log(2, "%s %s: %s\n", prefix,
			    hdr->hdr_name, hdr->hdr_value);
	}

	return (0);
}


static int x2i( const char *s)
{
	char	ss[3];

	ss[0] = s[0];
	ss[1] = s[1];
	ss[2] = 0;

	if (!isxdigit(s[0]) || !isxdigit(s[1])) {
		h_log(2, "string needs to be specified in hex digits\n");
		return (-1);
	}
	return ((int)strtoul(ss, NULL, 16));
}



static ssize_t parsehex( const char * hex, unsigned char *buf, size_t len)
{
	ssize_t		  datalen;
	unsigned int	  i;
	int		  c;

	memset(buf, 0, len);
	if (strncmp(hex, "0x", 2) == 0)
		hex += 2;
	datalen = strlen(hex) / 2;
	if (datalen > (ssize_t)len)
		return (-1);

	for (i = 0; i < datalen; i++) {
		if ((c = x2i(hex + 2 * i)) == -1)
			return (-1);
		buf[i] =  (unsigned char)c;
	}

	return (datalen);
}




static int parse_data( unsigned char * buf, size_t len, struct hpack_headerblock * test, struct hpack_table * hpack)
{
	struct hpack_headerblock	*hdrs = NULL;
	int				 ret = -1;

	if ((hdrs = hpack_decode(buf, len, hpack)) == NULL) 
	{
		h_log(2, "hpack_decode\n");
		goto fail;
	}

	if (test != NULL && hpack_headerblock_print(NULL, test) == -1) 
	{
		h_log(2, "test headers invalid %d\n", __LINE__);
		goto fail;
	}
	
	if (hpack_headerblock_print(NULL, hdrs) == -1) 
	{
		h_log(2, "parsed headers invalid %d\n", __LINE__);
		goto fail;
	}
	
	if (test != NULL && (ret = hpack_headerblock_cmp(hdrs, test)) != 0) 
	{
		h_log(2, "test headers mismatched (returned %d  %d)\n", ret, __LINE__);
		ret = -1;
		goto fail;
	}

	ret = 0;
 fail:
	if (ret != 0) 
	{
		hpack_headerblock_print(">>> header:", test);
		hpack_headerblock_print("<<< parsed:", hdrs);
	}
	
	hpack_headerblock_free(hdrs);
	return (ret);
}



static int parse_hex( const char * hex, struct hpack_headerblock * test, struct hpack_table * hpack)
{
	unsigned char			 buf[8192];
	ssize_t				 len;

	if ((len = parsehex(hex, buf, sizeof(buf))) == -1) {
		h_log(2, "wire format is not a hex string\n");
		return (-1);
	}


	printf( "len=%ld  %s|%d\n", len, __FUNCTION__, __LINE__);


	if (parse_data(buf, len, test, hpack) == -1)
		return (-1);

	h_log(2, ">>> wire: %s\n", hex);
	return (0);
}



static ssize_t parse_input(const char *name, size_t init_table_size)
{
	struct hpack_table	*hpack = NULL;
	FILE			*fp;
	char			 buf[BUFSIZ];
	ssize_t			 ok = 0, ret = -1;

	if (encode)
		return (-1);
	if (strcmp("-", name) == 0)
		fp = stdin;
	else if ((fp = fopen(name, "r")) == NULL)
		return (-1);

	if ((hpack = hpack_table_new(init_table_size)) == NULL)
		goto done;

	while (fgets(buf, sizeof(buf), fp) != NULL) {
		buf[strcspn(buf, "\r\n")] = '\0';
		if (parse_hex(buf, NULL, hpack) == -1) {
			h_log(1, "hex HPACK decoding failed\n");
			goto done;
		}
		ok++;
	}

	ret = ok;
 done:
	if (fp != NULL && fp != stdin)
		fclose(fp);
	hpack_table_free(hpack);

	return (ret);
}

static int parse_raw(const char *name, size_t init_table_size)
{
	char				 buf[65535], *ptr = NULL, *k, *v;
	struct hpack_table		*hpack = NULL;
	struct hpack_headerblock	*hdrs = NULL;
	FILE				*fp;
	int				 ret = -1;
	size_t				 len;

	if (strcmp("-", name) == 0)
		fp = stdin;
	else if ((fp = fopen(name, "r")) == NULL)
		goto done;
	if ((hpack = hpack_table_new(init_table_size)) == NULL)
		goto done;
	if (encode) {
		if ((hdrs = hpack_headerblock_new()) == NULL)
			goto done;
		while (fgets(buf, sizeof(buf), fp) != NULL) {
			buf[strcspn(buf, "\r\n")] = '\0';
			k = buf;
			if ((v = strchr(k + 1, ':')) != NULL) {
				*v++ = '\0';
				v += strspn(v, " \t");
			} else if (isupper(buf[0])) {
				/* cheap way to test for the method */
				buf[strcspn(buf, " \t")] = '\0';
				k = ":method";
				v = buf;
			} else
				v = "";
			h_log(2, "adding header '%s: %s'\n", k, v);
			if (hpack_header_add(hdrs,
			    k, v, HPACK_INDEX) == NULL)
				goto done;
		}
		if ((ptr = hpack_encode(hdrs, &len, hpack)) == NULL) {
			h_log(1, "raw HPACK decoding failed\n");
			goto done;
		}
	} else {
		if ((len = fread(buf, 1, sizeof(buf), fp)) < 1) {
			if (feof(fp))
				ret = 0;
			goto done;
		}
		ptr = buf;
	}

	if ((hdrs = hpack_decode(ptr, len, hpack)) == NULL) {
		h_log(1, "raw HPACK decoding failed\n");
		goto done;
	}

	ret = 0;
 done:
	if (ptr != buf)
		free(ptr);
	if (fp != NULL && fp != stdin)
		fclose(fp);
	hpack_headerblock_free(hdrs);
	hpack_table_free(hpack);

	return (ret);
}

static int encode_huffman(const char *name);
static int decode_huffman(const char *name);


static int parse_dir(char *argv[], size_t init_table_size)
{
	struct hpack_table		*hpack = NULL, *hpack2 = NULL;
	struct hpack_headerblock	*test = NULL;
	FTS				*fts;
	FTSENT				*ftsp = NULL;
	char				*str = NULL, *wire = NULL, *tblsz;
	FILE				*fp;
	off_t				 size;
	int				 ret = -1;
	struct jsmnn			*json = NULL, *cases, *obj, *hdr, *hdrs;
	size_t				 i = 0, j, k;
	ssize_t				 ok = 0;
	const char			*errstr = NULL;
	size_t				 table_size, file_table_size, len;

	if (encode)
		return (-1);
	if ((fts = fts_open(argv, FTS_COMFOLLOW|FTS_NOCHDIR, NULL)) == NULL) {
		errstr = "failed to open directory";
		goto done;
	}

	while ((ftsp = fts_read(fts)) != NULL) {
		if (ftsp->fts_info != FTS_F)
			continue;

		file_table_size = init_table_size;

		if (fnmatch("*.hpacktest", ftsp->fts_name,
		    FNM_PATHNAME) != FNM_NOMATCH) {
			if ((ok = parse_input(ftsp->fts_accpath,
			    file_table_size)) < 0) {
				errstr = "hex input file parsing failed";
				goto done;
			}
			goto next;
		} else if (fnmatch("*.hpackraw", ftsp->fts_name,
		    FNM_PATHNAME) != FNM_NOMATCH) {
			if (parse_raw(ftsp->fts_accpath,
			    file_table_size) == -1) {
				errstr = "raw input file parsing failed";
				goto done;
			}
			ok = 1;
			goto next;
		} else if (fnmatch("headers_??.txt", ftsp->fts_name,
		    FNM_PATHNAME) != FNM_NOMATCH) {
			if (encode_huffman(ftsp->fts_accpath) == -1) {
				errstr = "huffman encoding failed";
				goto done;
			}
			ok = 1;
			goto next;
		} else if (fnmatch("story_*.json", ftsp->fts_name,
		    FNM_PATHNAME) == FNM_NOMATCH)
			continue;
		size = ftsp->fts_statp->st_size;
		ok = 0;

		if ((fp = fopen(ftsp->fts_accpath, "r")) == NULL)
			continue;

		if ((str = malloc(size)) == NULL) {
			fclose(fp);
			continue;
		}
		if ((off_t)fread(str, 1, size, fp) != size) {
			fclose(fp);
			free(str);
			continue;
		}
		fclose(fp);

		if ((json = json_parse(str, size)) == NULL) {
			errstr = "json parsing failed";
			goto done;
		}
		if ((cases = json_getarray(json, "cases")) == NULL) {
			errstr = "no test cases found";
			goto done;
		}

		for (i = 0; i < cases->fields; i++) {
			if ((obj =
			    json_getarrayobj(cases->d.array[i])) == NULL)
				continue;
			if ((wire = json_getstr(obj, "wire")) == NULL) {
				/* ignore test case without wire definition */
				ret = 0;
				goto done;
			}
			if ((tblsz = json_getstr(obj,
			    "header_table_size")) != NULL) {
				//table_size = strtonum(tblsz, 0, LONG_MAX, &errstr);
				table_size = atoi(tblsz);
				free(tblsz);
				if (errstr != NULL)
					goto done;
			} else
				table_size = file_table_size;
			if ((hdrs = json_getarray(obj, "headers")) == NULL) {
				errstr = "no headers found";
				goto done;
			}
			if ((test = hpack_headerblock_new()) == NULL)
				goto done;
			for (j = 0; j < hdrs->fields; j++) {
				if ((hdr = json_getarrayobj(hdrs->d.array[j]))
				    == NULL)
					continue;
				for (k = 0; k < hdr->fields; k++) {
					if (hdr->d.obj[k].lhs->type !=
					    JSMN_STRING &&
					    hdr->d.obj[k].lhs->type !=
					    JSMN_PRIMITIVE)
						continue;
					if (json_uascii_decode(
					    hdr->d.obj[k].rhs->d.str) == NULL) {
						errstr = "invalid JSON value";
						goto done;
					}
					if (hpack_header_add(test,
					    hdr->d.obj[k].lhs->d.str,
					    hdr->d.obj[k].rhs->d.str, 0) == NULL) {
						errstr = "failed to add header";
						goto done;
					}
				}
			}

			if (hpack == NULL) {
				if (table_size > file_table_size)
					file_table_size = table_size;
				if ((hpack =
				    hpack_table_new(file_table_size)) == NULL) {
					errstr = "failed to get HPACK table";
					goto done;
				}
				if ((hpack2 =
				    hpack_table_new(file_table_size)) == NULL) {
					errstr = "failed to get HPACK table";
					goto done;
				}
			}

			if (parse_hex(wire, test, hpack) == -1) {
				errstr = "failed to parse HPACK";
				goto done;
			}

			if (hpack_table_size(hpack) > table_size) {
				errstr = "invalid table size";
				goto done;
			}

			/* Test encoding by re-encoding of the header */
			free(wire);
			if ((wire = hpack_encode(test, &len, hpack2)) == NULL) {
				errstr = "re-encoding failed";
				goto done;
			}
			if (parse_data(wire, len, test, hpack2) == -1) {
				errstr = "re-decoding failed";
				goto done;
			}

			ok++;
			hpack_headerblock_free(test);
			test = NULL;
			free(wire);
			wire = NULL;
		}

		i = 0;
		hpack_table_free(hpack);
		hpack_table_free(hpack2);
		hpack = hpack2 = NULL;
		json_free(json);
		json = NULL;
		free(str);
		str = NULL;

 next:
		h_log(1, "SUCCESS: %s: %zu tests\n", ftsp->fts_path, ok);
	}

	ret = 0;
 done:
	if (errstr != NULL && ftsp != NULL && i != 0)
		h_log(1, "FAILED: %s: %s in test %zu\n",
		    ftsp->fts_path, errstr, i);
	else if (errstr != NULL && ftsp != NULL)
		h_log(1, "FAILED: %s: %s\n",
		    ftsp->fts_path, errstr);
	else if (errstr != NULL)
		h_log(1, "FAILED: %s\n", errstr);
	free(wire);
	hpack_table_free(hpack);
	hpack_table_free(hpack2);
	hpack_headerblock_free(test);
	json_free(json);
	free(str);
	fts_close(fts);

	return (ret);
}


static int encode_huffman(const char *name)
{
	char				 buf[65535];
	char				*enc = NULL, *dec = NULL;
	FILE				*fp;
	int				 ret = -1;
	size_t				 len = 0, enclen = 0, declen = 0;

	if (strcmp("-", name) == 0)
		fp = stdin;
	else if ((fp = fopen(name, "r")) == NULL)
		goto done;
	if ((len = fread(buf, 1, sizeof(buf), fp)) < 1) {
		if (feof(fp))
			ret = 0;
		goto done;
	}
	if ((enc = hpack_huffman_encode(buf, len, &enclen)) == NULL)
		goto done;
	if ((dec = hpack_huffman_decode(enc, enclen, &declen)) == NULL)
		goto done;
	if (memcmp(dec, buf, len) != 0)
		goto done;

	ret = 0;
 done:
	h_log(2, "%s: huffman lengths: raw input %zu,"
	    " encoded output %zu, decoded %zu\n",
	    ret == 0 ? "SUCCESS": "FAILED" , len, enclen, declen);
	if (fp != NULL && fp != stdin)
		fclose(fp);
	free(enc);
	free(dec);

	return (ret);
}

static int decode_huffman(const char *name)
{
	char				 buf[65535];
	char				*enc = NULL, *dec = NULL;
	FILE				*fp;
	int				 ret = -1;
	size_t				 len = 0, enclen = 0, declen = 0;

	if (strcmp("-", name) == 0)
		fp = stdin;
	else if ((fp = fopen(name, "r")) == NULL)
		goto done;
	if ((len = fread(buf, 1, sizeof(buf), fp)) < 1) {
		if (feof(fp))
			ret = 0;
		goto done;
	}
	if ((dec = hpack_huffman_decode(buf, len, &declen)) == NULL)
		goto done;
	if ((enc = hpack_huffman_encode(dec, declen, &enclen)) == NULL)
		goto done;
	if (memcmp(enc, buf, len) != 0)
		goto done;

	ret = 0;
 done:
	h_log(2, "%s: huffman lengths: raw input %zu,"
	    " decoded output %zu, encoded %zu\n",
	    ret == 0 ? "SUCCESS": "FAILED" , len, declen, enclen);
	if (fp != NULL && fp != stdin)
		fclose(fp);
	free(enc);
	free(dec);

	return (ret);
}


int main( int argc, char * argv[])
{
	if (hpack_init() == -1)
		return (1);	
	
	printf( "htest  %s|%d\n", __FUNCTION__, __LINE__);
	
	char * data = "0085b9495339e483c5837f0085b8824e5a4b839d29af0088b83b5339ec327d7f87eabfa35332fd2b0084b958d33f9b60d48e62a1849eb611589825353141e63ad52160b206c4f2f5d537";

	
	struct hpack_table * hpack = hpack_table_new( 8192);

	
	struct hpack_headerblock * test = hpack_headerblock_new();
	
	int psts = parse_hex( data, test, hpack);
	
	printf( "psts=%d   %s|%d\n", psts, __FUNCTION__, __LINE__);
	
	


	// unsigned char buf[8192];
	// ssize_t len;

	// len = parsehex( data, buf, sizeof(buf));
	
	// printf( "len = %ld\n", len);
	
	// if( len > 0)
	// {
		// int i = 0;
		// for( i = 0; i < len; i++)
		// {
			// printf( "%02X ", buf[i] & 0xFF);
		// }
		// printf("\n");
		
		// struct hpack_table * hpack = hpack_table_new( 8192);
		
		// // if( hpack)
		// // {
			// // size_t declen = 0;
			// // char * dec = hpack_huffman_decode( buf, len, &declen);
			
			// // printf( "dec=%p declen=%ld\n", dec, declen);
			
		// // }
		// // else
		// // {
			// // printf( "hpack creation failed\n");
		// // }
		
		// int psts = parse_hex( data, NULL, hpack);
		
		
	// }
	
	
	return 0;
}



