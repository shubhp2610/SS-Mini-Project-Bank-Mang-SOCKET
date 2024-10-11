#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "utils.h"
//#define PORT 8080
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
            const char *welcome_message = "Welcome! Please enter your user_id : ";
            write(new_socket, welcome_message, strlen(welcome_message));
            // Read user_id from client
            int user_id;
            read(new_socket, &user_id, sizeof(user_id));
            printf("Received user_id: %d\n", user_id);
            // Send password prompt to client
            const char *password_prompt = "Enter password: ";
            write(new_socket, password_prompt, strlen(password_prompt));
            // Read password from client
            char password[200];
            read_bytes = read(new_socket, password, 200);
            password[read_bytes] = '\0';
            printf("Received password: %s|\n", password);
            // Send authentication status to client
            User current_user;
            Session current_session;
            if (get_user_by_id(user_id, &current_user) == -1 || current_user.active == 0 || !is_password_correct(password, current_user.password)) {
                const char *auth_status = "Error : Authentication failed!";
                write(new_socket, auth_status, strlen(auth_status));
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
            write(new_socket, auth_status, strlen(auth_status));
            // Read and write data to client
                switch(current_user.role){
                    case CUSTOMER:
                    while (1) {
                        printf("inside customer\n");
                        memset(&buffer, 0, sizeof(buffer));
                        const char *customer_menu = "1. View Balance\n2. Deposit\n3. Withdraw\n4. Transfer\n5. Apply for Loan\n6. Change Password\n7. Provide Feedback\n8. View Transactions\n9. Logout\n10. EXIT\nEnter choice: \0";
                        strcpy(buffer, customer_menu);
                        write(new_socket, &buffer, sizeof(buffer)); // Send the message structure to the client

                        memset(&buffer, 0, sizeof(buffer));
                        
                        read(new_socket, &buffer, sizeof(buffer)); // Wait for client's choice
                        int choice = atoi(buffer);
                        printf("Received choice: %d\n", choice);
                        
                        switch (choice) {
                            case 1: {
                                get_user_by_location(&current_user); // Fetch user info
                                char balance[BUFFER_SIZE];
                                memset(buffer, 0, sizeof(buffer));
                                snprintf(balance, sizeof(balance), "Your account balance is %.2f\n\n\nN", current_user.balance);
                                strcpy(buffer, balance);
                                write(new_socket, &buffer, sizeof(buffer)); // Send balance message
                                break;
                            }
                            case 2:
                                const char *deposit_prompt = "Enter amount to deposit: ";
                                write(new_socket, deposit_prompt, strlen(deposit_prompt));
                                memset(&buffer, 0, sizeof(buffer));
                                double deposit_amount;
                                read(new_socket, &buffer, sizeof(buffer));
                                deposit_amount = atof(buffer);
                                if(deposit_amount <= 0){
                                    const char *deposit_status = "Error : Invalid amount!\n\nN";
                                    write(new_socket, deposit_status, strlen(deposit_status));
                                    break;
                                }
                                current_user.balance += deposit_amount;
                                if(update_user_by_location(current_user)==-1){
                                    const char *deposit_status = "Error : Deposit failed!\n\nN";
                                    write(new_socket, deposit_status, strlen(deposit_status));
                                }
                                get_user_by_location(&current_user);
                                add_transaction(current_user, DEPOSIT, deposit_amount, SUCCESS,0,0);
                                char deposit_status[BUFFER_SIZE];
                                snprintf(deposit_status, sizeof(deposit_status), "Deposit successful. Your new balance is %.2f\n\n\nN", current_user.balance);
                                write(new_socket, deposit_status, strlen(deposit_status));
                                break;
                            case 3:
                                const char *withdrawal_prompt = "Enter amount to withdraw: ";
                                write(new_socket, withdrawal_prompt, strlen(withdrawal_prompt));
                                memset(&buffer, 0, sizeof(buffer));
                                double withdrawal_amount;
                                read(new_socket, &buffer, sizeof(buffer));
                                withdrawal_amount = atof(buffer);
                                if(withdrawal_amount <= 0){
                                    const char *withdrawal_status = "Error : Invalid amount!\n\nN";
                                    write(new_socket, withdrawal_status, strlen(withdrawal_status));
                                    break;
                                }
                                if(withdrawal_amount > current_user.balance){
                                    const char *withdrawal_status = "Error : Insufficient balance!\n\nN";
                                    add_transaction(current_user, WITHDRAWAL, withdrawal_amount, INSUFFICIENT_BALANCE,0,0);
                                    write(new_socket, withdrawal_status, strlen(withdrawal_status));
                                    break;
                                }
                                current_user.balance -= withdrawal_amount;
                                if(update_user_by_location(current_user)==-1){
                                    const char *withdrawal_status = "Error : Withdrawal failed!\n\nN";
                                    write(new_socket, withdrawal_status, strlen(withdrawal_status));
                                }
                                get_user_by_location(&current_user);
                                add_transaction(current_user, WITHDRAWAL, withdrawal_amount, SUCCESS,0,0);
                                memset(&buffer, 0, sizeof(buffer));
                                snprintf(buffer, sizeof(buffer), "Withdrawal successful. Your new balance is %.2f\n\n\nN", current_user.balance);
                                write(new_socket, buffer, strlen(buffer));
                                break;
                            case 4:
                                printf("TRANSFER REQUEST\n");
                                const char* transfer_prompt = "Enter destination account number: ";
                                write(new_socket, transfer_prompt, strlen(transfer_prompt));
                                memset(&buffer, 0, sizeof(buffer));
                                int destination_account;
                                read(new_socket, &buffer, sizeof(buffer));
                                destination_account = atoi(buffer);
                                User destination_user;
                                if(get_user_by_id(destination_account, &destination_user) == -1 || destination_user.active == 0){
                                    const char *transfer_status = "Error : Destination account not found!\n\nN";
                                    add_transaction(current_user, TRANSFER, 0, DESTINATION_ACCOUNT_NOT_FOUND,current_user.user_id,destination_account);
                                    write(new_socket, transfer_status, strlen(transfer_status));
                                    break;
                                }
                                const char* transfer_amount_prompt = "Enter amount to transfer: ";
                                write(new_socket, transfer_amount_prompt, strlen(transfer_amount_prompt));
                                memset(&buffer, 0, sizeof(buffer));
                                double transfer_amount;
                                read(new_socket, &buffer, sizeof(buffer));
                                transfer_amount = atof(buffer);
                                if(transfer_amount <= 0){
                                    const char *transfer_status = "Error : Invalid amount!\n\nN";
                                    write(new_socket, transfer_status, strlen(transfer_status));
                                    break;
                                }
                                if(transfer_amount > current_user.balance){
                                    const char *transfer_status = "Error : Insufficient balance!\n\nN";
                                    add_transaction(current_user, TRANSFER, transfer_amount, INSUFFICIENT_BALANCE,current_user.user_id,destination_account);
                                    write(new_socket, transfer_status, strlen(transfer_status));
                                    break;
                                }
                                //TODO: MAKE ATOMIC
                                current_user.balance -= transfer_amount;
                                destination_user.balance += transfer_amount;
                                if(update_user_by_location(current_user)==-1 || update_user_by_location(destination_user)==-1){
                                    const char *transfer_status = "Error : Transfer failed!\n\nN";
                                    write(new_socket, transfer_status, strlen(transfer_status));
                                }
                                get_user_by_location(&current_user);
                                get_user_by_location(&destination_user);
                                add_transaction(current_user, TRANSFER, transfer_amount, SUCCESS,current_user.user_id, destination_user.user_id);
                                add_transaction(destination_user, TRANSFER, transfer_amount, SUCCESS,current_user.user_id, destination_user.user_id);
                                snprintf(buffer, sizeof(buffer), "Transfer successful. Your new balance is %.2f\n\nN", current_user.balance);
                                write(new_socket, buffer, strlen(buffer));
                                break;
                            case 5:
                                printf("APPLY FOR LOAN\n");
                                const char *loan_prompt = "Enter loan amount: ";
                                write(new_socket, loan_prompt, strlen(loan_prompt));
                                memset(&buffer, 0, sizeof(buffer));
                                double loan_amount;
                                read(new_socket, &buffer, sizeof(buffer));
                                loan_amount = atof(buffer);
                                if(loan_amount <= 0){
                                    const char *loan_status = "Error : Invalid amount!\n\nN";
                                    write(new_socket, loan_status, strlen(loan_status));
                                    break;
                                }
                                const char *loan_purpose_prompt = "Enter loan purpose: ";
                                write(new_socket, loan_purpose_prompt, strlen(loan_purpose_prompt));
                                memset(&buffer, 0, sizeof(buffer));
                                read(new_socket, &buffer, sizeof(buffer));
                                int app_number = add_loan_application(current_user, loan_amount, buffer);
                                printf("app_number: %d\n", app_number);
                                if(app_number==-1){
                                    const char *loan_status = "Error : Loan application failed!\n\nN";
                                    write(new_socket, loan_status, strlen(loan_status));
                                }
                                snprintf(buffer, sizeof(buffer), "Loan application submitted successfully. Your application number is %d\n\nN", app_number);
                                write(new_socket, buffer, strlen(buffer));
                                break;
                            case 6:
                                printf("CHANGE PASSWORD\n");
                                wrapper_change_password(new_socket, &current_user);
                                break;
                            case 7:
                                printf("PROVIDE FEEDBACK\n");
                                const char *feedback_prompt = "Enter your valuable feedback: ";
                                write(new_socket, feedback_prompt, strlen(feedback_prompt));
                                memset(&buffer, 0, sizeof(buffer));
                                read(new_socket, &buffer, sizeof(buffer));
                                add_feedback(current_user, buffer);
                                const char *feedback_status = "Feedback submitted successfully!\n\nN";
                                write(new_socket, feedback_status, strlen(feedback_status));
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
                                const char *invalid_choice = "Invalid choice!\n\nN";
                                write(new_socket, invalid_choice, strlen(invalid_choice));
                                break;
                        }
                    }
                    break;
                    case EMPLOYEE:
                    printf("inside employee\n");
                    while (1) {
                        memset(&buffer, 0, sizeof(buffer));
                        const char *customer_menu = "1. Add New Customer\n2. Modify Customer Details\n3. View Assigned Loan Applications\n4. Approve/Reject Loans\n5. View Customer Transactions\n6. Change Password\n7. Logout\n8. Exit\nEnter choice: \0";
                        strcpy(buffer, customer_menu);
                        write(new_socket, &buffer, sizeof(buffer)); // Send the message structure to the client
                        memset(&buffer, 0, sizeof(buffer));
                        read(new_socket, &buffer, sizeof(buffer)); // Wait for client's choice
                        int choice = atoi(buffer);
                        printf("Received choice: %d\n", choice);
                        switch (choice) {
                            case 1:
                                printf("ADDING NEW CUSTOMER\n");
                                memset(&buffer, 0, sizeof(buffer));
                                const char *new_cust_prompt = "Enter new customer name: ";
                                write(new_socket, new_cust_prompt, strlen(new_cust_prompt));
                                User new_cust;
                                read(new_socket, &buffer, sizeof(buffer));
                                strcpy(new_cust.name, buffer);

                                memset(&buffer, 0, sizeof(buffer));
                                const char *new_cust_pass_prompt = "Enter new customer password: ";
                                write(new_socket, new_cust_pass_prompt, strlen(new_cust_pass_prompt));
                                read(new_socket, &buffer, sizeof(buffer));
                                sha256_hash(buffer, new_cust.password);
                                
                                int new_user_id = add_user(new_cust, CUSTOMER);
                                if(new_user_id == -1){
                                    const char *new_cust_status = "Error : Customer addition failed!\n\nN";
                                    write(new_socket, new_cust_status, strlen(new_cust_status));
                                }
                                memset(&buffer, 0, sizeof(buffer));
                                snprintf(buffer, sizeof(buffer), "Customer added successfully. Customer ID is %d\n\nN", new_user_id);
                                write(new_socket, buffer, strlen(buffer));
                                break;
                            case 2:
                                printf("MODIFY CUSTOMER \n");
                                const char *modify_prompt = "Enter user_id to modify: ";
                                write(new_socket, modify_prompt, strlen(modify_prompt));
                                memset(&buffer, 0, sizeof(buffer));
                                int modify_user_id;
                                read(new_socket, &buffer, sizeof(buffer));
                                modify_user_id = atoi(buffer);
                                User modify_user;
                                if(get_user_by_id(modify_user_id, &modify_user) == -1 || modify_user.active == 0){
                                    const char *modify_status = "Error : User not found!\n\nN";
                                    write(new_socket, modify_status, strlen(modify_status));
                                    break;
                                }
                                if(modify_user.role != CUSTOMER){
                                    const char *modify_status = "Error : You can only modify customer details!\n\nN";
                                    write(new_socket, modify_status, strlen(modify_status));
                                    break;
                                }
                                memset(&buffer, 0, sizeof(buffer));
                                const char *modify_name_prompt = "Enter new name: ";
                                write(new_socket, modify_name_prompt, strlen(modify_name_prompt));
                                read(new_socket, &buffer, sizeof(buffer));
                                strcpy(modify_user.name, buffer);
                                memset(&buffer, 0, sizeof(buffer));
                                const char *modify_pass_prompt = "Enter new password: ";
                                write(new_socket, modify_pass_prompt, strlen(modify_pass_prompt));
                                read(new_socket, &buffer, sizeof(buffer));
                                sha256_hash(buffer, modify_user.password);
                                if(update_user_by_location(modify_user)==-1){
                                    const char *modify_status = "Error : User modification failed!\n\nN";
                                    write(new_socket, modify_status, strlen(modify_status));
                                }
                                const char *modify_status = "User modified successfully!\n\nN";
                                write(new_socket, modify_status, strlen(modify_status));
                                break;
                            case 3:
                                printf("VIEW ASSIGNED LOAN APPLICATIONS\n");
                                get_loan_applications(new_socket,current_user.user_id);
                                break;
                            case 4:
                                printf("APPROVE/REJECT LOANS\n");
                                const char *approve_prompt = "Enter application_id to approve/reject: ";
                                write(new_socket, approve_prompt, strlen(approve_prompt));
                                memset(&buffer, 0, sizeof(buffer));
                                int approve_application_id;
                                read(new_socket, &buffer, sizeof(buffer));
                                approve_application_id = atoi(buffer);
                                const char *approve_choice_prompt = "Enter 1 to approve, 0 to reject: ";
                                write(new_socket, approve_choice_prompt, strlen(approve_choice_prompt));
                                memset(&buffer, 0, sizeof(buffer));
                                read(new_socket, &buffer, sizeof(buffer));
                                int approve_choice = atoi(buffer);
                                LoanStatus status;
                                if(approve_choice == 1){
                                    status = LOAN_APPROVED;
                                }else if(approve_choice == 0){
                                    status = LOAN_REJECTED;
                                }else{
                                    const char *approve_status = "Error : Invalid choice!\n\nN";
                                    write(new_socket, approve_status, strlen(approve_status));
                                    break;
                                }
                                if(update_loan_status(approve_application_id, current_user.user_id, status)==-1){
                                    const char *approve_status = "Error : Loan approval/rejection failed!\n\nN";
                                    write(new_socket, approve_status, strlen(approve_status));
                                }
                                const char *approve_status = "Loan application approved/rejected successfully!\n\nN";
                                write(new_socket, approve_status, strlen(approve_status));
                                break;
                            case 5:
                                printf("VIEW CUSTOMER TRANSACTIONS\n");
                                const char *get_cust_id = "Enter user_id to view transactions: ";
                                write(new_socket, get_cust_id, strlen(get_cust_id));
                                memset(&buffer, 0, sizeof(buffer));
                                int cust_id;
                                read(new_socket, &buffer, sizeof(buffer));
                                cust_id = atoi(buffer);
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
                        memset(&buffer, 0, sizeof(buffer));
                        read(new_socket, &buffer, sizeof(buffer)); // Wait for client's choice
                        int choice = atoi(buffer);
                        printf("Received choice: %d\n", choice);
                        switch (choice) {
                            case 1:
                                printf("ACTIVATE/DEACTIVATE CUSTOMER ACCOUNTS\n");
                                const char *activate_prompt = "Enter user_id to activate/deactivate: ";
                                write(new_socket, activate_prompt, strlen(activate_prompt));
                                memset(&buffer, 0, sizeof(buffer));
                                int activate_user_id;
                                read(new_socket, &buffer, sizeof(buffer));
                                activate_user_id = atoi(buffer);
                                User activate_user;
                                if(get_user_by_id(activate_user_id, &activate_user) == -1){
                                    const char *activate_status = "Error : User not found!\n\nN";
                                    write(new_socket, activate_status, strlen(activate_status));
                                    break;
                                }
                                if(activate_user.role != CUSTOMER){
                                    const char *activate_status = "Error : You can only activate/deactivate customer account!\n\nN";
                                    write(new_socket, activate_status, strlen(activate_status));
                                    break;
                                }
                                const char *activate_choice_prompt = "Enter 1 to activate, 0 to deactivate: ";
                                write(new_socket, activate_choice_prompt, strlen(activate_choice_prompt));
                                memset(&buffer, 0, sizeof(buffer));
                                read(new_socket, &buffer, sizeof(buffer));
                                int activate_choice = atoi(buffer);
                                if(activate_choice != 0 && activate_choice != 1){
                                    const char *activate_status = "Error : Invalid choice!\n\nN";
                                    write(new_socket, activate_status, strlen(activate_status));
                                    break;
                                }
                                activate_user.active = activate_choice;
                                if(update_user_by_location(activate_user)==-1){
                                    const char *activate_status = "Error : Account activation/deactivation failed!\n\nN";
                                    write(new_socket, activate_status, strlen(activate_status));
                                }
                                const char *activate_status = "Account activation/deactivation successful!\n\nN";
                                write(new_socket, activate_status, strlen(activate_status));
                                break;
                            case 2:
                                printf("ASSIGN LOAN APPLICATIONS\n");
                                get_loan_applications(new_socket,0);
                                const char *assign_prompt = "Enter application_id to assign: ";
                                write(new_socket, assign_prompt, strlen(assign_prompt));
                                memset(&buffer, 0, sizeof(buffer));
                                int assign_application_id;
                                read(new_socket, &buffer, sizeof(buffer));
                                assign_application_id = atoi(buffer);
                                const char *assign_emp_prompt = "Enter employee_id to assign: ";
                                write(new_socket, assign_emp_prompt, strlen(assign_emp_prompt));
                                memset(&buffer, 0, sizeof(buffer));
                                int assign_emp_id;
                                read(new_socket, &buffer, sizeof(buffer));
                                assign_emp_id = atoi(buffer);
                                User assign_emp;
                                if(get_user_by_id(assign_emp_id, &assign_emp) == -1 || assign_emp.active == 0 || assign_emp.role != EMPLOYEE){
                                    const char *assign_status = "Error : Employee not found!\n\nN";
                                    write(new_socket, assign_status, strlen(assign_status));
                                    break;
                                }
                                if(update_loan_status(assign_application_id, assign_emp_id, LOAN_ASSIGNED)==-1){
                                    const char *assign_status = "Error : Loan assignment failed!\n\nN";
                                    write(new_socket, assign_status, strlen(assign_status));
                                }
                                const char *assign_status = "Loan application assigned successfully!\n\nN";
                                write(new_socket, assign_status, strlen(assign_status));
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
                        memset(&buffer, 0, sizeof(buffer));
                        read(new_socket, &buffer, sizeof(buffer)); // Wait for client's choice
                        int choice = atoi(buffer);
                        printf("Received choice: %d\n", choice);
                        switch (choice) {
                            case 1:
                                printf("ADDING NEW EMP\n");
                                memset(&buffer, 0, sizeof(buffer));
                                const char *new_emp_prompt = "Enter new employee name: ";
                                write(new_socket, new_emp_prompt, strlen(new_emp_prompt));
                                User new_emp;
                                read(new_socket, &buffer, sizeof(buffer));
                                strcpy(new_emp.name, buffer);

                                memset(&buffer, 0, sizeof(buffer));
                                const char *new_emp_pass_prompt = "Enter new employee password: ";
                                write(new_socket, new_emp_pass_prompt, strlen(new_emp_pass_prompt));
                                read(new_socket, &buffer, sizeof(buffer));
                                sha256_hash(buffer, new_emp.password);
                                
                                int new_user_id = add_user(new_emp, EMPLOYEE);
                                if(new_user_id == -1){
                                    const char *new_emp_status = "Error : Employee addition failed!\n\nN";
                                    write(new_socket, new_emp_status, strlen(new_emp_status));
                                }
                                memset(&buffer, 0, sizeof(buffer));
                                snprintf(buffer, sizeof(buffer), "Employee added successfully. Employee ID is %d\n\nN", new_user_id);
                                write(new_socket, buffer, strlen(buffer));
                                break;
                            case 2:
                                printf("MODIFY CUSTOMER/EMPLOYEE \n");
                                const char *modify_prompt = "Enter user_id to modify: ";
                                write(new_socket, modify_prompt, strlen(modify_prompt));
                                memset(&buffer, 0, sizeof(buffer));
                                int modify_user_id;
                                read(new_socket, &buffer, sizeof(buffer));
                                modify_user_id = atoi(buffer);
                                User modify_user;
                                if(get_user_by_id(modify_user_id, &modify_user) == -1 || modify_user.active == 0){
                                    const char *modify_status = "Error : User not found!\n\nN";
                                    write(new_socket, modify_status, strlen(modify_status));
                                    break;
                                }
                                if(modify_user.role != CUSTOMER && modify_user.role != EMPLOYEE){
                                    const char *modify_status = "Error : Cannot modify manager/administrator!\n\nN";
                                    write(new_socket, modify_status, strlen(modify_status));
                                    break;
                                }
                                memset(&buffer, 0, sizeof(buffer));
                                const char *modify_name_prompt = "Enter new name: ";
                                write(new_socket, modify_name_prompt, strlen(modify_name_prompt));
                                read(new_socket, &buffer, sizeof(buffer));
                                strcpy(modify_user.name, buffer);
                                memset(&buffer, 0, sizeof(buffer));
                                const char *modify_pass_prompt = "Enter new password: ";
                                write(new_socket, modify_pass_prompt, strlen(modify_pass_prompt));
                                read(new_socket, &buffer, sizeof(buffer));
                                sha256_hash(buffer, modify_user.password);
                                if(update_user_by_location(modify_user)==-1){
                                    const char *modify_status = "Error : User modification failed!\n\nN";
                                    write(new_socket, modify_status, strlen(modify_status));
                                }
                                const char *modify_status = "User modified successfully!\n\nN";
                                write(new_socket, modify_status, strlen(modify_status));
                                break;
                            case 3:
                                printf("MANAGE USER ROLE\n");
                                const char *role_prompt = "Enter user_id to modify role: ";
                                write(new_socket, role_prompt, strlen(role_prompt));
                                memset(&buffer, 0, sizeof(buffer));
                                int role_user_id;
                                read(new_socket, &buffer, sizeof(buffer));
                                role_user_id = atoi(buffer);
                                User role_user;
                                if(get_user_by_id(role_user_id, &role_user) == -1 || role_user.active == 0){
                                    const char *role_status = "Error : User not found!\n\nN";
                                    write(new_socket, role_status, strlen(role_status));
                                    break;
                                }
                                const char *role_change_prompt = "Enter new role (1 for customer, 2 for employee, 3 for manager, 4 for administrator): ";
                                write(new_socket, role_change_prompt, strlen(role_change_prompt));
                                memset(&buffer, 0, sizeof(buffer));
                                read(new_socket, &buffer, sizeof(buffer));
                                int new_role = atoi(buffer);
                                if(new_role < 1 || new_role > 4){
                                    const char *role_status = "Error : Invalid role!\n\nN";
                                    write(new_socket, role_status, strlen(role_status));
                                    break;
                                }
                                if(new_role == 1){role_user.role = CUSTOMER;}
                                else if(new_role == 2){role_user.role = EMPLOYEE;}
                                else if(new_role == 3){role_user.role = MANAGER;}
                                else if(new_role == 4){role_user.role = ADMINISTRATOR;}
                                if(update_user_by_location(role_user)==-1){
                                    const char *role_status = "Error : Role change failed!\n\nN";
                                    write(new_socket, role_status, strlen(role_status));
                                }
                                const char *role_status = "Role changed successfully!\n\nN";
                                write(new_socket, role_status, strlen(role_status));
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
