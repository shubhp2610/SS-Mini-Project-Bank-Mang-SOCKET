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

int update_user_by_location(int fd, User user) {
    lseek(fd, user.db_index, SEEK_SET);
    write(fd, &user, sizeof(User));
    return 0;
}

int start_session(User user, Session *session) {
    int fd = open("data/sessions.db", O_RDWR);
    if (fd == -1) {
        perror("Error opening file");
        return -1;
    }
    int pid = getpid();
    int new_session_id = 1;
    acquire_write_lock(fd, pid);
    Session temp;
    while (read(fd, &temp, sizeof(Session)) > 0) {
        if (temp.session_id >= new_session_id) {
            new_session_id = temp.session_id + 1;
        }
        if (temp.user_id == user.user_id && temp.active && temp.login_time + SESSION_TIMEOUT > time(NULL)) {
            printf("UTILS_START_SESSION : Session already exists for user %d\n",user.user_id);
            release_lock(fd, pid);
            close(fd);
            return -1;
        }
    }
    memset(&temp, 0, sizeof(Session));
    session->active = 1;
    session->login_time = time(NULL);
    session->logout_time = 0;
    session->session_id = new_session_id;
    session->user_id = user.user_id;
    session->db_index = lseek(fd, 0, SEEK_END); // store the start position
    lseek(fd, 0, SEEK_END);
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

int next_available_transaction_id(int fd) {
    Transaction temp;
    int max_id = 0;
    while (read(fd, &temp, sizeof(Transaction)) > 0) {
        if (temp.trx_id > max_id) {
            max_id = temp.trx_id;
        }
    }
    return max_id + 1;
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
    temp.trx_id = next_available_transaction_id(fd);
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
    "LOAN_DISBURSEMENT"
    };
    const char* transactionStatusNames[] = {
    "DESTINATION_ACCOUNT_NOT_FOUND",
    "INSUFFICIENT_BALANCE",
    "SUCCESS"
    };
    write_line(socket_conn,"Transaction ID Amount     New Balance  Type           Status              To Account    From Account  Timestamp\nN");
    while (read(fd, &temp, sizeof(Transaction)) > 0) {
        if (temp.user_id == user_id) {
            if(temp.type == TRANSFER){
            snprintf(buffer, sizeof(buffer), "%-14d %-10.2f %-12.2f %-14s %-20s %-12d %-14d %sN", temp.trx_id, temp.amount,temp.balance, transactionTypeNames[temp.type], transactionStatusNames[temp.status],temp.to_account,temp.from_account,ctime(&temp.timestamp));
            }else{
            snprintf(buffer, sizeof(buffer), "%-14d %-10.2f %-12.2f %-14s %-20s %26s %sN", temp.trx_id, temp.amount,temp.balance, transactionTypeNames[temp.type], transactionStatusNames[temp.status],"",ctime(&temp.timestamp));
            }
            write(socket_conn, buffer, strlen(buffer));
            memset(buffer, 0, sizeof(buffer));
        }
    }
    release_lock(fd, pid);
    close(fd);
    return 0;
}

int get_loan_applications(int socket_conn, int emp_id) {
    int count = 0;
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
    write_line(socket_conn,"App ID\tUser ID\tEmp ID\tAmount\tPurpose\t\tStatus\t\tApplication Date\tAssignment Date\tDecision Date\nN");
    memset(buffer, 0, sizeof(buffer));
    while (read(fd, &temp, sizeof(Loan)) > 0) {
        if(emp_id != 0 && temp.employee_id != emp_id){
            continue;
        }
        count++;
        char application_date[50];
        strcpy(application_date,ctime(&temp.application_date));
        application_date[strcspn(application_date, "\n")] = '\0';
        char assigned_date[50];
        if(temp.assignment_date == 0){
            strcpy(assigned_date,"-");
        }else{
        strcpy(assigned_date,ctime(&temp.assignment_date));
        assigned_date[strcspn(assigned_date, "\n")] = '\0';
        }
        char decision_date[50];
        if(temp.decision_date == 0){
            strcpy(decision_date,"-");
        }else{
        strcpy(decision_date,ctime(&temp.decision_date));
        decision_date[strcspn(decision_date, "\n")] = '\0';
        }
        snprintf(buffer, sizeof(buffer), "%d\t%d\t%d\t%.2f\t%s\t%s\t%s\t%s\t%s\nN", temp.application_id, temp.user_id,temp.employee_id, temp.amount, temp.purpose, loanStatusNames[temp.status], application_date,assigned_date,decision_date);
        write(socket_conn, buffer, strlen(buffer));
        memset(buffer, 0, sizeof(buffer));
    }
    release_lock(fd, pid);
    close(fd);
    return count;
}

