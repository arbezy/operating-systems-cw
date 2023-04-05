/*
 * The Buddy Page Allocator
 * SKELETON IMPLEMENTATION TO BE FILLED IN FOR TASK 2
 */

#include <infos/mm/page-allocator.h>
#include <infos/mm/mm.h>
#include <infos/kernel/kernel.h>
#include <infos/kernel/log.h>
#include <infos/util/math.h>
#include <infos/util/printf.h>

using namespace infos::kernel;
using namespace infos::mm;
using namespace infos::util;

#define MAX_ORDER	18

/**
 * A buddy page allocation algorithm.
 */
class BuddyPageAllocator : public PageAllocatorAlgorithm
{
private:

	/**
	 * Returns TRUE if the supplied page descriptor is correctly aligned for the 
	 * given order.  Returns FALSE otherwise. Taken from old cw skeleton.
	 * @param pgd The page descriptor to test alignment for.
	 * @param order The order to use for calculations.
	 */
	static inline bool is_correct_alignment_for_order(const PageDescriptor *pgd, int order)
	{
		// Calculate the page-frame-number for the page descriptor, and return TRUE if
		// it divides evenly into the number pages in a block of the given order.
		return (sys.mm().pgalloc().pgd_to_pfn(pgd) % (1 << order)) == 0;
	}

		/**
	 * Inserts a block into the free list of the given order.  The block is inserted in ascending order. 
	 * Taken from old cw skeleton.
	 * @param pgd The page descriptor of the block to insert.
	 * @param order The order in which to insert the block.
	 * @return Returns the slot (i.e. a pointer to the pointer that points to the block) that the block
	 * was inserted into.
	 */
	PageDescriptor **insert_block(PageDescriptor *pgd, int order)
	{
		//mm_log.messagef(LogLevel::DEBUG, "Inserting block @ %x", sys.mm().pgalloc().pgd_to_pfn(pgd));

		// Starting from the _free_area array, find the slot in which the page descriptor
		// should be inserted.

		PageDescriptor **slot = &_free_areas[order];

		// Iterate whilst there is a slot, and whilst the page descriptor pointer is numerically
		// greater than what the slot is pointing to.
		while (*slot && pgd > *slot) {
			slot = &(*slot)->next_free;
		}

		// Insert the page descriptor into the linked list.
		pgd->next_free = *slot;
		*slot = pgd;

		// Return the insert point (i.e. slot)
		return slot;
	}

	/**
	 * Removes a block from the free list of the given order.  The block MUST be present in the free-list, otherwise
	 * the system will panic. Taken from old cw skeleton.
	 * @param pgd The page descriptor of the block to remove.
	 * @param order The order in which to remove the block from.
	 */
	void remove_block(PageDescriptor *pgd, int order)
	{
		//mm_log.messagef(LogLevel::DEBUG, "Removing block @ %x", sys.mm().pgalloc().pgd_to_pfn(pgd));

		// Starting from the _free_area array, iterate until the block has been located in the linked-list. !!!
		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		PageDescriptor **slot = &_free_areas[order];
		while (*slot && pgd != *slot) {
			slot = &(*slot)->next_free;
		}

		// Make sure the block actually exists.  Panic the system if it does not.
		assert(*slot == pgd);

		// Remove the block from the free list.
		*slot = pgd->next_free;
		pgd->next_free = NULL;
	}


