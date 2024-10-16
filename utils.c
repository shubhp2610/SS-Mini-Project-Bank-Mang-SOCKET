#include "utils.h"
void sha256_hash(const char *password, unsigned char outputHash[SHA256_DIGEST_LENGTH]) {
    SHA256((unsigned char*)password, strlen(password), outputHash);
}

int is_password_correct(const char *password, const unsigned char hash[SHA256_DIGEST_LENGTH]) {
    printf("checking password : %s\n",password);
    unsigned char outputHash[SHA256_DIGEST_LENGTH];
    sha256_hash(password, outputHash);
    return memcmp(hash, outputHash, SHA256_DIGEST_LENGTH) == 0;
}

void random_init() {
    srand(time(NULL));
}

int random_id(){
    return rand();
}

int read_int(int socket_conn) {
    char buffer[MAX_BUFFER_SIZE];
    read(socket_conn, &buffer, sizeof(buffer));
    return atoi(buffer);
}

double read_double(int socket_conn) {
    char buffer[MAX_BUFFER_SIZE];
    read(socket_conn, &buffer, sizeof(buffer));
    return atof(buffer);
}

void write_line(int socket_conn, const char *line) {
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));
    strncpy(buffer, line, sizeof(buffer) - 1);   

    size_t length = strlen(buffer);     
    if (length < sizeof(buffer)) {
        buffer[length] = '\0';         
    }
    write(socket_conn, buffer, sizeof(buffer));
}


int acquire_write_lock(int fd, int pid) {
    struct flock lock;
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    lock.l_pid = pid;
    return fcntl(fd, F_SETLKW, &lock);
}

int acquire_read_lock(int fd, int pid) {
    struct flock lock;
    lock.l_type = F_RDLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    lock.l_pid = pid;
    return fcntl(fd, F_SETLKW, &lock);
}

int acquire_write_lock_partial(int fd, int pid, int start, int len) {
    struct flock lock;
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = start;
    lock.l_len = len;
    lock.l_pid = pid;
    return fcntl(fd, F_SETLKW, &lock);
}

int acquire_read_lock_partial(int fd, int pid, int start, int len) {
    struct flock lock;
    lock.l_type = F_RDLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = start;
    lock.l_len = len;
    lock.l_pid = pid;
    return fcntl(fd, F_SETLKW, &lock);
}

int release_lock(int fd, int pid) {
    struct flock lock;
    lock.l_type = F_UNLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    lock.l_pid = pid;
    return fcntl(fd, F_SETLK, &lock);
}

int get_user_by_id(int user_id, User *user) {
    User temp;
    int fd = open("data/users.db", O_RDONLY);
    if (fd == -1) {
        perror("Error opening file");
        return -1;
    }
    int pid = getpid();
    acquire_read_lock(fd, pid);
    while (read(fd, &temp, sizeof(User)) > 0) {
        if (temp.user_id == user_id) {
            printf("UTILS_GET_USER_BY_ID : User found with id %d\n",user_id);
            *user = temp;
            release_lock(fd, pid);
            close(fd);
            return 0;
        }
    }
    release_lock(fd, pid);
    close(fd);
    return -1;
}

int get_user_by_location(User *user) {
    int fd = open("data/users.db", O_RDWR);
    if (fd == -1) {
        perror("Error opening file");
        return -1;
    }
    int pid = getpid();
    acquire_read_lock_partial(fd, pid, user->db_index, sizeof(User));
    lseek(fd, user->db_index, SEEK_SET);
    read(fd, user, sizeof(User));
    release_lock(fd, pid);
    close(fd);
    return 0;
}

int update_user_by_location(User user) {
    int fd = open("data/users.db", O_RDWR);
    if (fd == -1) {
        perror("Error opening file");
        return -1;
    }
    int pid = getpid();
    acquire_write_lock_partial(fd, pid, user.db_index, sizeof(User));
    lseek(fd, user.db_index, SEEK_SET);
    write(fd, &user, sizeof(User));
    release_lock(fd, pid);
    close(fd);
    return 0;
}

int start_session(User user, Session *session) {
    int fd = open("data/sessions.db", O_RDWR);
    if (fd == -1) {
        perror("Error opening file");
        return -1;
    }
    int pid = getpid();
    acquire_read_lock(fd, pid);
    Session temp;
    while (read(fd, &temp, sizeof(Session)) > 0) {
        if (temp.user_id == user.user_id && temp.active && temp.login_time + SESSION_TIMEOUT > time(NULL)) {
            printf("UTILS_START_SESSION : Session already exists for user %d\n",user.user_id);
            release_lock(fd, pid);
            close(fd);
            return -1;
        }
    }
    session->active = 1;
    session->login_time = time(NULL);
    session->logout_time = 0;
    session->session_id = random_id();
    session->user_id = user.user_id;
    session->db_index = lseek(fd, 0, SEEK_END); // Store the start position
    lseek(fd, 0, SEEK_END);
    acquire_write_lock(fd, pid);
    write(fd, session, sizeof(Session));
    release_lock(fd, pid);
    close(fd);
    return 0;
}

