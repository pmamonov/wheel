#ifndef __MENU_H__
#define __MENU_H__

struct menu_it;

enum menu_act {
	MENU_LEFT = 0,
	MENU_RIGHT,
	MENU_UP,
	MENU_DOWN,

	MENU_NUM_ACT,
};

struct menu_it {
	void (*render)(struct menu_it *m);
	unsigned x;
	void *lrud[MENU_NUM_ACT];
};

struct menu_it *menu_proc(struct menu_it *m, enum menu_act i);

#endif /* __MENU_H__ */