	/** Given a page descriptor, and an order, returns the buddy PGD.  The buddy could either be
	 * to the left or the right of PGD, in the given order.
	 * @param pgd The page descriptor to find the buddy for.
	 * @param order The order in which the page descriptor lives.
	 * @return Returns the buddy of the given page descriptor, in the given order.
	 */
	PageDescriptor *buddy_of(PageDescriptor *pgd, int order)
	{
		//mm_log.messagef(LogLevel::DEBUG, "buddy_of: pfn = %x", sys.mm().pgalloc().pgd_to_pfn(pgd));
		// code from previous year skeleton code:

        // (1) Make sure 'order' is within range
		if (order >= MAX_ORDER) {
			return NULL;
		}

		// (2) Check to make sure that PGD is correctly aligned in the order
		if (!is_correct_alignment_for_order(pgd, order)) {
			return NULL;
		}

		// (3) Calculate the page-frame-number of the buddy of this page.
		// * If the PFN is aligned to the next order, then the buddy is the next block in THIS order.
		// * If it's not aligned, then the buddy must be the previous block in THIS order.
		uint64_t buddy_pfn = is_correct_alignment_for_order(pgd, order + 1) ?
			sys.mm().pgalloc().pgd_to_pfn(pgd) + (1 << order) : 
			sys.mm().pgalloc().pgd_to_pfn(pgd) - (1 << order);

		// (4) Return the page descriptor associated with the buddy page-frame-number.
		return sys.mm().pgalloc().pfn_to_pgd(buddy_pfn);
	}


	/**
	 * Given a pointer to a block of free memory in the order "source_order", this function will
	 * split the block in half, and insert it into the order below.
	 * @param block_pointer A pointer to a pointer containing the beginning of a block of free memory.
	 * @param source_order The order in which the block of free memory exists.  Naturally,
	 * the split will insert the two new blocks into the order below.
	 * @return Returns the left-hand-side of the new block.
	 */
	PageDescriptor *split_block(PageDescriptor **block_pointer, int source_order)
	{
		//mm_log.messagef(LogLevel::DEBUG, "splitting block: block pfn = %x, order = %p", sys.mm().pgalloc().pgd_to_pfn(*block_pointer), source_order);

		// Make sure there is an incoming pointer.
		assert(*block_pointer);

		// Make sure the block_pointer is correctly aligned.
		assert(is_correct_alignment_for_order(*block_pointer, source_order));

		/**
		*	Remove og block and insert 2 blocks of order-1 (then return the first of the 2 blocks) 
		*/

		// 1. remove initial block
		remove_block(*block_pointer, source_order);

		// 2. insert 2 blocks of order-1
		PageDescriptor **left_block = insert_block(*block_pointer, source_order-1);
		PageDescriptor **right_block = insert_block(*block_pointer + (1 << (source_order-1)), source_order-1);

		assert(left_block < right_block);
		
		// 3. return the left block
		return *left_block;

	}

