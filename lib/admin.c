#include "admin.h"

void add_new_employee(int socket_conn){
    char buffer[MAX_BUFFER_SIZE];
    write_line(socket_conn,"Enter new employee name: ");
    memset(&buffer, 0, sizeof(buffer));
    User new_emp;
    read(socket_conn, &buffer, sizeof(buffer));
    strcpy(new_emp.name, buffer);

    memset(&buffer, 0, sizeof(buffer));
    write_line(socket_conn,"Enter new employee password: ");
    read(socket_conn, &buffer, sizeof(buffer));
    sha256_hash(buffer, new_emp.password);
    
    int new_user_id = add_user(new_emp, EMPLOYEE);
    if(new_user_id == -1){
        write_line(socket_conn,"Error : Employee addition failed!\n\nN");
    }
    memset(&buffer, 0, sizeof(buffer));
    snprintf(buffer, sizeof(buffer), "Employee added successfully. Employee ID is %d\n\nN", new_user_id);
    write_line(socket_conn,buffer);
}

void modify_user_details(int socket_conn){
    char buffer[MAX_BUFFER_SIZE];
    write_line(socket_conn,"Enter user_id to modify: ");
    int modify_user_id = read_int(socket_conn);
    User modify_user;
    if(get_user_by_id(modify_user_id, &modify_user) == -1 || modify_user.active == 0){
        write_line(socket_conn,"Error : User not found!\n\nN");
        return;
    }
    if(modify_user.role != CUSTOMER && modify_user.role != EMPLOYEE){
        write_line(socket_conn,"Error : Cannot modify manager/administrator!\n\nN");
        return;
    }
    memset(&buffer, 0, sizeof(buffer));
    write_line(socket_conn,"Enter new name: ");
    read(socket_conn, &buffer, sizeof(buffer));
    strcpy(modify_user.name, buffer);
    memset(&buffer, 0, sizeof(buffer));
    write_line(socket_conn,"Enter new password: ");
    read(socket_conn, &buffer, sizeof(buffer));
    sha256_hash(buffer, modify_user.password);
    if(update_user_by_location(modify_user)==-1){
        write_line(socket_conn,"Error : User modification failed!\n\nN");
        return;
    }
    write_line(socket_conn,"User modified successfully!\n\nN");
}

void manage_user_roles(int socket_conn){
    write_line(socket_conn,"Enter user_id to modify role: ");
    int role_user_id = read_int(socket_conn);
    User role_user;
    if(get_user_by_id(role_user_id, &role_user) == -1 || role_user.active == 0){
        write_line(socket_conn,"Error : User not found!\n\nN");
        return;
    }
    write_line(socket_conn,"Enter new role (1 for customer, 2 for employee, 3 for manager, 4 for administrator): ");
    int new_role = read_int(socket_conn);
    if(new_role < 1 || new_role > 4){
        write_line(socket_conn,"Error : Invalid role!\n\nN");
        return;
    }
    role_user.role = new_role-1;
    if(update_user_by_location(role_user)==-1){
        write_line(socket_conn,"Error : Role modification failed!\n\nN");
        return;
    }
    write_line(socket_conn,"Role modified successfully!\n\nN");
}