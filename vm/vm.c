/* vm.c: Generic interface for virtual memory objects. */

#include "threads/malloc.h"
#include "vm/vm.h"
#include "vm/inspect.h"
#include "threads/mmu.h"

/* Initializes the virtual memory subsystem by invoking each subsystem's
 * intialize codes. */
struct frame_table global_ft;
void
vm_init (void) {
	vm_anon_init ();
	vm_file_init ();
#ifdef EFILESYS  /* For project 4 */
	pagecache_init ();
#endif
	register_inspect_intr ();
	/* DO NOT MODIFY UPPER LINES. */
	/* TODO: Your code goes here. 👻 */
	list_init(&global_ft.frame_list);
}

/* Get the type of the page. This function is useful if you want to know the
 * type of the page after it will be initialized.
 * This function is fully implemented now. */
enum vm_type
page_get_type (struct page *page) {
	int ty = VM_TYPE (page->operations->type);
	switch (ty) {
		case VM_UNINIT:
			return VM_TYPE (page->uninit.type);
		default:
			return ty;
	}
}

/* Helpers */
static struct frame *vm_get_victim (void);
static bool vm_do_claim_page (struct page *page);
static struct frame *vm_evict_frame (void);

/* 👻 선언 */
unsigned page_hash_func(const struct hash_elem *e, void *aux UNUSED);
bool page_hash_less(const struct hash_elem *a, const struct hash_elem *b, void *aux UNUSED);

/* Create the pending page object with initializer. If you want to create a
 * page, do not create it directly and make it through this function or
 * `vm_alloc_page`. */
bool
vm_alloc_page_with_initializer (enum vm_type type, void *upage, bool writable,
		vm_initializer *init, void *aux) {

	ASSERT (VM_TYPE(type) != VM_UNINIT)

	struct supplemental_page_table *spt = &thread_current ()->spt;

	/* Check wheter the upage is already occupied or not. */
	if (spt_find_page (spt, upage) == NULL) {
		/* TODO: Create the page, fetch the initialier according to the VM type,
		 * TODO: and then create "uninit" page struct by calling uninit_new. You
		 * TODO: should modify the field after calling the uninit_new. */
		struct page *page = (struct page *)malloc(sizeof(struct page));
		vm_initializer *initializer = NULL;
		switch (VM_TYPE(type))
		{
		case VM_ANON:
			initializer = anon_initializer;
			break;
		case VM_FILE:
			initializer = file_backed_initializer;
			break;
		default:
			goto err;
		}

		uninit_new(page, upage, init, type, aux, initializer);
		page->writable = writable;

		/* TODO: Insert the page into the spt. */
		if(!spt_insert_page(spt, page)){
			free(page);
			goto err;
		}
		
		return true;
	}
err:
	return false;
}

/* Find VA from spt and return page. On error, return NULL. 👻 */
struct page *
spt_find_page (struct supplemental_page_table *spt UNUSED, void *va UNUSED) {
	struct page page;
	page.va = pg_round_down(va);

	struct hash_elem *found_elem = hash_find(&spt->page_hash, &page.hash_elem); // hash 먼저 찾고 그걸로
	if (!found_elem)
		return NULL;

	struct page *found_page = hash_entry(found_elem, struct page, hash_elem); // vm_entry 찾고
	// if (!found_page->is_loaded)
	// 	return NULL;

	return found_page;
}

/* Insert PAGE into spt with validation. 👻 */
bool
spt_insert_page (struct supplemental_page_table *spt UNUSED,
		struct page *page UNUSED) {
	int succ = false;

	if(hash_insert(&spt->page_hash, &page->hash_elem) == NULL){
		succ = true;
	}

	return succ;
}

void
spt_remove_page (struct supplemental_page_table *spt, struct page *page) {
	vm_dealloc_page (page);
	return true;
}

