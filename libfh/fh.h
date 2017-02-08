/*
 *
 * Content   header file for fast hash
 *
 * Author(s): Paul Stephen Borile
 *
*/

#ifndef FH_H
#define FH_H

#ifdef __cplusplus
extern "C" {
#endif

/** hash magic */
#define FH_MAGIC_ID			0xCACCA
/** magic assegnato alla hash in fase di destroy */
//#define FH_MAGIC_ID_DESTROY  0xFF00

/* massima dimensione della key */
#define HMAXLENKEY			128

//#define FH_INC  			1
//#define FH_DEC  			0

/** codice di errore ritornato quando il puntatore passato non punta
	ad una hash table */
#define FH_NOT_HASHTABLE	-1
/** codice di errore ritornato quando l'elemento ricercato non e'
	presente nella hash */
#define FH_ELEMENT_NOT_FOUND	-2000
/** codice di errore ritornato quando si tenta di inserire una chiave
	(con relativo oggetto opaco) che e' gia' presente nella hash */
#define FH_DUPLICATED_ELEMENT	-3000
/** codice di errore ritornato quando e' impossibile allocare memoria */
#define FH_NO_MEMORY	-4000
/** codice di errore ritornato quando occorre andare a vedere il contenuto
	vella var 'errno' */
#define FH_SEE_ERRNO	-5000
/** codice di errore ritornato dalla HashLoad quando non matcha la
	dimensione dei dati	dell'oggetto opaco */
#define FH_DATALEN_MISMATCH	-6000
/** codice di errore ritornato dalla HashLoad quando la dimensione della
	hash passata in input e' < 0 */
#define FH_DIM_INVALID	-7000
/** codice di errore ritornato quando si supera la dimensione max della chiave */
#define FH_BAD_LENKEY		-8000
/** codice di errore ritornato quando l'attributo non e' valido */
#define FH_BAD_ATTR		-9000
#define FH_SCAN_END		-100

/** attributo della hash corrispondente al numero di entry nella hashtable */
#define FH_ATTR_ELEMENT  100
#define FH_ATTR_DIM	101
#define FH_ATTR_COLLISION	102

/*  Local types                                 */

/** single structures used to contain data */
struct _fh_slot {
	char *key;
	struct _fh_slot *next;
	void *opaque_obj;
};
typedef struct _fh_slot fh_slot;

/** struttura equivalente ad un' entry della hash table */
struct _f_hash {
	/** mutex associato all' entry della hash */
	/** puntatore alla generica struttura strHSlot da inserire nella hash*/
	fh_slot *h_slot;
};
typedef struct _f_hash f_hash;

/* header for hasdump */
struct _dump_info {
	// key len
	int key_len;
};
typedef struct _dump_info dump_info;

/** struttura ritornata dal metodo di HashInit contenente le informazioni
	sulla hash*/
struct _fh_t{
	int	 h_magic;
	int  h_dim; // hash size
	int	 h_datalen; // opaque_obj size
	int  h_elements; // elements in hash
	int	 h_collision; // collisions during insert
	unsigned int  (*hash_function)();
	pthread_mutex_t	h_lock;
	f_hash *hash_table;
};
typedef struct _fh_t fh_t;

fh_t	*fh_create( int dim, int opaque_len, unsigned int (*hash_function)());
int 	fh_setattr(fh_t *fh, int attr, int value);
int 	fh_getattr(fh_t *fh, int attr, int *value);
int 	fh_destroy(fh_t *fh );
int 	fh_insert(fh_t *fh, char *key, void *opaque);
int 	fh_del(fh_t *fh, char *HKey );
int 	fh_search(fh_t *fh, char *HKey, void *block );

int 	fh_scan_start(fh_t *fh, int pos, void **slot);
int 	fh_scan_next(fh_t *fh, int *pos, void **slot, char *key, void *opaque);
void *fh_searchlock(fh_t *fh, char *key, int *slot);
void	fh_releaselock(fh_t *fh, int slot);

/*
int 	FHashDump(strFHashInfo *Hid, char *acFile);
int 	FHashLoad(strFHashInfo *Hid, char *acFile );
*/

#ifdef __cplusplus
}
#endif

#endif
