/*
 * fastmem.c
 *
 * Memory related functions (fast version without virtual memory)
 *
 */

#include <stdio.h>
#include <string.h>
#include "frotz.h"

#ifdef __MSDOS__

#include <alloc.h>

#define malloc(size)	farmalloc (size)
#define realloc(size,p)	farrealloc (size,p)
#define free(size)	farfree (size)
#define memcpy(d,s,n)	_fmemcpy (d,s,n)

#else

#include <stdlib.h>

#ifndef SEEK_SET
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#endif

#define far

#endif

extern void seed_random (int);
extern void restart_screen (void);
extern void refresh_text_style (void);
extern void call (zword, int, zword *, int);
extern void split_window (zword);
extern void script_open (void);
extern void script_close (void);

extern void (*op0_opcodes[]) (void);
extern void (*op1_opcodes[]) (void);
extern void (*op2_opcodes[]) (void);
extern void (*var_opcodes[]) (void);

#if 0
char save_name[MAX_FILE_NAME + 1] = DEFAULT_SAVE_NAME;
char auxilary_name[MAX_FILE_NAME + 1] = DEFAULT_AUXILARY_NAME;

//zbyte far *zmp = NULL;
//zbyte far *pcp = NULL;

static FILE *story_fp = NULL;

static zbyte far *undo[MAX_UNDO_SLOTS];

static undo_slots = 0;
static undo_count = 0;
static undo_valid = 0;
#endif

/*
 * get_header_extension
 *
 * Read a value from the header extension (former mouse table).
 *
 */

zword get_header_extension (int entry)
{
    zword addr;
    zword val;

    if (h_extension_table == 0 || entry > hx_table_size)
	return 0;

    addr = h_extension_table + 2 * entry;
    LOW_WORD (addr, val)

    return val;

}/* get_header_extension */

/*
 * set_header_extension
 *
 * Set an entry in the header extension (former mouse table).
 *
 */

void set_header_extension (int entry, zword val)
{
    zword addr;

    if (h_extension_table == 0 || entry > hx_table_size)
	return;

    addr = h_extension_table + 2 * entry;
    SET_WORD (addr, val)

}/* set_header_extension */

/*
 * restart_header
 *
 * Set all header fields which hold information about the interpreter.
 *
 */

#if 0
void restart_header (void)
{
    zword screen_x_size;
    zword screen_y_size;
    zbyte font_x_size;
    zbyte font_y_size;

    int i;

    SET_BYTE (H_CONFIG, h_config)
    SET_WORD (H_FLAGS, h_flags)

    if (h_version >= V4) {
	SET_BYTE (H_INTERPRETER_NUMBER, h_interpreter_number)
	SET_BYTE (H_INTERPRETER_VERSION, h_interpreter_version)
	SET_BYTE (H_SCREEN_ROWS, h_screen_rows)
	SET_BYTE (H_SCREEN_COLS, h_screen_cols)
    }

    /* It's less trouble to use font size 1x1 for V5 games, especially
       because of a bug in the unreleased German version of "Zork 1" */

    if (h_version != V6) {
	screen_x_size = (zword) h_screen_cols;
	screen_y_size = (zword) h_screen_rows;
	font_x_size = 1;
	font_y_size = 1;
    } else {
	screen_x_size = h_screen_width;
	screen_y_size = h_screen_height;
	font_x_size = h_font_width;
	font_y_size = h_font_height;
    }

    if (h_version >= V5) {
	SET_WORD (H_SCREEN_WIDTH, screen_x_size)
	SET_WORD (H_SCREEN_HEIGHT, screen_y_size)
	SET_BYTE (H_FONT_HEIGHT, font_y_size)
	SET_BYTE (H_FONT_WIDTH, font_x_size)
	SET_BYTE (H_DEFAULT_BACKGROUND, h_default_background)
	SET_BYTE (H_DEFAULT_FOREGROUND, h_default_foreground)
    }

    if (h_version == V6)
	for (i = 0; i < 8; i++)
	    storeb ((zword) (H_USER_NAME + i), h_user_name[i]);

    SET_BYTE (H_STANDARD_HIGH, h_standard_high)
    SET_BYTE (H_STANDARD_LOW, h_standard_low)

}/* restart_header */
#endif

/*
 * init_memory
 *
 * Allocate memory and load the story file.
 *
 */


