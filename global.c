/* $Id$ */
/**************************************************************************
 *   global.c                                                             *
 *                                                                        *
 *   Copyright (C) 1999-2002 Chris Allegretta                             *
 *   This program is free software; you can redistribute it and/or modify *
 *   it under the terms of the GNU General Public License as published by *
 *   the Free Software Foundation; either version 2, or (at your option)  *
 *   any later version.                                                   *
 *                                                                        *
 *   This program is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of       *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        *
 *   GNU General Public License for more details.                         *
 *                                                                        *
 *   You should have received a copy of the GNU General Public License    *
 *   along with this program; if not, write to the Free Software          *
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.            *
 *                                                                        *
 **************************************************************************/

#include "config.h"

#include <assert.h>
#include <sys/stat.h>
#include "nano.h"
#include "proto.h"

#ifdef ENABLE_NLS
#include <libintl.h>
#define _(string) gettext(string)
#else
#define _(string) (string)
#endif

/*
 * Global variables
 */

int flags = 0;			/* Our new flag containing many options */
WINDOW *edit;			/* The file portion of the editor  */
WINDOW *topwin;			/* Top line of screen */
WINDOW *bottomwin;		/* Bottom buffer */
char *filename = NULL;		/* Name of the file */
int editwinrows = 0;		/* How many rows long is the edit
				   window? */
filestruct *current;		/* Current buffer pointer */
int current_x = 0, current_y = 0;	/* Current position of X and Y in
					   the editor - relative to edit
					   window (0,0) */
filestruct *fileage = NULL;	/* Our file buffer */
filestruct *edittop = NULL;	/* Pointer to the top of the edit
				   buffer with respect to the
				   file struct */
filestruct *editbot = NULL;	/* Same for the bottom */
filestruct *filebot = NULL;	/* Last node in the file struct */
filestruct *cutbuffer = NULL;	/* A place to store cut text */

#ifdef ENABLE_MULTIBUFFER
openfilestruct *open_files = NULL;	/* The list of open files */
#endif

#ifndef DISABLE_JUSTIFY
char *quotestr = "> ";		/* Quote string */
#endif

char *answer = NULL;			/* Answer str to many questions */
int totlines = 0;		/* Total number of lines in the file */
long totsize = 0;		/* Total number of bytes in the file */
int placewewant = 0;		/* The column we'd like the cursor
				   to jump to when we go to the
				   next or previous line */

int tabsize = 8;		/* Our internal tabsize variable */

char *hblank;			/* A horizontal blank line */
char *help_text;		/* The text in the help window */

/* More stuff for the marker select */

filestruct *mark_beginbuf;	/* the begin marker buffer */
int mark_beginx;		/* X value in the string to start */

#ifndef DISABLE_OPERATINGDIR
char *operating_dir = NULL;	/* Operating directory, which we can't */
char *full_operating_dir = NULL;/* go higher than */
#endif

#ifndef DISABLE_SPELLER
char *alt_speller = NULL;		/* Alternative spell command */
#endif

shortcut *main_list = NULL;
shortcut *whereis_list = NULL;
shortcut *replace_list = NULL;
shortcut *replace_list_2 = NULL; 	/* 2nd half of replace dialog */
shortcut *goto_list = NULL;
shortcut *gotodir_list = NULL;
shortcut *writefile_list = NULL;
shortcut *insertfile_list = NULL;
shortcut *help_list = NULL;
shortcut *spell_list = NULL;
#ifndef NANO_SMALL
shortcut *extcmd_list = NULL;
#endif
#ifndef DISABLE_BROWSER
shortcut *browser_list = NULL;
#endif

#ifdef ENABLE_COLOR
    colorstruct colors[NUM_NCOLORS];
    colortype *colorstrings = NULL;
    syntaxtype *syntaxes = NULL;
    char *syntaxstr = NULL;
#endif

#if !defined(DISABLE_BROWSER) || !defined(DISABLE_MOUSE) || !defined (DISABLE_HELP)
shortcut *currshortcut;			/* Current shortcut list we're using */
#endif

#ifndef NANO_SMALL
toggle *toggles = NULL;
#endif

/* Regular expressions */

#ifdef HAVE_REGEX_H
regex_t search_regexp;		/* Global to store compiled search regexp */
regmatch_t regmatches[10];	/* Match positions for parenthetical
				   subexpressions, max of 10 */
#ifdef ENABLE_COLOR
regex_t color_regexp;		/* Global to store compiled search regexp */
regmatch_t colormatches[1];	/* Match positions for parenthetical */

regex_t syntaxfile_regexp;	/* Global to store compiled search regexp */
regmatch_t synfilematches[1];	/* Match positions for parenthetical */
#endif /* ENABLE_COLOR */

#endif

