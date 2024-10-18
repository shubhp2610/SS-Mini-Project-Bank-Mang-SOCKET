#include "customer.h"

void view_balance(int socket_conn, User current_user){
    char buffer[MAX_BUFFER_SIZE];
    get_user_by_location(&current_user); // Fetch user info
    memset(buffer, 0, sizeof(buffer));
    snprintf(buffer, sizeof(buffer), "Your account balance is %.2f\n\n\nN", current_user.balance);
    write_line(socket_conn, buffer);
}

void deposit(int socket_conn, User *current_user){
    write_line(socket_conn, "Enter amount to deposit: ");
    double deposit_amount = read_double(socket_conn);
    if(deposit_amount <= 0){
        write_line(socket_conn, "Error : Invalid amount!\n\nN");
        return;
    }
    current_user->balance += deposit_amount;
    if(update_user_by_location(*current_user)==-1){
        write_line(socket_conn, "Error : Deposit user update failed!\n\nN");
        return;
    }
    get_user_by_location(current_user);
    add_transaction(*current_user, DEPOSIT, deposit_amount, SUCCESS,0,0);
    char deposit_status[MAX_BUFFER_SIZE];
    snprintf(deposit_status, sizeof(deposit_status), "Deposit successful. Your new balance is %.2f\n\n\nN", current_user->balance);
    write_line(socket_conn, deposit_status);
}

void withdraw(int socket_conn, User *current_user){
    write_line(socket_conn, "Enter amount to withdraw: ");
    double withdrawal_amount = read_double(socket_conn);
    if(withdrawal_amount <= 0){
        write_line(socket_conn, "Error : Invalid amount!\n\nN");
        return;
    }
    if(withdrawal_amount > current_user->balance){
        write_line(socket_conn, "Error : Insufficient balance!\n\nN");
        add_transaction(*current_user, WITHDRAWAL, withdrawal_amount, INSUFFICIENT_BALANCE,0,0);
        return;
    }
    current_user->balance -= withdrawal_amount;
    if(update_user_by_location(*current_user)==-1){
        write_line(socket_conn, "Error : Withdrawal user update failed!\n\nN");
        return;
    }
    get_user_by_location(current_user);
    add_transaction(*current_user, WITHDRAWAL, withdrawal_amount, SUCCESS,0,0);
    char widthraw_status[MAX_BUFFER_SIZE];
    snprintf(widthraw_status, sizeof(widthraw_status), "Withdrawal successful. Your new balance is %.2f\n\n\nN", current_user->balance);
    write_line(socket_conn, widthraw_status);
}

int atomic_transfer(User *current_user, User *destination_user, double transfer_amount) {
    int fd = open("data/users.db", O_RDWR);
    if (fd == -1) {
        perror("Error opening file");
        return -1;
    }
    int pid = getpid();
    // lock both users based on their db_index to avoid deadlocks
    if (current_user->db_index < destination_user->db_index) {
        acquire_write_lock_partial(fd, pid, current_user->db_index, sizeof(User));
        acquire_write_lock_partial(fd, pid, destination_user->db_index, sizeof(User));
    } else {
        acquire_write_lock_partial(fd, pid, destination_user->db_index, sizeof(User));
        acquire_write_lock_partial(fd, pid, current_user->db_index, sizeof(User));
    }
    current_user->balance -= transfer_amount;
    destination_user->balance += transfer_amount;
    if (update_user_by_location(*current_user) == -1) {
        release_lock(fd, pid);
        close(fd);
        return -1;
    }
    if (update_user_by_location(*destination_user) == -1) {
        current_user->balance += transfer_amount;
        // relocking to ensure safe rollback
        acquire_write_lock_partial(fd, pid, current_user->db_index, sizeof(User));
        update_user_by_location(*current_user);
        release_lock(fd, pid);
        close(fd);
        return -1;
    }
    add_transaction(*current_user, TRANSFER, transfer_amount, SUCCESS, current_user->user_id, destination_user->user_id);
    add_transaction(*destination_user, TRANSFER, transfer_amount, SUCCESS, current_user->user_id, destination_user->user_id); 
    release_lock(fd, pid);
    close(fd);
    return 0;
}