	/**
	 * Takes a block in the given source order, and merges it (and its buddy) into the next order.
	 * @param block_pointer A pointer to a pointer containing a block in the pair to merge.
	 * @param source_order The order in which the pair of blocks live.
	 * @return Returns the new slot that points to the merged block.
	 */
	PageDescriptor **merge_block(PageDescriptor **block_pointer, int source_order)
	{
		//mm_log.messagef(LogLevel::DEBUG, "merge block: order = %p", source_order);
        // does this function need to check if a merge is possible? yes

		// todo: add error checking / is aligned
		assert(is_correct_alignment_for_order(*block_pointer, source_order));


		// Remove both the provided block and its buddy, then insert a new block of order+1 to replace both.
		// 1. init 2 blocks to be merged
		PageDescriptor *block1 = *block_pointer;
		PageDescriptor *block2 = buddy_of(block1, source_order);
		uint64_t block1_pfn = sys.mm().pgalloc().pgd_to_pfn(block1);
		uint64_t block2_pfn = sys.mm().pgalloc().pgd_to_pfn(block2);

		// 2. remove block1 & block2
		remove_block(block1, source_order);
		remove_block(block2, source_order);

		// 3. insert merged block of order + 1
		PageDescriptor **merged_block;
		if (block1_pfn < block2_pfn) {
			merged_block = insert_block(block1, source_order + 1);
		}
		else {
			merged_block = insert_block(block2, source_order + 1);
		}

		// 4. return this new merged block

		return merged_block;

	}

public:
	/**
	 * Allocates 2^order number of contiguous pages
	 * @param order The power of two, of the number of contiguous pages to allocate.
	 * @return Returns a pointer to the first page descriptor for the newly allocated page range, or NULL if
	 * allocation failed.
	 */
	PageDescriptor *allocate_pages(int order) override
	{
		//mm_log.messagef(LogLevel::DEBUG, "allocate pages: order=%u", order);

		// todo: write tests/assertions/error checking

		// todo: rewrite using the helper funcs, also with new understanding of wahtt he function is actually
		// 		 supposed to do. DONE?


		// Need to look through all free areas to find a free block, starting with the desired order and going up.
		// Then split this free block up into the target order.
		// note: if we do go up an order then this block will need to be split into the correct size
		// note: may need to start w/ curr_order = order+1???
		PageDescriptor *found_block = NULL;
		int found_order;
		int curr_order = order;
		while (curr_order <= MAX_ORDER) {
			// search through all free areas of that order for a free block
			if (_free_areas[curr_order] == NULL) {
				// no free area for that order
				curr_order++;
			}
			else {
				// free area found
				found_block = _free_areas[curr_order];
				found_order = curr_order;
				break;
			}
		}

		if (curr_order > MAX_ORDER) { return NULL; }

		// split found block into correct size
		while (found_order > order) {
			found_block = split_block(&found_block, found_order);
			found_order--;
		}

		remove_block(found_block, found_order);
		 
		//mm_log.messagef(LogLevel::DEBUG, "allocating page: pfn=%u, type=%p", sys.mm().pgalloc().pgd_to_pfn(found_block), found_block->type);

		return found_block;

	}

	// todo: make private
	/**
	 * gets the length of a block of page descriptors by following the next free pointer until there is no next pgd.
	*/
	int get_block_length(PageDescriptor *pgd)
	{
		// COULD INSTEAD BE DONE BY PERFORMING A LOOKUP IN FREE AREAS, WHEN ITS FOUND WE WILL KNOW ITS ORDER
		uint64_t target_pfn = sys.mm().pgalloc().pgd_to_pfn(pgd);
		for (int i = 0; i < MAX_ORDER+1; i++) {
			PageDescriptor *curr_pgd = _free_areas[i];
			while (curr_pgd != NULL) {
				if (sys.mm().pgalloc().pgd_to_pfn(curr_pgd) == target_pfn) {
					return (1 << i);
				}
				curr_pgd = curr_pgd->next_free;
			}
		}

	}

	/**
	 * Return the order of a block given its length
	 * @param b_len length of the block
	 * @return order of the block
	*/
	uint32_t get_block_order(int b_len)
	{
		return ilog2_floor(b_len);
	}

    /**
	 * Frees 2^order contiguous pages.
	 * @param pgd A pointer to an array of page descriptors to be freed.
	 * @param order The power of two number of contiguous pages to free.
	 */
    void free_pages(PageDescriptor *pgd, int order) override
    {
		//mm_log.messagef(LogLevel::DEBUG, "free pages: pgd=%x, order=%u", sys.mm().pgalloc().pgd_to_pfn(pgd), order);

		/*
		// beautiful code ;_;
		// still doesnt work tho 

		// 1. Make all pages available
		// note: this impl is WRONG as type should NEVER be modified (by me at least)
		pgd->type = PageDescriptorType::AVAILABLE;
		for (int i = 0; i < (1 << order); i++) {
			PageDescriptor *next = pgd->next_free;
			next->type = PageDescriptorType::AVAILABLE;
		}

		// 2. Merge all newly free pages
		uint64_t base_pfn = sys.mm().pgalloc().pgd_to_pfn(pgd);
		for (int i = 0; i < order; i++) {
			int step = 2 << (i);
			for (int j = 0; j < (1 << order); j+step) {
				uint64_t curr_pfn = base_pfn + j;
				PageDescriptor *curr_pgd = sys.mm().pgalloc().pfn_to_pgd(curr_pfn);
				merge_block(&curr_pgd, i);
			}
		}
		*/
		
		PageDescriptor **inserted_block = insert_block(pgd, order);
		//mm_log.messagef(LogLevel::DEBUG, "Inserted block @ %x", sys.mm().pgalloc().pgd_to_pfn(*inserted_block));

		int new_order = order;

		PageDescriptor *buddy = buddy_of(*inserted_block, order);

		// need to check buddy is the same order
		while (is_buddy_free(buddy, new_order) && new_order < MAX_ORDER) {
			// merge block with its buddy
			//mm_log.messagef(LogLevel::DEBUG, "Buddy to merge with @ %x", sys.mm().pgalloc().pgd_to_pfn(buddy));
			inserted_block = merge_block(inserted_block, new_order);
			new_order++;
			// get the new buddy for the next order
			buddy = buddy_of(*inserted_block, new_order);
		}
		

    }

