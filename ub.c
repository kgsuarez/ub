#include <ncurses.h> 
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include <time.h>

#include <string.h>

#define N_UB   1
#define N_DD  10 
#define N_MM 105

#define N_CO  2

#define S_CO 50

#define N_WP 100

#define N_BS 10

#define BLOCK 1000

#define FUEL 10000000
#define BATT 1000

#define Y    24
#define X    80
#define MY   22
#define MX   80
#define AMY  48
#define AMX 160

#define FPC 60
#define PRINT_W 1

int CONVOY = 1;
int TRANSIT = 0;

struct ub_data {
	int fire;
	int fire_n;

	int   tube[5];
	int tube_t[5];

	int  b_rel[3];
	int  s_rel[3];

	int b_ex;
	int s_ex;

	int peri;
	int z;
};

struct dd_data {
	int ang;
	int fire;
	int fire_t;
	int guess;
	int free;
	int asdic;
	int attack;
};

struct mm_data {
	int ton;
	int speed;
	char type;
};

struct ship { // CLEAN
	int y;
	int x;
	int ay;
	int ax;

	int yw;
	int xw;

	int dy;
	int dx;
	int ycount;

	int speed;
	int leg;

	int ty;
	int tx;
	int tay;
	int tax;

	int in_con;
	int con_n;
	int cty;
	int ctx;

	int base;
	int waypoint;

	int fire;
	int fire_t;

	int player;

	int spawn_t;

	char type;
};

struct weapon {
	int y;
	int x;
	int ay;
	int ax;

	int z;

	int dy;
	int dx;
	int ycount;

	int speed;
	int leg;
	int run;

	int act;

	int player;

	char type;
};


struct player_input {
	int dir;
	int speed;
	int z;

	int fire1;
	int fire2;

	int peri;

	int fire_n;

	int tube[5];

	int load;
};

struct player_data {
	int fuel;
	int batt;
	int decorations;
	int tonnage;
	int mm_hits;
	int dd_hits;
	int score;
	int completed_patrols;

	int ship_sight;

	char message[100];
};

struct misc {
	int  base_ay[N_BS];
	int  base_ax[N_BS];
	char base_id[N_BS];

	int way_ay[10];
	int way_ax[10];
};


struct ship      ub[N_UB];
struct ub_data ub_d[N_UB];

struct ship      dd[N_DD];
struct dd_data dd_d[N_DD];

struct ship      mm[N_MM];
struct mm_data mm_d[N_MM];

struct ship co[N_CO];

struct weapon w[N_WP];

struct player_input pi;
struct player_data pd;

struct misc m;

int ub_hit = 0;  // RC

int w_count = 0;

int ch, mode = 0;  // mx and my are RC

int AI_SPEED = 0; // RC

int view_y = 0;
int view_x = 0;
int view_ay = 0; // RC
int view_ax = 0; // RC

double vis = 1;


int gclock = 0;
int timer = 0; // RC
int delay = 0;

char *dir_c[] = {" N","NE"," E","SE"," S","SW"," W","NW"," C" };
int   dir_y[] = { -1 , -1 ,  0 ,  1 ,  1 ,  1 ,  0 , -1 ,  0  };
int   dir_x[] = {  0 ,  1 ,  1 ,  1 ,  0 , -1 , -1 , -1 ,  0  };

int atlas[AMY][AMX] = {{0}};

WINDOW *main_win;
WINDOW *stat_win;

void init_curses();
void kill_curses();
void proc_input();
void grab_input();

void init_s(struct ship *s, int n);

void init_all_s();

void print_ship(struct ship *s, int n);

void print_all_ships();

void advance_s(struct ship *s, int n);

void advance_all_s();

void print(int y, int x, char c);

void print_status();

void move_towards(int a, int b);


void wrap_y(int *y);
void wrap_x(int *x);

void print_atlas();


void gen_atlas();



void print_sector(int ay, int ax, char c);
void print_sectors();

void center_view();

int in_range(int ty, int tx, int y, int x, double v);


void ai_moves(struct ship *s, int n);

void print_fog();

void init_w(struct weapon *w, int n);
void init_all_w();
void fire_w();
void advance_w(struct weapon *w, int n);
void advance_all_w();
void print_w(struct weapon *w, int n);
void print_all_w();

void print_wake_stats();

void game_loop();


void update_clock();

void input_things();

void init_pi();
void init_pd();

void print_ub_info();

void init_mm_d(struct mm_data *mm_d, int n);
void init_ub_d(int n);

void init_co();

void gen_convoy();
void advance_convoy();

void print_bomb_info();

void init_m();

void respawn();


void temp_print(int y, int x, char c);

void ai_nav_wp(struct ship *s, int n);

void init_mm(int n);

void co_nav(struct ship *s, int n);


int merge_coords_y(int ay, int y);

int merge_coords_x(int ax, int x);

void watch();



/****************************************************************************/

int main()
{
	init_curses();
	init_pi();
        init_pd();
	init_m();
	gen_atlas();
	init_all_s();
	init_all_w();
	gen_convoy();

	ub[0].player = 1;

	while ((ch = getch()) && mode != 9) {
		if (TRANSIT == 1) {
			if (pd.ship_sight == 0) {
				game_loop();
			} else {
				TRANSIT = 0;
			}
		} else {
			usleep(1);
			game_loop();
		}
	}

	kill_curses();

	printf("Goodbye\n");

	return 0;
}

void watch()
{
	int i;

	int mty;
	int mtx;

	int my = merge_coords_y(ub[0].ay, ub[0].y);
	int mx = merge_coords_x(ub[0].ax, ub[0].x);

	pd.ship_sight = 0;

	for (i = 0; i < N_DD; i++) {
		mty = merge_coords_y(dd[i].ay, dd[i].y);
		mtx = merge_coords_x(dd[i].ax, dd[i].x);

		if (in_range(my, mx, mty, mtx, vis) == 1) {
			pd.ship_sight = 1;
		}
	}

	for (i = 0; i < N_MM; i++) {
		mty = merge_coords_y(mm[i].ay, mm[i].y);
		mtx = merge_coords_x(mm[i].ax, mm[i].x);

		if (in_range(my, mx, mty, mtx, vis) == 1) {
			pd.ship_sight = 1;
		}
	}

	/*
	for () { // uboats
	}
	*/


/*****/

}


void init_m()
{
	int i;
	for (i = 0; i < N_BS; i++) {
		m.base_ay[i] = 0;
		m.base_ax[i] = 0;
		m.base_id[i] = 0;
	}
}
	

void game_loop()
{
	watch();

	if (delay > 0) {
		//usleep(delay);
	}

	grab_input();
        proc_input();

	if (timer <= 0) {
		timer = FPC;
		gclock++;
	} else {
		timer--;
	}

	respawn();

	werase(main_win); /// THIS DOESN"T BELONG HERE!!! TESTING!!!

	advance_all_s();
	advance_all_w();

        //werase(main_win);

	if (   1     /* TRANSIT == 0 */) {
		if (mode == 0) {
			center_view();
			print_sectors();
			print_all_ships();
			print_all_w();
			
		}
		if (mode == 1) {
			view_y = ub[0].ay - (MY/2); //0;
			view_x = ub[0].ax - (MX/2); //0;
			print_atlas();
		}
		if (mode == 2) {
			print_ub_info();
		}
		if (mode == 3) {
			print_bomb_info();
		}
	}

	wrefresh(main_win);

	werase(stat_win);
	print_status();
	wrefresh(stat_win);
}