void transfer(int socket_conn, User *current_user){
    write_line(socket_conn, "Enter destination account number: ");
    int destination_account = read_int(socket_conn);
    User destination_user;
    if(get_user_by_id(destination_account, &destination_user) == -1 || destination_user.active == 0){
        write_line(socket_conn, "Error : Destination account not found!\n\nN");
        add_transaction(*current_user, TRANSFER, 0, DESTINATION_ACCOUNT_NOT_FOUND,current_user->user_id,destination_account);
        return;
    }
    write_line(socket_conn, "Enter amount to transfer: ");
    double transfer_amount = read_double(socket_conn);
    if(transfer_amount <= 0){
        write_line(socket_conn, "Error : Invalid amount!\n\nN");
        return;
    }
    if(transfer_amount > current_user->balance){
        write_line(socket_conn, "Error : Insufficient balance!\n\nN");
        add_transaction(*current_user, TRANSFER, transfer_amount, INSUFFICIENT_BALANCE,current_user->user_id,destination_account);
        return;
    }
    if(atomic_transfer(current_user,&destination_user,transfer_amount) == -1){
        write_line(socket_conn, "Error : Atomic transfer failed!\n\nN");
    }
    get_user_by_location(current_user);
    get_user_by_location(&destination_user);
    char transfer_status[MAX_BUFFER_SIZE];
    snprintf(transfer_status, sizeof(transfer_status), "Transfer successful. Your new balance is %.2f\n\n\nN", current_user->balance);
    write_line(socket_conn, transfer_status);
}

int next_available_application_id(int fd) {
    int pid = getpid();
    acquire_read_lock(fd, pid);
    Loan temp;
    int max_id = 0;
    while (read(fd, &temp, sizeof(Loan)) > 0) {
        if (temp.application_id > max_id) {
            max_id = temp.application_id;
        }
    }
    return max_id + 1;
}

int add_loan_application(User user, double amount, char *purpose) {
    int fd = open("data/loans.db", O_RDWR);
    if (fd == -1) {
        perror("Error opening file");
        return -1;
    }
    int pid = getpid();
    acquire_write_lock(fd, pid);
    Loan temp;
    temp.user_id = user.user_id;
    temp.application_date = time(NULL);
    temp.application_id = next_available_application_id(fd);
    temp.amount = amount;
    temp.assignment_date = 0;
    temp.decision_date = 0;
    temp.employee_id = 0;
    strcpy(temp.purpose, purpose);
    temp.status = LOAN_PENDING;
    lseek(fd, 0, SEEK_END);
    write(fd, &temp, sizeof(Loan));
    release_lock(fd, pid);
    close(fd);
    return temp.application_id;
}

void apply_loan(int socket_conn, User *current_user){
    char buffer[MAX_BUFFER_SIZE];
    write_line(socket_conn, "Enter loan amount: ");
    double loan_amount = read_double(socket_conn);
    if(loan_amount <= 0){
        write_line(socket_conn, "Error : Invalid amount!\n\nN");
        return;
    }
    write_line(socket_conn, "Enter loan purpose: ");
    memset(&buffer, 0, sizeof(buffer));
    read(socket_conn, &buffer, sizeof(buffer));
    int app_number = add_loan_application(*current_user, loan_amount, buffer);
    if(app_number==-1){
        write_line(socket_conn, "Error : Loan application failed!\n\nN");
    }
    char app_status[MAX_BUFFER_SIZE];
    snprintf(app_status, MAX_BUFFER_SIZE, "Loan application submitted successfully. Your application number is %d\n\nN", app_number);
    write_line(socket_conn, app_status);
}

int next_available_feedback_id(int fd) {
    int pid = getpid();
    acquire_read_lock(fd, pid);
    Feedback temp;
    int max_id = 0;
    while (read(fd, &temp, sizeof(Feedback)) > 0) {
        if (temp.feedback_id > max_id) {
            max_id = temp.feedback_id;
        }
    }
    return max_id + 1;
}

int add_feedback(User user, char *feedback) {
    int fd = open("data/feedbacks.db", O_RDWR);
    if (fd == -1) {
        perror("Error opening file");
        return -1;
    }
    int pid = getpid();
    acquire_write_lock(fd, pid);
    Feedback temp;
    temp.user_id = user.user_id;
    temp.timestamp = time(NULL);
    temp.feedback_id = next_available_feedback_id(fd);
    strcpy(temp.feedback, feedback);
    lseek(fd, 0, SEEK_END);
    write(fd, &temp, sizeof(Feedback));
    release_lock(fd, pid);
    close(fd);
    return 0;
}


void provide_feedback(int socket_conn, User *current_user){
    char buffer[MAX_BUFFER_SIZE];
    write_line(socket_conn, "Enter your valuable feedback: ");
    memset(&buffer, 0, sizeof(buffer));
    read(socket_conn, &buffer, sizeof(buffer));
    if(add_feedback(*current_user, buffer)==-1){
        write_line(socket_conn, "Error : Feedback submission failed!\n\nN");
        return;
    }
    write_line(socket_conn, "Feedback submitted successfully!\n\nN");
}