void init_memory (void)
{
#if 0
    long size;
    zword addr;
    unsigned n;
    int i, j;

    static struct {
	enum story story_id;
	zword release;
	zbyte serial[6];
    } records[] = {
	{       SHERLOCK,  21, "871214" },
	{       SHERLOCK,  26, "880127" },
	{    BEYOND_ZORK,  47, "870915" },
	{    BEYOND_ZORK,  49, "870917" },
	{    BEYOND_ZORK,  51, "870923" },
	{    BEYOND_ZORK,  57, "871221" },
	{      ZORK_ZERO, 296, "881019" },
	{      ZORK_ZERO, 366, "890323" },
	{      ZORK_ZERO, 383, "890602" },
	{      ZORK_ZERO, 393, "890714" },
	{         SHOGUN, 292, "890314" },
	{         SHOGUN, 295, "890321" },
	{         SHOGUN, 311, "890510" },
	{         SHOGUN, 322, "890706" },
	{         ARTHUR,  54, "890606" },
	{         ARTHUR,  63, "890622" },
	{         ARTHUR,  74, "890714" },
	{        JOURNEY,  26, "890316" },
	{        JOURNEY,  30, "890322" },
	{        JOURNEY,  77, "890616" },
	{        JOURNEY,  83, "890706" },
	{ LURKING_HORROR, 203, "870506" },
	{ LURKING_HORROR, 219, "870912" },
	{ LURKING_HORROR, 221, "870918" },
	{        UNKNOWN,   0, "------" }
    };
#endif

    /* Open story file */
	//OpenZFile();

    /* Copy header fields to global variables */
    LOW_BYTE (H_VERSION, h_version)

   // if (h_version < V1 || h_version > V8)
	//os_fatal ("Unknown Z-code version");

    LOW_BYTE (H_CONFIG, h_config)

   // if (h_version == V3 && (h_config & CONFIG_BYTE_SWAPPED))
	//os_fatal ("Byte swapped story file");

    //LOW_WORD (H_RELEASE, h_release)
    //LOW_WORD (H_RESIDENT_SIZE, h_resident_size)
    LOW_WORD (H_START_PC, h_start_pc)
    LOW_WORD (H_DICTIONARY, h_dictionary)
    LOW_WORD (H_OBJECTS, h_objects)
    LOW_WORD (H_GLOBALS, h_globals)
    LOW_WORD (H_DYNAMIC_SIZE, h_dynamic_size)
    //LOW_WORD (H_FLAGS, h_flags)

    //for (i = 0, addr = H_SERIAL; i < 6; i++, addr++)
	//LOW_BYTE (addr, h_serial[i])

#if 0
    /* Auto-detect buggy story files that need special fixes */

    for (i = 0; records[i].story_id != UNKNOWN; i++) {

	if (h_release == records[i].release) {

	    for (j = 0; j < 6; j++)
		if (h_serial[j] != records[i].serial[j])
			break;
		    //goto no_match;

	    story_id = records[i].story_id;

	}

   // no_match:

    }
#endif

    LOW_WORD (H_ABBREVIATIONS, h_abbreviations)
    LOW_WORD (H_FILE_SIZE, h_file_size)

    /* Calculate story file size in bytes */

    if (h_file_size != 0) {

	story_size = (long) 2 * h_file_size;

	if (h_version >= V4)
	    story_size *= 2;
	if (h_version >= V6)
	    story_size *= 2;

    } else {		/* some old games lack the file size entry */

	//fseek (story_fp, 0, SEEK_END);
	//story_size = ftell (story_fp);
	//fseek (story_fp, 64, SEEK_SET);

    }

    LOW_WORD (H_CHECKSUM, h_checksum)
    LOW_WORD (H_ALPHABET, h_alphabet)
    LOW_WORD (H_FUNCTIONS_OFFSET, h_functions_offset)
    LOW_WORD (H_STRINGS_OFFSET, h_strings_offset)
    //LOW_WORD (H_TERMINATING_KEYS, h_terminating_keys)
    LOW_WORD (H_EXTENSION_TABLE, h_extension_table)

    /* Zork Zero Macintosh doesn't have the graphics flag set */

    //if (story_id == ZORK_ZERO && h_release == 296)
	//h_flags |= GRAPHICS_FLAG;

    /* Adjust opcode tables */

    if (h_version <= V4) {
	//	op0_opcodes[0x09] = z_pop;
	//	op1_opcodes[0x0f] = z_not;
    } else {
		op0_opcodes[0x09] = z_catch;
		op1_opcodes[0x0f] = z_call_n;
    }

#if 1
	{
	zbyte h_screen_rows = 255;
	zbyte h_screen_cols = 72;
	SET_BYTE (H_SCREEN_ROWS, h_screen_rows)
	SET_BYTE (H_SCREEN_COLS, h_screen_cols)
	}
#endif

    /* Read header extension table */

    hx_table_size = get_header_extension (HX_TABLE_SIZE);
    hx_unicode_table = get_header_extension (HX_UNICODE_TABLE);

}/* init_memory */

/*
 * init_undo
 *
 * Allocate memory for multiple undo. It is important not to occupy
 * all the memory available, since the IO interface may need memory
 * during the game, e.g. for loading sounds or pictures.
 *
 */

void init_undo (void)
{
#if 0
    void far *reserved;

    if (reserve_mem != 0)
	if ((reserved = malloc (reserve_mem)) == NULL)
	    return;

    while (undo_slots < option_undo_slots && undo_slots < MAX_UNDO_SLOTS) {

	void far *mem = malloc ((long) sizeof (stack) + h_dynamic_size);

	if (mem == NULL)
	    break;

	undo[undo_slots++] = mem;

    }

    if (reserve_mem != 0)
	free (reserved);
#endif

}/* init_undo */

