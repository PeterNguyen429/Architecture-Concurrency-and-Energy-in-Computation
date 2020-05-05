#include "paging.h"
#include "page_splitting.h"
#include "swapops.h"
#include "stats.h"

 /* The frame table pointer. You will set this up in system_init. */
fte_t *frame_table;

/*  --------------------------------- PROBLEM 2 --------------------------------------
    In this problem, you will initialize the frame table.

    The frame table will be located at physical address 0 in our simulated
    memory. You will first assign the frame_table global variable to point to
    this location in memory. You should zero out the frame table, in case for
    any reason physical memory is not clean.

    You should then mark the first entry in the frame table as protected. We do
    this because we do not want our free frame allocator to give out the frame
    used by the frame table.

    HINTS:
        You will need to use the following global variables:
        - mem: Simulated physical memory already allocated for you.
        - PAGE_SIZE: The size of one page.
        You will need to initialize (set) the following global variable:
        - frame_table: a pointer to the first entry of the frame table

    -----------------------------------------------------------------------------------
*/
void system_init(void) {
    /*
     * 1. Set the frame table pointer to point to the first frame in physical
     * memory. Zero out the memory used by the frame table.
     *
     * Address "0" in memory will be used for the frame table. This table will
     * contain n frame table entries (fte_t), where n is the number of
     * frames in memory. The frame table will be useful later if we need to
     * evict pages during page faults.
     */
     frame_table = (fte_t*) mem; // points to first frame table entry
     memset(mem, 0, PAGE_SIZE); // intialize everything to 0

    /*
     * 2. Mark the first frame table entry as protected.
     *
     * The frame table contains entries for all of physical memory,
     * however, there are some frames we never want to evict.
     * We mark these special pages as "protected" to indicate this.
     */
    frame_table -> protected = 1; // sets the protected bit to 1.

}

/*  --------------------------------- PROBLEM 3 --------------------------------------
    This function gets called every time a new process is created.
    You will need to allocate a new page table for the process in memory using the
    free_frame function so that the process can store its page mappings. Then, you
    will need to store the PFN of this page table in the process's PCB.

    HINTS:
        - Look at the pcb_t struct defined in pagesim.h to know what to set inside.
        - You are not guaranteed that the memory returned by the free frame allocator
        is empty - an existing frame could have been evicted for our new page table.
        - As in the previous problem, think about whether we need to mark any entries
        in the frame_table as protected after we allocate memory for our page table.
    -----------------------------------------------------------------------------------
*/
void proc_init(pcb_t *proc) {
    /*
     * 1. Call the free frame allocator (free_frame) to return a free frame for
     * this process's page table. You should zero-out the memory.
     */
     pfn_t fnum = free_frame(); // returns an index in the frame table
     void* frame_pointer = mem + (fnum*PAGE_SIZE); // returns the pointer in mem of the frame frame_table
     memset(frame_pointer, 0, PAGE_SIZE);

    /*
     * 2. Update the process's PCB with the frame number
     * of the newly allocated page table.
     *
     * Additionally, mark the frame's frame table entry as protected. You do not
     * want your page table to be accidentally evicted.
     */
     proc -> saved_ptbr = fnum; //updates the frame number
     frame_table[fnum].protected = 1; // sets the protected bit

}

/*  --------------------------------- PROBLEM 4 --------------------------------------
    Swaps the currently running process on the CPU to another process.

    Every process has its own page table, as you allocated in proc_init. You will
    need to tell the processor to use the new process's page table.

    HINTS:
        - Look at the global variables defined in pagesim.h. You may be interested in
        the definition of pcb_t as well.
    -----------------------------------------------------------------------------------
 */
void context_switch(pcb_t *proc) {
  PTBR = proc->saved_ptbr; // sets the PTBR to the saved one in the process
}

