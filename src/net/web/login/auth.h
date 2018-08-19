#ifndef __AUTH_H__
#define __AUTH_H__

#define MAX_USERNAME_SIZE	32	/* the max user name size */
#define MAX_PASSWORD_SIZE	32	/* the max password size  */

typedef enum {priv_null = 0, priv_developer, priv_admin, priv_guest} priv_t;

/* the user and it's attributes */
typedef struct
{
	char 	username[MAX_USERNAME_SIZE];
	char 	password[MAX_PASSWORD_SIZE];
	priv_t	user_priv;
}AUTH_USERS;

/*ADMINµÄÈ±Ê¡ÃÜÂë*/
#define DLFT_AUTH_USRNAME	"admin"
#define DLFT_AUTH_PASS		"hctel"

char* auth_pass_get(char *name);
STATUS auth_pass_check(char *name, char *pass);
STATUS auth_pass_set(char *name, char*pass);

#endif
