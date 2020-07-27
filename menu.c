#include "menu.h"

struct menu_it *menu_proc(struct menu_it *m, enum menu_act i)
{
	struct menu_it *n;
	
	if (!m)
		return m;

	n = m->lrud[i];
	if (!n)
		return m;

	if (m->x & (1 << (MENU_DOWN - i))) {
		struct menu_it *(*f)(struct menu_it *m) = (void *)n;

		n = f(m);
	}

	if (n && n->render)
		n->render(n);

	return n ? n : m;
}
