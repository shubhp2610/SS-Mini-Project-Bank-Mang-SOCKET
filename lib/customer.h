#ifndef CUSTOMER_H
#define CUSTOMER_H

#include "../shared/utils.h"

void view_balance(int socket_conn, User current_user);

void deposit(int socket_conn, User *current_user);

void withdraw(int socket_conn, User *current_user);

int atomic_transfer(User *current_user, User *destination_user, double transfer_amount);

void transfer(int socket_conn, User *current_user);

int next_available_application_id(int fd);

int add_loan_application(User user, double amount, char *purpose);

void apply_loan(int socket_conn, User *current_user);

int next_available_feedback_id(int fd);

int add_feedback(User user, char *feedback);

void provide_feedback(int socket_conn, User *current_user);

#endif