int length_of_list(const shortcut *s) 
{
    int i = 0;

    for (; s != NULL; s = s->next)
	i++;

    return i;
}

/* Initialize a struct *without* our lovely braces =( */
static void sc_init_one(shortcut **shortcutage, int key,
		const char *desc,
#ifndef DISABLE_HELP
		const char *help,
#endif /* !DISABLE_HELP */
		int alt, int misc1, int misc2, int view,
		int (*func) (void))
{
    shortcut *s;

    if (*shortcutage == NULL) {
	*shortcutage = nmalloc(sizeof(shortcut));
	s = *shortcutage;
    } else {
	for (s = *shortcutage; s->next != NULL; s = s->next)
	    ;
	s->next = nmalloc(sizeof(shortcut));
	s = s->next; 
    }

    s->val = key;
    s->desc = desc;
#ifndef DISABLE_HELP
    s->help = help;
#endif /* !DISABLE_HELP */
    s->altval = alt;
    s->misc1 = misc1;
    s->misc2 = misc2;
    s->viewok = view;
    s->func = func;
    s->next = NULL;
}

#ifndef NANO_SMALL
/* Create one new toggle structure, at the end of the toggles
 * linked list. */
static void toggle_init_one(int val, const char *desc, int flag)
{
    toggle *u;

    if (toggles == NULL) {
	toggles = nmalloc(sizeof(toggle));
	u = toggles;
    } else {
	for (u = toggles; u->next != NULL; u = u->next)
	    ;
	u->next = nmalloc(sizeof(toggle));
	u = u->next;
    }

    u->val = val;
    u->desc = desc;
    u->flag = flag;
    u->next = NULL;
}

/* Deallocate all of the toggles */
static void free_toggles(void)
{
    toggle *pt;	/* Think "previous toggle" */

    while (toggles != NULL) {
	pt = toggles;
	toggles = toggles->next;
	free(pt);
    }
    toggles = NULL;
}

static void toggle_init(void)
{
    char *toggle_const_msg, *toggle_autoindent_msg, *toggle_suspend_msg,
	*toggle_nohelp_msg, *toggle_picomode_msg, *toggle_mouse_msg,
	*toggle_cuttoend_msg, *toggle_wrap_msg, *toggle_case_msg, 
	*toggle_backwards_msg, *toggle_noconvert_msg, *toggle_dos_msg,
	*toggle_mac_msg, *toggle_smooth_msg;

#ifdef HAVE_REGEX_H
    char *toggle_regexp_msg;
#endif

#ifdef ENABLE_MULTIBUFFER
    char *toggle_load_msg;
#endif

    toggle_const_msg = _("Constant cursor position");
    toggle_autoindent_msg = _("Auto indent");
    toggle_suspend_msg = _("Suspend");
    toggle_nohelp_msg = _("Help mode");
    toggle_picomode_msg = _("Pico mode");
    toggle_mouse_msg = _("Mouse support");
    toggle_cuttoend_msg = _("Cut to end");
    toggle_backwards_msg = _("Backwards search");
    toggle_case_msg = _("Case sensitive search");

#ifdef HAVE_REGEX_H
    toggle_regexp_msg = _("Regular expression search");
#endif

    toggle_noconvert_msg = _("No conversion from DOS/Mac format");
    toggle_dos_msg = _("Writing file in DOS format");
    toggle_mac_msg = _("Writing file in Mac format");
    toggle_smooth_msg = _("Smooth scrolling");
    toggle_wrap_msg = _("Auto wrap");

#ifdef ENABLE_MULTIBUFFER
    toggle_load_msg = _("Multiple file buffers");
#endif

    free_toggles();

    toggle_init_one(TOGGLE_CONST_KEY, toggle_const_msg, CONSTUPDATE);
    toggle_init_one(TOGGLE_AUTOINDENT_KEY, toggle_autoindent_msg, AUTOINDENT);
    toggle_init_one(TOGGLE_SUSPEND_KEY, toggle_suspend_msg, SUSPEND);
    toggle_init_one(TOGGLE_NOHELP_KEY, toggle_nohelp_msg, NO_HELP);
    toggle_init_one(TOGGLE_PICOMODE_KEY, toggle_picomode_msg, PICO_MODE);
    toggle_init_one(TOGGLE_WRAP_KEY, toggle_wrap_msg, NO_WRAP);
    toggle_init_one(TOGGLE_MOUSE_KEY, toggle_mouse_msg, USE_MOUSE);
    toggle_init_one(TOGGLE_CUTTOEND_KEY, toggle_cuttoend_msg, CUT_TO_END);
    toggle_init_one(TOGGLE_BACKWARDS_KEY, toggle_backwards_msg, REVERSE_SEARCH);
    toggle_init_one(TOGGLE_CASE_KEY, toggle_case_msg, CASE_SENSITIVE);
#ifdef HAVE_REGEX_H
    toggle_init_one(TOGGLE_REGEXP_KEY, toggle_regexp_msg, USE_REGEXP);
#endif
#ifdef ENABLE_MULTIBUFFER
    toggle_init_one(TOGGLE_LOAD_KEY, toggle_load_msg, MULTIBUFFER);
#endif
    toggle_init_one(TOGGLE_NOCONVERT_KEY, toggle_noconvert_msg, NO_CONVERT);
    toggle_init_one(TOGGLE_DOS_KEY, toggle_dos_msg, DOS_FILE);
    toggle_init_one(TOGGLE_MAC_KEY, toggle_mac_msg, MAC_FILE);
    toggle_init_one(TOGGLE_SMOOTH_KEY, toggle_smooth_msg, SMOOTHSCROLL);
}
#endif /* !NANO_SMALL */


