#include "mem.h"
#include "common.h"

#include <assert.h>
#include <stddef.h>
#include <string.h>

// constante définie dans gcc seulement
#ifdef __BIGGEST_ALIGNMENT__
#define ALIGNMENT __BIGGEST_ALIGNMENT__
#else
#define ALIGNMENT 16
#endif
#define align(v,a) ((intptr_t)(v)+(a-1)&~((a)-1))

#define ALIGNEMENT sizeof(struct fb*)

struct fb {
	size_t size;
	struct fb* next;
	/* ... */
};

struct fb* PremZL;
size_t SizeMem;

void mem_init(void* mem, size_t taille)
{
	assert(mem == get_memory_adr());
	assert(taille == get_memory_size());

	struct fb* a;
	a = mem;
	a->size = taille;
	a->next = NULL;

	PremZL = mem;
	SizeMem = taille;

	mem_fit(&mem_fit_first);
}

void mem_show(void (*print)(void *, size_t, int)) {
	size_t taillelue=0;
	char c;
	struct fb* mem = get_memory_adr();
	struct fb* currentZL = PremZL;
	size_t taille;
	while (taillelue < SizeMem) {				// On lit tant qu'on a pas atteint la taille de la mémoire
		taille=*((size_t*)(mem));							// On récupère la taille de la zone
		if(mem==(void*)currentZL){						// si la zone est dans la liste libre
			print(mem,taille,1);
			currentZL = currentZL->next;
		}
		else{
			print(mem,taille,0);
		}
		taillelue += taille;								// on augmente la taille lue
		mem = (void*) mem + taille;						// on avance dans la memoire
	}
}

static mem_fit_function_t *mem_fit_fn;
void mem_fit(mem_fit_function_t *f) {
	mem_fit_fn=f;
}

void *mem_alloc(size_t taille) {

	__attribute__((unused)) /* juste pour que gcc compile ce squelette avec -Werror */

	struct fb *fb=mem_fit_fn(PremZL,taille + sizeof(size_t));		// on récupère la zone libre

  if(fb == NULL){
		return NULL;
	}

	size_t taillealign =  (taille + sizeof(size_t)) + ALIGNEMENT - (taille + sizeof(size_t))%ALIGNEMENT; // on aligne la zone libre sur notre valeur d'alignement
	if((taille + sizeof(size_t))%ALIGNEMENT == 0) {				// si elle était déjà alignée, on reréduit à cette valeur
		taillealign -= ALIGNEMENT;
	}

	if(PremZL == fb) {																		// Cas où notre zone libre est la première
		if (PremZL->next == NULL){
			size_t t = PremZL->size;													// si on a une seule zone libre, on crée une nouvelle première zone libre
			PremZL = ((void*)PremZL + taillealign);
			PremZL->size = t - taillealign;
		}
		else {																							// si on a d'autres zones libres
			if(PremZL->size <= taillealign + ALIGNEMENT){			// si on n'a pas la place pour créer une nouvelle zone libre
				taillealign += ALIGNEMENT;											// on place dans le padding la place inutilisable
				PremZL = PremZL->next;
			}
			else{
				struct fb *tmp = PremZL->next;									// on conserve bien la liste des zones libres
				size_t t = PremZL->size;
				PremZL = ((void*)PremZL + taillealign);					// on crée une nouvelle zone libre de la nouvelle taille
				PremZL->size = t - taillealign;
				PremZL->next = tmp;
			}

		}
	}
	else{																									// si on est pas sur la première zone libre
		struct fb *currentZL = PremZL;
		struct fb *suivZL ;
		while (currentZL->next != NULL){										// on cherche la zone dans notre liste de zone libres
			suivZL = currentZL->next;
			if(fb == suivZL){																	// quand on l'a trouvée
				if(suivZL->next == NULL){												// on regarde si c'est la dernière
					size_t t = suivZL->size;
					suivZL = ((void*)suivZL + taillealign);				// dans ce cas, on en crée une nouvelle
					suivZL->size = t - taillealign;
					currentZL->next = suivZL;
				}
				else{
					if(suivZL->size >taillealign && suivZL->size <= taillealign + ALIGNEMENT){ // si on n'a pas la place de créer une nouvelle zone libre
						taillealign += ALIGNEMENT;									// On place dans la padding la place perdue
						currentZL->next = suivZL->next;
					}
					else{
						struct fb *tmp = suivZL->next;							// sinon, on crée une nouvelle zone libre à cet emplacement
						size_t t = suivZL->size;
						suivZL = ((void*)suivZL + taillealign);
						suivZL->size = t - taillealign;
						suivZL->next = tmp;
					}
				}
			}
			if(currentZL -> next != NULL){
				currentZL = currentZL->next;
			}
		}
	}

	void* zo = (size_t*)fb;																// on place dans un pointeur l'adresse de la zone libre
	*(size_t*)zo = taillealign;														// on met a cette adresse la taille de la zone occupée
	return zo+sizeof(size_t);															// on retourne l'adresse de la zone occupée, sans inclure les metadonnées
}


void mem_free(void* mem) {
	mem = mem - sizeof(size_t);														// on recule pour acceder aux metadonnées
	struct fb *currentZL = PremZL;
	struct fb *memoire = mem;
	memoire->size = *(size_t*)mem;												// on place la taille de la zone dans sa structure

	if(memoire<PremZL){																		// si on est avant la première zone libre, on devient la première ZL
		PremZL = memoire;
		PremZL->next = currentZL;
	}
	else{
		while(memoire > currentZL->next){										// sinon, on cherche où elle se situe dans la lite
			currentZL =  currentZL->next;
		}
		memoire->next = currentZL->next;
		currentZL->next =  memoire;
	}

	currentZL = PremZL;
	struct fb* suivZL;
	while(currentZL->next != NULL){												// tant qu'on est pas à la fin de la liste de zones libres
		suivZL = currentZL->next;
		if((void*)currentZL + currentZL->size == currentZL->next){			// si deux zones libres se suivent
			currentZL->size += suivZL->size;									// on les fusionne
			currentZL->next = suivZL->next;
		}
		else{
			currentZL = currentZL->next;
		}
	}
	return;
}


struct fb* mem_fit_first(struct fb *list, size_t size) {
	while(list !=NULL && list->size < size + sizeof(size_t)){
		list = list->next;
	}
	return list;
}

/* Fonction à faire dans un second temps
 * - utilisée par realloc() dans malloc_stub.c
 * - nécessaire pour remplacer l'allocateur de la libc
 * - donc nécessaire pour 'make test_ls'
 * Lire malloc_stub.c pour comprendre son utilisation
 * (ou en discuter avec l'enseignant)
 */
size_t mem_get_size(void *zone) {
	/* zone est une adresse qui a été retournée par mem_alloc() */

	/* la valeur retournée doit être la taille maximale que
	 * l'utilisateur peut utiliser dans cette zone */
	return 0;
}

/* Fonctions facultatives
 * autres stratégies d'allocation
 */
struct fb* mem_fit_best(struct fb *list, size_t size) {
	return NULL;
}

struct fb* mem_fit_worst(struct fb *list, size_t size) {
	return NULL;
}
