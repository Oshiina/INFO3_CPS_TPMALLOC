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

struct fb {
	size_t size;
	struct fb* next;
	/* ... */
};

struct fb* PremZL;
struct fb* DebMem;
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
	DebMem = mem;
	SizeMem = taille;

	mem_fit(&mem_fit_first);
}

void mem_show(void (*print)(void *, size_t, int)) {
	size_t taille=0;
	struct fb* mem = DebMem;
	struct fb* ZL = PremZL;
	size_t k;
	while (taille < SizeMem) {
		k=*((size_t*)(mem));
		if(mem==ZL){
			print(mem,k,1);
			ZL = ZL->next;
		}
		else{
			print(mem,k,0);
		}
		taille += k;
		mem += k;
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
	if(PremZL == fb) {
		if (PremZL->next == NULL){
			size_t t = PremZL->size;
			PremZL += taille + sizeof(size_t);
			PremZL->size = t - (taille + sizeof(size_t));
		}
		else {
			PremZL = PremZL->next;
		}
	}
	else{
		struct fb *ZL = PremZL;
		while (ZL != NULL){
			if(fb == ZL->next){
				ZL->next = ZL->next->next;
			}
			ZL = ZL->next;
		}
	}
	size_t* zo = (size_t*)fb;
	*zo = taille + sizeof(size_t);
	printf ("%ld ", *zo);
	return zo + sizeof(size_t);
}


void mem_free(void* mem) {
	struct fb* mem2 = mem - sizeof(size_t);
	mem2->size = *(size_t*)(mem - sizeof(size_t));
	printf("%ld ", mem2->size);
	struct fb* ZL = PremZL;
	if(mem2 < PremZL){
		mem2->next = PremZL;
		PremZL = mem2;
		printf("%ld \n\n",PremZL->size);
	}
	else{
		while(mem2 > ZL->next){
			ZL =  ZL->next;
		}
		mem2->next = ZL->next;
		ZL->next =  mem2;
	}
/*	if(mem2+mem2->size == mem2->next){
		mem2->size += mem2->next->size;
		mem2->next = mem2->next->next;
	}
  if(ZL+ZL->size == mem2){
		 ZL->size += mem2->size;
		 ZL->next = mem2->next;
	 }*/
	 return;
}


struct fb* mem_fit_first(struct fb *list, size_t size) {
	while(list !=NULL && list->size < size){
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