void respawn()
{
	int i;

	for (i = 0; i < N_MM; i++) {
		if (mm[i].type == 'X' && mm[i].spawn_t < gclock) {
			init_s(mm, i);
			init_mm_d(mm_d, i);
			init_mm(i);
		}
	}
}

void print_bomb_info()
{
	int i, j;
	for (i = 0; i < MY; i++) {
		for (j = 0; j < MX; j+=3) {
			//mvwprintw(main_win, i, j, "%d", w[i].z);
		}
	}

	for (i = 0; i < 20; i++) {
		mvwprintw(main_win, i, 40, "|");
	}

	char *mess = "To U-333." "\n" "This is a test message" "\n" "END";
	char mess2[100];
	char c;
	int x = 0;

	mvwprintw(main_win, 0, 0, "%s", mess);

	while ( mess[x] != '\0'      ) {
		c = mess[x];
		if (c != '\n') {
			c -= pi.fire_n;
		}
		mess2[x] = c;
		x++;
	}
	mess2[x] = '\0';

	mvwprintw(main_win, 5, 0, "%s", mess2);

	for (i = 0; i < N_BS; i++) {
		mvwprintw(main_win, 10+i, 0, "Base #%d: %d, %d - %c", i, m.base_ay[i], m.base_ax[i], m.base_id[i]);
	}

	
	for (i = 0; i < 10; i++) {
		mvwprintw(main_win, 10+i, 45, "WP #%d, %d, %d", i, m.way_ay[i], m.way_ax[i] );
	}


	mvwprintw(main_win, 22, 0, "Tonnage: %d", pd.tonnage);
	mvwprintw(main_win, 23, 0, "MM hits: %d", pd.mm_hits);
	mvwprintw(main_win, 24, 0, "DD hits: %d", pd.dd_hits);



}

void fire_w(struct ship *s, int n)
{
	/*** should run init_w to reset ***/

	int  trig  = 0;
	int   run  = 0;
	int speed  = 0;
	char type  = 0;
	int player = 0;

	int  y = s[n].y + s[n].dy;
	int  x = s[n].x + s[n].dx;
	int dy = s[n].dy;
	int dx = s[n].dx;

	if (s[n].player == 1) {
		player = 1;
	}


	if (s[n].fire == 1) {
		s[n].fire = 0;

		if (s[n].fire_t < gclock) {
			s[n].fire_t = gclock + 0;
			run = (rand() % 20) + 21;
			speed = 500; //BLOCK;
			type = '*';
			trig = 1;
		}
	}

	if (s[n].type == '@' && ub_d[n].fire == 1) {
		ub_d[n].fire = 0;

		if (ub_d[n].tube[ub_d[n].fire_n] > 0) {
			ub_d[n].tube[ub_d[n].fire_n] = 0;

			if (ub_d[n].fire_n == 4) {	
				y   = s[n].y - s[n].dy;
				x   = s[n].x - s[n].dx;
				dy *= -1;
				dx *= -1;
			}

			run   = 37;
			speed = 30;
			type  = 'T';
			trig  = 1;
		}
	}

	if (s[n].type == 'D' && dd_d[n].fire == 1) {
		dd_d[n].fire = 0;

		if (dd_d[n].fire_t < gclock) {
			dd_d[n].fire_t = gclock + 1;
			run = dd_d[n].guess; //30; // dd's guess run equals target depth minus 2
			speed = 50; //125;
			y = s[n].y - s[n].dy;
			x = s[n].x - s[n].dx;
			dy = 0;
			dx = 0;
			type = 'B';
			trig = 1;
		}
	}

	if (trig == 1) {
		if (w_count >= N_WP) {
			w_count = 0;
		}

		
		w[w_count].y      = y;         //s[n].y + s[n].dy;
		w[w_count].x      = x;         //s[n].x + s[n].dx;
		w[w_count].ay     = s[n].ay;
		w[w_count].ax     = s[n].ax;
		w[w_count].dy     = dy;        //s[n].dy;
		w[w_count].dx     = dx;        //s[n].dx;
		w[w_count].act    = 1;
		w[w_count].run    = run;
		w[w_count].speed  = speed;
		w[w_count].type   = type;
		w[w_count].z      = 0;
		w[w_count].player = player;

		w_count++;
	}
}

void print_w(struct weapon *w, int n)
{

	int i, j;

	int   y,   x;
	int  by,  bx;

	char c = w[n].type;


        y = w[n].y + (w[n].ay * MY);
        x = w[n].x + (w[n].ax * MX);

	/*
	if (w[n].type == 'B') {
		int z = w[n].z + 50;
		print(y, x, z);
	} else {

		print(y, x, c);
	}
	*/

	print(y, x, c);

	if (w[n].type == '#') {

		mvwprintw(main_win, 5, 5, "%d", w[n].z);

		for (i = 0; i < 9; i++) {
			for (j = 0; j < 9; j++) {
				by  = y;
				bx  = x;
				by += dir_y[i];
				bx += dir_x[i];
				print(by, bx, w[n].type);
			}
		}
	}
}

void print_all_w()
{
	int i;
	for (i = 0; i < N_WP; i++) {
		if (w[i].act == 1) {
			print_w(w, i);
		}
	}
}

void advance_all_w()
{
	int i;
	for (i = 0; i < N_WP; i++) {
		if (w[i].act == 1) {
			advance_w(w, i);
		}
	}
}

int bounds_check_y(int y)
{
	int r = 0;

	if (y < 0) {
		r = -1;
	}
	if (y >= MY) {
		r = 1;
	}
	
	return r;
}

int bounds_check_x(int x)
{
	int r = 0;

	if (x < 0) {
		r = -1;
        }
        if (x >= MX) {
		r = 1;
        }

	return r;

}

void alert_dd_torp(struct weapon *w, int n)
{
	int i;

int mty = merge_coords_y(w[n].ay, w[n].y);
int mtx = merge_coords_x(w[n].ax, w[n].x);

int my, mx;

	for (i = 0; i < N_DD; i++) {
		my = merge_coords_y(dd[i].ay, dd[i].y);
		mx = merge_coords_x(dd[i].ax, dd[i].x);

		if (in_range(my, mx, mty, mtx, 1)) {
			dd[i].tay    = w[n].ay;
			dd[i].tax    = w[n].ax;
			dd[i].ty     = w[n].y;
			dd[i].tx     = w[n].x;
			dd_d[i].free = 0;
			dd[i].speed  = 10;
		}
	}
}


void w_col_det(struct weapon *w, struct ship *s, int wn, int sn)
{
	int i;

	for (i = 0; i < sn; i++) {
		if (w[wn].ay == s[i].ay && w[wn].ax == s[i].ax) {
			if (w[wn].y == s[i].y && w[wn].x == s[i].x) {
				//s[i].type = 'X';
				w[wn].run = 2;
				w[wn].leg = BLOCK;
				if (w[wn].player == 1 && s[i].type != 'X') {
					alert_dd_torp(w, wn);
					s[i].spawn_t = gclock + 60;
					if (s[i].type == 'M') {
						pd.tonnage += mm_d[i].ton;
						pd.mm_hits++;
					}
				}
				s[i].type = 'X';
			}
		}
	}
}

