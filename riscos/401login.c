/*
 * This file is part of NetSurf, http://netsurf.sourceforge.net/
 * Licensed under the GNU General Public License,
 *                http://www.opensource.org/licenses/gpl-license
 * Copyright 2003 John M Bell <jmb202@ecs.soton.ac.uk>
 */


#include <assert.h>
#include <ctype.h>
#include <string.h>
#include "oslib/wimp.h"
#include "netsurf/utils/config.h"
#include "netsurf/content/content.h"
#include "netsurf/desktop/browser.h"
#include "netsurf/desktop/401login.h"
#include "netsurf/desktop/gui.h"
#include "netsurf/riscos/dialog.h"
#include "netsurf/riscos/wimp_event.h"
#include "netsurf/utils/log.h"
#include "netsurf/utils/messages.h"
#include "netsurf/utils/url.h"
#include "netsurf/utils/utils.h"

#ifdef WITH_AUTH

static void ro_gui_401login_close(wimp_w w);
static bool ro_gui_401login_apply(wimp_w w);
static void ro_gui_401login_open(struct browser_window *bw, char *host,
		char *realm, char *fetchurl);


static wimp_window *dialog_401_template;

struct session_401 {
  	char *host;
  	char *realm;
	char uname[256];
	char *url;
	char pwd[256];
	struct browser_window *bwin;
};


/**
 * Load the 401 login window template.
 */

void ro_gui_401login_init(void)
{
	dialog_401_template = ro_gui_dialog_load_template("login");
}


void gui_401login_open(struct browser_window *bw, struct content *c, char *realm)
{
	char *murl, *host;
	url_func_result res;

	murl = c->url;
	res = url_host(murl, &host);
	assert(res == URL_FUNC_OK);

	ro_gui_401login_open(bw, host, realm, murl);

	free(host);
}


/**
 * Open a 401 login window.
 */

void ro_gui_401login_open(struct browser_window *bw, char *host, char *realm,
		char *fetchurl)
{
	struct session_401 *session;
	wimp_w w;
	
	session = calloc(1, sizeof(struct session_401));
	if (!session) {
		warn_user("NoMemory", 0);
		return;
	}

	session->url = strdup(fetchurl);
	if (!session->url) {
	  	free(session);
		warn_user("NoMemory", 0);
		return;
	}
	session->uname[0] = '\0';
	session->pwd[0] = '\0';
	session->host = strdup(host);
	session->realm = strdup(realm);
	session->bwin = bw;
	if ((!session->host) || (!session->realm)) {
	  	free(session->host);
	  	free(session->realm);
	  	free(session);
	}

	/* fill in download window icons */
	dialog_401_template->icons[ICON_401LOGIN_HOST].data.indirected_text.text =
		session->host;
	dialog_401_template->icons[ICON_401LOGIN_HOST].data.indirected_text.size =
		strlen(host) + 1;
	dialog_401_template->icons[ICON_401LOGIN_REALM].data.indirected_text.text =
		session->realm;
	dialog_401_template->icons[ICON_401LOGIN_REALM].data.indirected_text.size =
		strlen(realm) + 1;
	dialog_401_template->icons[ICON_401LOGIN_USERNAME].data.indirected_text.text =
		session->uname;
	dialog_401_template->icons[ICON_401LOGIN_USERNAME].data.indirected_text.size =
		256;
	dialog_401_template->icons[ICON_401LOGIN_PASSWORD].data.indirected_text.text =
		session->pwd;
	dialog_401_template->icons[ICON_401LOGIN_PASSWORD].data.indirected_text.size =
		256;

	/* create and open the window */
	w = wimp_create_window(dialog_401_template);
	
	ro_gui_wimp_event_register_text_field(w, ICON_401LOGIN_USERNAME);
	ro_gui_wimp_event_register_text_field(w, ICON_401LOGIN_PASSWORD);
	ro_gui_wimp_event_register_cancel(w, ICON_401LOGIN_CANCEL);
	ro_gui_wimp_event_register_ok(w, ICON_401LOGIN_LOGIN,
			ro_gui_401login_apply);
	ro_gui_wimp_event_register_close_window(w, ro_gui_401login_close);
	ro_gui_wimp_event_set_user_data(w, session);

	ro_gui_dialog_open_persistent(bw->window->window, w, false);

}


void ro_gui_401login_close(wimp_w w) {
	os_error *error;
  	struct session_401 *session;
  	
  	session = (struct session_401 *)ro_gui_wimp_event_get_user_data(w);
  	
  	assert(session);
  	
  	free(session->host);
  	free(session->realm);
  	free(session->url);
  	free(session);

	ro_gui_wimp_event_finalise(w);

	error = xwimp_delete_window(w);
	if (error)
		LOG(("xwimp_delete_window: 0x%x: %s",
				error->errnum, error->errmess));

}


/* Login Clicked -> create a new fetch request, specifying uname & pwd
 *                  CURLOPT_USERPWD takes a string "username:password"
 */
bool ro_gui_401login_apply(wimp_w w)
{
	struct session_401 *session;
	char *lidets;

  	session = (struct session_401 *)ro_gui_wimp_event_get_user_data(w);
  	
  	assert(session);

	lidets = calloc(strlen(session->uname) + strlen(session->pwd) + 2,
			sizeof(char));
	if (!lidets) {
		LOG(("Insufficient memory for calloc"));
		warn_user("NoMemory", 0);
		return false;
	}

	sprintf(lidets, "%s:%s", session->uname, session->pwd);

	login_list_add(session->url, lidets);
	browser_window_go(session->bwin, session->url, 0);
	return true;
}

#endif