    /**
     * Marks a range of pages as available for allocation.
     * @param start A pointer to the first page descriptors to be made available.
     * @param count The number of page descriptors to make available.
     */
    virtual void insert_page_range(PageDescriptor *start, uint64_t count) override
    {
		//mm_log.messagef(LogLevel::DEBUG, "Insert page range: start=%p, count=%u", start, count);
		//mm_log.messagef(LogLevel::DEBUG, "start pfn =%p", sys.mm().pgalloc().pgd_to_pfn(start));

		uint64_t target_order = ilog2_floor(count);
		//uint64_t start_block_length = get_block_length(start);
		//uint64_t start_block_order = get_block_order(start_block_length);


		//mm_log.messagef(LogLevel::DEBUG, "start block order=%p, desired order=%u", start_block_order, target_order);

		uint64_t remaining = count;
		int i = target_order;
		// should def be >= to but doesnt converge uness >???
		while (i >= 0) {
		// if aligned, insert block of this order
			//mm_log.messagef(LogLevel::DEBUG, "consdering = %x", sys.mm().pgalloc().pgd_to_pfn(start));

			if (is_correct_alignment_for_order(start, i) && ((1 << i) <= remaining) && (i <= MAX_ORDER)) {
				insert_block(start, i);
				start = start + (1 << i);
				remaining -= (1 << i);
				i = target_order;
			}
			else {
				i--;
			}
		}
		dump_state();
		
    }

	bool is_buddy_free(PageDescriptor *buddy, uint64_t order) {
		uint64_t target_pfn = sys.mm().pgalloc().pgd_to_pfn(buddy);
		PageDescriptor *curr_pgd = _free_areas[order];
		while (curr_pgd != NULL) {
			if (sys.mm().pgalloc().pgd_to_pfn(curr_pgd) == target_pfn) {
				return true;
			}
			curr_pgd = curr_pgd->next_free;
		}
		return false;
	}

	void check_for_merge(PageDescriptor **pgd, uint64_t order) {
		int new_order = order;
		PageDescriptor **curr_pgd = pgd;
		//mm_log.messagef(LogLevel::DEBUG, "BUDDY pfn = %x, PGD pfn = %x", sys.mm().pgalloc().pgd_to_pfn(buddy_of(*pgd, order)), sys.mm().pgalloc().pgd_to_pfn(*pgd));
		// todo: need to cehck whether aligned
		if (is_buddy_free(buddy_of(*curr_pgd, new_order), new_order) && new_order < MAX_ORDER) {
			
			//mm_log.messagef(LogLevel::DEBUG, "WE MERGING MFER");
			// merge block with its buddy,
			curr_pgd = merge_block(curr_pgd, new_order);
			// get the new buddy for the next order
			
			//mm_log.messagef(LogLevel::DEBUG, "BUDDY pfn = %x, PGD pfn = %x", sys.mm().pgalloc().pgd_to_pfn(buddy_of(*pgd, new_order)), sys.mm().pgalloc().pgd_to_pfn(*pgd));
		}
	}