/*
 * reset_memory
 *
 * Close the story file and deallocate memory.
 *
 */

#if 0

void reset_memory (void)
{

    fclose (story_fp);

    while (undo_slots--)
	free (undo[undo_slots]);

//    free (zmp);

}/* reset_memory */

#endif
/*
 * storeb
 *
 * Write a byte value to the dynamic Z-machine memory.
 *
 */

void storeb (zword addr, zbyte value)
{

//    if (addr >= h_dynamic_size)
//	runtime_error ("Store out of dynamic memory");

    if (addr == H_FLAGS + 1) {	/* flags register is modified */

#if 0
	h_flags &= ~(SCRIPTING_FLAG | FIXED_FONT_FLAG);
	h_flags |= value & (SCRIPTING_FLAG | FIXED_FONT_FLAG);

	if (value & SCRIPTING_FLAG) {
	    if (!ostream_script)
		script_open ();
	} else {
	    if (ostream_script)
		script_close ();
	}
#endif
	refresh_text_style ();

    }

    SET_BYTE (addr, value)

}/* storeb */

/*
 * storew
 *
 * Write a word value to the dynamic Z-machine memory.
 *
 */

void storew (zword addr, zword value)
{

    storeb ((zword) (addr + 0), hi (value));
    storeb ((zword) (addr + 1), lo (value));

}/* storew */

/*
 * z_restart, re-load dynamic area, clear the stack and set the PC.
 *
 * 	no zargs used
 *
 */

void z_restart (void)
{
    //static bool first_restart = TRUE;

    flush_buffer ();

    os_restart_game (RESTART_BEGIN);

    seed_random (0);

    //if (!first_restart) {
	//	OpenZFile();
#if 0

	fseek (story_fp, 0, SEEK_SET);
	if (fread (zmp, 1, h_dynamic_size, story_fp) != h_dynamic_size)
	    os_fatal ("Story file read error");
    } else first_restart = FALSE;
#endif

    //restart_header ();
    restart_screen ();

    sp = fp = STACK_SIZE;

    if (h_version != V6) {

	long pc = (long) h_start_pc;
	SET_PC (pc)

    } else call (h_start_pc, 0, NULL, 0);

    os_restart_game (RESTART_END);

}/* z_restart */

/*
 * get_default_name
 *
 * Read a default file name from the memory of the Z-machine and
 * copy it to a string.
 *
 */



/*
 * restore_undo
 *
 * This function does the dirty work for z_restore_undo.
 *
 */

int restore_undo (void)
{
	return -1;
#if 0

    if (undo_slots == 0)	/* undo feature unavailable */

	return -1;

    else if (undo_valid == 0)	/* no saved game state */

	return 0;

    else {			/* undo possible */

	long pc;

	if (undo_count == 0)
	    undo_count = undo_slots;

	memcpy (stack, undo[undo_count - 1], sizeof (stack));
	memcpy (zmp, undo[undo_count - 1] + sizeof (stack), h_dynamic_size);

	pc = ((long) stack[0] << 16) | stack[1];
	sp = stack + stack[2];
	fp = stack + stack[3];

	SET_PC (pc)

	restart_header ();

	undo_count--;
	undo_valid--;

	return 2;

    }
#endif
}/* restore_undo */

/*
 * z_restore_undo, restore a Z-machine state from memory.
 *
 *	no zargs used
 *
 */

void z_restore_undo (void)
{

    store ((zword) restore_undo ());

}/* restore_undo */

/*
 * save_undo
 *
 * This function does the dirty work for z_save_undo.
 *
 */

int save_undo (void)
{
	return -1;
#if 0
    long pc;
    if (undo_slots == 0)	/* undo feature unavailable */

	return -1;

    else {			/* save undo possible */

	if (undo_count == undo_slots)
	    undo_count = 0;

	GET_PC (pc)

	stack[0] = (zword) (pc >> 16);
	stack[1] = (zword) (pc & 0xffff);
	stack[2] = (zword) (sp - stack);
	stack[3] = (zword) (fp - stack);

	memcpy (undo[undo_count], stack, sizeof (stack));
	memcpy (undo[undo_count] + sizeof (stack), zmp, h_dynamic_size);

	if (++undo_count == undo_slots)
	    undo_count = 0;
	if (++undo_valid > undo_slots)
	    undo_valid = undo_slots;

	return 1;

    }
#endif

}/* save_undo */

/*
 * z_save_undo, save the current Z-machine state for a future undo.
 *
 *	no zargs used
 *
 */

void z_save_undo (void)
{

    store ((zword) save_undo ());

}/* z_save_undo */

/*
 * z_verify, check the story file integrity.
 *
 *	no zargs used
 *
 */

void z_verify (void)
{
	branch (TRUE);
}/* z_verify */
