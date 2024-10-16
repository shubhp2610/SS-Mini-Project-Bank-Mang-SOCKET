#ifndef MANAGER_H
#define MANAGER_H

#include "../shared/utils.h"

void activate_deactivate_customer_accounts(int socket_conn);

void assign_loan_application_processes(int socket_conn);

void review_customer_feedback(int socket_conn);

#endif