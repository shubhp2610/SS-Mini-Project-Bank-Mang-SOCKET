#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "lib/customer.h"
#include "lib/employee.h"
#include "lib/manager.h"
#include "lib/manager.h"
#include "lib/admin.h"

int main(int argc, char *argv[]) {
    int PORT = atoi(argv[1]);
    int server_fd, new_socket;
    struct sockaddr_in address;

    int addrlen = sizeof(address);
    char buffer[MAX_BUFFER_SIZE];
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
            char auth_status[MAX_BUFFER_SIZE]; // Make sure this is large enough
            snprintf(auth_status, sizeof(auth_status), "Authentication successful. Welcome %s!\n\n", current_user.name);
            write_line(new_socket, auth_status);
            // Read and write data to client
                switch(current_user.role){
                    case CUSTOMER:
                    while (current_user.active) {
                        printf("inside customer\n");
                        memset(&buffer, 0, sizeof(buffer));
                        write_line(new_socket, "1. View Balance\n2. Deposit\n3. Withdraw\n4. Transfer\n5. Apply for Loan\n6. Change Password\n7. Provide Feedback\n8. View Transactions\n9. Logout\n10. EXIT\nEnter choice: \0");
                        int choice = read_int(new_socket);
                        printf("Received choice: %d\n", choice);
                        switch (choice) {
                            case 1: {
                                view_balance(new_socket, current_user);
                                break;
                            }
                            case 2:
                                deposit(new_socket, &current_user);
                                break;
                            case 3:
                                withdraw(new_socket, &current_user);
                                break;
                            case 4:
                                transfer(new_socket, &current_user);
                                break;
                            case 5:
                                apply_loan(new_socket, &current_user);
                                break;
                            case 6:
                                wrapper_change_password(new_socket, &current_user);
                                break;
                            case 7:
                                provide_feedback(new_socket, &current_user);
                                break;
                            case 8:
                                get_transactions(new_socket,current_user.user_id);
                                break;
                            case 9:
                                wrapper_logout(new_socket, &current_session);
                                close(new_socket);
                                exit(0);
                                break;
                            case 10:
                                wrapper_exit(new_socket, &current_session);
                                close(new_socket);
                                exit(0);
                                break;
                            default:
                                write_line(new_socket, "Invalid choice!\n\nN");
                                break;
                        }
                        get_user_by_location(&current_user);
                    }
                    break;
                    case EMPLOYEE:
                    printf("inside employee\n");
                    while (current_user.active) {
                        write_line(new_socket,"1. Add New Customer\n2. Modify Customer Details\n3. View Assigned Loan Applications\n4. Approve/Reject Loans\n5. View Customer Transactions\n6. Change Password\n7. Logout\n8. Exit\nEnter choice: \0");
                        int choice = read_int(new_socket);
                        printf("Received choice: %d\n", choice);
                        switch (choice) {
                            case 1:
                                add_new_customer(new_socket);
                                break;
                            case 2:
                                modify_customer_details(new_socket);
                                break;
                            case 3:
                                view_assigned_loan_applications(new_socket,current_user.user_id);
                                break;
                            case 4:
                                approve_reject_loans(new_socket,current_user.user_id);
                                break;
                            case 5:
                                view_customer_transactions(new_socket);
                                break;
                            case 6:
                                wrapper_change_password(new_socket, &current_user);
                                break;
                            case 7:
                                wrapper_logout(new_socket, &current_session);
                                close(new_socket);
                                exit(0);
                                break;
                            case 8:
                                wrapper_exit(new_socket, &current_session);
                                close(new_socket);
                                exit(0);
                                break;
                            default:
                                write_line(new_socket, "Invalid choice!\n\nN");
                                break;
                        }
                        get_user_by_location(&current_user);
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
                                activate_deactivate_customer_accounts(new_socket);
                                break;
                            case 2:
                                assign_loan_application_processes(new_socket);
                                break;
                            case 3:
                                review_customer_feedback(new_socket);
                                break;
                            case 4:
                                wrapper_change_password(new_socket, &current_user);
                                break;
                            case 5:
                                wrapper_logout(new_socket, &current_session);
                                close(new_socket);
                                exit(0);
                                break;
                            case 6:
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
                                add_new_employee(new_socket);
                                break;
                            case 2:
                                modify_user_details(new_socket);
                                break;
                            case 3:
                                manage_user_roles(new_socket);
                                break;
                            case 4:
                                wrapper_change_password(new_socket, &current_user);
                                break;
                            case 5:
                                wrapper_logout(new_socket, &current_session);
                                close(new_socket);
                                exit(0);
                                break;
                            case 6:
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
