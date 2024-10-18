#ifndef ADMIN_H
#define ADMIN_H

#include "../shared/utils.h"

void add_new_employee(int socket_conn);

void modify_user_details(int socket_conn);

void manage_user_roles(int socket_conn);

#endif