	/**
	* 	For a given pgd and order, get the head of the block in which the pgd is located.
	*/
	PageDescriptor *get_nearest_block_head(PageDescriptor *pgd) {
		uint64_t target_pfn = sys.mm().pgalloc().pgd_to_pfn(pgd);
		for (uint64_t i = MAX_ORDER; i >= 0; i--) {
			PageDescriptor *current_free_area = _free_areas[i];
			while (current_free_area != NULL) {
				uint64_t current_free_area_pfn = sys.mm().pgalloc().pgd_to_pfn(current_free_area);
				// note: THIS MIGHT NOT BE i - 1!!!!!!!!!! temp change
				PageDescriptor *next_free_area = current_free_area + (1 << (i));
				uint64_t next_free_area_pfn = sys.mm().pgalloc().pgd_to_pfn(next_free_area);
				if (current_free_area_pfn <= target_pfn && next_free_area_pfn > target_pfn) {
					// block found
					//mm_log.messagef(LogLevel::DEBUG, "Block head for pfn %x found at %x", target_pfn, current_free_area_pfn);
					//mm_log.messagef(LogLevel::DEBUG, "Block head for pfn %x not found at %x", target_pfn, next_free_area_pfn);
					return current_free_area;
				}
				current_free_area = current_free_area->next_free;
			}
		}
		return nullptr;
	}

    /**
     * Marks a range of pages as unavailable for allocation.
     * @param start A pointer to the first page descriptors to be made unavailable.
     * @param count The number of page descriptors to make unavailable.
     */
    virtual void remove_page_range(PageDescriptor *start, uint64_t count) override
    {
		//mm_log.messagef(LogLevel::DEBUG, "Remove page range: start=%x, count=%u", sys.mm().pgalloc().pgd_to_pfn(start), count);

        // start at order = 0 and incr up removing blocks as we go if block < order
		// if block is greater than order, we need to split that block into smaller chunks
		// i.e. if we have a block of order 3 but only want to remove a single pgd (block of order=0)
		// 		then we need to split the block once into 2 order 2s, again into 2 blocks of order 1 
		//		and 1 block of order 2, then splitting the leftmost block again into order = 0 and
		//		removing this new little block.


		int pgds_left_to_remove = count;
		PageDescriptor *current_block = start;

		PageDescriptor *block_head = get_nearest_block_head(start);
		uint64_t block_head_pfn = sys.mm().pgalloc().pgd_to_pfn(block_head);

		// if page range to remove starts at the beginning of a block, then the removal alg is simpler...
		if (block_head_pfn == sys.mm().pgalloc().pgd_to_pfn(start)) {

			while (pgds_left_to_remove > 0) {
				// note: current block length is possibly incorrect
				int curr_block_length = get_block_length(current_block);
				int ord = get_block_order(curr_block_length);
				int pgds_removed = 0;
				if (curr_block_length <= pgds_left_to_remove) {
					// easy removal of whole block
					remove_block(current_block, ord);
					pgds_left_to_remove -= curr_block_length;
					pgds_removed = curr_block_length;
					//mm_log.messagef(LogLevel::DEBUG, "Whole block removed @ pfn %x", sys.mm().pgalloc().pgd_to_pfn(current_block));
				}
				else {
					// Block too big to be removed in one go.
					// Need to split down to size.
					uint64_t order_of_pages_to_remove = ilog2_floor(pgds_left_to_remove);
					for (int i = ord; i > order_of_pages_to_remove; i--)
					{
						current_block = split_block(&current_block, i);
					}
					//mm_log.messagef(LogLevel::DEBUG, "Removing block @ %x", sys.mm().pgalloc().pgd_to_pfn(current_block));
					remove_block(current_block, order_of_pages_to_remove);

					pgds_left_to_remove -= (1 << order_of_pages_to_remove);
					pgds_removed = (1 << order_of_pages_to_remove);
				}
				current_block = current_block + pgds_removed;
			}
		}
		else {
			//mm_log.messagef(LogLevel::DEBUG, "block head not equal to start: start pfn=%x, block head pfn=%x", sys.mm().pgalloc().pgd_to_pfn(start), block_head_pfn);
			// todo: write function that lets remove_page_range work if start is pointing at the middle of a block rather than the start of a block
			bool stop = false;
			while (!stop) {
				uint64_t block_order = get_block_order(get_block_length(block_head));
				//mm_log.messagef(LogLevel::DEBUG, "Block head order == %d, for block %x", block_order, sys.mm().pgalloc().pgd_to_pfn(block_head));
				// note: left block now has order = block_order - 1
				PageDescriptor *left_block = split_block(&block_head, block_order);
				PageDescriptor *right_block = left_block + (1 << (block_order - 1));

				 

				int target_block_pfn = sys.mm().pgalloc().pgd_to_pfn(start);
				if ((sys.mm().pgalloc().pgd_to_pfn(left_block) == target_block_pfn) || (sys.mm().pgalloc().pgd_to_pfn(right_block) == target_block_pfn)) {
					// target block found on left
					dump_state();
					remove_page_range(start, count);
					stop = true;
					break;
				}
				block_head = get_nearest_block_head(start);
			}
		}
    }