int end_session(Session session) {
    int fd = open("data/sessions.db", O_RDWR);
    if (fd == -1) {
        perror("Error opening file");
        return -1;
    }
    int pid = getpid();
    acquire_write_lock_partial(fd, pid, session.db_index, sizeof(Session));
    lseek(fd, session.db_index, SEEK_SET);
    session.active = 0;
    session.logout_time = time(NULL);
    write(fd, &session, sizeof(Session));
    release_lock(fd, pid);
    close(fd);
    return 0;
}

int add_transaction(User user, TransactionType type, double amount, TransactionStatus status,int from_account , int to_account) {
    int fd = open("data/transactions.db", O_RDWR);
    if (fd == -1) {
        perror("Error opening file");
        return -1;
    }
    int pid = getpid();
    acquire_write_lock(fd, pid);
    Transaction temp;
    temp.amount = amount;
    temp.timestamp = time(NULL);
    temp.trx_id = random_id();
    temp.user_id = user.user_id;
    temp.type = type;
    temp.status = status;
    temp.balance = user.balance;
    temp.from_account = from_account;
    temp.to_account = to_account;
    lseek(fd, 0, SEEK_END);
    write(fd, &temp, sizeof(Transaction));
    release_lock(fd, pid);
    close(fd);
    return 0;
}

int get_transactions(int socket_conn,int user_id) {
    int fd = open("data/transactions.db", O_RDONLY);
    if (fd == -1) {
        perror("Error opening file");
        return -1;
    }
    int pid = getpid();
    acquire_read_lock(fd, pid);
    Transaction temp;
    char buffer[1024];
    const char* transactionTypeNames[] = {
    "DEPOSIT",
    "WITHDRAWAL",
    "TRANSFER",
    "LOAN_PAYMENT",
    "LOAN_DISBURSEMENT"
    };
    const char* transactionStatusNames[] = {
    "DESTINATION_ACCOUNT_NOT_FOUND",
    "INSUFFICIENT_BALANCE",
    "SUCCESS"
    };
    while (read(fd, &temp, sizeof(Transaction)) > 0) {
        if (temp.user_id == user_id) {
            if(temp.type == TRANSFER){
            snprintf(buffer, sizeof(buffer), "Transaction ID : %d  Amount : %.2f New Balance : %.2f Type : %s  Status : %s To Account : %d From Account : %d Timestamp : %s \nN", temp.trx_id, temp.amount,temp.balance, transactionTypeNames[temp.type], transactionStatusNames[temp.status],temp.to_account,temp.from_account,ctime(&temp.timestamp));
            }else{
            snprintf(buffer, sizeof(buffer), "Transaction ID : %d  Amount : %.2f New Balance : %.2f Type : %s  Status : %s Timestamp : %s \nN", temp.trx_id, temp.amount,temp.balance, transactionTypeNames[temp.type], transactionStatusNames[temp.status],ctime(&temp.timestamp));
            }
            write(socket_conn, buffer, strlen(buffer));
            memset(buffer, 0, sizeof(buffer));
        }
    }
    release_lock(fd, pid);
    close(fd);
    return 0;
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
    temp.feedback_id = random_id();
    printf("UTILS_ADD_FEEDBACK : Feedback ID : %d\n",temp.feedback_id);
    printf("UTILS_ADD_FEEDBACK : User ID : %d\n",temp.user_id);
    printf("UTILS_ADD_FEEDBACK : Timestamp : %s\n",ctime(&temp.timestamp));
    printf("UTILS_ADD_FEEDBACK : Feedback : %s\n",feedback);
    strcpy(temp.feedback, feedback);
    lseek(fd, 0, SEEK_END);
    write(fd, &temp, sizeof(Feedback));
    release_lock(fd, pid);
    close(fd);
    return 0;
}

