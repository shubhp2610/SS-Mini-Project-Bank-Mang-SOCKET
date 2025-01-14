#include "manager.h"

void activate_deactivate_customer_accounts(int socket_conn){
    write_line(socket_conn,"Enter user_id to activate/deactivate: ");
    int activate_user_id = read_int(socket_conn);
    User activate_user;
    if(get_user_by_id(activate_user_id, &activate_user) == -1){
        write_line(socket_conn,"Error : User not found!\n\nN");
        return;
    }
    if(activate_user.role != CUSTOMER){
        write_line(socket_conn,"Error : You can only activate/deactivate customer accounts!\n\nN");
        return;
    }
    int fd = open("data/users.db", O_RDWR);
    if (fd == -1) {
        perror("Error opening file");
        return;
    }
    acquire_write_lock_partial(fd, getpid(), activate_user.db_index, sizeof(User));
    write_line(socket_conn,"Enter 1 to activate, 0 to deactivate: ");
    int activate_choice = read_int(socket_conn);
    if(activate_choice != 1 && activate_choice != 0){
        write_line(socket_conn,"Error : Invalid choice!\n\nN");
        release_lock(fd, getpid());
        close(fd);
        return;
    }
    else{
        activate_user.active = activate_choice;
    }
    if(update_user_by_location(fd,activate_user)==-1){
        write_line(socket_conn,"Error : Account activation/deactivation failed!\n\nN");
        release_lock(fd, getpid());
        close(fd);
        return;
    }
    release_lock(fd, getpid());
    close(fd);
    if(activate_choice == 1){
        write_line(socket_conn,"Account activated successfully!\n\nN");
    }else{
        write_line(socket_conn,"Account deactivated successfully!\n\nN");
    }
}

void assign_loan_application_processes(int socket_conn){
    if(get_loan_applications(socket_conn,0)==0){
        write_line(socket_conn,"No loan applications to assign!\n\nN");
        return;
    }
    write_line(socket_conn,"Enter application_id to assign: ");
    int assign_application_id = read_int(socket_conn);
    write_line(socket_conn,"Enter employee_id to assign: ");
    int assign_emp_id = read_int(socket_conn);
    int fd = open("data/loans.db", O_RDWR);
    if (fd == -1) {
        perror("Error opening file");
        return;
    }
    acquire_write_lock(fd, getpid());
    User assign_emp;
    if(get_user_by_id(assign_emp_id, &assign_emp) == -1 || assign_emp.active == 0 || assign_emp.role != EMPLOYEE){
        write_line(socket_conn,"Error : Employee not found!\n\nN");
        release_lock(fd, getpid());
        close(fd);
        return;
    }
    if(update_loan_status(fd,assign_application_id, assign_emp_id, LOAN_ASSIGNED)==-1){
        write_line(socket_conn,"Error : Loan assignment failed!\n\nN");
        release_lock(fd, getpid());
        close(fd);
        return;
    }
    write_line(socket_conn,"Loan application assigned successfully!\n\nN");
    release_lock(fd, getpid());
    close(fd);
}


void review_customer_feedback(int socket_conn) {
    int fd = open("data/feedbacks.db", O_RDONLY);
    if (fd == -1) {
        perror("Error opening file");
        return;
    }
    int pid = getpid();
    acquire_read_lock(fd, pid);
    Feedback temp;
    char buffer[MAX_BUFFER_SIZE];
    write_line(socket_conn, "Feedback ID\tUser ID\tTimestamp\t\t\tFeedback\nN");
    memset(buffer, 0, sizeof(buffer));
    while (read(fd, &temp, sizeof(Feedback)) > 0) {
        char* timestamp = ctime(&temp.timestamp);
        timestamp[strcspn(timestamp, "\n")] = '\0'; 
        snprintf(buffer, sizeof(buffer), "%d\t\t%d\t%s\t%s\nN", temp.feedback_id, temp.user_id, timestamp, temp.feedback);
        write(socket_conn, buffer, strlen(buffer));
        memset(buffer, 0, sizeof(buffer));
    }
    release_lock(fd, pid);
    close(fd);
}