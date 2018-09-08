/*
 * Based on arch/arm/include/asm/tlb.h
 *
 * Copyright (C) 2002 Russell King
 * Copyright (C) 2012 ARM Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef __ASM_TLB_H
#define __ASM_TLB_H

#define  __tlb_remove_pmd_tlb_entry __tlb_remove_pmd_tlb_entry

#include <asm/pgalloc.h>
#include <asm/tlbflush.h>

#define MMU_GATHER_BUNDLE	8

/*
 * TLB handling.  This allows us to remove pages from the page
 * tables, and efficiently handle the TLB issues.
 */
struct mmu_gather {
	struct mm_struct	*mm;
	unsigned int		fullmm;
	struct vm_area_struct	*vma;
	unsigned long		start, end;
	unsigned long		range_start;
	unsigned long		range_end;
	unsigned int		nr;
	unsigned int		max;
	struct page		**pages;
	struct page		*local[MMU_GATHER_BUNDLE];
};

/*
 * There's three ways the TLB shootdown code is used:
 *  1. Unmapping a range of vmas.  See zap_page_range(), unmap_region().
 *     tlb->fullmm = 0, and tlb_start_vma/tlb_end_vma will be called.
 *  2. Unmapping all vmas.  See exit_mmap().
 *     tlb->fullmm = 1, and tlb_start_vma/tlb_end_vma will be called.
 *     Page tables will be freed.
 *  3. Unmapping argument pages.  See shift_arg_pages().
 *     tlb->fullmm = 0, but tlb_start_vma/tlb_end_vma will not be called.
 */
static inline void tlb_flush(struct mmu_gather *tlb)
{
	if (tlb->fullmm) {
		flush_tlb_mm(tlb->mm);
	} else if (tlb->end > 0) {
		struct vm_area_struct vma = { .vm_mm = tlb->mm, };
		flush_tlb_range(&vma, tlb->start, tlb->end);
		tlb->start = TASK_SIZE;
		tlb->end = 0;
	}
}

static inline void tlb_add_flush(struct mmu_gather *tlb, unsigned long addr)
{
	if (!tlb->fullmm) {
		tlb->start = min(tlb->start, addr);
		tlb->end = max(tlb->end, addr + PAGE_SIZE);
	}
}

static inline void __tlb_alloc_page(struct mmu_gather *tlb)
{
	unsigned long addr = __get_free_pages(GFP_NOWAIT | __GFP_NOWARN, 0);

	if (addr) {
		tlb->pages = (void *)addr;
		tlb->max = PAGE_SIZE / sizeof(struct page *);
	}
}

static inline void tlb_flush_mmu(struct mmu_gather *tlb)
{
	tlb_flush(tlb);
	free_pages_and_swap_cache(tlb->pages, tlb->nr);
	tlb->nr = 0;
	if (tlb->pages == tlb->local)
		__tlb_alloc_page(tlb);
}

static inline void
tlb_gather_mmu(struct mmu_gather *tlb, struct mm_struct *mm, unsigned long start, unsigned long end)
{
	tlb->mm = mm;
	tlb->fullmm = !(start | (end+1));
	tlb->start = start;
	tlb->end = end;
	tlb->vma = NULL;
	tlb->max = ARRAY_SIZE(tlb->local);
	tlb->pages = tlb->local;
	tlb->nr = 0;
	__tlb_alloc_page(tlb);
}

static inline void
tlb_finish_mmu(struct mmu_gather *tlb, unsigned long start, unsigned long end)
{
	tlb_flush_mmu(tlb);

	/* keep the page table cache within bounds */
	check_pgt_cache();

	if (tlb->pages != tlb->local)
		free_pages((unsigned long)tlb->pages, 0);
}

/*
 * Memorize the range for the TLB flush.
 */
static inline void __tlb_remove_tlb_entry(struct mmu_gather *tlb, pte_t *ptep,
					  unsigned long addr)
{
	tlb_add_flush(tlb, addr);
}

/*
 * In the case of tlb vma handling, we can optimise these away in the
 * case where we're doing a full MM flush.  When we're doing a munmap,
 * the vmas are adjusted to only cover the region to be torn down.
 */
static inline void tlb_start_vma(struct mmu_gather *tlb,
				 struct vm_area_struct *vma)
{
	if (!tlb->fullmm) {
		tlb->start = TASK_SIZE;
		tlb->end = 0;
	}
}

static inline void tlb_end_vma(struct mmu_gather *tlb,
			       struct vm_area_struct *vma)
{
	if (!tlb->fullmm)
		tlb_flush(tlb);
}

static inline void __pte_free_tlb(struct mmu_gather *tlb, pgtable_t pte,
				  unsigned long addr)
{
	pgtable_page_dtor(pte);
	tlb_add_flush(tlb, addr);
	tlb_remove_page(tlb, pte);
}

#ifndef CONFIG_ARM64_64K_PAGES
static inline void __pmd_free_tlb(struct mmu_gather *tlb, pmd_t *pmdp,
				  unsigned long addr)
{
	tlb_add_flush(tlb, addr);
	tlb_remove_page(tlb, virt_to_page(pmdp));
}
#endif

static inline void __tlb_remove_pmd_tlb_entry(struct mmu_gather *tlb, pmd_t *pmdp,
						unsigned long address)
{
	tlb_add_flush(tlb, address);
}

#endif