int get_feedbacks(int socket_conn) {
    int fd = open("data/feedbacks.db", O_RDONLY);
    if (fd == -1) {
        perror("Error opening file");
        return -1;
    }
    int pid = getpid();
    acquire_read_lock(fd, pid);
    Feedback temp;
    char buffer[2048];
    while (read(fd, &temp, sizeof(Feedback)) > 0) {
        snprintf(buffer, sizeof(buffer), "Feedback ID : %d  User ID : %d Timestamp : %sFeedback : %s \nN", temp.feedback_id, temp.user_id, ctime(&temp.timestamp), temp.feedback);
        write(socket_conn, buffer, strlen(buffer));
        memset(buffer, 0, sizeof(buffer));
    }
    release_lock(fd, pid);
    close(fd);
    return 0;
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
    temp.application_id = random_id();
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

int get_loan_applications(int socket_conn, int emp_id) {
    int fd = open("data/loans.db", O_RDONLY);
    if (fd == -1) {
        perror("Error opening file");
        return -1;
    }
    int pid = getpid();
    acquire_read_lock(fd, pid);
    Loan temp;
    char buffer[2048];
    const char* loanStatusNames[] = {
    "LOAN_PENDING",
    "LOAN_ASSIGNED",
    "LOAN_APPROVED",
    "LOAN_REJECTED"
    };
    while (read(fd, &temp, sizeof(Loan)) > 0) {
        if(emp_id != 0 && temp.employee_id != emp_id){
            continue;
        }
        snprintf(buffer, sizeof(buffer), "Application ID : %d  User ID : %d Amount : %.2f Purpose : %s Status : %s Application Date : %s \nN", temp.application_id, temp.user_id, temp.amount, temp.purpose, loanStatusNames[temp.status], ctime(&temp.application_date));
        write(socket_conn, buffer, strlen(buffer));
        memset(buffer, 0, sizeof(buffer));
    }
    release_lock(fd, pid);
    close(fd);
    return 0;
}

int update_loan_status(int application_id, int emp_id, LoanStatus status) {
    int fd = open("data/loans.db", O_RDWR);
    if (fd == -1) {
        perror("Error opening file");
        return -1;
    }
    int pid = getpid();
    acquire_write_lock(fd, pid);
    Loan temp;
    while (read(fd, &temp, sizeof(Loan)) > 0) {
        if (temp.application_id == application_id) {
            if(status == LOAN_ASSIGNED){
                temp.employee_id = emp_id;
                temp.status = status;
                temp.assignment_date = time(NULL);
            }
            else if(status == LOAN_APPROVED){
                temp.decision_date = time(NULL);
                User user;
                get_user_by_id(temp.user_id, &user);
                user.balance += temp.amount;
                update_user_by_location(user);
                add_transaction(user, LOAN_DISBURSEMENT, temp.amount, SUCCESS, 0, 0);
                temp.status = status;
            }
            else if(status == LOAN_REJECTED){
                temp.decision_date = time(NULL);
                temp.status = status;
            }
            lseek(fd, -sizeof(Loan), SEEK_CUR);
            write(fd, &temp, sizeof(Loan));
            release_lock(fd, pid);
            close(fd);
            return 0;
        }
    }
    release_lock(fd, pid);
    close(fd);
    return -1;
}

int next_available_user_id() {
    int fd = open("data/users.db", O_RDONLY);
    if (fd == -1) {
        perror("Error opening file");
        return -1;
    }
    int pid = getpid();
    acquire_read_lock(fd, pid);
    User temp;
    int max_id = 0;
    while (read(fd, &temp, sizeof(User)) > 0) {
        if (temp.user_id > max_id) {
            max_id = temp.user_id;
        }
    }
    release_lock(fd, pid);
    close(fd);
    return max_id + 1;
}

int add_user(User new_user, Role role) {
    int fd = open("data/users.db", O_RDWR);
    if (fd == -1) {
        perror("Error opening file");
        return -1;
    }
    printf("UTILS_ADD_USER : Adding user %s\n",new_user.name);
    printf("UTILS_ADD_USER : Password |%s|\n",new_user.password);
    int pid = getpid();
    new_user.user_id = next_available_user_id();
    new_user.role = role;
    new_user.created_at = time(NULL);
    new_user.updated_at = time(NULL);
    new_user.active = 1;
    new_user.balance = 0;
    new_user.db_index = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_END);
    acquire_write_lock(fd, pid);
    write(fd, &new_user, sizeof(User));
    release_lock(fd, pid);
    close(fd);
    return new_user.user_id;
}

int wrapper_change_password(int socket_conn, User *current_user) {
    char buffer[1024];
    const char *password_prompt = "Enter new password: ";
    write(socket_conn, password_prompt, strlen(password_prompt));
    memset(&buffer, 0, sizeof(buffer));
    read(socket_conn, &buffer, sizeof(buffer));
    sha256_hash(buffer, current_user->password);
    if(update_user_by_location(*current_user)==-1){
        const char *password_status = "Error : Password change failed!\n\nN";
        write(socket_conn, password_status, strlen(password_status));
        return -1;
    }
    const char *password_status = "Password changed successfully!\n\nN";
    write(socket_conn, password_status, strlen(password_status));
    return 0;
}

int wrapper_logout(int socket_conn, Session *current_session) {
    end_session(*current_session);
    const char *logout_status = "LOGOUT\n";
    write(socket_conn, logout_status, strlen(logout_status));
    end_session(*current_session);
    return 0;
}

int wrapper_exit(int socket_conn , Session *current_session) {
    const char *exit_status = "exit_client\n";
    write(socket_conn, exit_status, strlen(exit_status));
    end_session(*current_session);
    return 0;
}