/*
 * screen.c
 *
 * Generic screen manipulation
 *
 */

#include "frotz.h"

#if 1

void z_nothing(void)
{
}

void z_buffer_mode (void)
{
}

void z_draw_picture (void)
{
}

void z_erase_line (void)
{
}

void z_window_style()
{
}

void z_get_cursor (void) {}
void z_get_wind_prop (void) {}
void z_mouse_window (void) {}
void z_move_window (void) {}
void z_picture_data (void) {}
void z_picture_table (void) {}

void z_set_colour (void) {}
void z_split_window (void) {}
void z_set_font (void) {}
void z_set_cursor (void) {}
void z_set_text_style (void) {}

static zword winarg0 (void)
{
    return zargs[0];
}

void z_erase_window (void)
{
	/*
	if ((short) zargs[0] == -1 || (short) zargs[0] == -2)
		erase_screen (zargs[0]);
	else
		erase_window (winarg0());
		*/
}

void set_window (zword win);
void z_set_window (void)
{
    set_window (winarg0 ());
}

void z_print_table (void)
{
    zword addr = zargs[0];
    //zword x;
    int i, j;

    //flush_buffer ();

    /* Supply default arguments */

    if (zargc < 3)
	zargs[2] = 1;
    if (zargc < 4)
	zargs[3] = 0;

    /* Write text in width x height rectangle */

    //x = cwp->x_cursor;

    for (i = 0; i < zargs[2]; i++) {

	if (i != 0) {

	   // flush_buffer ();

	    //cwp->y_cursor += font_height;
	   // cwp->x_cursor = x;

	    //update_cursor ();

	}

	for (j = 0; j < zargs[1]; j++) {

	    zbyte c;

	    LOW_BYTE (addr, c)
	    addr++;

	    print_char (c);

	}

	addr += zargs[3];

    }

}/* z_print_table */

static void pad_status_line(zbyte n)
{
	while (n--)
		print_char(' ');
}

void set_status_x(int x);

void z_show_status (void)
{
    zword global0;
    zword global1;
    zword global2;
    zword addr;

    //bool brief = FALSE;

    /* One V5 game (Wishbringer Solid Gold) contains this opcode by
       accident, so just return if the version number does not fit */

    if (h_version >= V4)
	return;

    /* Read all relevant global variables from the memory of the
       Z-machine into local variables */

    addr = h_globals;
    LOW_WORD (addr, global0)
    addr += 2;
    LOW_WORD (addr, global1)
    addr += 2;
    LOW_WORD (addr, global2)

    /* Frotz uses window 7 for the status line. Don't forget to select
       reverse and fixed width text style */

    set_window (7);

	//print_char (ZC_NEW_STYLE);
	//print_char (REVERSE_STYLE | FIXED_WIDTH_STYLE);

    /* If the screen width is below 55 characters then we have to use
       the brief status line format */

   // if (h_screen_cols < 55)
	//brief = TRUE;

    /* Print the object description for the global variable 0 */

    //print_char (' ');
		print_object (global0);

    /* A header flag tells us whether we have to display the current
       time or the score/moves information */

    if (h_config & CONFIG_TIME) {	/* print hours and minutes */

	zword hours = (global1 + 11) % 12 + 1;

		pad_status_line (5);

		print_string ("Time: ");

		if (hours < 10)
			print_char (' ');
		print_num (hours);

		print_char (':');

		if (global2 < 10)
			print_char ('0');
		print_num (global2);

		print_char (' ');

		print_char ((global1 >= 12) ? 'p' : 'a');	// TODO
		print_char ('m');

    } else {				/* print score and moves */

		set_status_x (136);

		print_string ("Score: ");
		print_num (global1);

		set_status_x (185);

		print_string("Moves: ");
		print_num (global2);

    }

    /* Pad the end of the status line with spaces */

    //pad_status_line (0);

    /* Return to the lower window */

    set_window (0);

}/* z_show_status */

void refresh_text_style (void)
{
}

void restart_screen (void)
{
}

#else

#endif