/* Get the struct frame, that will be evicted. */
static struct frame *
vm_get_victim (void) {
	struct list_elem *e = list_begin(&global_ft.frame_list);
	while (e!=list_end(&global_ft.frame_list))
	{
		struct frame *f = list_entry(e, struct frame, frame_elem);
		struct page *p = f->page;

		if(pml4_is_accessed(thread_current()->pml4,p->va)){
			pml4_set_accessed(thread_current()->pml4, p->va, false);
			e = list_next(e);
		} else{
			return f;
		}
	}

	return list_entry(list_begin(&global_ft.frame_list), struct frame, frame_elem);
}

/* Evict one page and return the corresponding frame.
 * Return NULL on error.*/
static struct frame *
vm_evict_frame (void) {
	struct frame *victim UNUSED = vm_get_victim ();
	ASSERT(victim != NULL);
	
	struct page *page = victim->page;
    ASSERT(page != NULL);

	swap_out(page);

	return NULL;
}

struct frame *get_available_frame (void) {
	for (struct list_elem *e = list_begin(&global_ft); e != list_end(&global_ft); e = list_next(e)){
		struct frame *f = list_entry(e, struct frame, frame_elem);
		if (!f->is_used) {
			f->is_used = true;
			return f;
		}
	}
	return NULL;
}

/* palloc() and get frame. If there is no available page, evict the page
 * and return it. This always return valid address. That is, if the user pool
 * memory is full, this function evicts the frame to get the available memory
 * space.*/
static struct frame *
vm_get_frame (void) {
	/* TODO: Fill this function. 👻 */
	struct frame *old_frame = get_available_frame();
	if (old_frame != NULL)
		return old_frame;
		
	void *kva = palloc_get_page(PAL_USER);
	if (kva == NULL) {
		// struct frame *f = vm_evict_frame();
		// kva = f->kva;
		PANIC("evict");
	}
	struct frame *frame = malloc(sizeof(struct frame));
	ASSERT (frame != NULL);
	
	frame->kva = kva;
	frame->is_used = true;
	frame->page = NULL;
	ASSERT (frame->page == NULL);

	list_push_back(&global_ft, &frame->frame_elem);

	// void *p_addr = vtop(kva);
	// pml4_set_page(thread_current()->pml4, kva, p_addr, true);

	return frame;
}

/* Growing the stack. */
// static void
// vm_stack_growth (void *addr UNUSED, uintptr_t rsp) {
// 	// 하나 이상의 anonymous 페이지를 할당하여 스택 크기를 늘립니다. 
// 	// 이로써 addr은 faulted 주소(폴트가 발생하는 주소) 에서 유효한 주소가 됩니다.  
// 	// 페이지를 할당할 때는 주소를 PGSIZE 기준으로 내림하세요.
// 	uintptr_t align_addr = pg_round_down(addr);
//     uintptr_t align_rsp = pg_round_down(rsp);

// 	for (uintptr_t p = align_rsp - PGSIZE; p >= align_addr; p -= PGSIZE) {
//         if (!spt_find_page(&thread_current()->spt, p)) {
//             vm_alloc_page(VM_ANON, p, true);
//         }
//     }
// }
static void
vm_stack_growth (void *addr UNUSED) {
	uintptr_t aligned = pg_round_down(addr);
	for (; aligned < USER_STACK - PGSIZE; aligned += PGSIZE) {
		if (!spt_find_page(&thread_current()->spt, aligned))
			vm_alloc_page(VM_ANON, aligned, true);
	}
}

/* Handle the fault on write_protected page */
static bool
vm_handle_wp (struct page *page UNUSED) {
}

/* Return true on success */
bool
vm_try_handle_fault (struct intr_frame *f UNUSED, void *addr UNUSED,
		bool user UNUSED, bool write UNUSED, bool not_present UNUSED) {
	struct supplemental_page_table *spt UNUSED = &thread_current ()->spt;
	struct page *page = spt_find_page(spt, addr);
	if (page == NULL) {
		if (addr >= USER_STACK_LIMIT) {
			uintptr_t diff = f->rsp - (uintptr_t)addr;
			if (diff <= 8){
				vm_stack_growth(addr);
				page = spt_find_page(spt, addr);
				if(page == NULL)
					return false;
			} else {
				return false;
			}
		} else 
			return false;
	}
	if (write && !page->writable)
		return false;
	return vm_do_claim_page(page);
}

