#include "manager.h"

void activate_deactivate_customer_accounts(int socket_conn){
    write_line(socket_conn,"Enter user_id to activate/deactivate: ");
    int activate_user_id = read_int(socket_conn);
    User activate_user;
    if(get_user_by_id(activate_user_id, &activate_user) == -1 || activate_user.active == 0){
        write_line(socket_conn,"Error : User not found!\n\nN");
        return;
    }
    if(activate_user.role != CUSTOMER){
        write_line(socket_conn,"Error : You can only activate/deactivate customer accounts!\n\nN");
        return;
    }
    write_line(socket_conn,"Enter 1 to activate, 0 to deactivate: ");
    int activate_choice = read_int(socket_conn);
    if(activate_choice == 1){
        activate_user.active = 1;
    }else if(activate_choice == 0){
        activate_user.active = 0;
    }else{
        write_line(socket_conn,"Error : Invalid choice!\n\nN");
        return;
    }
    if(update_user_by_location(activate_user)==-1){
        write_line(socket_conn,"Error : Account activation/deactivation failed!\n\nN");
    }
    if(activate_choice == 1){
        write_line(socket_conn,"Account activated successfully!\n\nN");
    }else{
        write_line(socket_conn,"Account deactivated successfully!\n\nN");
    }
}

void assign_loan_application_processes(int socket_conn){
    get_loan_applications(socket_conn,0);
    write_line(socket_conn,"Enter application_id to assign: ");
    int assign_application_id = read_int(socket_conn);
    write_line(socket_conn,"Enter employee_id to assign: ");
    int assign_emp_id = read_int(socket_conn);
    User assign_emp;
    if(get_user_by_id(assign_emp_id, &assign_emp) == -1 || assign_emp.active == 0 || assign_emp.role != EMPLOYEE){
        write_line(socket_conn,"Error : Employee not found!\n\nN");
        return;
    }
    if(update_loan_status(assign_application_id, assign_emp_id, LOAN_ASSIGNED)==-1){
        write_line(socket_conn,"Error : Loan assignment failed!\n\nN");
        return;
    }
    write_line(socket_conn,"Loan application assigned successfully!\n\nN");
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
    while (read(fd, &temp, sizeof(Feedback)) > 0) {
        snprintf(buffer, sizeof(buffer), "Feedback ID : %d  User ID : %d Timestamp : %sFeedback : %s \nN", temp.feedback_id, temp.user_id, ctime(&temp.timestamp), temp.feedback);
        write(socket_conn, buffer, strlen(buffer));
        memset(buffer, 0, sizeof(buffer));
    }
    release_lock(fd, pid);
    close(fd);
}