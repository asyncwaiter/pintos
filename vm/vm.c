/* vm.c: Generic interface for virtual memory objects. */

#include "threads/malloc.h"
#include "vm/vm.h"
#include "vm/inspect.h"
#include "mmu.h"

/* Initializes the virtual memory subsystem by invoking each subsystem's
 * intialize codes. */
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
	// 여기에 frame_table 이니셜라이즈하기
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
struct supplemental_page_table global_spt;

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

		/* TODO: Insert the page into the spt. */
	}
err:
	return false;
}

/* Find VA from spt and return page. On error, return NULL. 👻 */
struct page *
spt_find_page (struct supplemental_page_table *spt UNUSED, void *va UNUSED) {
	struct page page;
	page.va = va;

	struct hash_elem *found_elem = hash_find(&spt->page_hash, &page.hash_elem); // hash 먼저 찾고 그걸로
	if (!found_elem)
		return NULL;

	struct page *found_page = hash_entry(found_elem, struct page, hash_elem); // vm_entry 찾고
	if (!found_page->is_loaded)
		return NULL;

	return found_page;
}

/* Insert PAGE into spt with validation. 👻 */
bool
spt_insert_page (struct supplemental_page_table *spt UNUSED,
		struct page *page UNUSED) {
	int succ = false;
	/* 문제 1. 어디서 vm_entry를 생성할 것인가?
	* spt_insert_page 함수는 언제 실행되는가를 먼저 알아야함
	* 매개 변수로 전해지는 page가 이미 생성된 이후라면?
	* vm_entry는 page 하나마다 생겨야함 -> 그렇다면 여기서 생성하고 초기화 해도되지 않나?
	*/

	// struct vm_entry *vm_entry = malloc(sizeof(struct vm_entry));
	// if (!vm_entry) // 메모리 할당 실패
	// 	return false;
	
	// vm_entry->vaddr = page->va;
	// 여기서 어떻게 page로 type에 대한 정보를 찾을건가?
	// 아니면 그냥 매개변수로 vm_entry를 전달받아서 그걸로 페이지를 할당받고 찾는방식?
	// 아냐 그럼 이 함수는 페이지를 해쉬에 넣는건데 그건 범위를 벗어나버림
	// 그러면 페이지를 할당 받을때마다 vm_entry를 따로 밖에서 만들어주는게 더 자연스럽게 느껴지긴함
	// 그리고 그렇게 vm_entry와 연결된 page를 그냥 여기서는 해쉬에만 넣어주고 끝내자.
	
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
	struct frame *victim = NULL;
	 /* TODO: The policy for eviction is up to you. */

	return victim;
}

/* Evict one page and return the corresponding frame.
 * Return NULL on error.*/
static struct frame *
vm_evict_frame (void) {
	struct frame *victim UNUSED = vm_get_victim ();
	/* TODO: swap out the victim and return the evicted frame. */

	return NULL;
}

/* palloc() and get frame. If there is no available page, evict the page
 * and return it. This always return valid address. That is, if the user pool
 * memory is full, this function evicts the frame to get the available memory
 * space.*/
static struct frame *
vm_get_frame (void) {
	struct frame *frame = NULL;
	/* TODO: Fill this function. 👻 */
	/**vm_get_frame 함수는 메모리 풀에서 새로운 물리메모리 페이지를 가져오는 함수임.
	유저 메모리 풀에서 페이지를 성공적으로 가져오면 프레임을 할당하고 프레임 구조체의 멤버들을 초기화하고 프레임을 반환한다.

	그러면 일단 프레임 테이블에서 확인을 해보고, 프레임 테이블에 빈 공간이 있는지 확인한 후에 해당 프레임을 줘야겠지.
	하지만 만약에 프레임이 꽉찼을경우? 하나를 쫓아낸다.
	그러나 어떻게 쫓아낼 것인가? 그럼 프레임에 어떤 필드를 추가해야할까? 지금은 is_used를 관리중인데 얘는 매일 사용중으로 뜨지않을까?

	 */
	

	ASSERT (frame != NULL);
	ASSERT (frame->page == NULL);

	return frame;
}

/* Growing the stack. */
static void
vm_stack_growth (void *addr UNUSED) {
}

/* Handle the fault on write_protected page */
static bool
vm_handle_wp (struct page *page UNUSED) {
}

/* Return true on success
 * page fault가 나면 영역을 찾아야함
 */
bool
vm_try_handle_fault (struct intr_frame *f UNUSED, void *addr UNUSED,
		bool user UNUSED, bool write UNUSED, bool not_present UNUSED) {
	struct supplemental_page_table *spt UNUSED = &thread_current ()->spt;
	struct page *page = NULL;
	/* TODO: Validate the fault */
	/* TODO: Your code goes here */

	return vm_do_claim_page (page);
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
	struct page *page = NULL;
	/* TODO: Fill this function */

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
bool
supplemental_page_table_copy (struct supplemental_page_table *dst UNUSED,
		struct supplemental_page_table *src UNUSED) {
}

/* Free the resource hold by the supplemental page table 👻*/
void
supplemental_page_table_kill (struct supplemental_page_table *spt UNUSED) {
	/* TODO: Destroy all the supplemental_page_table hold by thread and
	 * TODO: writeback all the modified contents to the storage. */
}


unsigned page_hash_func(const struct hash_elem *e, void *aux UNUSED) {
    const struct page *p = hash_entry(e, struct page, hash_elem);
    return hash_bytes(p->va, strlen(p->va));
}

bool page_hash_less(const struct hash_elem *a, const struct hash_elem *b, void *aux UNUSED) {
    const struct page *p_a = hash_entry(a, struct page, hash_elem);
    const struct page *p_b = hash_entry(b, struct page, hash_elem);
    return strcmp(p_a->va, p_a->va) < 0;
}