void advance_w(struct weapon *w, int n)
{
	w[n].leg += w[n].speed;

	if (w[n].leg >= BLOCK) {
		w[n].leg = 0;

		w[n].run--;
		if (w[n].run == 1) {
			w[n].type = '#';
		}

		if (w[n].run <= 0) {
			w[n].act = 0;
		}

		if (w[n].ycount <= 0) {
			w[n].ycount  = 3;
			w[n].y      += w[n].dy;
		} else {
			w[n].ycount--;
		}

		w[n].x += w[n].dx;

		if (w[n].type == 'B') {
			w[n].z++;
		}

		w[n].ay += bounds_check_y(w[n].y);
		w[n].ax += bounds_check_x(w[n].x);

		wrap_y(&w[n].y);
		wrap_x(&w[n].x);



	}

	if (w[n].type == '*') {
		//w_col_det(w, dd, n, N_DD);
		//w_col_det(w, ub, n, N_UB);
		;
	}

	if (w[n].type == 'B') { // not just collision
		;
		//w_col_det(w, ub, n, N_UB);
	}

	if (w[n].type == 'T') {
		;
		//w_col_det(w, dd, n, N_DD);
		w_col_det(w, mm, n, N_MM);
	}
}

void init_all_w()
{
	int i;
	for (i = 0; i < N_WP; i++) {
		init_w(w, i);
	}
}

void init_w(struct weapon *w, int n)
{
	w[n].y      = 0;
	w[n].x      = 0;
	w[n].ay     = 0;
	w[n].ax     = 0;
	w[n].act    = 0;
	w[n].dy     = 0;
	w[n].dx     = 0;
	w[n].run    = 0;
	w[n].speed  = 0;
	w[n].leg    = 0;
	w[n].ycount = 0;
	w[n].z      = 0;
	w[n].player = 0;
}

/***************************************************/

void print_fog()
{
	int i, j;

	for (i = 0; i < MY; i++) {
		for (j = 0; j < MX; j++) {
			//wattron(main_win, COLOR_PAIR(2));
			//wattron(main_win, A_BOLD);
			//mvwaddch(main_win, i, j, ACS_CKBOARD);
			//wattroff(main_win, COLOR_PAIR(2));
			//wattroff(main_win, A_BOLD);
		}
	}
}

int in_range(int ty, int tx, int y, int x, double v)
{
	int r = 0;

	int vy = ((MY * v) / 2) + 1;
	int vx = ((MX * v) / 2) + 1;

	if ( (ty - vy) < y && (ty + vy) > y  ) {
		if ( (tx - vx) < x && (tx + vx) > x  ) {
			r = 1;
		}
	}

	return r;
}


void center_view()
{
	view_y = (((ub[0].ay * MY) + ub[0].y) - MY/2) + 0;
	view_x = (((ub[0].ax * MX) + ub[0].x) - MX/2) + 0;

	//view_y = (((co.ay * MY) + co.y) - MY/2) + 0;
	//view_x = (((co.ax * MX) + co.x) - MX/2) + 0;

	//view_y = (((mm[0].ay * MY) + mm[0].y) - MY/2) + 0;
	//view_x = (((mm[0].ax * MX) + mm[0].x) - MX/2) + 0;
}

void print_sectors()
{
	int i;
	int ay = 0;
	int ax = 0;
	char c = ' ';

	for (i = 0; i < 9; i++) {
		ay = (view_y / MY);
		ax = (view_x / MX);

		ay += dir_y[i]; 
		ax += dir_x[i];

		if (atlas[ay][ax] == 1) {
                        c = '.';
                }
                if (atlas[ay][ax] == 0) {
                        c = ' ';
                }
		if (atlas[ay][ax] == 2) {
			c = '_';
		}

		print_sector(ay, ax, c);
	}
}


void print_sector(int ay, int ax, char c)
{
	/*** give it sector coords ***/

	int i, j;

	//char c = '.';

	ay = ay * MY;
	ax = ax * MX;
	int y, x;

	double v = vis;


	for (i = 0; i < MY; i++) {
		for (j = 0; j < MX; j++) {
			//wattron(main_win, COLOR_PAIR(1));
			//wattron(main_win, A_BOLD);
			//print(i + ay + off_y, j + ax + off_x, c);
			y = i + ay;
			x = j + ax;

			if (ub_d[0].z > 0 && ub_d[0].peri == 0) {
				v = 0;
			}

			if (in_range(y, x, (view_y + (MY/2)), (view_x + (MX/2)), v) != 0) {
				//if (c != ' ') {
					print(i + ay, j + ax, c);
				//}
				if (i % 2 == 0 && j % 2 == 0) {
					;
					//print(i + ay, j + ax, '`');
				}
                        }

			//wattroff(main_win, COLOR_PAIR(1));
			//wattroff(main_win, A_BOLD);
		}
	}
}

void gen_atlas()
{
	int c;

	int y = 0;
	int x = 0;

	int b_i = 0;
	int w_i = 0;

	FILE *fp = fopen("map.txt", "r");

	while ((c = getc(fp)) != EOF) {
		if (c == 'x') {
			atlas[y][x] = 1;
			x++;
		}
		if (c == ' ') {
			atlas[y][x] = 0;
			x++;
		}
		if (c == 'A' || c == 'I' || c == 'E') {
			atlas[y][x] = 2;
			m.base_ay[b_i] = y;
			m.base_ax[b_i] = x;
			m.base_id[b_i] = c;
			b_i++;
			x++;
		}
		if (isdigit(c)) {
			w_i = c - '0';
			m.way_ay[w_i] = y;
			m.way_ax[w_i] = x;
			x++;
		}
		if (c == '\n') {
			y++;
			x = 0;
		}
	}

	fclose(fp);
}

void print_atlas()
{
	int i, j;
	for (i = 0; i < AMY; i++) {
		for (j = 0; j < AMX; j++) {
			if (atlas[i][j] == 0) {
				//wattron(main_win, COLOR_PAIR(3));
				//wattron(main_win, A_BOLD);
				//print(i, j, 'ACS_PLUS');

				//mvwaddch(main_win, i , j, ACS_PLUS);
				//mvwaddch(main_win, i , j, ' ');
				print(i, j, ' ');

				//wattroff(main_win, COLOR_PAIR(3));
				//wattroff(main_win, A_BOLD);
			}
			if (atlas[i][j] == 1) {
				//print(i, j, 'x');

				//mvwaddch(main_win, i , j, ACS_CKBOARD);
				print(i, j, 'x');
			}
			if (atlas[i][j] == 2) {
				print(i, j, 'B');
			}
		}
	}

	print(ub[0].ay, ub[0].ax, '@');

	for (i = 0; i < N_DD; i++) {
		print(dd[i].ay, dd[i].ax, 'D');
	}

	for (i = 0; i < N_MM; i++) {
		print(mm[i].ay, mm[i].ax, 'M');
	}

	for (i = 0; i < 10; i++) {
		print(m.way_ay[i], m.way_ax[i], 'W');
	}
}

void print_all_ships()
{
	int i;

	for (i = 0; i < N_UB; i++) {
		print_ship(ub, i);
	}

	for (i = 0; i < N_DD; i++) {
		print_ship(dd, i);
	}

	for (i = 0; i < N_MM; i++) {
		print_ship(mm, i);
	}

}

