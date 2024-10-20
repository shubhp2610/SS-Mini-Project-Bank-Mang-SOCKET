#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "shared/utils.h"
int main(){
    int fd = open("data/users.db",O_RDWR);
    User user;
    printf("================= USERS =================\n");
    printf("User ID\tName                \tRole\tActive\tBalance\tCreate Time\t\t\t\tUpdate Time\n");
    while(read(fd,&user,sizeof(User))>0){
        printf("%d\t%-20s\t%d\t%d\t%f\t%s\t%s\n",user.user_id,user.name,user.role,user.active,user.balance,ctime(&user.created_at),ctime(&user.updated_at));
    }
    close(fd);
    fd = open("data/feedbacks.db",O_RDWR);
    Feedback feedback;
    printf("================= FEEDBACKS =================\n");
    printf("Feedback ID\tUser ID\tTimestamp\t\t\tFeedback\n");
    while(read(fd,&feedback,sizeof(Feedback))>0){
        char* timestamp = ctime(&feedback.timestamp);
        timestamp[strcspn(timestamp, "\n")] = '\0'; 
        printf("%d\t\t%d\t%s\t%s\nN", feedback.feedback_id, feedback.user_id, timestamp, feedback.feedback);
    }
    close(fd);
    fd = open("data/loans.db",O_RDWR);
    Loan loan;
    printf("================= LOANS =================\n");
    printf("App ID\tUser ID\tAmount\t\tPurpose\t\tStatus\t\tApplication Date\t\tAssignment Date\t\tDecision Date\n");
    while(read(fd,&loan,sizeof(Loan))>0){
        const char* loanStatusNames[] = {
            "LOAN_PENDING",
            "LOAN_ASSIGNED",
            "LOAN_APPROVED",
            "LOAN_REJECTED"
        };
        printf("%d\t%d\t%.2f\t%s\t%s\t%s\t%s\t%s\n", loan.application_id, loan.user_id, loan.amount, loan.purpose, loanStatusNames[loan.status], ctime(&loan.application_date),ctime(&loan.assignment_date),ctime(&loan.decision_date));
    }
    close(fd);
    fd = open("data/transactions.db",O_RDWR);
    Transaction transaction;
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
    printf("================= TRANSACTIONS =================\n");
    printf("Transaction ID Amount     New Balance  Type           Status              To Account    From Account  Timestamp\n");
    while(read(fd,&transaction,sizeof(Transaction))>0){
        if(transaction.type == TRANSFER){
            printf("%-14d %-10.2f %-12.2f %-14s %-20s %-12d %-14d %s\n", transaction.trx_id, transaction.amount,transaction.balance, transactionTypeNames[transaction.type], transactionStatusNames[transaction.status],transaction.to_account,transaction.from_account,ctime(&transaction.timestamp));
        }else{
            printf("%-14d %-10.2f %-12.2f %-14s %-20s %26s %s\n", transaction.trx_id, transaction.amount,transaction.balance, transactionTypeNames[transaction.type], transactionStatusNames[transaction.status],"",ctime(&transaction.timestamp));
        }
    }
    close(fd);
    fd = open("data/sessions.db",O_RDWR);
    Session session;
    printf("================= SESSIONS =================\n");
    printf("Session ID\tUser ID\tLogin Time\t\t\tLogout Time\t\t\tActive\n");
    while(read(fd,&session,sizeof(Session))>0){
        char* inTime = ctime(&session.login_time);
        inTime[strcspn(inTime, "\n")] = '\0';
        char* outTime = ctime(&session.logout_time);
        outTime[strcspn(outTime, "\n")] = '\0';
        printf("%d\t\t%d\t%s\t%s\t%ld\t%ld\t%d\n", session.session_id, session.user_id, inTime,outTime, session.login_time,session.logout_time, session.active);
        memset(&session, 0, sizeof(session));
    }
    close(fd);
    return 0;
}