	/**
	 * Initialises the allocation algorithm.
	 * @return Returns TRUE if the algorithm was successfully initialised, FALSE otherwise.
	 */
	bool init(PageDescriptor *page_descriptors, uint64_t nr_page_descriptors) override
	{
        // a bit confused about this as we are NOT allowed to assume the memory starts as empty

		// populate free areas with the head of a pgd linked list
		// set all pgd types to reserved except the heads which should be set to available.

		mm_log.messagef(LogLevel::DEBUG, "Buddy Allocator Initialising pd=%p, nr=0x%lx", page_descriptors, nr_page_descriptors);

        PageDescriptor *pgd;
        int remaining_pdgs = nr_page_descriptors;
        uint64_t indx_for_pdg = 0;
        int order = MAX_ORDER;
        int blocks;
        int remaining_blocks;

		for (int i = 0; i < MAX_ORDER; i++) {
			_free_areas[i] = NULL;
		}
        return true;
	}

	/**
	 * Returns the friendly name of the allocation algorithm, for debugging and selection purposes.
	 */
	const char* name() const override { return "buddy"; }

	/**
	 * Dumps out the current state of the buddy system
	 */
	void dump_state() const override
	{
		// Print out a header, so we can find the output in the logs.
		mm_log.messagef(LogLevel::DEBUG, "BUDDY STATE:");

		// Iterate over each free area.
		for (unsigned int i = 0; i < ARRAY_SIZE(_free_areas); i++) {
			char buffer[256];
			snprintf(buffer, sizeof(buffer), "[%d] ", i);

			// Iterate over each block in the free area.
			PageDescriptor *pg = _free_areas[i];
			while (pg) {
				// Append the PFN of the free block to the output buffer.
				snprintf(buffer, sizeof(buffer), "%s%lx ", buffer, sys.mm().pgalloc().pgd_to_pfn(pg));
				pg = pg->next_free;
			}

			mm_log.messagef(LogLevel::DEBUG, "%s", buffer);
		}
	}


private:
	/**
	 * contains the pointers to the heads of the block linked lists (made up of page descriptors)
	*/
	PageDescriptor *_free_areas[MAX_ORDER+1];
};

/* --- DO NOT CHANGE ANYTHING BELOW THIS LINE --- */

/*
 * Allocation algorithm registration framework
 */
RegisterPageAllocator(BuddyPageAllocator);

































// if you r reading this then send me ur paypal and name ur price ;)
// jk