void init_dd_d(struct dd_data *dd_d, int n)
{
	dd_d[n].ang     = 0;
	dd_d[n].fire    = 0;
	dd_d[n].fire_t  = 0;
	dd_d[n].guess   = 0;
	dd_d[n].free    = 0;
	dd_d[n].asdic   = 0;
	dd_d[n].attack  = 0;
}

void init_ub(int n)
{
	int i;

	ub[n].y        = 0;
	ub[n].x        = 79;
	ub[n].ay       = 26;
	ub[n].ax       = 133;

	ub[n].ty       = 0;
	ub[n].tx       = 0;
	ub[n].tay      = 0;
	ub[n].tax      = 0;

	ub[n].base     = 0;
	ub[n].waypoint = 0;

	ub[n].type     = '@';

	for (i = 0; i < 5; i++) {
		ub_d[n].tube[i] = 2;
	}

	ub_d[n].b_rel[1] = 1;
	ub_d[n].b_rel[2] = 7;

	ub_d[n].s_rel[1] = 1;
	ub_d[n].s_rel[2] = 1;

	ub_d[n].b_ex = 0;
	ub_d[n].s_ex = 0;
}

void init_pos_s(struct ship *s, int n)
{

	s[n].base = rand() % 5;

	if (m.base_id[s[n].base] == 'A') {
		s[n].waypoint = 0;
	}
	if (m.base_id[s[n].base] == 'I') {
		s[n].waypoint = 4;
	}
	if (m.base_id[s[n].base] == 'E') {
		s[n].waypoint = 5;
	}

	s[n].y  = rand() % MY;
	s[n].x  = rand() % MX;
	s[n].ay = m.base_ay[s[n].base];
	s[n].ax = m.base_ax[s[n].base];

s[n].ty       = 0;
s[n].tx       = 0;





	s[n].tay      = m.way_ay[s[n].waypoint];
	s[n].tax      = m.way_ax[s[n].waypoint];





s[n].in_con   = 0;
s[n].cty      = 0;
s[n].ctx      = 0;


/*****
s[n].tay = m.way_ay[s[n].waypoint] + (dir_y[rand() % 9] * (rand() % 3));

s[n].tax = m.way_ax[s[n].waypoint] + (dir_x[rand() % 9] * (rand() % 3));
*****/











}

void init_dd(int n)
{
/******************************
	dd[n].y        = 0;
	dd[n].x        = 0;
	dd[n].ay       = 0;
	dd[n].ax       = 0;

	dd[n].ty       = 0;
	dd[n].tx       = 0;
	dd[n].tay      = 0;
	dd[n].tax      = 0;

	dd[n].in_con   = 0;
	dd[n].cty      = 0;
	dd[n].ctx      = 0;

	dd[n].base     = rand() % 5;
	dd[n].waypoint = [dd[n].base];

	dd[n].spawn_t  = 0;

*******************************/

	init_pos_s(dd, n);

	dd[n].type = 'D';

	dd_d[n].free  = 1;
	dd_d[n].asdic = rand() % 2;
}

void init_mm(int n)
{
/********************************************************
mm[n].y        = 0;
mm[n].x        = 0;
mm[n].ay       = 0;
mm[n].ax       = 0;

mm[n].ty       = 0;
mm[n].tx       = 0;
mm[n].tay      = 0;
mm[n].tax      = 0;

mm[n].in_con   = 0;
mm[n].cty      = 0;
mm[n].ctx      = 0;

mm[n].base     = 0;
mm[n].waypoint = 0;



mm[n].spawn_t  = 0;
*******************************************************/
	init_pos_s(mm, n);

	mm[n].type = 'M';

	mm_d[n].ton = (rand() % 20000) + 5000;
	mm_d[n].speed = (rand() % 15) + 5;

	if (mm_d[n].ton >= 10000) {
		mm_d[n].type = 'M';
	} else {
		mm_d[n].type = 'm';
	}
}

void init_co(int n)
{
	co[n].type = 'C';
	init_pos_s(co, n);
}

void init_all_s()
{
	int i;
	for (i = 0; i < N_UB; i++) {
		init_s(ub, i);
		init_ub_d(i);
		init_ub(i);
	}

	for (i = 0; i < N_DD; i++) {
		init_s(dd, i);
		init_dd_d(dd_d, i);
		init_dd(i);
	}

	for (i = 0; i < N_MM; i++) {
		init_s(mm, i);
		init_mm_d(mm_d, i);
		init_mm(i);
	}

	for (i = 0; i < N_CO; i++) {
		init_s(co, i);
		init_co(i);
	}

}

void advance_all_s()
{
	int i;
	for (i = 0; i < N_UB; i++) {
		advance_s(ub, i);
	}

	for (i = 0; i < N_DD; i++) {
		advance_s(dd, i);
	}

	for (i = 0; i < N_MM; i++) {
		advance_s(mm, i);
	}

	for (i = 0; i < N_CO; i++) {
		advance_s(co, i);
	}
}

void wrap_y(int *y)
{
	if (*y < 0) {
		*y = MY-1;
	}
	if (*y >= MY) {
		*y = 0;
	}
}

void wrap_x(int *x)
{
	if (*x < 0) {
		*x = MX-1;
	}
	if (*x >= MX) {
		*x = 0;
	}
}

int merge_coords_y(int ay, int y)
{
	int r;
	r = (ay * MY) + y;
	return r;
}

int merge_coords_x(int ax, int x)
{
	int r;
	r = (ax * MX) + x;
	return r;
}

void dir_ai(struct ship *s, int n)
{
	if (s[n].tay > s[n].ay) {
		s[n].dy = 1;
	}
	if (s[n].tay < s[n].ay) {
		s[n].dy = -1;
	}
	if (s[n].tay == s[n].ay) {
		s[n].dy = 0;
	}
	if (s[n].tax > s[n].ax) {
		s[n].dx = 1;
	}
	if (s[n].tax < s[n].ax) {
		s[n].dx = -1;
	}
	if (s[n].tax == s[n].ax) {
		s[n].dx = 0;
	}

	if (s[n].tay == s[n].ay && s[n].tax == s[n].ax) {
		if (s[n].ty > s[n].y) {
			s[n].dy = 1; //++;
		}
		if (s[n].ty < s[n].y) {
			s[n].dy = -1; //--;
		}
		if (s[n].ty == s[n].y) {
			s[n].dy = 0;
		}
		if (s[n].tx > s[n].x) {
			s[n].dx = 1; //++;
		}
		if (s[n].tx < s[n].x) {
			s[n].dx = -1; //--;
		}
		if (s[n].tx == s[n].x) {
			s[n].dx = 0;
		}
	}
}

int on_target(struct ship *s, int n)
{
	int r = 0;
	if (s[n].tay == s[n].ay && s[n].tax == s[n].ax) {
		if (s[n].ty == s[n].y && s[n].tx == s[n].x) {
			r = 1;
		}
	}
	return r;
}

