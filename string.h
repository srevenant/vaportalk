/* This source code is in the public domain. */

/* string.h: String types */

typedef struct cstr	Cstr;
typedef struct string	String;
typedef struct rstr	Rstr;
typedef struct istr	Istr;

/* Counted string */
struct cstr {
	char *s;	/* Text	  */
	int l;		/* Length */
};

/* Stretchybuffer */
struct string {
	Cstr c;		/* Contents */
	int sz;		/* Space allocated */
};

/* Keeps track of physical representation of strings */
struct rstr {
	String str;	/* Contents */
	int refs;	/* Number of istrs pointing to it */
};

/* Keeps track of interpreter copies of strings */
struct istr {
	Rstr *rs;	/* Physical copy */
	int refs;	/* Number of dframes pointing to it */
};

