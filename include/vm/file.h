#ifndef VM_FILE_H
#define VM_FILE_H
#include "filesys/file.h"
#include "vm/vm.h"

struct page;
enum vm_type;

/* 👻 */
struct file_page {
	struct file* file;	/* 매핑된 파일 */
	size_t offset;
	size_t read_bytes;	/* 가상주소에 쓰인 데이터 크기 */
	size_t zero_bytes;
};

void vm_file_init (void);
bool file_backed_initializer (struct page *page, enum vm_type type, void *kva);
void *do_mmap(void *addr, size_t length, int writable,
		struct file *file, off_t offset);
void do_munmap (void *va);
#endif
