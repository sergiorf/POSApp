#include "../Include/login_screen.h"
#include "../Include/operadores.h"
#include "../Include/keyboard.h"
#include <string.h>
#include <stdlib.h>

static int h_clock = 0; // clock device handle

ret_code loginScreenInit(int* cancel)
{
	ret_code ret = ERROR;
	point top, bottom;
	const char* err;	
	signed char login_buf[200];	
	char        pwd_buf[100];
    short       len;
    int         ret_val;
    memset (login_buf, '\0', sizeof (login_buf));
    memset (pwd_buf, '\0', sizeof (pwd_buf));
	getScreenDims(&top, &bottom);

	for (;SUCCESS!=ret;) {
		int ret_val, len, height = bottom.y/2, width = bottom.x/2;
		memset (login_buf, '\0', sizeof (login_buf));
		window(top.x,top.y,bottom.x,bottom.y);	
		clrscr ();	
		CHECK(SUCCESS==set_cursor(1));		
		show_bmp(LOGO_FILENAME);
		WRITE_AT("Operador:", 3, height-1);
		WRITE_AT("Senha:", 6, height);
		window (13, height-1, 29, height-1);		
		gotoxy (1, 1);
		ret_val = getkbd_entry (h_clock, "", (signed char *) login_buf, 0/*no timeout*/, NUMERIC,
                            (char *) szKeyMapVx680, sizeof (szKeyMapVx680), 10, 1);
		if (ret_val>0) {
			ret = op_checkId(login_buf,NULL,&err);
			if (SUCCESS!=ret) {
				CHECK(SUCCESS==set_cursor(0));		
				clrscr();				
				window(top.x,top.y,bottom.x,bottom.y);	
				WRITE_AT(err, bottom.x/2-strlen(err)/2, 5);
				SVC_WAIT(2000);
			} else {
				// Now get the pwd
				window (13, height, 29, height);		
				gotoxy(1, 1);
				ret_val = getkbd_entry (h_clock, "", (signed char *) pwd_buf, 0/*no timeout*/, NUMERIC,
                            (char *) szKeyMapVx680, sizeof (szKeyMapVx680), 10, 1);
				if (ret_val>0) {
					ret = op_checkId(login_buf,pwd_buf,&err);
					if (SUCCESS!=ret) {
						CHECK(SUCCESS==set_cursor(0));		
						clrscr();				
						window(top.x,top.y,bottom.x,bottom.y);	
						WRITE_AT(err, bottom.x/2-strlen(err)/2, 5);
						SVC_WAIT(2000);
					} 
				}
			}
		} else if(-3==ret_val) {
			/* go to prev screen when user presses ESC*/
			*cancel = 1;
			ret = SUCCESS;
		}
	}
	CHECK(SUCCESS==set_cursor(0));		
	window(top.x,top.y,bottom.x,bottom.y);	
	if (0==*cancel) {		
		memset (login_buf, '\0', sizeof (login_buf));
		sprintf (login_buf, "Bem-vindo %s", g_operador);
		SCREEN_WARNING(login_buf);
	}
	return ret;
}


