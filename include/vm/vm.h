#ifndef VM_VM_H
#define VM_VM_H
#include <stdbool.h>
#include "threads/palloc.h"
#include "kernel/hash.h"

enum vm_type {
	/* page not initialized */
	VM_UNINIT = 0,
	/* page not related to the file, aka anonymous page */
	VM_ANON = 1,
	/* page that realated to the file */
	VM_FILE = 2,
	/* page that hold the page cache, for project 4 */
	VM_PAGE_CACHE = 3,

	/* Bit flags to store state */

	/* Auxillary bit flag marker for store information. You can add more
	 * markers, until the value is fit in the int. */
	VM_MARKER_0 = (1 << 3),
	VM_MARKER_1 = (1 << 4),

	/* DO NOT EXCEED THIS VALUE. */
	VM_MARKER_END = (1 << 31),
};

#include "vm/uninit.h"
#include "vm/anon.h"
#include "vm/file.h"
#ifdef EFILESYS
#include "filesys/page_cache.h"
#endif

struct page_operations;
struct thread;

#define VM_TYPE(type) ((type) & 7)

/* The representation of "page".
 * This is kind of "parent class", which has four "child class"es, which are
 * uninit_page, file_page, anon_page, and page cache (project4).
 * DO NOT REMOVE/MODIFY PREDEFINED MEMBER OF THIS STRUCTURE. */
struct page {
	const struct page_operations *operations; // page별 메소드를 위한 정적 구조체
	void *va;              /* Address in terms of user space */
	struct frame *frame;   /* Back reference for frame */

	/* Your implementation 👻*/
	struct vm_entry *vm_entry;

	/* Per-type data are binded into the union.
	 * Each function automatically detects the current union */
	union {
		struct uninit_page uninit; // 초기화되지 않은 페이지 
		struct anon_page anon;
		struct file_page file;
#ifdef EFILESYS
		struct page_cache page_cache;
#endif
	};
};

/* The representation of "frame 다른 멤버 추가 가능 👻 */
struct frame {
	void *kva;
	struct page *page;
};

/* The function table for page operations.
 * This is one way of implementing "interface" in C.
 * Put the table of "method" into the struct's member, and
 * call it whenever you needed. */
struct page_operations {
	bool (*swap_in) (struct page *, void *);
	bool (*swap_out) (struct page *);
	void (*destroy) (struct page *);
	enum vm_type type;
};

#define swap_in(page, v) (page)->operations->swap_in ((page), v)
#define swap_out(page) (page)->operations->swap_out (page)
#define destroy(page) \
	if ((page)->operations->destroy) (page)->operations->destroy (page)

/* Representation of current process's memory space.
 * We don't want to force you to obey any specific design for this struct.
 * All designs up to you for this. 👻 */

struct vm_entry {
	uint16_t type;		/* VM_BIN, VM_FILE, VM_ANON's types */
	void *vaddr; 		/* vm-entry가 관리하는 가상페이지 번호 */
	bool writable;		

	bool is_loaded;		/* 물리메모리 탑재 여부 */
	struct file* file;	/* 매핑된 파일 */
	
	size_t offset;
	size_t read_bytes;	/* 가상주소에 쓰인 데이터 크기 */
	size_t zero_bytes;

	struct hash_elem hash_elem;
};

struct supplemental_page_table {
	struct hash vm_page_map;
};

struct frame_table {

};

/* 사용중인 스왑 슬롯과 빈 스왑 슬롯들을 추적, 
pintos-mkdisk swap.dsk --swap-size=n으로 스왑파티션 생성 가능 */
struct swap_table {

};

struct vm_entry {
	uint16_t type;		/* VM_BIN, VM_FILE, VM_ANON's types */
	void *vaddr; 		/* vm-entry가 관리하는 가상페이지 번호 */
	bool writable;		

	bool is_loaded;		/* 물리메모리 탑재 여부 */
	struct file* file;	/* 매핑된 파일 */
	
	size_t offset;
	size_t read_bytes;	/* 가상주소에 쓰인 데이터 크기 */
	size_t zero_bytes;

	struct hash_elem hash_elem;
};

#include "threads/thread.h"
void supplemental_page_table_init (struct supplemental_page_table *spt);
bool supplemental_page_table_copy (struct supplemental_page_table *dst,
		struct supplemental_page_table *src);
void supplemental_page_table_kill (struct supplemental_page_table *spt);
struct page *spt_find_page (struct supplemental_page_table *spt,
		void *va);
bool spt_insert_page (struct supplemental_page_table *spt, struct page *page);
void spt_remove_page (struct supplemental_page_table *spt, struct page *page);

void vm_init (void);
bool vm_try_handle_fault (struct intr_frame *f, void *addr, bool user,
		bool write, bool not_present);

#define vm_alloc_page(type, upage, writable) \
	vm_alloc_page_with_initializer ((type), (upage), (writable), NULL, NULL)
bool vm_alloc_page_with_initializer (enum vm_type type, void *upage,
		bool writable, vm_initializer *init, void *aux);
void vm_dealloc_page (struct page *page);
bool vm_claim_page (void *va);
enum vm_type page_get_type (struct page *page);

#endif  /* VM_VM_H */
