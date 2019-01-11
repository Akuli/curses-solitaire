#include <curses.h>
#include <limits.h>
#include <stdbool.h>
#include "scroll.h"

#define BOTTOM_BAR_SIZE 1

struct ScrollState {
	WINDOW *win;
	WINDOW *pad;
	int firstlineno;   // 0 means first line
};

// makes sure that it's not scrolled too far up or down
static void bounds_check(struct ScrollState *st)
{
	int winh, padh, w;
	getmaxyx(st->win, winh, w);
	getmaxyx(st->pad, padh, w);
	padh -= BOTTOM_BAR_SIZE;
	(void) w;  // w is needed only because getmaxyx wants it, this suppresses warning

	// it's important that the negativeness check is last
	// this way firstlineno is set to 0 if padh < winh
	if (st->firstlineno > padh - winh)
		st->firstlineno = padh - winh;
	if (st->firstlineno < 0)
		st->firstlineno = 0;
}

static void draw_pad_to_window(struct ScrollState *st)
{
	bounds_check(st);

	int winw, winh, padw, padh;
	getmaxyx(st->win, winh, winw);
	getmaxyx(st->pad, padh, padw);
	winh -= BOTTOM_BAR_SIZE;

	wclear(st->win);  // werase() doesn't fill window with dark background in color mode

	// min stuff and -1 are needed because curses is awesome
	// if this code is wrong, it either segfaults or shows nothing in the help
#define min(a, b) ((a)<(b) ? (a) : (b))
	copywin(st->pad, st->win, st->firstlineno, 0, 0, 0, min(winh, padh)-1, min(winw, padw)-1, true);
#undef min

	attron(A_STANDOUT);
	mvwaddstr(st->win, winh, 0, "Move with ↑ and ↓, or press q to quit this help.");
	attroff(A_STANDOUT);

	wrefresh(st->win);
}

static bool handle_key(struct ScrollState *st, int k)
{
	int winw, winh;
	getmaxyx(st->win, winh, winw);
	winh -= BOTTOM_BAR_SIZE;
	(void) winw;

	switch(k) {
	case 'q':
		return false;

	case KEY_UP:
	case 'p':
		st->firstlineno--;
		break;

	case KEY_DOWN:
	case 'n':
		st->firstlineno++;
		break;

	case KEY_PPAGE:
		st->firstlineno -= winh;
		break;

	case KEY_NPAGE:
		st->firstlineno += winh;
		break;

	case KEY_HOME:
		st->firstlineno = 0;
		break;

	case KEY_END:
		st->firstlineno = INT_MAX;
		break;

	default:
		break;
	}

	return true;
}

// must wrefresh(win) after this, but not before
void scroll_showpad(WINDOW *win, WINDOW *pad)
{
	struct ScrollState st;
	st.win = win;
	st.pad = pad;
	st.firstlineno = 0;

	do {
		werase(win);
		draw_pad_to_window(&st);
		wrefresh(win);
	} while( handle_key(&st, getch()) );
}