/*  --------------------------------- PROBLEM 5 --------------------------------------
    Takes an input virtual address and returns the data from the corresponding
    physical memory address. The steps to do this are:

    1) Translate the virtual address into a physical address using the page table.
    2) Go into the memory and read/write the data at the translated address.

    Parameters:
        1) address     - The virtual address to be translated.
        2) rw          - 'r' if the access is a read, 'w' if a write
        3) data        - One byte of data to write to our memory, if the access is a write.
                         This byte will be NULL on read accesses.

    Return:
        The data at the address if the access is a read, or
        the data we just wrote if the access is a write.

    HINTS:
        - You will need to use the macros we defined in Problem 1 in this function.
        - You will need to access the global PTBR value. This will tell you where to
        find the page table. Be very careful when you think about what this register holds!
        - On a page fault, simply call the page_fault function defined in page_fault.c.
        You may assume that the pagefault handler allocated a page for your address
        in the page table after it returns.
        - Make sure to set the referenced bit in the frame table entry since we accessed the page.
        - Make sure to set the dirty bit in the page table entry if it's a write.
        - Make sure to update the stats variables correctly (see stats.h)
    -----------------------------------------------------------------------------------
 */
uint8_t mem_access(vaddr_t address, char rw, uint8_t data) {

    /* Split the address and find the page table entry */
    vpn_t vpn = vaddr_vpn(address); // returns the virtual address
    uint16_t offset = vaddr_offset(address); //returns the offset

    /* If an entry is invalid, just page fault to allocate a page for the page table. */
    pte_t* page_table = (pte_t*) (mem+(PAGE_SIZE * PTBR)); // finds the first page table in the frame table

      if (page_table[vpn].valid == 0){ //if its not valid, its a page fault
        stats.page_faults++;
        page_fault(address);
      }
    /* Set the "referenced" bit to reduce the page's likelihood of eviction */


    /*
        The physical address will be constructed like this:
        -------------------------------------
        |     PFN    |      Offset          |
        -------------------------------------
        where PFN is the value stored in the page table entry.
        We need to calculate the number of bits are in the offset.

        Create the physical address using your offset and the page
        table entry.
    */


      paddr_t physical_addr = (paddr_t) ((page_table[vpn].pfn << OFFSET_LEN) | offset); // create a physical address by OR
      stats.accesses++;
    /* Either read or write the data to the physical address
       depending on 'rw' */
    if (rw == 'r') {
        stats.reads++; // increments statistics on read
        return mem[physical_addr]; //returns the data at the address
    } else {
        page_table[vpn].dirty = 1; // wrtiting causes it to be dirty
        mem[physical_addr] = data; // the memory is now whatever data is
        stats.writes++; //increments writes
        return data;
    }
}

/*  --------------------------------- PROBLEM 8 --------------------------------------
    When a process exits, you need to free any pages previously occupied by the
    process. Otherwise, every time you closed and re-opened Microsoft Word, it
    would gradually eat more and more of your computer's usable memory.

    To free a process, you must clear the "mapped" bit on every page the process
    has mapped. If the process has swapped any pages to disk, you must call
    swap_free() using the page table entry pointer as a parameter.

    You must also clear the "protected" bits for the page table itself.
    -----------------------------------------------------------------------------------
*/
void proc_cleanup(pcb_t *proc) {
    /* Look up the process's page table */
    pfn_t saved = proc -> saved_ptbr; // returns the saved ptbr
    pte_t* page_table = (pte_t*) (mem + (saved * PAGE_SIZE)); //returns the page_table
    /* Iterate the page table and clean up each valid page */
    for (size_t i = 0; i < NUM_PAGES; i++) { // iterates through the number of pages
      if(page_table[i].valid == 1) { //clear map bit if its valid
        pfn_t pfn = page_table[i].pfn;
        frame_table[pfn].mapped = 0;
      }
      if (page_table[i].swap){ //if the swap bit is not 0 swap_free the page_table entry
        swap_free(&page_table[i]);
      }

    /* Free the page table itself in the frame table */

}

  frame_table[saved].mapped = 0; //sets the frame table mapped to 0
  frame_table[saved].protected = 0; // sets protected to 0



}
