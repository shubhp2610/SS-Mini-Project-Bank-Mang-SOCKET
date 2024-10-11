#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "utils.h"
int main(){
    User user;
    user.user_id = 1;
    user.active = 1;
    user.balance = 1000;
    user.created_at = time(NULL);
    strcpy(user.name, "Shubh");
    sha256_hash("password",user.password);
    user.role = ADMINISTRATOR;
    user.updated_at = time(NULL);
    user.db_index = 0;
    int fd = open("data/users.db",O_RDWR | O_CREAT, 0644);
    write(fd,&user,sizeof(User));
    close(fd);
    return 0;

}