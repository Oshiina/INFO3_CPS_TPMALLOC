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
	size_t tailletotale=0;
	char c;
	struct fb* mem = get_memory_adr();
	struct fb* currentZL = PremZL;
	size_t taille;
	while (tailletotale < SizeMem) {
		taille=*((size_t*)(mem));
		if(mem==(void*)currentZL){
			print(mem,taille,1);
			currentZL = currentZL->next;
		}
		else{
			print(mem,taille,0);
		}
		tailletotale += taille;
		mem = (void*) mem + taille;
		scanf("%c",&c);
	}
}

static mem_fit_function_t *mem_fit_fn;
void mem_fit(mem_fit_function_t *f) {
	mem_fit_fn=f;
}

void *mem_alloc(size_t taille) {

	__attribute__((unused)) /* juste pour que gcc compile ce squelette avec -Werror */

	struct fb *fb=mem_fit_fn(PremZL,taille + sizeof(size_t));
  if(fb == NULL){
		return NULL;
	}
	size_t taillealign =  (taille + sizeof(size_t)) + ALIGNEMENT - (taille + sizeof(size_t))%ALIGNEMENT;
	if((taille + sizeof(size_t))%ALIGNEMENT == 0) {
		taillealign -= ALIGNEMENT;
	}
	if(PremZL == fb) {
		if (PremZL->next == NULL){
			size_t t = PremZL->size;
			PremZL = ((void*)PremZL + taillealign);
			PremZL->size = t - taillealign;
		}
		else {
			if(PremZL->size <= taillealign + ALIGNEMENT){
				taillealign += ALIGNEMENT;
				PremZL = PremZL->next;
			}
			else{
				struct fb *tmp = PremZL->next;
				size_t t = PremZL->size;
				PremZL = ((void*)PremZL + taillealign);
				PremZL->size = t - taillealign;
				PremZL->next = tmp;
			}

		}
	}
	else{
		struct fb *currentZL = PremZL;
		struct fb *suivZL ;
		while (currentZL->next != NULL){
			suivZL = currentZL->next;
			if(fb == suivZL){
				if(suivZL->next == NULL){
					size_t t = suivZL->size;
					suivZL = ((void*)suivZL + taillealign);
					suivZL->size = t - taillealign;
					currentZL->next = suivZL;
					printf("sal");
				}
				else{
					printf("%ld %ld", suivZL->size,taillealign + ALIGNEMENT);
					if(suivZL->size >taillealign && suivZL->size <= taillealign + ALIGNEMENT){
						printf("alo");
						taillealign += ALIGNEMENT;
						currentZL->next = suivZL->next;
					}
					else{
						struct fb *tmp = suivZL->next;
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
	void* zo = (size_t*)fb;
	*(size_t*)zo = taillealign;
	return zo+sizeof(size_t);
}


void mem_free(void* mem) {
	mem = mem - sizeof(size_t);
	struct fb *currentZL = PremZL;
	struct fb *memoire = mem;
	memoire->size = *(size_t*)mem;

	if(memoire<PremZL){
		PremZL = memoire;
		PremZL->next = currentZL;
	}
	else{
		while(memoire > currentZL->next){
			currentZL =  currentZL->next;
		}
		memoire->next = currentZL->next;
		currentZL->next =  memoire;
	}

	currentZL = PremZL;
	struct fb* suivZL;
	while(currentZL->next != NULL){
		suivZL = currentZL->next;
		if((void*)currentZL + currentZL->size == currentZL->next){
			currentZL->size += suivZL->size;
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
