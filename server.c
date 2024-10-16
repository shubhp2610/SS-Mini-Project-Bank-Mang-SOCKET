#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "utils.h"
#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    int PORT = atoi(argv[1]);
    int server_fd, new_socket;
    struct sockaddr_in address;

    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE];
    ssize_t read_bytes;
    // Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }
    // Configure address structure
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    random_init();
    //fork with semafore to maintain 5 clients
    while(1){
        // Accept a connection
        new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        if (new_socket < 0) {
            perror("Accept failed");
            close(server_fd);
            exit(EXIT_FAILURE);
        }
        if(!fork()){
            // Send welcome message to client
            while(1){
            write_line(new_socket, "Welcome! Please enter your user_id : ");
            int user_id;
            read(new_socket, &user_id, sizeof(user_id));
            printf("Received user_id: %d\n", user_id);            
            write_line(new_socket, "Enter password: ");
            // Read password from client
            char password[200];
            read_bytes = read(new_socket, password, 200);
            password[read_bytes] = '\0';
            printf("Received password: %s|\n", password);
            
            User current_user;
            Session current_session;
            if (get_user_by_id(user_id, &current_user) == -1 || current_user.active == 0 || !is_password_correct(password, current_user.password)) {
                write_line(new_socket, "Error : Authentication failed!");
                close(new_socket);
                exit(0);
            }
            // if(start_session(current_user,&current_session) == -1){
            //     const char *auth_status = "Error : Only one session allowed at a time!";
            //     write(new_socket, auth_status, strlen(auth_status));
            //     close(new_socket);
            //     exit(0);
            // }
            char auth_status[BUFFER_SIZE]; // Make sure this is large enough
            snprintf(auth_status, sizeof(auth_status), "Authentication successful. Welcome %s!\n\n", current_user.name);
            write_line(new_socket, auth_status);
            // Read and write data to client
                switch(current_user.role){
                    case CUSTOMER:
                    while (1) {
                        printf("inside customer\n");
                        memset(&buffer, 0, sizeof(buffer));
                        write_line(new_socket, "1. View Balance\n2. Deposit\n3. Withdraw\n4. Transfer\n5. Apply for Loan\n6. Change Password\n7. Provide Feedback\n8. View Transactions\n9. Logout\n10. EXIT\nEnter choice: \0");
                        int choice = read_int(new_socket);
                        printf("Received choice: %d\n", choice);
                        
                        switch (choice) {
                            case 1: {
                                get_user_by_location(&current_user); // Fetch user info
                                memset(buffer, 0, sizeof(buffer));
                                snprintf(buffer, sizeof(buffer), "Your account balance is %.2f\n\n\nN", current_user.balance);
                                write_line(new_socket, buffer);
                                break;
                            }
                            case 2:
                                write_line(new_socket, "Enter amount to deposit: ");
                                double deposit_amount = read_double(new_socket);
                                if(deposit_amount <= 0){
                                    write_line(new_socket, "Error : Invalid amount!\n\nN");
                                    break;
                                }
                                current_user.balance += deposit_amount;
                                if(update_user_by_location(current_user)==-1){
                                    write_line(new_socket, "Error : Deposit failed!\n\nN");
                                }
                                get_user_by_location(&current_user);
                                add_transaction(current_user, DEPOSIT, deposit_amount, SUCCESS,0,0);
                                char deposit_status[BUFFER_SIZE];
                                snprintf(deposit_status, sizeof(deposit_status), "Deposit successful. Your new balance is %.2f\n\n\nN", current_user.balance);
                                write_line(new_socket, deposit_status);
                                break;
                            case 3:
                                write_line(new_socket, "Enter amount to withdraw: ");
                                double withdrawal_amount = read_double(new_socket);
                                if(withdrawal_amount <= 0){
                                    write_line(new_socket, "Error : Invalid amount!\n\nN");
                                    break;
                                }
                                if(withdrawal_amount > current_user.balance){
                                    write_line(new_socket, "Error : Insufficient balance!\n\nN");
                                    add_transaction(current_user, WITHDRAWAL, withdrawal_amount, INSUFFICIENT_BALANCE,0,0);
                                    break;
                                }
                                current_user.balance -= withdrawal_amount;
                                if(update_user_by_location(current_user)==-1){
                                    write_line(new_socket, "Error : Withdrawal user update failed!\n\nN");
                                    break;
                                }
                                get_user_by_location(&current_user);
                                add_transaction(current_user, WITHDRAWAL, withdrawal_amount, SUCCESS,0,0);
                                char widthraw_status[BUFFER_SIZE];
                                snprintf(widthraw_status, sizeof(widthraw_status), "Withdrawal successful. Your new balance is %.2f\n\n\nN", current_user.balance);
                                write_line(new_socket, widthraw_status);
                                break;
                            case 4:
                                printf("TRANSFER REQUEST\n");
                                write_line(new_socket, "Enter destination account number: ");
                                int destination_account = read_int(new_socket);
                                User destination_user;
                                if(get_user_by_id(destination_account, &destination_user) == -1 || destination_user.active == 0){
                                    write_line(new_socket, "Error : Destination account not found!\n\nN");
                                    add_transaction(current_user, TRANSFER, 0, DESTINATION_ACCOUNT_NOT_FOUND,current_user.user_id,destination_account);
                                    break;
                                }
                                write_line(new_socket, "Enter amount to transfer: ");
                                double transfer_amount = read_double(new_socket);
                                if(transfer_amount <= 0){
                                    write_line(new_socket, "Error : Invalid amount!\n\nN");
                                    break;
                                }
                                if(transfer_amount > current_user.balance){
                                    write_line(new_socket, "Error : Insufficient balance!\n\nN");
                                    add_transaction(current_user, TRANSFER, transfer_amount, INSUFFICIENT_BALANCE,current_user.user_id,destination_account);
                                    break;
                                }
                                if(atomic_transfer(&current_user,&destination_user,transfer_amount) == -1){
                                    write_line(new_socket, "Error : Atomic transfer failed!\n\nN");
                                }
                                get_user_by_location(&current_user);
                                get_user_by_location(&destination_user);
                                char transfer_status[BUFFER_SIZE];
                                snprintf(transfer_status, sizeof(transfer_status), "Transfer successful. Your new balance is %.2f\n\nN", current_user.balance);
                                write_line(new_socket, transfer_status);
                                break;
                            case 5:
                                printf("APPLY FOR LOAN\n");
                                write_line(new_socket,"Enter loan amount: ");
                                double loan_amount = read_double(new_socket);
                                if(loan_amount <= 0){
                                    write_line(new_socket,"Error : Invalid amount!\n\nN");
                                    break;
                                }
                                write_line(new_socket,"Enter loan purpose: ");
                                memset(&buffer, 0, sizeof(buffer));
                                read(new_socket, &buffer, sizeof(buffer));
                                int app_number = add_loan_application(current_user, loan_amount, buffer);
                                printf("app_number: %d\n", app_number);
                                if(app_number==-1){
                                    write_line(new_socket,"Error : Loan application failed!\n\nN");
                                }
                                char app_status[MAX_BUFFER_SIZE];
                                snprintf(app_status, MAX_BUFFER_SIZE, "Loan application submitted successfully. Your application number is %d\n\nN", app_number);
                                write_line(new_socket,app_status);
                                break;
                            case 6:
                                printf("CHANGE PASSWORD\n");
                                wrapper_change_password(new_socket, &current_user);
                                break;
                            case 7:
                                printf("PROVIDE FEEDBACK\n");
                                write_line(new_socket,"Enter your valuable feedback: ");
                                memset(&buffer, 0, sizeof(buffer));
                                read(new_socket, &buffer, sizeof(buffer));
                                add_feedback(current_user, buffer);
                                write_line(new_socket,"Feedback submitted successfully!\n\nN");
                                break;
                            case 8:
                                printf("inside 8\n");
                                get_transactions(new_socket,current_user.user_id);
                                break;
                            case 9:
                                printf("inside 9\n");
                                wrapper_logout(new_socket, &current_session);
                                close(new_socket);
                                exit(0);
                                break;
                            case 10:
                                printf("inside 10\n");
                                wrapper_exit(new_socket, &current_session);
                                close(new_socket);
                                exit(0);
                                break;
                            default:
                                write_line(new_socket, "Invalid choice!\n\nN");
                                break;
                        }
                    }
                    break;
                    case EMPLOYEE:
                    printf("inside employee\n");
                    while (1) {
                        write_line(new_socket,"1. Add New Customer\n2. Modify Customer Details\n3. View Assigned Loan Applications\n4. Approve/Reject Loans\n5. View Customer Transactions\n6. Change Password\n7. Logout\n8. Exit\nEnter choice: \0");
                        int choice = read_int(new_socket);
                        printf("Received choice: %d\n", choice);
                        switch (choice) {
                            case 1:
                                printf("ADDING NEW CUSTOMER\n");
                                write_line(new_socket,"Enter new customer name: ");
                                memset(&buffer, 0, sizeof(buffer));
                                User new_cust;
                                read(new_socket, &buffer, sizeof(buffer));
                                strcpy(new_cust.name, buffer);

                                memset(&buffer, 0, sizeof(buffer));
                                write_line(new_socket,"Enter new customer password: ");
                                read(new_socket, &buffer, sizeof(buffer));
                                sha256_hash(buffer, new_cust.password);
                                
                                int new_user_id = add_user(new_cust, CUSTOMER);
                                if(new_user_id == -1){
                                    write_line(new_socket,"Error : Customer addition failed!\n\nN");
                                }
                                memset(&buffer, 0, sizeof(buffer));
                                snprintf(buffer, sizeof(buffer), "Customer added successfully. Customer ID is %d\n\nN", new_user_id);
                                write_line(new_socket,buffer);
                                break;
                            case 2:
                                printf("MODIFY CUSTOMER \n");
                                write_line(new_socket,"Enter user_id to modify: ");
                                int modify_user_id = read_int(new_socket);
                                User modify_user;
                                if(get_user_by_id(modify_user_id, &modify_user) == -1 || modify_user.active == 0){
                                    write_line(new_socket,"Error : User not found!\n\nN");
                                    break;
                                }
                                if(modify_user.role != CUSTOMER){
                                    write_line(new_socket,"Error : You can only modify customer details!\n\nN");
                                    break;
                                }
                                
                                memset(&buffer, 0, sizeof(buffer));
                                write_line(new_socket,"Enter new name: ");
                                read(new_socket, &buffer, sizeof(buffer));
                                strcpy(modify_user.name, buffer);
                                memset(&buffer, 0, sizeof(buffer));
                                write_line(new_socket,"Enter new password: ");
                                read(new_socket, &buffer, sizeof(buffer));
                                sha256_hash(buffer, modify_user.password);
                                if(update_user_by_location(modify_user)==-1){
                                    write_line(new_socket,"Error : User modification failed!\n\nN");
                                    break;
                                }
                                write_line(new_socket,"User modified successfully!\n\nN");
                                break;
                            case 3:
                                printf("VIEW ASSIGNED LOAN APPLICATIONS\n");
                                get_loan_applications(new_socket,current_user.user_id);
                                break;
                            case 4:
                                printf("APPROVE/REJECT LOANS\n");
                                write_line(new_socket,"Enter application_id to approve/reject: ");
                                int approve_application_id = read_int(new_socket);
                                write_line(new_socket,"Enter 1 to approve, 0 to reject: ");
                                int approve_choice = read_int(new_socket);
                                LoanStatus status;
                                if(approve_choice == 1){
                                    status = LOAN_APPROVED;
                                }else if(approve_choice == 0){
                                    status = LOAN_REJECTED;
                                }else{
                                    write_line(new_socket,"Error : Invalid choice!\n\nN");
                                    break;
                                }
                                if(update_loan_status(approve_application_id, current_user.user_id, status)==-1){
                                    write_line(new_socket,"Error : Loan approval/rejection failed!\n\nN");
                                    break;
                                }
                                if(approve_choice == 1){
                                    write_line(new_socket,"Loan application approved successfully!\n\nN");
                                }else{
                                    write_line(new_socket,"Loan application rejected successfully!\n\nN");
                                }
                                break;
                            case 5:
                                printf("VIEW CUSTOMER TRANSACTIONS\n");
                                write_line(new_socket,"Enter user_id to view transactions: ");
                                int cust_id = read_int(new_socket);
                                get_transactions(new_socket,cust_id);
                                break;
                            case 6:
                                printf("CHANGE PASSWORD\n");
                                wrapper_change_password(new_socket, &current_user);
                                break;
                            case 7:
                                printf("inside 7\n");
                                wrapper_logout(new_socket, &current_session);
                                close(new_socket);
                                exit(0);
                                break;
                            case 8:
                                printf("inside 8\n");
                                wrapper_exit(new_socket, &current_session);
                                close(new_socket);
                                exit(0);
                                break;
                        }
                    }
                    break;
                    case MANAGER:
                    printf("inside manager\n");
                    while (1) {
                        memset(&buffer, 0, sizeof(buffer));
                        const char *customer_menu = "1. Activate/Deactivate Customer Accounts\n2. Assign Loan Application Processes to Employees\n3. Review Customer Feedback\n4. Change Password\n5. Logout\n6. Exit\nEnter choice: \0";
                        strcpy(buffer, customer_menu);
                        write(new_socket, &buffer, sizeof(buffer)); // Send the message structure to the client
                        int choice = read_int(new_socket);
                        printf("Received choice: %d\n", choice);
                        switch (choice) {
                            case 1:
                                printf("ACTIVATE/DEACTIVATE CUSTOMER ACCOUNTS\n");
                                write_line(new_socket,"Enter user_id to activate/deactivate: ");
                                int activate_user_id = read_int(new_socket);
                                User activate_user;
                                if(get_user_by_id(activate_user_id, &activate_user) == -1){
                                    write_line(new_socket,"Error : User not found!\n\nN");
                                    break;
                                }
                                if(activate_user.role != CUSTOMER){
                                    write_line(new_socket,"Error : You can only activate/deactivate customer account!\n\nN");
                                    break;
                                }
                                write_line(new_socket,"Enter 1 to activate, 0 to deactivate: ");
                                int activate_choice = read_int(new_socket);
                                if(activate_choice != 0 && activate_choice != 1){
                                    write_line(new_socket,"Error : Invalid choice!\n\nN");
                                    break;
                                }
                                activate_user.active = activate_choice;
                                if(update_user_by_location(activate_user)==-1){
                                    write_line(new_socket,"Error : Account activation/deactivation failed!\n\nN");
                                    break;
                                }
                                if(activate_choice == 0){
                                    write_line(new_socket,"Account deactivation successful!\n\nN");
                                }
                                else{
                                    write_line(new_socket,"Account activation successful!\n\nN");
                                }
                                break;
                            case 2:
                                printf("ASSIGN LOAN APPLICATIONS\n");
                                get_loan_applications(new_socket,0);
                                write_line(new_socket,"Enter application_id to assign: ");
                                int assign_application_id = read_int(new_socket);
                                write_line(new_socket,"Enter employee_id to assign: ");
                                int assign_emp_id = read_int(new_socket);
                                User assign_emp;
                                if(get_user_by_id(assign_emp_id, &assign_emp) == -1 || assign_emp.active == 0 || assign_emp.role != EMPLOYEE){
                                    write_line(new_socket,"Error : Employee not found!\n\nN");
                                    break;
                                }
                                if(update_loan_status(assign_application_id, assign_emp_id, LOAN_ASSIGNED)==-1){
                                    write_line(new_socket,"Error : Loan assignment failed!\n\nN");
                                    break;
                                }
                                write_line(new_socket,"Loan application assigned successfully!\n\nN");
                                break;
                            case 3:
                                printf("REVIEW CUSTOMER FEEDBACK\n");
                                get_feedbacks(new_socket);
                                break;
                            case 4:
                                printf("CHANGE PASSWORD\n");
                                wrapper_change_password(new_socket, &current_user);
                                break;
                            case 5:
                                printf("inside 5\n");
                                wrapper_logout(new_socket, &current_session);
                                close(new_socket);
                                exit(0);
                                break;
                            case 6:
                                printf("inside 6\n");
                                wrapper_exit(new_socket, &current_session);
                                close(new_socket);
                                exit(0);
                                break;
                        }
                    }
                    break;
                    case ADMINISTRATOR:
                    printf("inside administrator\n");
                    while (1) {
                        memset(&buffer, 0, sizeof(buffer));
                        const char *customer_menu = "1. Add New Bank Employee\n2. Modify Customer/Employee Details\n3. Manage User Roles\n4. Change Password\n5. Logout\n6. Exit\nEnter choice: \0";
                        strcpy(buffer, customer_menu);
                        write(new_socket, &buffer, sizeof(buffer)); // Send the message structure to the client
                        int choice = read_int(new_socket);
                        printf("Received choice: %d\n", choice);
                        switch (choice) {
                            case 1:
                                printf("ADDING NEW EMP\n");
                                memset(&buffer, 0, sizeof(buffer));
                                write_line(new_socket,"Enter new employee name: ");
                                User new_emp;
                                read(new_socket, &buffer, sizeof(buffer));
                                strcpy(new_emp.name, buffer);
                                memset(&buffer, 0, sizeof(buffer));
                                write_line(new_socket,"Enter new employee password: ");
                                read(new_socket, &buffer, sizeof(buffer));
                                sha256_hash(buffer, new_emp.password);
                                int new_user_id = add_user(new_emp, EMPLOYEE);
                                if(new_user_id == -1){
                                    write_line(new_socket,"Error : Employee addition failed!\n\nN");
                                    break;
                                }
                                memset(&buffer, 0, sizeof(buffer));
                                snprintf(buffer, sizeof(buffer), "Employee added successfully. Employee ID is %d\n\nN", new_user_id);
                                write_line(new_socket,buffer);
                                break;
                            case 2:
                                printf("MODIFY CUSTOMER/EMPLOYEE \n");
                                write_line(new_socket,"Enter user_id to modify: ");
                                int modify_user_id = read_int(new_socket);
                                User modify_user;
                                if(get_user_by_id(modify_user_id, &modify_user) == -1 || modify_user.active == 0){
                                    write_line(new_socket,"Error : User not found!\n\nN");
                                    break;
                                }
                                if(modify_user.role != CUSTOMER && modify_user.role != EMPLOYEE){
                                    write_line(new_socket,"Error : Cannot modify manager/administrator!\n\nN");
                                    break;
                                }
                                memset(&buffer, 0, sizeof(buffer));
                                write_line(new_socket,"Enter new name: ");
                                read(new_socket, &buffer, sizeof(buffer));
                                strcpy(modify_user.name, buffer);
                                memset(&buffer, 0, sizeof(buffer));
                                write_line(new_socket,"Enter new password: ");
                                read(new_socket, &buffer, sizeof(buffer));
                                sha256_hash(buffer, modify_user.password);
                                if(update_user_by_location(modify_user)==-1){
                                    write_line(new_socket,"Error : User modification failed!\n\nN");
                                    break;
                                }
                                write_line(new_socket,"User modified successfully!\n\nN");
                                break;
                            case 3:
                                printf("MANAGE USER ROLE\n");
                                write_line(new_socket,"Enter user_id to modify role: ");
                                int role_user_id = read_int(new_socket);
                                User role_user;
                                if(get_user_by_id(role_user_id, &role_user) == -1 || role_user.active == 0){
                                    write_line(new_socket,"Error : User not found!\n\nN");
                                    break;
                                }
                                write_line(new_socket,"Enter new role (1 for customer, 2 for employee, 3 for manager, 4 for administrator): ");
                                int new_role = read_int(new_socket);
                                if(new_role < 1 || new_role > 4){
                                    write_line(new_socket,"Error : Invalid role!\n\nN");
                                    break;
                                }
                                if(new_role == 1){role_user.role = CUSTOMER;}
                                else if(new_role == 2){role_user.role = EMPLOYEE;}
                                else if(new_role == 3){role_user.role = MANAGER;}
                                else if(new_role == 4){role_user.role = ADMINISTRATOR;}
                                if(update_user_by_location(role_user)==-1){
                                    write_line(new_socket,"Error : Role change failed!\n\nN");
                                }
                                write_line(new_socket,"Role changed successfully!\n\nN");
                                break;
                            case 4:
                                printf("CHANGE PASSWORD\n");
                                wrapper_change_password(new_socket, &current_user);
                                break;
                            case 5:
                                printf("inside 5\n");
                                wrapper_logout(new_socket, &current_session);
                                close(new_socket);
                                exit(0);
                                break;
                            case 6:
                                printf("inside 6\n");
                                wrapper_exit(new_socket, &current_session);
                                close(new_socket);
                                exit(0);
                                break;
                            default:
                                printf("inside default\n");
                                break;
                        }
                    }
                    break;
                    default:
                    printf("inside default\n");
                    break;
                }
            }
            close(new_socket);
            exit(0);
        }else{
            close(new_socket);
        }
    }
    close(server_fd);
    return 0;
}