void dd_ai_surf(struct ship *s, struct ship *u, int dn, int un)
{
	int mty = merge_coords_y(ub[un].ay, ub[un].y);
	int mtx = merge_coords_x(ub[un].ax, ub[un].x);

	int my = merge_coords_y(s[dn].ay, s[dn].y);
	int mx = merge_coords_x(s[dn].ax, s[dn].x);

	double v = vis * 0.75; // 0.50 at night

	if (ub_d[un].z > 0 && ub_d[un].peri == 1) {
		v = vis * 0.50; // use ub speed also
	}

	if (in_range(mty, mtx, my, mx, v) == 1) {
		dd_d[dn].ang  = 500;
		dd_d[dn].free = 0;
		s[dn].speed   = 35;
		s[dn].tay     = ub[un].ay;
		s[dn].tax     = ub[un].ax;
		s[dn].ty      = ub[un].y;
		s[dn].tx      = ub[un].x;
		s[dn].fire    = 1;
	}

	if (in_range(mty, mtx, my, mx, v) == 0 && dd_d[dn].ang != 0) {
		if ( /* dd_d[dn].ang > 0 && */ dd_d[dn].free == 1   /* && on_target(s, dn) != 0 */) {
			s[dn].speed   = 35;

			s[dn].ty      += dir_y[rand() % 9] * (rand() % 5);
			s[dn].tx      += dir_x[rand() % 9] * (rand() % 5);
			s[dn].tay     += bounds_check_y(s[dn].ty);
			s[dn].tax     += bounds_check_x(s[dn].tx);

			wrap_y(&s[dn].ty);
			wrap_x(&s[dn].tx);

			dd_d[dn].free = 0;
		}
	}
}

void dd_ai_sub(struct ship *s, struct ship *u, int dn, int un)
{
	int mty = merge_coords_y(ub[un].ay, ub[un].y);
	int mtx = merge_coords_x(ub[un].ax, ub[un].x);

	 int my = merge_coords_y(s[dn].ay, s[dn].y);
	 int mx = merge_coords_x(s[dn].ax, s[dn].x);

	double v = ((double)ub[un].speed * 0.1);

	if (in_range(mty, mtx, my, mx, v) == 1 && dd_d[dn].free == 1 &&
	    dd_d[dn].ang != 0) {
		dd_d[dn].ang = 500;
		s[dn].speed  = 25;
		s[dn].tay    = ub[un].ay;
		s[dn].tax    = ub[un].ax;
		s[dn].ty     = ub[un].y;
		s[dn].tx     = ub[un].x;

		if (s[dn].y == ub[un].y && s[dn].x == ub[un].x) {

			dd_d[dn].guess++;
			int len = rand() % 10;

			s[dn].tay = ub[un].ay;
			s[dn].tax = ub[un].ax;

			s[dn].ty = ub[un].y + (ub[un].dy * len);
			s[dn].tx = ub[un].x + (ub[un].dx * (len*3));
			
			s[dn].tay += bounds_check_y(s[dn].ty);
			s[dn].tax += bounds_check_x(s[dn].tx);

			wrap_y(&s[dn].ty);
			wrap_x(&s[dn].tx);

			dd_d[dn].attack = 1;
			//dd_d[dn].free = 0;

		}
	}

	if (in_range(mty, mtx, my, mx, v) == 0 && dd_d[dn].ang != 0  /* && dd_d[dn].free == 1*/ ) {

		if (/* dd_d[dn].ang != 0 && dd_d[dn].ang <= 80 */ 1) {
			s[dn].speed = 15;

			if (dd_d[dn].free == 1) {
				s[dn].ty  += dir_y[rand() % 9];
				s[dn].tx  += dir_x[rand() % 9];
				s[dn].tay += bounds_check_y(s[dn].ty);
				s[dn].tax += bounds_check_x(s[dn].tx);
				wrap_y(&s[dn].ty);
				wrap_x(&s[dn].tx);
			}

			int p;
			int pmy;
			int pmx;

			int py  = s[dn].y;
			int px  = s[dn].x; 
			int pdy = s[dn].dy;
			int pdx = s[dn].dx;

			if (dd_d[dn].asdic == 1) {
				for (p = 0; p < 20; p++) {
					py += pdy;
					px += pdx;
					pmy = merge_coords_y(s[dn].ay, py);
					pmx = merge_coords_x(s[dn].ax, px);
					print(pmy, pmx, '.');

					if (py == ub[un].y && px == ub[un].x) {
						dd_d[dn].ang     = 500;
						s[dn].tay        = ub[un].ay;
						s[dn].tax        = ub[un].ax;
						s[dn].ty         = ub[un].y;
						s[dn].tx         = ub[un].x;
						s[dn].speed      = 35;
						dd_d[dn].free    = 0;
					}

					
				}
			}

				


		}
	}
}

void dd_ai(struct ship *s, int n)
{
	int i;

	if (on_target(s, n) == 1) {
		dd_d[n].free   = 1;
		dd_d[n].attack = 0;
	}
	if (dd_d[n].attack == 1) {
		dd_d[n].free = 0;
		dd_d[n].fire = 1;
		s[n].speed   = 35;
	}

	for (i = 0; i < N_UB; i++) {

		if (ub_d[i].z == 0 || ub_d[i].peri == 1) {
			dd_ai_surf(s, ub, n, i);
		}

		if (ub_d[i].z > 0) {
			dd_ai_sub(s, ub, n, i);
		}
	}

	if (dd_d[n].ang == 0  && dd_d[n].free == 1) {

		dd_d[n].free  = 0;
		dd_d[n].guess = 9;
		s[n].speed    = 10;

		/*****
		s[n].ty  = rand() % MY;
		s[n].tx  = rand() % MX;
		s[n].tay = rand() % AMY;
		s[n].tax = rand() % AMX;
		*****/

		ai_nav_wp(s, n);

		if (s[n].in_con == 1) {
			s[n].speed = 35;
			co_nav(s, n);
		}
	}
}

void reduce(int *n)
{
	*n -= 1;
	if (*n < 0) {
		*n = 0;
	}
}

void advance_agnostic(struct ship *s, int n)
{
	if (s[n].fire == 1) {
		fire_w(s, n);
	}

	if (s[n].type == '@') {
		if (s[n].player == 1 && ub_d[n].z > 0) {
			pd.batt -= s[n].speed;
			if (pd.batt <= 0) {
				pd.batt    = 0;
				s[n].speed = 0;
				pi.speed   = 0;
			}
		}
		if (s[n].player == 1 && ub_d[n].z == 0) {
			pd.fuel = (pd.fuel - (2 * s[n].speed));
			if (pd.fuel > 0) {
				pd.batt += s[n].speed;
			}
			if (pd.fuel <= 0) {
				pd.fuel    = 0;
				s[n].speed = 0;
				pi.speed   = 0;
			}
			if (pd.batt >= BATT) {
				pd.batt = BATT;
			}
		}
		if (ub_d[n].fire == 1) {
			fire_w(s, n);
		}
	}

	if (s[n].type == 'D') {
		if (dd_d[n].fire == 1) {
			fire_w(s, n);
		}
	}

	s[n].leg += s[n].speed;

	if (s[n].leg >= BLOCK) {
		s[n].leg = 0;


		if (s[n].ycount <= 0) {
			s[n].ycount = 3;
			s[n].yw  = s[n].y;
			s[n].y  += s[n].dy;
		} else {
			s[n].ycount--;
		}

		s[n].xw  = s[n].x;
		s[n].x  += s[n].dx;


		if (s[n].type == 'D') {
			reduce(&dd_d[n].ang);
		}

		if (s[n].player == 1) {
			if (ub_d[n].z == 0) {  // suf'd
				//pd.fuel--; // reduce fuel
			}
			if (ub_d[n].z > 0) {  // sub'd
				//pd.batt--; // reduce batt
			}
		}
	}


	s[n].ay += bounds_check_y(s[n].y);
	s[n].ax += bounds_check_x(s[n].x);

	wrap_y(&s[n].y);
	wrap_x(&s[n].x);
}

