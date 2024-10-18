#ifndef MODELS_H
#define MODELS_H

#include <time.h>

/***************  Enums  *******************/
typedef enum {
    CUSTOMER,
    EMPLOYEE,
    MANAGER,
    ADMINISTRATOR
} Role;

typedef enum {
    DEPOSIT,
    WITHDRAWAL,
    TRANSFER,
    LOAN_DISBURSEMENT
} TransactionType;

typedef enum{
    DESTINATION_ACCOUNT_NOT_FOUND,
    INSUFFICIENT_BALANCE,
    SUCCESS
} TransactionStatus;

typedef enum {
    DATA_TYPE_INT,
    DATA_TYPE_DOUBLE,
    DATA_TYPE_STRING,
    DATA_TYPE_NA
} DataType;


typedef enum{
    LOAN_PENDING,
    LOAN_ASSIGNED,
    LOAN_APPROVED,
    LOAN_REJECTED
} LoanStatus;

#define SESSION_TIMEOUT 30
#define MAX_BUFFER_SIZE 1024
#define SHA256_DIGEST_LENGTH 32
/***************  Structures  *******************/
typedef struct {
    int user_id;
    char name[200];
    unsigned char password[SHA256_DIGEST_LENGTH];  
    Role role;           
    time_t created_at;
    time_t updated_at;
    int active;
    double balance;
    int db_index;
} User;

typedef struct {
    int session_id;            
    int user_id;               
    time_t login_time;         
    time_t logout_time;        
    int active;
    int db_index;     
} Session;

typedef struct {
    int trx_id;                     
    int user_id;                
    TransactionType type;      
    double amount;
    double balance;              
    time_t timestamp;           
    char description[256];
    int from_account;
    int to_account;
    TransactionStatus status;                       
} Transaction;


typedef struct{
    int feedback_id;
    int user_id;
    char feedback[1024];
    time_t timestamp;
} Feedback;

typedef struct {
    int application_id;       
    int user_id;
    int employee_id;              
    double amount;   
    char purpose[MAX_BUFFER_SIZE];
    time_t assignment_date;        
    time_t application_date;
    time_t decision_date;
    LoanStatus status; 
} Loan;

#endif