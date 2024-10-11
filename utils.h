#include <stdio.h>
#include <string.h>
#include <openssl/sha.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include "models.h"

// Function to hash a password using SHA-256
void sha256_hash(const char *password, unsigned char outputHash[SHA256_DIGEST_LENGTH]);

// Function to check if a password matches the hashed version
int is_password_correct(const char *password, const unsigned char hash[SHA256_DIGEST_LENGTH]);

int get_user_by_id(int user_id, User *user);

int get_user_by_location(User *user);

int update_user_by_location(User user);

int acquire_write_lock(int fd, int pid);

int acquire_read_lock(int fd, int pid);

int acquire_write_lock_partial(int fd, int pid, int start, int len);

int acquire_read_lock_partial(int fd, int pid, int start, int len);

int release_lock(int fd, int pid);

int create_user();

int start_session(User user, Session *session);

int end_session(Session session);

int add_transaction(User user, TransactionType type, double amount, TransactionStatus status, int from_account, int to_account);

int get_transactions(int socket_conn, int user_id);

int add_feedback(User user, char *feedback);

int get_feedbacks(int socket_conn);

int add_loan_application(User user, double amount, char *purpose);

int get_loan_applications(int socket_conn, int emp_id);

int update_loan_status(int application_id, int emp_id,LoanStatus status);

int add_user(User new_user, Role role);

int next_available_user_id();

int wrapper_change_password(int socket_conn, User *current_user);

int wrapper_logout(int socket_conn, Session *current_session);

int wrapper_exit(int socket_conn , Session *current_session);