void ai_nav_wp(struct ship *s, int n)
{
	if (on_target(s, n) == 0) {
		//s[n].tay = ;
		//s[n].tax = ;
	}

	if (on_target(s, n) == 1) {
		if (n % 2 == 0) {
			s[n].waypoint--;
			if (s[n].waypoint < 0) {
				s[n].waypoint = 9;
			}
		} else {
			s[n].waypoint++;
			if (s[n].waypoint > 9) {
				s[n].waypoint = 0;
			}
		}
		s[n].tay = m.way_ay[s[n].waypoint] +
		           (dir_y[rand() % 9] * (rand() % 3));
		s[n].tax = m.way_ax[s[n].waypoint] +
		           (dir_x[rand() % 9] * (rand() % 3));
	}
}

void co_nav(struct ship *s, int n)
{

	int mty, mtx;

                mty  = merge_coords_y(co[s[n].con_n].ay, co[s[n].con_n].y);
                mtx  = merge_coords_x(co[s[n].con_n].ax, co[s[n].con_n].x);
                mty += s[n].cty;
                mtx += s[n].ctx;

                s[n].ty  = mty % MY;
                s[n].tx  = mtx % MX;
                s[n].tay = mty / MY;
                s[n].tax = mtx / MX;


}

void mm_ai(struct ship *s, int n)
{
	s[n].speed = mm_d[n].speed; //1000;

	if (s[n].type == 'C') {
		s[n].speed = 5;
	}

	ai_nav_wp(s, n);

	if (s[n].in_con == 1) {
		co_nav(s, n);

		/*
		int mty, mtx;

		mty  = merge_coords_y(co[s[n].con_n].ay, co[s[n].con_n].y);
		mtx  = merge_coords_x(co[s[n].con_n].ax, co[s[n].con_n].x);
		mty += s[n].cty;
		mtx += s[n].ctx;

		s[n].ty  = mty % MY;
		s[n].tx  = mtx % MX;
		s[n].tay = mty / MY;
		s[n].tax = mtx / MX;
		*/

	} else {
		/**********************
		s[n].ty  = rand() % MY;
		s[n].tx  = rand() % MX;
		s[n].tay = rand() % MY;
		s[n].tax = rand() % MX;
		**********************/
	}
}

void ub_ai(struct ship *s, int n)
{
}

void advance_s(struct ship *s, int n)
{
	if (s[n].player == 0) {
		if (s[n].type == 'D') {
			dd_ai(s, n);
		}

		if (s[n].type == 'M') {
			mm_ai(s, n);
		}

		if (s[n].type == 'C') {
			mm_ai(s, n);
		}

		if (s[n].type == '@') {
			ub_ai(s, n);
		}

		dir_ai(s, n);
	}

	if (s[n].player == 1) {
	}

	if (s[n].type != 'X') {
		advance_agnostic(s, n);
	}

}

void print_status()
{
	mvwprintw(stat_win, 0, 0, "CLOCK: %d min, %d hr, DELAY: %d, SPEED: %d, LEG: %d, DIR: %s, %d, POS: %d %d, %d %d",
	          gclock, gclock/60, delay, ub[0].speed, ub[0].leg, dir_c[pi.dir], pi.dir, ub[0].ay, ub[0].y, ub[0].ax, ub[0].x);

	mvwprintw(stat_win, 1, 0, "TUBE: %d, P: %d, MESSAGE: , VIS: %f, DEP: %d, ANG: %d, SIGHTED: %d", 
	pi.fire_n+1, pi.peri,  vis, ub_d[0].z, dd_d[0].ang, pd.ship_sight);




}

void print(int y, int x, char c)
{

	/****************************************************
	        handle all color and formatting in here
	        one single check for color mode and then
	        branch out
	******************************************************/

	if (c == '#') {
		//wattron(main_win, COLOR_PAIR(5));
		mvwprintw(main_win, y - view_y, x - view_x, "%c", c);
		//wattroff(main_win, COLOR_PAIR(5));
	
	} else {

	mvwprintw(main_win, y - view_y, x - view_x, "%c", c);

	}
}

void print_ship(struct ship *s, int n)
{
	int y, x, yw, xw;
	double v;

	y  = s[n].y + (s[n].ay * MY);
        x  = s[n].x + (s[n].ax * MX);
	yw = s[n].yw + (s[n].ay * MY);
	xw = s[n].xw + (s[n].ax * MX);

	int uy = ub[0].y + (ub[0].ay * MY);
	int ux = ub[0].x + (ub[0].ax * MX);

	if (ub_d[0].z == 0 || ub_d[0].peri == 1) {
		v = vis;
	} else {
		v = ((double)s[n].speed * 0.1);
	}

	if (in_range(y, x,  uy, ux, v) == 1) {


		//print(yw, xw, '.');

		if (s[n].type == 'M') {
			print(y, x, mm_d[n].type);
		} else {
			print(y, x, s[n].type);
		}

		//print(yw, xw, '.');
	}
}

void init_s(struct ship *s, int n)
{
	s[n].y        = 0;
	s[n].x        = 0;
	s[n].ay       = 0;
	s[n].ax       = 0;

	s[n].yw       = 0;
	s[n].xw       = 0;

	s[n].dy       = 0;
	s[n].dx       = 0;
	s[n].ycount   = 0;

	s[n].speed    = 0;
	s[n].leg      = 0;

	s[n].ty       = 0;
	s[n].tx       = 0;
	s[n].tay      = 0;
	s[n].tax      = 0;

	s[n].in_con   = 0;
	s[n].con_n    = 0;
	s[n].cty      = 0;
	s[n].ctx      = 0;

	s[n].base     = 0;
	s[n].waypoint = 0;

	s[n].fire     = 0;
	s[n].fire_t   = 0;

	s[n].player   = 0;

	s[n].spawn_t  = 0;

	s[n].type     = 0;
}

void init_s_old(struct ship *s, int n, char c)
{

	s[n].base = rand() % 5;
	s[n].waypoint = 0;

	s[n].y = rand() % MY; //10;
	s[n].x = rand() % MX; //10;

	s[n].ay = m.base_ay[s[n].base];
	s[n].ax = m.base_ax[s[n].base];

	if (m.base_id[s[n].base]  == 'A') {
		s[n].waypoint = 0;
	}
	if (m.base_id[s[n].base]  == 'I') {
		s[n].waypoint = 4;
	}
	if (m.base_id[s[n].base]  == 'E') {
		s[n].waypoint = 5;
	}

	s[n].ty = s[n].y;
	s[n].tx = s[n].x;
	s[n].tay = s[n].ay;
	s[n].tax = s[n].ax;
	s[n].leg = 0;
	s[n].speed = 0;
	s[n].type = c;
	s[n].player = 0;
	s[n].dy = 0;
	s[n].dx = 0;
	s[n].ycount = 0;

	s[n].fire   = 0;
	s[n].fire_t = 0;

	s[n].in_con = 0;

	s[n].cty = 0;
	s[n].ctx = 0;


	if (s[n].type == 'C') { // IF CONVOY
		s[n].in_con = 0;
		;
		
	}

	s[n].spawn_t = gclock;
}

