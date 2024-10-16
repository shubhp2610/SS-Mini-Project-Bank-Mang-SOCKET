#ifndef EMPLOYEE_H
#define EMPLOYEE_H

#include "../shared/utils.h"

void add_new_customer(int socket_conn);

void modify_customer_details(int socket_conn);

void view_assigned_loan_applications(int socket_conn, int emp_id);

void approve_reject_loans(int socket_conn, int emp_id);

void view_customer_transactions(int socket_conn);

#endif