#include "employee.h"

void add_new_customer(int socket_conn){
    char buffer[MAX_BUFFER_SIZE];
    write_line(socket_conn,"Enter new customer name: ");
    memset(&buffer, 0, sizeof(buffer));
    User new_cust;
    read(socket_conn, &buffer, sizeof(buffer));
    strcpy(new_cust.name, buffer);

    memset(&buffer, 0, sizeof(buffer));
    write_line(socket_conn,"Enter new customer password: ");
    read(socket_conn, &buffer, sizeof(buffer));
    sha256_hash(buffer, new_cust.password);
    
    int new_user_id = add_user(new_cust, CUSTOMER);
    if(new_user_id == -1){
        write_line(socket_conn,"Error : Customer addition failed!\n\nN");
    }
    memset(&buffer, 0, sizeof(buffer));
    snprintf(buffer, sizeof(buffer), "Customer added successfully. Customer ID is %d\n\nN", new_user_id);
    write_line(socket_conn,buffer);
}

void modify_customer_details(int socket_conn){
    char buffer[MAX_BUFFER_SIZE];
    write_line(socket_conn,"Enter user_id to modify: ");
    int modify_user_id = read_int(socket_conn);
    User modify_user;
    if(get_user_by_id(modify_user_id, &modify_user) == -1 || modify_user.active == 0){
        write_line(socket_conn,"Error : User not found!\n\nN");
        return;
    }
    if(modify_user.role != CUSTOMER){
        write_line(socket_conn,"Error : You can only modify customer details!\n\nN");
        return;
    }
    int fd = open("data/users.db", O_RDWR);
    if (fd == -1) {
        perror("Error opening file");
        return;
    }
    acquire_write_lock_partial(fd, getpid(), modify_user.db_index, sizeof(User));
    memset(&buffer, 0, sizeof(buffer));
    write_line(socket_conn,"Enter new name: ");
    read(socket_conn, &buffer, sizeof(buffer));
    strcpy(modify_user.name, buffer);
    memset(&buffer, 0, sizeof(buffer));
    write_line(socket_conn,"Enter new password: ");
    read(socket_conn, &buffer, sizeof(buffer));
    sha256_hash(buffer, modify_user.password);
    if(update_user_by_location(fd,modify_user)==-1){
        write_line(socket_conn,"Error : User modification failed!\n\nN");
        release_lock(fd, getpid());
        close(fd);
        return;
    }
    write_line(socket_conn,"User modified successfully!\n\nN");
    release_lock(fd, getpid());
    close(fd);
}

void view_assigned_loan_applications(int socket_conn, int emp_id) {
    if(get_loan_applications(socket_conn, emp_id)==0){
        write_line(socket_conn,"No loan applications assigned!\n\nN");
    }
    return;
}

void approve_reject_loans(int socket_conn, int emp_id){
    write_line(socket_conn,"Enter application_id to approve/reject: ");
    int approve_application_id = read_int(socket_conn);
    write_line(socket_conn,"Enter 1 to approve, 0 to reject: ");
    int approve_choice = read_int(socket_conn);
    int fd = open("data/loans.db", O_RDWR);
    if (fd == -1) {
        perror("Error opening file");
        return;
    }
    acquire_write_lock(fd, getpid());
    if(approve_choice != 1 && approve_choice !=0 ){
        write_line(socket_conn,"Error : Invalid choice!\n\nN");
        release_lock(fd, getpid());
        close(fd);
        return;
    }
    printf("approve_choice : %d\n",approve_choice);
    printf("approve_application_id : %d\n",approve_application_id);
    LoanStatus status;
    if(approve_choice==1){status =LOAN_APPROVED;}
    if(approve_choice==0){status =LOAN_REJECTED;}
    if(update_loan_status(fd,approve_application_id, emp_id, status)==-1){
        write_line(socket_conn,"Error : Loan approval/rejection failed!\n\nN");
        release_lock(fd, getpid());
        close(fd);
        return;
    }
    release_lock(fd, getpid());
    close(fd);
    if(approve_choice == 1){
        write_line(socket_conn,"Loan application approved successfully!\n\nN");
    }else{
        write_line(socket_conn,"Loan application rejected successfully!\n\nN");
    }
}

void view_customer_transactions(int socket_conn){
    write_line(socket_conn,"Enter user_id to view transactions: ");
    int cust_id = read_int(socket_conn);
    get_transactions(socket_conn,cust_id);
}

