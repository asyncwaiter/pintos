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
#include "lib/kernel/hash.h"
#ifdef EFILESYS
#include "filesys/page_cache.h"
#endif

struct page_operations;
struct thread;

#define VM_TYPE(type) ((type) & 7)

/* "페이지"의 표현입니다.
 * 이 구조체는 "부모 클래스"이며, 네 가지 "자식 클래스"를 가지고 있습니다.
 * 이 자식 클래스들은 uninit_page, file_page, anon_page, 그리고 페이지 캐시 (project4)입니다.
 * 이 구조체의 미리 정의된 멤버는 삭제하거나 수정하지 마세요. */
struct page {
<<<<<<< HEAD
	const struct page_operations *operations;  /* 페이지에 대해 수행할 수 있는 동작에 대한 포인터를 저장 */
	void *va;              /* 사용자 공간에서의 가상 주소 */
	struct frame *frame;   /* 이 페이지와 연결된 물리적 frame을 역참조하는 포인터 */

	/* 여러분의 구현이 들어갈 부분입니다 */

	/* 유형별 데이터는 union에 바인딩되어 있습니다.
	 * 각 함수는 현재 union을 자동으로 감지합니다 */
	union {	/* 한번에 한 멤버만 값을 가질 수 있다. */
		struct uninit_page uninit;		/* 초기화 되지 않은 페이지 정보 */
		struct anon_page anon;			/* 파일에 매핑되지 않은 메모리 영역(익명) 페이지 , 스왑 영역으로 부터 데이터를 로드 */
		struct file_page file;			/* 파일에 매핑된 페이지, 매핑된 파일로부터 데이터를 로드 */
=======
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
>>>>>>> master
#ifdef EFILESYS
		struct page_cache page_cache;   /* 페이지 캐시 정보 */
#endif
	};
};

/* The representation of "frame 다른 멤버 추가 가능 👻 */
// 커널에서 페이지를 물리 메모리 프레임에 매핑할 때 사용하는 구조체입니다. 
// 이 구조체는 주로 페이지를 관리하고, 각 프레임이 어떤 페이지와 연결되어 있는지 
// 추적하는 역할을 합니다.
struct frame {
	void *kva; // 커널이 프레임에 접근할 때 사용하는 가상 주소
	struct page *page; // 해당 프레임에 매핑된 페이지를 가리키는 포인터
};

/* The function table for page operations.
 * This is one way of implementing "interface" in C.
 * Put the table of "method" into the struct's member, and
 * call it whenever you needed. */
struct page_operations {
	bool (*swap_in) (struct page *, void *);	/* 페이지를 스왑 인 하는 함수 포인터 */
	bool (*swap_out) (struct page *);			/* 페이지를 스왑 아웃하는 함수 포인터 */
	void (*destroy) (struct page *);			/* 페이지를 파괴하는 함수 포인터 */	
	enum vm_type type;							/* 페이지의 타입을 나타내는 열거형 */
};

#define swap_in(page, v) (page)->operations->swap_in ((page), v)
#define swap_out(page) (page)->operations->swap_out (page)
#define destroy(page) \
	if ((page)->operations->destroy) (page)->operations->destroy (page)

<<<<<<< HEAD

struct spt_entry {
    void *user_vaddr;           // 사용자 가상 주소
	struct page *page;             // 페이지 정보
    struct hash_elem elem;    	// 해시 테이블 요소
};
/* 현재 프로세스의 메모리 공간을 표현한 구조체입니다.
 * 이 구조체에 대해 특정한 설계를 강요하지 않습니다.
 * 모든 설계는 여러분에게 달려 있습니다. */
struct supplemental_page_table {
	struct hash spt_hash;
=======
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
>>>>>>> master
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
