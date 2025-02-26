#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>
#include "lib/customer.h"
#include "lib/employee.h"
#include "lib/manager.h"
#include "lib/manager.h"
#include "lib/admin.h"

#define MAX_CONNECTIONS 5

union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

int init_semaphore() {
    int sem_id = semget(IPC_PRIVATE, 1, IPC_CREAT | 0666);
    if (sem_id == -1) {
        perror("semget failed");
        exit(EXIT_FAILURE);
    }

    union semun sem_union;
    sem_union.val = MAX_CONNECTIONS;
    
    if (semctl(sem_id, 0, SETVAL, sem_union) == -1) {
        perror("semctl failed");
        exit(EXIT_FAILURE);
    }
    
    return sem_id;
}

int sem_wait(int sem_id) {
    struct sembuf sem_op = {0, -1, SEM_UNDO};
    return semop(sem_id, &sem_op, 1);
}

int sem_signal(int sem_id) {
    struct sembuf sem_op = {0, 1, SEM_UNDO};
    return semop(sem_id, &sem_op, 1);
}

int main(int argc, char *argv[]) {
    int PORT = atoi(argv[1]);
    int server_fd, new_socket;
    struct sockaddr_in address;
    int sem_id = init_semaphore();
    int addrlen = sizeof(address);
    char buffer[MAX_BUFFER_SIZE];
    ssize_t read_bytes;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

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

    while(1) {
        new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        if (new_socket < 0) {
            perror("Accept failed");
            continue;
        }

        if (sem_wait(sem_id) == -1) {
            if (errno == EINTR) {
                close(new_socket);
                continue;
            }
            perror("sem_wait failed");
            close(new_socket);
            continue;
        }

        pid_t pid = fork();
        if (pid < 0) {
            perror("Fork failed");
            sem_signal(sem_id);
            close(new_socket);
            continue;
        }

        if (pid == 0) {
            close(server_fd);
            
            while(1) {
                write_line(new_socket, "Welcome! Please enter your user_id : ");
                int user_id;
                read(new_socket, &user_id, sizeof(user_id));
                printf("Received user_id: %d\n", user_id);            
                write_line(new_socket, "Enter password: ");
          
                char password[200];
                read_bytes = read(new_socket, password, 200);
                password[read_bytes] = '\0';
                printf("Received password: %s|\n", password);
            
                User current_user;
                Session current_session;
                if (get_user_by_id(user_id, &current_user) == -1 || !is_password_correct(password, current_user.password)) {
                    write_line(new_socket, "Error : Authentication failed!");
                    sem_signal(sem_id);
                    close(new_socket);
                    exit(0);
                }
                if(current_user.active == 0){
                    write_line(new_socket, "Error : User is deactivated!");
                    sem_signal(sem_id);
                    close(new_socket);
                    exit(0);
                }
                if(start_session(current_user,&current_session) == -1){
                    const char *auth_status = "Error : Only one session allowed at a time!";
                    write(new_socket, auth_status, strlen(auth_status));
                    sem_signal(sem_id);
                    close(new_socket);
                    exit(0);
                }
                char auth_status[MAX_BUFFER_SIZE];
                snprintf(auth_status, sizeof(auth_status), "Authentication successful. Welcome %s!\n\n", current_user.name);
                write_line(new_socket, auth_status);

                switch(current_user.role) {
                    case CUSTOMER:
                        while (current_user.active) {
                            printf("inside customer\n");
                            memset(&buffer, 0, sizeof(buffer));
                            write_line(new_socket, "===========================================\n1. View Balance\n2. Deposit\n3. Withdraw\n4. Transfer\n5. Apply for Loan\n6. Change Password\n7. Provide Feedback\n8. View Transactions\n9. Logout\n10. EXIT\nEnter choice: \0");
                            int choice = read_int(new_socket);
                            printf("Received choice: %d\n", choice);
                            switch (choice) {
                                case 1:
                                    view_balance(new_socket, current_user);
                                    break;
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
                                    sem_signal(sem_id);
                                    exit(0);
                                case 10:
                                    wrapper_exit(new_socket, &current_session);
                                    close(new_socket);
                                    sem_signal(sem_id);
                                    exit(0);
                                default:
                                    write_line(new_socket, "Invalid choice!\n\nN");
                                    break;
                            }
                            get_user_by_location(&current_user);
                        }
                        break;
                    case EMPLOYEE:
                        while (current_user.active) {
                            write_line(new_socket,"===========================================\n1. Add New Customer\n2. Modify Customer Details\n3. View Assigned Loan Applications\n4. Approve/Reject Loans\n5. View Customer Transactions\n6. Change Password\n7. Logout\n8. Exit\nEnter choice: \0");
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
                                    sem_signal(sem_id);
                                    exit(0);
                                case 8:
                                    wrapper_exit(new_socket, &current_session);
                                    close(new_socket);
                                    sem_signal(sem_id);
                                    exit(0);
                                default:
                                    write_line(new_socket, "Invalid choice!\n\nN");
                                    break;
                            }
                            get_user_by_location(&current_user);
                        }
                        break;
                    case MANAGER:
                        while (1) {
                            memset(&buffer, 0, sizeof(buffer));
                            const char *customer_menu = "===========================================\n1. Activate/Deactivate Customer Accounts\n2. Assign Loan Application Processes to Employees\n3. Review Customer Feedback\n4. Change Password\n5. Logout\n6. Exit\nEnter choice: \0";
                            strcpy(buffer, customer_menu);
                            write(new_socket, &buffer, sizeof(buffer));
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
                                    sem_signal(sem_id);
                                    exit(0);
                                case 6:
                                    wrapper_exit(new_socket, &current_session);
                                    close(new_socket);
                                    sem_signal(sem_id);
                                    exit(0);
                                default:
                                    write_line(new_socket, "Invalid choice!\n\nN");
                                    break;
                            }
                        }
                        break;
                    case ADMINISTRATOR:
                        while (1) {
                            memset(&buffer, 0, sizeof(buffer));
                            const char *customer_menu = "===========================================\n1. Add New Bank Employee\n2. Modify Customer/Employee Details\n3. Manage User Roles\n4. Change Password\n5. Logout\n6. Exit\nEnter choice: \0";
                            strcpy(buffer, customer_menu);
                            write(new_socket, &buffer, sizeof(buffer));
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
                                    sem_signal(sem_id);
                                    exit(0);
                                case 6:
                                    wrapper_exit(new_socket, &current_session);
                                    close(new_socket);
                                    sem_signal(sem_id);
                                    exit(0);
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
            sem_signal(sem_id);
            exit(0);
        } else {
            close(new_socket);
        }
    }

    close(server_fd);
    semctl(sem_id, 0, IPC_RMID);
    return 0;
}