/* Deallocate the given shortcut. */
static void free_shortcutage(shortcut **shortcutage)
{
    shortcut *s, *ps;

    assert(shortcutage != NULL);
    s = *shortcutage;
    while (s != NULL) {
	ps = s;
	s = s->next;
	free(ps);
    }
    *shortcutage = NULL;
}

void shortcut_init(int unjustify)
{
#ifndef DISABLE_HELP
    char *nano_help_msg = "", *nano_writeout_msg = "", *nano_exit_msg = "",
	*nano_goto_msg = "", *nano_justify_msg = "", *nano_replace_msg =
	"", *nano_insert_msg = "", *nano_whereis_msg =
	"", *nano_prevpage_msg = "", *nano_nextpage_msg =
	"", *nano_cut_msg = "", *nano_uncut_msg = "", *nano_cursorpos_msg =
	"", *nano_spell_msg = "", *nano_up_msg = "", *nano_down_msg =
	"", *nano_forward_msg = "", *nano_back_msg = "", *nano_home_msg =
	"", *nano_end_msg = "", *nano_firstline_msg =
	"", *nano_lastline_msg = "", *nano_refresh_msg =
	"", *nano_mark_msg = "", *nano_delete_msg =
	"", *nano_backspace_msg = "", *nano_tab_msg =
	"", *nano_enter_msg = "", *nano_cancel_msg = 
	"", *nano_unjustify_msg = "", *nano_append_msg =
	"", *nano_prepend_msg = ""; 

#ifdef ENABLE_MULTIBUFFER
    char *nano_openprev_msg = "", *nano_opennext_msg = "";
#endif

    char *nano_tofiles_msg = "", *nano_gotodir_msg = "", *nano_case_msg =
	"", *nano_reverse_msg = "", *nano_execute_msg = "";
    char *nano_dos_msg = "", *nano_mac_msg = ""; 

#ifdef HAVE_REGEX_H
    char *nano_regexp_msg = "", *nano_bracket_msg = "";
#endif

    nano_help_msg = _("Invoke the help menu");
    nano_writeout_msg = _("Write the current file to disk");

#ifdef ENABLE_MULTIBUFFER
    nano_exit_msg = _("Close currently loaded file/Exit from nano");
#else
    nano_exit_msg = _("Exit from nano");
#endif

    nano_goto_msg = _("Goto a specific line number");
    nano_justify_msg = _("Justify the current paragraph");
    nano_unjustify_msg = _("Unjustify after a justify");
    nano_replace_msg = _("Replace text within the editor");
    nano_insert_msg = _("Insert another file into the current one");
    nano_whereis_msg = _("Search for text within the editor");
    nano_prevpage_msg = _("Move to the previous screen");
    nano_nextpage_msg = _("Move to the next screen");
    nano_cut_msg = _("Cut the current line and store it in the cutbuffer");
    nano_uncut_msg = _("Uncut from the cutbuffer into the current line");
    nano_cursorpos_msg = _("Show the position of the cursor");
    nano_spell_msg = _("Invoke the spell checker (if available)");
    nano_up_msg = _("Move up one line");
    nano_down_msg = _("Move down one line");
    nano_forward_msg = _("Move forward one character");
    nano_back_msg = _("Move back one character");
    nano_home_msg = _("Move to the beginning of the current line");
    nano_end_msg = _("Move to the end of the current line");
    nano_firstline_msg = _("Go to the first line of the file");
    nano_lastline_msg = _("Go to the last line of the file");
    nano_refresh_msg = _("Refresh (redraw) the current screen");
    nano_mark_msg = _("Mark text at the current cursor location");
    nano_delete_msg = _("Delete the character under the cursor");
    nano_backspace_msg =
	_("Delete the character to the left of the cursor");
    nano_tab_msg = _("Insert a tab character");
    nano_enter_msg = _("Insert a carriage return at the cursor position");
    nano_case_msg =
	_("Make the current search or replace case (in)sensitive");
    nano_tofiles_msg = _("Go to file browser");
    nano_execute_msg = _("Execute external command");
    nano_gotodir_msg = _("Goto Directory");
    nano_cancel_msg = _("Cancel the current function");
    nano_append_msg = _("Append to the current file");
    nano_prepend_msg = _("Prepend to the current file");
    nano_reverse_msg = _("Search backwards");
    nano_dos_msg = _("Write file out in DOS format");
    nano_mac_msg = _("Write file out in Mac format");
#ifdef HAVE_REGEX_H
    nano_regexp_msg = _("Use Regular expressions");
    nano_bracket_msg = _("Find other bracket");
#endif
#ifdef ENABLE_MULTIBUFFER
    nano_openprev_msg = _("Open previously loaded file");
    nano_opennext_msg = _("Open next loaded file");
#endif
#endif /* !DISABLE_HELP */

    free_shortcutage(&main_list);

/* The following macro is to be used in calling sc_init_one.  The point is
 * that sc_init_one takes 9 arguments, unless DISABLE_HELP is defined,
 * when the fourth one should not be there. */
#ifdef DISABLE_HELP
#  define IFHELP(help, nextvar) nextvar
#else
#  define IFHELP(help, nextvar) help, nextvar
#endif

    sc_init_one(&main_list, NANO_HELP_KEY, _("Get Help"),
		IFHELP(nano_help_msg,) 0, NANO_HELP_FKEY, 0, VIEW,
		do_help);

#ifdef ENABLE_MULTIBUFFER
    if (open_files != NULL && (open_files->prev || open_files->next))
	sc_init_one(&main_list, NANO_EXIT_KEY, _("Close"),
		IFHELP(nano_exit_msg,) 0, NANO_EXIT_FKEY, 0, VIEW,
		do_exit);
    else
#endif
	sc_init_one(&main_list, NANO_EXIT_KEY, _("Exit"),
		IFHELP(nano_exit_msg,) 0, NANO_EXIT_FKEY, 0, VIEW,
		do_exit);

    sc_init_one(&main_list, NANO_WRITEOUT_KEY, _("WriteOut"),
		    IFHELP(nano_writeout_msg,)
		    0, NANO_WRITEOUT_FKEY, 0, NOVIEW, do_writeout_void);

    if (ISSET(PICO_MODE))
	sc_init_one(&main_list, NANO_JUSTIFY_KEY, _("Justify"),
		    IFHELP(nano_justify_msg,) 0, NANO_JUSTIFY_FKEY, 0,
		    NOVIEW, do_justify);
    else

#ifdef ENABLE_MULTIBUFFER
	/* this is so we can view multiple files */
	sc_init_one(&main_list, NANO_INSERTFILE_KEY, _("Read File"),
		IFHELP(nano_insert_msg,)
		0, NANO_INSERTFILE_FKEY, 0, VIEW, do_insertfile_void);
#else
	sc_init_one(&main_list, NANO_INSERTFILE_KEY, _("Read File"),
		IFHELP(nano_insert_msg,)
		0, NANO_INSERTFILE_FKEY, 0, NOVIEW, do_insertfile_void);
#endif

    if (ISSET(PICO_MODE))

#ifdef ENABLE_MULTIBUFFER
	/* this is so we can view multiple files */
	sc_init_one(&main_list, NANO_INSERTFILE_KEY, _("Read File"),
		IFHELP(nano_insert_msg,)
		0, NANO_INSERTFILE_FKEY, 0, VIEW, do_insertfile_void);
#else
	sc_init_one(&main_list, NANO_INSERTFILE_KEY, _("Read File"),
		IFHELP(nano_insert_msg,)
		0, NANO_INSERTFILE_FKEY, 0, NOVIEW, do_insertfile_void);
#endif

    else
	sc_init_one(&main_list, NANO_REPLACE_KEY, _("Replace"),
		    IFHELP(nano_replace_msg,)
		    NANO_ALT_REPLACE_KEY, NANO_REPLACE_FKEY, 0, NOVIEW, do_replace);

    sc_init_one(&main_list, NANO_WHEREIS_KEY, _("Where Is"),
		IFHELP(nano_whereis_msg,)
		0, NANO_WHEREIS_FKEY, 0, VIEW, do_search);

    sc_init_one(&main_list, NANO_PREVPAGE_KEY, _("Prev Page"),
		IFHELP(nano_prevpage_msg,)
		0, NANO_PREVPAGE_FKEY, KEY_PPAGE, VIEW, do_page_up);

    sc_init_one(&main_list, NANO_NEXTPAGE_KEY, _("Next Page"),
		IFHELP(nano_nextpage_msg,)
		0, NANO_NEXTPAGE_FKEY, KEY_NPAGE, VIEW, do_page_down);

    sc_init_one(&main_list, NANO_CUT_KEY, _("Cut Text"),
		IFHELP(nano_cut_msg,) 
		0, NANO_CUT_FKEY, 0, NOVIEW, do_cut_text);

    if (unjustify)
	sc_init_one(&main_list, NANO_UNJUSTIFY_KEY, _("UnJustify"),
		IFHELP(nano_unjustify_msg,) 
		0, 0, 0, NOVIEW, do_uncut_text);
    else
	sc_init_one(&main_list, NANO_UNCUT_KEY, _("UnCut Txt"),
		IFHELP(nano_uncut_msg,)
		0, NANO_UNCUT_FKEY, 0, NOVIEW, do_uncut_text);

    sc_init_one(&main_list, NANO_CURSORPOS_KEY, _("Cur Pos"),
		IFHELP(nano_cursorpos_msg,)
		0, NANO_CURSORPOS_FKEY, 0, VIEW, do_cursorpos_void);

    sc_init_one(&main_list, NANO_SPELL_KEY, _("To Spell"),
		IFHELP(nano_spell_msg,) 
		0, NANO_SPELL_FKEY, 0, NOVIEW, do_spell);


    sc_init_one(&main_list, NANO_UP_KEY, _("Up"),
		IFHELP(nano_up_msg,) 
		0, KEY_UP, 0, VIEW, do_up);

    sc_init_one(&main_list, NANO_DOWN_KEY, _("Down"),
		IFHELP(nano_down_msg,) 
		0, KEY_DOWN, 0, VIEW, do_down);

    sc_init_one(&main_list, NANO_FORWARD_KEY, _("Forward"),
		IFHELP(nano_forward_msg,) 
		0, KEY_RIGHT, 0, VIEW, do_right);

    sc_init_one(&main_list, NANO_BACK_KEY, _("Back"),
		IFHELP(nano_back_msg,) 
		0, KEY_LEFT, 0, VIEW, do_left);

    sc_init_one(&main_list, NANO_HOME_KEY, _("Home"),
		IFHELP(nano_home_msg,) 
		0, KEY_HOME, 362, VIEW, do_home);

    sc_init_one(&main_list, NANO_END_KEY, _("End"),
		IFHELP(nano_end_msg,) 
		0, KEY_END, 385, VIEW, do_end);

    sc_init_one(&main_list, NANO_REFRESH_KEY, _("Refresh"),
		IFHELP(nano_refresh_msg,) 
		0, 0, 0, VIEW, total_refresh);

    sc_init_one(&main_list, NANO_MARK_KEY, _("Mark Text"),
		IFHELP(nano_mark_msg,) 
		NANO_ALT_MARK_KEY, 0, 0, NOVIEW, do_mark);

    sc_init_one(&main_list, NANO_DELETE_KEY, _("Delete"),
		IFHELP(nano_delete_msg,) 0, KEY_DC,
		NANO_CONTROL_D, NOVIEW, do_delete);

    sc_init_one(&main_list, NANO_BACKSPACE_KEY, _("Backspace"),
		IFHELP(nano_backspace_msg,) 0,
		KEY_BACKSPACE, 127, NOVIEW, do_backspace);

    sc_init_one(&main_list, NANO_TAB_KEY, _("Tab"),
		IFHELP(nano_tab_msg,) 
		0, 0, 0, NOVIEW, do_tab);

    if (ISSET(PICO_MODE))
	sc_init_one(&main_list, NANO_REPLACE_KEY, _("Replace"),
		    IFHELP(nano_replace_msg,)
		    NANO_ALT_REPLACE_KEY, NANO_REPLACE_FKEY, 0, NOVIEW, do_replace);
    else
	sc_init_one(&main_list, NANO_JUSTIFY_KEY, _("Justify"),
		    IFHELP(nano_justify_msg,) 
		    0, NANO_JUSTIFY_FKEY, 0, NOVIEW, do_justify);

    sc_init_one(&main_list, NANO_ENTER_KEY, _("Enter"),
		IFHELP(nano_enter_msg,)
		0, KEY_ENTER, NANO_CONTROL_M, NOVIEW, do_enter_void);

    sc_init_one(&main_list, NANO_GOTO_KEY, _("Goto Line"),
		    IFHELP(nano_goto_msg,)
		    NANO_ALT_GOTO_KEY, NANO_GOTO_FKEY, 0, VIEW, do_gotoline_void);

#if (!defined NANO_SMALL) && (defined HAVE_REGEX_H)
    sc_init_one(&main_list, -9, _("Find Other Bracket"),
		    IFHELP(nano_bracket_msg,)
		    NANO_BRACKET_KEY, 0, 0, VIEW, do_find_bracket);
#endif

#ifdef ENABLE_MULTIBUFFER
    sc_init_one(&main_list, -9, _("Previous File"),
		    IFHELP(nano_openprev_msg,)
		    NANO_OPENPREV_KEY, 0, 0, VIEW, open_prevfile_void);
    sc_init_one(&main_list, -9, _("Next File"),
		    IFHELP(nano_opennext_msg,)
		    NANO_OPENNEXT_KEY, 0, 0, VIEW, open_nextfile_void);
#endif

#ifndef NANO_SMALL
    sc_init_one(&main_list, NANO_NEXTWORD_KEY, _("Next Word"),
		IFHELP(_("Move forward one word"),)
		0, 0, 0, VIEW, do_next_word_void);
    sc_init_one(&main_list, -9, _("Prev Word"),
		IFHELP(_("Move backward one word"),) NANO_PREVWORD_KEY, 0, 0,
		VIEW, do_prev_word_void);
#endif

    free_shortcutage(&whereis_list);

    sc_init_one(&whereis_list, NANO_HELP_KEY,
		_("Get Help"), IFHELP(nano_help_msg,) 
		0, 0, 0, VIEW, do_help);

    sc_init_one(&whereis_list, NANO_CANCEL_KEY,
		_("Cancel"), IFHELP(nano_cancel_msg,) 
		0, 0, 0, VIEW, 0);

    sc_init_one(&whereis_list, NANO_FIRSTLINE_KEY, _("First Line"),
		IFHELP(nano_firstline_msg,) 
		0, 0, 0, VIEW, do_first_line);

    sc_init_one(&whereis_list, NANO_LASTLINE_KEY, _("Last Line"),
		IFHELP(nano_lastline_msg,) 
		0, 0, 0, VIEW, do_last_line);

    sc_init_one(&whereis_list, NANO_OTHERSEARCH_KEY, _("Replace"),
		IFHELP(nano_replace_msg,) 
		0, 0, 0, VIEW, do_replace);

    sc_init_one(&whereis_list, NANO_FROMSEARCHTOGOTO_KEY, _("Goto Line"), 
		IFHELP(nano_goto_msg,) 
		0, 0, 0, VIEW, do_gotoline_void);

#ifndef NANO_SMALL
    sc_init_one(&whereis_list, TOGGLE_CASE_KEY, _("Case Sens"),
		IFHELP(nano_case_msg,)
		0, 0, 0, VIEW, 0);

    sc_init_one(&whereis_list, TOGGLE_BACKWARDS_KEY, _("Direction"),
		IFHELP(nano_reverse_msg,) 
		0, 0, 0, VIEW, 0);

#ifdef HAVE_REGEX_H
    sc_init_one(&whereis_list, TOGGLE_REGEXP_KEY,
		_("Regexp"), IFHELP(nano_regexp_msg,)
		0, 0, 0, VIEW, 0);
#endif
#endif /* !NANO_SMALL */

    free_shortcutage(&replace_list);

    sc_init_one(&replace_list, NANO_HELP_KEY,
		_("Get Help"), IFHELP(nano_help_msg,) 
		0, 0, 0, VIEW, do_help);

    sc_init_one(&replace_list, NANO_CANCEL_KEY,
		_("Cancel"), IFHELP(nano_cancel_msg,) 
		0, 0, 0, VIEW, 0);

    sc_init_one(&replace_list, NANO_FIRSTLINE_KEY, _("First Line"),
		IFHELP(nano_firstline_msg,) 
		0, 0, 0, VIEW, do_first_line);

    sc_init_one(&replace_list, NANO_LASTLINE_KEY, _("Last Line"),
		IFHELP(nano_lastline_msg,) 
		0, 0, 0, VIEW, do_last_line);

    sc_init_one(&replace_list, NANO_OTHERSEARCH_KEY, _("No Replace"),
		IFHELP(nano_whereis_msg,) 
		0, 0, 0, VIEW, do_search);

    sc_init_one(&replace_list, NANO_FROMSEARCHTOGOTO_KEY,
		_("Goto Line"),  IFHELP(nano_goto_msg,) 
		0, 0, 0, VIEW, do_gotoline_void);

#ifndef NANO_SMALL
    sc_init_one(&replace_list, TOGGLE_CASE_KEY, _("Case Sens"),
		IFHELP(nano_case_msg,) 
		0, 0, 0, VIEW, 0);

    sc_init_one(&replace_list, TOGGLE_BACKWARDS_KEY, _("Direction"),
		IFHELP(nano_reverse_msg,) 
		0, 0, 0, VIEW, 0);

#ifdef HAVE_REGEX_H
    sc_init_one(&replace_list, TOGGLE_REGEXP_KEY, 
		_("Regexp"), IFHELP(nano_regexp_msg,) 
		0, 0, 0, VIEW, 0);
#endif
#endif /* !NANO_SMALL */

    free_shortcutage(&replace_list_2);

    sc_init_one(&replace_list_2, NANO_HELP_KEY,
		_("Get Help"), IFHELP(nano_help_msg,) 
		0, 0, 0, VIEW, do_help);

    sc_init_one(&replace_list_2, NANO_CANCEL_KEY,
		_("Cancel"), IFHELP(nano_cancel_msg,) 
		0, 0, 0, VIEW, 0);

    sc_init_one(&replace_list_2, NANO_FIRSTLINE_KEY, _("First Line"),
		IFHELP(nano_firstline_msg,) 
		0, 0, 0, VIEW, do_first_line);

    sc_init_one(&replace_list_2, NANO_LASTLINE_KEY, _("Last Line"),
		IFHELP(nano_lastline_msg,) 
		0, 0, 0, VIEW, do_last_line);

    free_shortcutage(&goto_list);

    sc_init_one(&goto_list, NANO_HELP_KEY,
		_("Get Help"), IFHELP(nano_help_msg,) 
		0, 0, 0, VIEW, do_help);

    sc_init_one(&goto_list, NANO_CANCEL_KEY, _("Cancel"),
		IFHELP(nano_cancel_msg,) 
		0, 0, 0, VIEW, 0);

    sc_init_one(&goto_list, NANO_FIRSTLINE_KEY, _("First Line"),
		IFHELP(nano_firstline_msg,) 
		0, 0, 0, VIEW, &do_first_line);

    sc_init_one(&goto_list, NANO_LASTLINE_KEY, _("Last Line"),
		IFHELP(nano_lastline_msg,) 
		0, 0, 0, VIEW, &do_last_line);

    free_shortcutage(&help_list);

    sc_init_one(&help_list, NANO_PREVPAGE_KEY, _("Prev Page"),
		IFHELP(nano_prevpage_msg,)
		0, NANO_PREVPAGE_FKEY, KEY_PPAGE, VIEW, do_page_up);

    sc_init_one(&help_list, NANO_NEXTPAGE_KEY, _("Next Page"),
		IFHELP(nano_nextpage_msg,)
		0, NANO_NEXTPAGE_FKEY, KEY_NPAGE, VIEW, do_page_down);

    sc_init_one(&help_list, NANO_EXIT_KEY, _("Exit"),
		IFHELP(nano_exit_msg,)
		0, NANO_EXIT_FKEY, 0, VIEW, do_exit);

    free_shortcutage(&writefile_list);

    sc_init_one(&writefile_list, NANO_HELP_KEY,
		_("Get Help"), IFHELP(nano_help_msg,) 
		0, 0, 0, VIEW, do_help);

#ifndef DISABLE_BROWSER
    sc_init_one(&writefile_list, NANO_TOFILES_KEY, _("To Files"),
		IFHELP(nano_tofiles_msg,) 
		0, 0, 0, NOVIEW, 0);
#endif

#ifndef NANO_SMALL
    sc_init_one(&writefile_list, TOGGLE_DOS_KEY, 
		_("DOS Format"), IFHELP(nano_dos_msg,) 
		0, 0, 0, NOVIEW, 0);

    sc_init_one(&writefile_list, TOGGLE_MAC_KEY, 
		_("Mac Format"), IFHELP(nano_mac_msg,) 
		0, 0, 0, NOVIEW, 0);
#endif

    sc_init_one(&writefile_list, 
		NANO_APPEND_KEY, _("Append"),
		IFHELP(nano_append_msg,) 
		0, 0, 0, NOVIEW, 0);

    sc_init_one(&writefile_list, 
		NANO_PREPEND_KEY, _("Prepend"),
		IFHELP(nano_prepend_msg,) 
		0, 0, 0, NOVIEW, 0);

    sc_init_one(&writefile_list, NANO_CANCEL_KEY, 
		_("Cancel"), IFHELP(nano_cancel_msg,) 
		0, 0, 0, VIEW, 0);

    free_shortcutage(&insertfile_list);

    sc_init_one(&insertfile_list, NANO_HELP_KEY,
		_("Get Help"), IFHELP(nano_help_msg,) 
		0, 0, 0, VIEW, do_help);

    sc_init_one(&insertfile_list, NANO_CANCEL_KEY, _("Cancel"),
		IFHELP(nano_cancel_msg,) 
		0, 0, 0, VIEW, 0);

#ifndef DISABLE_BROWSER
    sc_init_one(&insertfile_list, NANO_TOFILES_KEY, _("To Files"),
		IFHELP(nano_tofiles_msg,) 
		0, 0, 0, NOVIEW, 0);
#endif
#ifndef NANO_SMALL
    sc_init_one(&insertfile_list, NANO_EXTCMD_KEY, _("Execute Command"),
		IFHELP(nano_execute_msg,) 
		0, 0, 0, NOVIEW, 0);
#endif

    free_shortcutage(&spell_list);

    sc_init_one(&spell_list, NANO_HELP_KEY,
		_("Get Help"), IFHELP(nano_help_msg,) 
		0, 0, 0, VIEW, do_help);

    sc_init_one(&spell_list, NANO_CANCEL_KEY, _("Cancel"),
		IFHELP(nano_cancel_msg,) 
		0, 0, 0, VIEW, 0);

#ifndef NANO_SMALL
    free_shortcutage(&extcmd_list);

    sc_init_one(&extcmd_list, NANO_HELP_KEY,
		_("Get Help"), IFHELP(nano_help_msg,) 
		0, 0, 0, VIEW, do_help);

    sc_init_one(&extcmd_list, NANO_CANCEL_KEY, _("Cancel"),
		IFHELP(nano_cancel_msg,) 
		0, 0, 0, VIEW, 0);
#endif

#ifndef DISABLE_BROWSER

    free_shortcutage(&browser_list);

    sc_init_one(&browser_list, NANO_HELP_KEY,
		_("Get Help"), IFHELP(nano_help_msg,) 
		0, 0, 0, VIEW, do_help);

    sc_init_one(&browser_list, NANO_EXIT_KEY, _("Exit"),
		IFHELP(nano_exit_msg,) 
		0, NANO_EXIT_FKEY, 0, VIEW, 0);

    sc_init_one(&browser_list, NANO_PREVPAGE_KEY, _("Prev Page"),
		IFHELP(nano_prevpage_msg,)
		0, NANO_PREVPAGE_FKEY, KEY_PPAGE, VIEW, 0);

    sc_init_one(&browser_list, NANO_NEXTPAGE_KEY, _("Next Page"),
		IFHELP(nano_nextpage_msg,)
		0, NANO_NEXTPAGE_FKEY, KEY_NPAGE, VIEW, 0);

    sc_init_one(&browser_list, NANO_GOTO_KEY, _("Goto"),
		IFHELP(nano_gotodir_msg,) 
		NANO_ALT_GOTO_KEY, NANO_GOTO_FKEY, 0, VIEW, 0);

    free_shortcutage(&gotodir_list);

    sc_init_one(&gotodir_list, NANO_HELP_KEY,
		_("Get Help"), IFHELP(nano_help_msg,) 
		0, 0, 0, VIEW, do_help);

    sc_init_one(&gotodir_list, NANO_CANCEL_KEY, _("Cancel"),
		IFHELP(nano_cancel_msg,) 
		0, 0, 0, VIEW, 0);

#endif


#if !defined(DISABLE_BROWSER) || !defined(DISABLE_MOUSE) || !defined (DISABLE_HELP)
    currshortcut = main_list;
#endif

#ifndef NANO_SMALL
    toggle_init();
#endif
}