void init_curses()
{
	initscr();

	keypad(stdscr, TRUE);

	noecho();
	curs_set(0);
	timeout(0);
	main_win = newwin(LINES-2, COLS, 0, 0);
	stat_win = newwin(2, COLS, LINES-2, 0);
	//getmaxyx(main_win, my, mx);
	//main_win = newwin(22, 80, 0, 0);
	//stat_win = newwin(2, 80, 22, 0);


	//main_win = newwin(MY-2, MX, 0, 0);
	//stat_win = newwin(2, MX, MY-2, 0);


}

void kill_curses()
{
	delwin(main_win);
	delwin(stat_win);
	endwin();
}

void teleport_baddies()
{
	int i;
	for (i = 0; i < N_DD; i++) {
		dd[i].ay = ub[0].ay;
		dd[i].ax = ub[0].ax;
	}
}

void proc_input()
{
	int t_speed;

	if (pi.z <= 0) {
		pi.z = 0;
	}
	ub_d[0].z = pi.z;

	if (pi.dir < 0) {     // USE CYCLE FUNC FOR THIS
                pi.dir = 7;
        }
        if (pi.dir > 7) {    // SEE ABOVE
                pi.dir = 0;
        }
	ub[0].dy = dir_y[pi.dir];
	ub[0].dx = dir_x[pi.dir];

	if (ub_d[0].z == 0) {
		t_speed = 18;
	}
	if (ub_d[0].z > 0) {
		t_speed = 8;
	}
	if (pi.speed < 0) {
		pi.speed = 0;
	}
	if (pi.speed > t_speed) {
		pi.speed = t_speed;
	}
	ub[0].speed = pi.speed;

	ub_d[0].peri = pi.peri;
	if (ub_d[0].peri == 1 && ub_d[0].z > 1) {
		ub_d[0].peri = 0;
		pi.peri      = 0;
	}

	if (pi.fire1 == 1) {
		pi.fire1 = 0;
		ub[0].fire = 1;
	}

	if (pi.fire2 == 1) {
		pi.fire2 = 0;
		ub_d[0].fire = 1;
	}

	ub_d[0].fire_n = pi.fire_n;

	if (pi.load > 0) {
		if (ub_d[0].fire_n < 4) {
			if (ub_d[0].tube[ub_d[0].fire_n] > 0) {
				pi.load = 0;
				return;
			}
			if (ub_d[0].b_rel[pi.load] > 0) {
				ub_d[0].tube[ub_d[0].fire_n] = pi.load;
				ub_d[0].tube_t[ub_d[0].fire_n] = gclock + 30;
				ub_d[0].b_rel[pi.load]--;
			}
			pi.load = 0;
		}

		if (ub_d[0].fire_n > 3) {
			if (ub_d[0].tube[ub_d[0].fire_n] > 0) {
				pi.load = 0;
				return;
			}
			if (ub_d[0].s_rel[pi.load] > 0) {
				ub_d[0].tube[ub_d[0].fire_n] = pi.load;
				ub_d[0].tube_t[ub_d[0].fire_n] = gclock + 30;
				ub_d[0].s_rel[pi.load]--;
			}
			pi.load = 0;
		}
	}
}

void init_pi()
{
	int i;

	pi.dir = 0;
	pi.fire1 = 0;
	pi.fire2 = 0;
	pi.fire_n = 0;
	pi.z = 0;

	for (i = 0; i < 5; i++) {
		pi.tube[i] = 0;
	}

	pi.load = 0;
}

void init_pd()
{
	pd.fuel = FUEL;
	pd.batt = BATT;
	pd.tonnage = 0;
	pd.mm_hits = 0;
	pd.dd_hits = 0;

	pd.ship_sight = 0;

	/*
	char *message = "welcome";
	strcpy(pd.message, message);
	*/
}

void random_pos_baddies()
{
	int i;

	for (i = 0; i < N_DD; i++) {
		dd[i].y = rand() % MY;
		dd[i].x = rand() % MX;
	}

	dd[0].y = 11;
	dd[0].x = 0;

	dd[1].y = 0;
	dd[1].x = 40;

	dd[2].y = 0;
	dd[2].x = 0;
}


void gen_convoy()
{


	int i;

	int y = 0;
	int x = 0;

	for (i = 0; i < S_CO; i++) {
		mm_d[i].speed = 10;
		mm[i].in_con = 1;
		mm[i].con_n = 0;
		mm[i].ay = co[0].ay;
		mm[i].ax = co[0].ax;

		mm[i].cty = y;
		mm[i].ctx = x;
		x += 5;

		if (x  == 50) {
			x  = 0;
			y += 3;
		}

	}

	y = 0;
	x = 0;

	for (i = S_CO; i < S_CO*2; i++) {
		mm_d[i].speed = 10;
                mm[i].in_con = 1;
		mm[i].con_n = 1;
                mm[i].ay = co[1].ay;
                mm[i].ax = co[1].ax;

                mm[i].cty = y;
                mm[i].ctx = x;
                x += 5;

                if (x  == 50) {
                        x  = 0;
                        y += 3;
                }


	}

	y = -5;
	x = -5;

	for (i = 0; i < 4; i++) {
		dd[i].in_con = 1;
                dd[i].con_n = 0;
                dd[i].ay = co[0].ay;
                dd[i].ax = co[0].ax;

                dd[i].cty = y;
                dd[i].ctx = x;
                x += 55;

                if (i == 1) {
                        x  = -5;
                        y += 22;
                }

	}


	y = -5;
        x = -5;

        for (i = 4; i < 8; i++) {
                dd[i].in_con = 1;
                dd[i].con_n = 1;
                dd[i].ay = co[1].ay;
                dd[i].ax = co[1].ax;

                dd[i].cty = y;
                dd[i].ctx = x;
                x += 55;

                if (i == 5) {
                        x  = -5;
                        y += 22;
                }

        }





	/*** for every ship, loop through and check if in_convoy ***/

	/*** give ship target of unique offset from convoy center ***/

/*****
	int i;

	int y = 0;
	int x = 0;

	for (i = 0; i < N_MM; i++) {
		if (mm[i].in_con == 1) {

			mm[i].ay = co[0].ay;
			mm[i].ax = co[0].ax;

			mm[i].cty = y;
			mm[i].ctx = x;
			x+=5;
			if (x % 11 == 0) {
				x = 0;
				y+=3;
			}
		}
	}
*****/
}

void init_ub_d(int n)
{
	int i;

	ub_d[n].fire   = 0;
	ub_d[n].fire_n = 0;

	for (i = 0; i < 5; i++) {
		ub_d[n].tube[i]   = 0;
		ub_d[n].tube_t[i] = 0;
	}

	for (i = 0; i < 3; i++) {
		ub_d[n].b_rel[i] = 0;
		ub_d[n].s_rel[i] = 0;
	}

	ub_d[n].b_ex = 0;
	ub_d[n].s_ex = 0;

	ub_d[n].peri = 0;
	ub_d[n].z    = 0;
}