/* Free the page.
 * DO NOT MODIFY THIS FUNCTION. */
void
vm_dealloc_page (struct page *page) {
	destroy (page);
	free (page);
}

/* Claim the page that allocate on VA. 👻 */
bool
vm_claim_page (void *va UNUSED) {
	struct page *page = spt_find_page(&thread_current()->spt, va);
	if(page == NULL)
		return false;

	return vm_do_claim_page (page);
}

/* Claim the PAGE and set up the mmu. 👻 */
static bool
vm_do_claim_page (struct page *page) {
	struct frame *frame = vm_get_frame ();

	/* Set links */
	frame->page = page;
	page->frame = frame;
	
	/* TODO: Insert page table entry to map page's VA to frame's PA. */
	if(!pml4_set_page(thread_current()->pml4, page->va, frame->kva, page->writable))
		return false;


	return swap_in (page, frame->kva);
}

/* Initialize new supplemental page table 👻*/
void
supplemental_page_table_init (struct supplemental_page_table *spt UNUSED) {
	bool initialized = hash_init(&spt->page_hash, page_hash_func, page_hash_less, NULL);
	if (initialized){
		// 성실처리 지금은 뭘로하는게 좋을지 모르겠음
		// 아마 밑에 실패시에 goto error나 exit을 해줘야하지 않을까 싶음
	}
}

/* Copy supplemental page table from src to dst 👻*/
void page_copy(struct hash_elem *e, void *aux UNUSED) {
	struct page *page = hash_entry(e, struct page, hash_elem);
	int type = VM_TYPE(page->operations->type);

	void *aux_copy = NULL;

	if (type == VM_UNINIT) {
		if (page->uninit.aux) {
			aux_copy = malloc(sizeof(struct file_page));
			memcpy(aux_copy, page->uninit.aux, sizeof(struct file_page));
		}
		vm_alloc_page_with_initializer(page->uninit.type, page->va, page->writable, page->uninit.init, aux_copy);
	}
	else if (type == VM_ANON) {
		// 이미 초기화된 ANON 타입의 페이지
		vm_alloc_page(page->operations->type, page->va, page->writable);

		// 만약 프레임이 있으면, 해당 프레임을 자식 프로세스의 페이지로 복사
		if (page->frame != NULL) {
			vm_claim_page(page->va);
			struct page *page_child = spt_find_page(&thread_current()->spt, page->va);
			memcpy(page_child->frame->kva, page->frame->kva, PGSIZE);
		}
	}
}
bool
supplemental_page_table_copy (struct supplemental_page_table *dst UNUSED,
		struct supplemental_page_table *src UNUSED) {
	hash_apply(&src->page_hash, page_copy);
	return true;
}

/* Free the resource hold by the supplemental page table 👻*/
void page_destroy(struct hash_elem *e, void *aux UNUSED) {
	struct page *page = hash_entry(e, struct page, hash_elem);
	vm_dealloc_page(page);
}

void
supplemental_page_table_kill (struct supplemental_page_table *spt UNUSED) {
	/* TODO: Destroy all the supplemental_page_table hold by thread and
	 * TODO: writeback all the modified contents to the storage. */
	hash_clear(&spt->page_hash, page_destroy);
	// hash_destroy(&spt->page_hash, NULL);
}

unsigned page_hash_func(const struct hash_elem *e, void *aux UNUSED) {
    const struct page *p = hash_entry(e, struct page, hash_elem);
    return hash_bytes(&p->va, sizeof(p->va));
}

bool page_hash_less(const struct hash_elem *a, const struct hash_elem *b, void *aux UNUSED) {
    const struct page *p_a = hash_entry(a, struct page, hash_elem);
    const struct page *p_b = hash_entry(b, struct page, hash_elem);
    return p_a->va < p_b->va;
}