int update_loan_status(int fd, int application_id, int emp_id, LoanStatus status) {
    Loan temp;
    lseek(fd, 0, SEEK_SET);
    while (read(fd, &temp, sizeof(Loan)) > 0) {
        if (temp.application_id == application_id) {
            if(status == LOAN_ASSIGNED){
                temp.employee_id = emp_id;
                temp.status = status;
                temp.assignment_date = time(NULL);
            }
            else if(status == LOAN_APPROVED){
                int user_fd = open("data/users.db", O_RDWR);
                if (user_fd == -1) {
                    perror("Error opening file");
                    return -1;
                }
                acquire_write_lock(user_fd, getpid());
                temp.decision_date = time(NULL);
                User user;
                get_user_by_id(temp.user_id, &user);
                user.balance += temp.amount;
                update_user_by_location(user_fd,user);
                add_transaction(user, LOAN_DISBURSEMENT, temp.amount, SUCCESS, 0, 0);
                temp.status = status;
                release_lock(user_fd, getpid());
                close(user_fd);
            }
            else if(status == LOAN_REJECTED){
                temp.decision_date = time(NULL);
                temp.status = status;
            }
            lseek(fd, -sizeof(Loan), SEEK_CUR);
            write(fd, &temp, sizeof(Loan));
            return 0;
        }
    }
    return -1;
}

int next_available_user_id(int fd) {
    User temp;
    int max_id = 0;
    while (read(fd, &temp, sizeof(User)) > 0) {
        if (temp.user_id > max_id) {
            max_id = temp.user_id;
        }
    }
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
    acquire_write_lock(fd, pid);
    new_user.user_id = next_available_user_id(fd);
    new_user.role = role;
    new_user.created_at = time(NULL);
    new_user.updated_at = time(NULL);
    new_user.active = 1;
    new_user.balance = 0;
    new_user.db_index = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_END);
    write(fd, &new_user, sizeof(User));
    release_lock(fd, pid);
    close(fd);
    return new_user.user_id;
}

int wrapper_change_password(int socket_conn, User *current_user) {
    char buffer[1024];
    write_line(socket_conn, "Enter current password: ");
    memset(&buffer, 0, sizeof(buffer));
    read(socket_conn, &buffer, sizeof(buffer));
    if(!is_password_correct(buffer, current_user->password)){
        write_line(socket_conn, "Error : Incorrect password!\n\nN");
        return -1;
    }
    int fd = open("data/users.db", O_RDWR);
    if (fd == -1) {
        perror("Error opening file");
        return -1;
    }
    acquire_write_lock_partial(fd, getpid(), current_user->db_index, sizeof(User));
    write_line(socket_conn, "Enter new password: ");
    memset(&buffer, 0, sizeof(buffer));
    read(socket_conn, &buffer, sizeof(buffer));
    sha256_hash(buffer, current_user->password);
    if(update_user_by_location(fd,*current_user)==-1){
        const char *password_status = "Error : Password change failed!\n\nN";
        write(socket_conn, password_status, strlen(password_status));
        release_lock(fd, getpid());
        close(fd);
        return -1;
    }
    release_lock(fd, getpid());
    close(fd);
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