void init_mm_d(struct mm_data *mm_d, int n)
{
	mm_d[n].ton   = 0;
	mm_d[n].speed = 0;
	mm_d[n].type  = 0;
}

void print_ub_info()
{
	int i;
	char *status;
	char *type;

	wattron(main_win, A_UNDERLINE);
	mvwprintw(main_win, 0, 0, "Bow Compartment");
	wattroff(main_win, A_UNDERLINE);

	for (i = 0; i < 4; i++) {
		status = "Loading";
		type   = "Empty";

		if (ub_d[0].tube[i] > 0) {
			if (ub_d[0].tube_t[i] < gclock) {
				status = "OK";
			}
			if (ub_d[0].tube[i] == 1) {
				type = "G7a T1";
			}
			if (ub_d[0].tube[i] == 2) {
				type = "G7e T2";
			}
			if (ub_d[0].fire_n == i) {
				wattron(main_win, A_BOLD);
				mvwprintw(main_win, 1+i, 1, "Tube %d: %s %s",
					  i+1, type, status);
				wattroff(main_win, A_BOLD);
			} else {
				mvwprintw(main_win, 1+i, 1, "Tube %d: %s %s",
				          i+1, type, status);
			}
		}

		if (ub_d[0].tube[i] == 0) {
			if (ub_d[0].fire_n == i) {
				wattron(main_win, A_BOLD);
				mvwprintw(main_win, 1+i, 1, "Tube %d: %s",
					  i+1, type);
				wattroff(main_win, A_BOLD);
			} else {
				mvwprintw(main_win, 1+i, 1, "Tube %d: %s",
				          i+1, type);
			}
		}

	}

	mvwprintw(main_win, 6, 1, "Reloads");
	mvwprintw(main_win, 7, 1, "G7a T1: %d", ub_d[0].b_rel[1]);
	mvwprintw(main_win, 8, 1, "G7e T2: %d", ub_d[0].b_rel[2]);

	wattron(main_win, A_UNDERLINE);
	mvwprintw(main_win, 10, 0, "Stern Compartment");
	wattroff(main_win, A_UNDERLINE);

	for (i = 4; i < 5; i++) {
		status = "Loading";
		type   = "Empty";

		if (ub_d[0].tube[i] > 0) {
			if (ub_d[0].tube_t[i] < gclock) {
				status = "OK";
			}
			if (ub_d[0].tube[i] == 1) {
				type = "G7a T1";
			}
			if (ub_d[0].tube[i] == 2) {
				type = "G7e T2";
			}
			if (ub_d[0].fire_n == i) {
				wattron(main_win, A_BOLD);
				mvwprintw(main_win, 11, 1, "Tube %d: %s %s",
					  i+1, type, status);
				wattroff(main_win, A_BOLD);
			} else {
				mvwprintw(main_win, 11, 1, "Tube %d: %s %s",
				          i+1, type, status);
			}
		}

		if (ub_d[0].tube[i] == 0) {
			if (ub_d[0].fire_n == i) {
				wattron(main_win, A_BOLD);
				mvwprintw(main_win, 11, 1, "Tube %d: %s",
					  i+1, type);
				wattroff(main_win, A_BOLD);
			} else {
				mvwprintw(main_win, 11, 1, "Tube %d: %s",
				          i+1, type);
			}
		}

	}

	mvwprintw(main_win, 13, 1, "Reloads");
	mvwprintw(main_win, 14, 1, "G7a T1: %d", ub_d[0].s_rel[1]);
	mvwprintw(main_win, 15, 1, "G7e T2: %d", ub_d[0].s_rel[2]);

	for (i = 0; i < 15; i++) {
		mvwprintw(main_win, i, 25, "|");
	}

	mvwprintw(main_win, 1, 30, "Fuel: %d", pd.fuel);
	mvwprintw(main_win, 2, 30, "Battery: %d", pd.batt);
	mvwprintw(main_win, 3, 30, "Oxygen:");
	mvwprintw(main_win, 4, 30, "Provisions:");
	mvwprintw(main_win, 7, 30, "Clock: %d min, %d hr, %d days", gclock, gclock/60, (gclock/60)/24);

/*****





*****/
}

void toggle(int *x)
{
	if (*x == 0) {
		*x = 1;
	} else {
		*x = 0;
	}
}

void cycle(int *x, int n)
{
	/* takes an int and increments it upto number provided
	   and then back to 0 if it goes over */

	*x += 1;
	if (*x > n) {
		*x = 0;
	}
}

void grab_input() /*** GRAB INPUT ***/
{
	switch(ch) {
		case 'T':
			toggle(&TRANSIT); // = 1;
			break;
		case '\\':
			cycle(&pi.fire_n, 4);
			break;
		case 'W':
			toggle(&CONVOY);
			break;
		case KEY_LEFT:
			co[0].x--;
			break;
		case KEY_RIGHT:
			co[0].x++;
			break;
		case KEY_UP:
			co[0].y--;
			break;
		case KEY_DOWN:
			co[0].y++;
			break;
		case '1':
			pi.load = 1;
			//toggle(&ub_d[0].tube[0]);
			//cycle(&ub_d[0].tube[0], 2);
			break;
		case '2':
			pi.load = 2;
			//toggle(&ub_d[0].tube[1]);
			break;
		case '3':
			//toggle(&ub_d[0].tube[2]);
			break;
		case '4':
			//toggle(&ub_d[0].tube[3]);
			break;
		case '5':
			//toggle(&ub_d[0].tube[4]);
			break;
		case 'c':
			cycle(&pi.fire_n, 4);
			break;
		case 'p':
			toggle(&pi.peri);
			break;
		case 'y':
			break;
		case 'Y':
			break;
		case ',':
			break;
		case '.':
			break;
		case 'r':
			random_pos_baddies();
			break;
		case 't':
			teleport_baddies();
			break;
		case 'w':
			pi.z++;
			break;
		case 's':
			pi.z--;
			break;
		case 'z':
			break;
		case 'x':
			break;
		case 'i':
			vis -= .01;
			break;
		case 'o':
			vis += .01;
			break;
		case 'H':
			ub[0].ax--;
			//view_x--;
			break;
		case 'J':
			ub[0].ay++;
			//view_y++;
			break;
		case 'K':
			ub[0].ay--;
			//view_y--;
			break;
		case 'L':
			ub[0].ax++;
			//view_x++;
			break;
		case '[':
			pi.dir--;
			break;
		case ']':
			pi.dir++;
			break;
		case '9':
			pi.speed--;
			break;
		case '0':
			pi.speed++;
			break;
		case 'h':
			ub[0].x--;
			break;
		case 'j':
			ub[0].y++;
			break;
		case 'k':
			ub[0].y--;
			break;
		case 'l':
			ub[0].x++;
			break;
		case '-':
			delay--;
			break;
		case '=':
			delay++;
			break;
		case 'm':
			mode = 1;
			break;
		case 'M':
			mode = 0;
			break;
		case 'q':
			mode = 9;
			break;
		case ' ':
			pi.fire2 = 1;
			break;
		case 'f':
			pi.fire1 = 1;
			break;
		case KEY_F(1):
			mode = 0;
			break;
		case KEY_F(2):
			mode = 1;
			break;
		case KEY_F(3):
			mode = 2;
			break;
		case KEY_F(4):
			mode = 3;
			break;
		default:
			break;
	}
}