/* This function is called just before calling exit().  Practically, the
 * only effect is to cause a segmentation fault if the various data
 * structures got bolloxed earlier.  Thus, we don't bother having this
 * function unless debugging is turned on.
 */
#ifdef DEBUG
/* added by SPK for memory cleanup, gracefully return our malloc()s */
void thanks_for_all_the_fish(void) 
{
#ifndef DISABLE_OPERATINGDIR
    if (operating_dir != NULL)
	free(operating_dir);
    if (full_operating_dir != NULL)
	free(full_operating_dir);
#endif
    if (last_search != NULL)
	free(last_search);
    if (last_replace != NULL)
	free(last_replace);
    if (hblank != NULL)
	free(hblank);
#ifndef DISABLE_SPELLER
    if (alt_speller != NULL)
	free(alt_speller);
#endif
    if (help_text != NULL)
	free(help_text);
    if (filename != NULL)
	free(filename);
    if (answer != NULL)
	free(answer);
    if (cutbuffer != NULL)
        free_filestruct(cutbuffer);

    free_shortcutage(&main_list);
    free_shortcutage(&whereis_list);
    free_shortcutage(&replace_list);
    free_shortcutage(&replace_list_2);
    free_shortcutage(&help_list);
    free_shortcutage(&writefile_list);
    free_shortcutage(&insertfile_list);
    free_shortcutage(&spell_list);
#ifndef DISABLE_BROWSER
    free_shortcutage(&browser_list);
#endif
    free_shortcutage(&gotodir_list);
    free_shortcutage(&goto_list);

#ifndef NANO_SMALL
    free_toggles();
#endif

#ifdef ENABLE_MULTIBUFFER
    if (open_files != NULL) {
	/* We free the memory associated with each open file. */
	openfilestruct *next;

	while (open_files->prev != NULL)
	    open_files = open_files->prev;
	do {
	    next = open_files->next;
	    free_openfilestruct(open_files);
	    open_files = next;
	} while (open_files != NULL);
    }
#else
    if (fileage != NULL)
	free_filestruct(fileage);
#endif
}
#endif /* DEBUG */
