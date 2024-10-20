/* file.c: Implementation of memory backed file object (mmaped object). */

#include "vm/vm.h"
#include "userprog/process.h"
#include "threads/vaddr.h"

static bool file_backed_swap_in (struct page *page, void *kva);
static bool file_backed_swap_out (struct page *page);
static void file_backed_destroy (struct page *page);
void do_munmap(void *addr);
void* do_mmap(void *addr, size_t length, int writable, struct file *file, off_t offset);

/* DO NOT MODIFY this struct */
static const struct page_operations file_ops = {
	.swap_in = file_backed_swap_in,
	.swap_out = file_backed_swap_out,
	.destroy = file_backed_destroy,
	.type = VM_FILE,
};

/* The initializer of file vm */
void
vm_file_init (void) {
}

/* Initialize the file backed page */
bool
file_backed_initializer (struct page *page, enum vm_type type, void *kva) {
	/* Set up the handler */
	page->operations = &file_ops;

	struct file_page *file_page = &page->file;
}

/* Swap in the page by read contents from the file. */
static bool
file_backed_swap_in (struct page *page, void *kva) {
	struct file_page *file_page UNUSED = &page->file;
}

/* Swap out the page by writeback contents to the file. */
static bool
file_backed_swap_out (struct page *page) {
	struct file_page *file_page UNUSED = &page->file;
}

/* Destory the file backed page. PAGE will be freed by the caller. */
static void
file_backed_destroy (struct page *page) {
	struct file_page *file_page UNUSED = &page->file;
}

/* Do the mmap */
void*
do_mmap (void *addr, size_t length, int writable, struct file *file, off_t offset) {
	struct file *get_file = file_reopen(file);
	void *start_addr = addr;

	size_t read_bytes = file_length(file) < length ? file_length(file) : length;
	size_t zero_bytes = PGSIZE - read_bytes % PGSIZE;

	ASSERT ((read_bytes + zero_bytes) % PGSIZE == 0);
	ASSERT (pg_ofs (addr) == 0);
	ASSERT (offset % PGSIZE == 0);
	while (read_bytes > 0 || zero_bytes > 0) {

		size_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;
		size_t page_zero_bytes = PGSIZE - page_read_bytes;

		struct container *container = (struct container *)malloc(sizeof(struct container));
		container->file = get_file;
		container->offset = offset;
		container->read_bytes = page_read_bytes;

		if (!vm_alloc_page_with_initializer (VM_FILE, addr, writable, lazy_load_segment, container)){
			return NULL;
		}
		// page fault가 호출되면 페이지가 타입별로 초기화되고 lazy_load_segment()가 실행된다. 

		//다음 페이지 
		read_bytes -= page_read_bytes;
		zero_bytes -= page_zero_bytes;
		addr += PGSIZE;
		offset += page_read_bytes;
	}
	return start_addr;
}

/* Do the munmap */
void
do_munmap (void *addr) {
	while(true){
		struct thread *curr = thread_current();
		struct page *find_page = spt_find_page(&curr->spt, addr);
		// struct frame *find_frame =find_page->frame;
		
		if (find_page == NULL) {
    		return NULL;
    	}

		// 연결 해제
		// find_page->frame = NULL;
		// find_frame->page = NULL;

		struct container* container = (struct container*)find_page->uninit.aux;
		// 페이지의 dirty bit이 1이면 true를, 0이면 false를 리턴한다.
		if (pml4_is_dirty(curr->pml4, find_page->va)){
			// 물리 프레임에 변경된 데이터를 다시 디스크 파일에 업데이트 buffer에 있는 데이터를 size만큼, file의 file_ofs부터 써준다.
			file_write_at(container->file, addr, container->read_bytes, container->offset);
			// dirty bit = 0
			// 인자로 받은 dirty의 값이 1이면 page의 dirty bit을 1로, 0이면 0으로 변경해준다.
			pml4_set_dirty(curr->pml4,find_page->va,0);
		} 
		// dirty bit = 0
		// 인자로 받은 dirty의 값이 1이면 page의 dirty bit을 1로, 0이면 0으로 변경해준다.
		
		// present bit = 0
		// 페이지의 present bit 값을 0으로 만들어주는 함수
		pml4_clear_page(curr->pml4, find_page->va); 
		addr += PGSIZE;
	}
}
