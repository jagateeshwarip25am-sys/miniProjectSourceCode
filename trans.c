// Enhanced bank-account program with authentication, admin mode,
// safer I/O, initialization, listing and transaction logging.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

// clientData structure definition (extended)
struct clientData
{
    unsigned int acctNum; // account number
    char lastName[15];    // account last name
    char firstName[10];   // account first name
    char email[30];       // contact email
    unsigned int pin;     // 4-digit PIN for authentication
    double balance;       // account balance
}; // end structure clientData

// prototypes
unsigned int enterChoice(void);
void textFile(FILE *readPtr);
void updateRecord(FILE *fPtr);
void newRecord(FILE *fPtr);
void deleteRecord(FILE *fPtr);
void initFile(const char *fileName);
int authenticateAccount(FILE *fPtr, unsigned int account);
int adminLogin(void);
void listAccounts(FILE *fPtr);

// simple admin password (in real apps do not hardcode)
#define ADMIN_PASS "admin123"

int main(int argc, char *argv[])
{
    FILE *cfPtr;         // credit.dat file pointer
    unsigned int choice; // user's choice

    initFile("credit.dat");

    // open the file for read/write
    if ((cfPtr = fopen("credit.dat", "rb+")) == NULL)
    {
        perror("Unable to open credit.dat");
        exit(EXIT_FAILURE);
    }

    // enable user to specify action
    while ((choice = enterChoice()) != 6)
    {
        switch (choice)
        {
        case 1: // create text file from record file
            textFile(cfPtr);
            break;
        case 2: // update record
            updateRecord(cfPtr);
            break;
        case 3: // create record
            newRecord(cfPtr);
            break;
        case 4: // delete existing record
            deleteRecord(cfPtr);
            break;
        case 5: // list accounts (admin)
            listAccounts(cfPtr);
            break;
        default:
            puts("Incorrect choice");
            break;
        }
    }

    fclose(cfPtr);
    return 0;
}

// initialize data file with blank records if missing
void initFile(const char *fileName)
{
    FILE *fptr = fopen(fileName, "rb+");
    if (fptr != NULL)
    {
        fclose(fptr);
        return; // file exists
    }

    // create file and write 100 blank records
    struct clientData blank = {0, "", "", "", 0, 0.0};
    fptr = fopen(fileName, "wb");
    if (fptr == NULL)
    {
        perror("Unable to create credit.dat");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < 100; ++i)
        fwrite(&blank, sizeof(struct clientData), 1, fptr);

    fclose(fptr);
}

// create formatted text file for printing
void textFile(FILE *readPtr)
{
    FILE *writePtr; // accounts.txt file pointer
    struct clientData client = {0, "", "", "", 0, 0.0};

    if ((writePtr = fopen("accounts.txt", "w")) == NULL)
    {
        puts("File could not be opened.");
        return;
    }

    rewind(readPtr);
    fprintf(writePtr, "%-6s%-16s%-11s%-32s%10s\n", "Acct", "Last Name", "First Name", "Email", "Balance");

    while (fread(&client, sizeof(struct clientData), 1, readPtr) == 1)
    {
        if (client.acctNum != 0)
        {
            fprintf(writePtr, "%-6u%-16s%-11s%-32s%10.2f\n",
                    client.acctNum, client.lastName, client.firstName, client.email, client.balance);
        }
    }

    fclose(writePtr);
}

// admin login
int adminLogin(void)
{
    char buf[64];
    printf("Enter admin password: ");
    if (fgets(buf, sizeof(buf), stdin) == NULL)
        return 0;
    // trim newline
    buf[strcspn(buf, "\n")] = '\0';
    if (strcmp(buf, ADMIN_PASS) == 0)
        return 1;
    printf("Admin authentication failed.\n");
    return 0;
}

// authenticate account owner by PIN
int authenticateAccount(FILE *fPtr, unsigned int account)
{
    if (account < 1 || account > 100)
        return 0;

    struct clientData client;
    fseek(fPtr, (account - 1) * sizeof(struct clientData), SEEK_SET);
    if (fread(&client, sizeof(struct clientData), 1, fPtr) != 1)
        return 0;

    if (client.acctNum == 0)
        return 0;

    unsigned int pin;
    printf("Enter 4-digit PIN for account %u: ", account);
    if (scanf("%u", &pin) != 1)
    {
        while (getchar() != '\n') ;
        return 0;
    }
    while (getchar() != '\n') ;

    if (pin == client.pin)
        return 1;

    printf("PIN incorrect.\n");
    return 0;
}

// append to transaction log
static void logTransaction(unsigned int account, double amount, double newBalance)
{
    FILE *log = fopen("transactions.txt", "a");
    if (!log)
        return;
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    char timebuf[64];
    strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", tm);
    fprintf(log, "%s | Account: %u | Amount: %+0.2f | Balance: %0.2f\n", timebuf, account, amount, newBalance);
    fclose(log);
}

// update balance in record (owner or admin)
void updateRecord(FILE *fPtr)
{
    unsigned int account; // account number
    double transaction;   // transaction amount
    struct clientData client = {0, "", "", "", 0, 0.0};

    printf("Enter account to update (1 - 100): ");
    if (scanf("%u", &account) != 1)
    {
        while (getchar() != '\n') ;
        puts("Invalid account number.");
        return;
    }
    while (getchar() != '\n') ;

    if (account < 1 || account > 100)
    {
        puts("Account out of range.");
        return;
    }

    fseek(fPtr, (account - 1) * sizeof(struct clientData), SEEK_SET);
    if (fread(&client, sizeof(struct clientData), 1, fPtr) != 1 || client.acctNum == 0)
    {
        printf("Account #%u has no information.\n", account);
        return;
    }

    // authenticate owner or admin
    if (!authenticateAccount(fPtr, account))
    {
        printf("Authenticate as admin? (y/n): ");
        int c = getchar();
        while (getchar() != '\n') ;
        if (c != 'y' && c != 'Y')
            return;
        if (!adminLogin())
            return;
    }

    printf("%-6u %-16s %-11s %10.2f\n\n", client.acctNum, client.lastName, client.firstName, client.balance);

    printf("Enter charge (+) or payment (-) amount: ");
    if (scanf("%lf", &transaction) != 1)
    {
        while (getchar() != '\n') ;
        puts("Invalid amount.");
        return;
    }
    while (getchar() != '\n') ;

    client.balance += transaction;

    // write updated record back
    fseek(fPtr, (account - 1) * sizeof(struct clientData), SEEK_SET);
    fwrite(&client, sizeof(struct clientData), 1, fPtr);

    printf("Updated: %-6u %-16s %-11s %10.2f\n", client.acctNum, client.lastName, client.firstName, client.balance);
    logTransaction(account, transaction, client.balance);
}

// delete an existing record
void deleteRecord(FILE *fPtr)
{
    struct clientData client;                       // stores record read from file
    struct clientData blankClient = {0, "", "", "", 0, 0.0}; // blank client
    unsigned int accountNum;                        // account number

    printf("Enter account number to delete (1 - 100): ");
    if (scanf("%u", &accountNum) != 1)
    {
        while (getchar() != '\n') ;
        puts("Invalid account number.");
        return;
    }
    while (getchar() != '\n') ;

    if (accountNum < 1 || accountNum > 100)
    {
        puts("Account out of range.");
        return;
    }

    fseek(fPtr, (accountNum - 1) * sizeof(struct clientData), SEEK_SET);
    if (fread(&client, sizeof(struct clientData), 1, fPtr) != 1 || client.acctNum == 0)
    {
        printf("Account %u does not exist.\n", accountNum);
        return;
    }

    // owner or admin required
    if (!authenticateAccount(fPtr, accountNum))
    {
        printf("Authenticate as admin to delete? (y/n): ");
        int c = getchar();
        while (getchar() != '\n') ;
        if (c != 'y' && c != 'Y')
            return;
        if (!adminLogin())
            return;
    }

    fseek(fPtr, (accountNum - 1) * sizeof(struct clientData), SEEK_SET);
    fwrite(&blankClient, sizeof(struct clientData), 1, fPtr);
    printf("Account %u deleted.\n", accountNum);
}

// create and insert record
void newRecord(FILE *fPtr)
{
    struct clientData client = {0, "", "", "", 0, 0.0};
    unsigned int accountNum; // account number

    printf("Enter new account number (1 - 100): ");
    if (scanf("%u", &accountNum) != 1)
    {
        while (getchar() != '\n') ;
        puts("Invalid account number.");
        return;
    }
    while (getchar() != '\n') ;

    if (accountNum < 1 || accountNum > 100)
    {
        puts("Account out of range.");
        return;
    }

    fseek(fPtr, (accountNum - 1) * sizeof(struct clientData), SEEK_SET);
    fread(&client, sizeof(struct clientData), 1, fPtr);
    if (client.acctNum != 0)
    {
        printf("Account #%u already contains information.\n", client.acctNum);
        return;
    }

    // gather details safely
    printf("Enter lastname (max 14): ");
    if (fgets(client.lastName, sizeof(client.lastName), stdin) == NULL) return;
    client.lastName[strcspn(client.lastName, "\n")] = '\0';

    printf("Enter firstname (max 9): ");
    if (fgets(client.firstName, sizeof(client.firstName), stdin) == NULL) return;
    client.firstName[strcspn(client.firstName, "\n")] = '\0';

    printf("Enter email (max 29): ");
    if (fgets(client.email, sizeof(client.email), stdin) == NULL) return;
    client.email[strcspn(client.email, "\n")] = '\0';

    printf("Enter initial balance: ");
    if (scanf("%lf", &client.balance) != 1)
    {
        while (getchar() != '\n') ;
        puts("Invalid balance.");
        return;
    }
    while (getchar() != '\n') ;

    printf("Set a 4-digit PIN: ");
    if (scanf("%u", &client.pin) != 1)
    {
        while (getchar() != '\n') ;
        puts("Invalid PIN.");
        return;
    }
    while (getchar() != '\n') ;

    client.acctNum = accountNum;
    fseek(fPtr, (client.acctNum - 1) * sizeof(struct clientData), SEEK_SET);
    fwrite(&client, sizeof(struct clientData), 1, fPtr);
    printf("Account %u created.\n", client.acctNum);
}

// list all accounts (admin only)
void listAccounts(FILE *fPtr)
{
    if (!adminLogin())
        return;

    struct clientData client = {0, "", "", "", 0, 0.0};
    rewind(fPtr);
    printf("%-6s %-16s %-11s %-30s %10s\n", "Acct", "Last", "First", "Email", "Balance");
    while (fread(&client, sizeof(struct clientData), 1, fPtr) == 1)
    {
        if (client.acctNum != 0)
        {
            printf("%-6u %-16s %-11s %-30s %10.2f\n",
                   client.acctNum, client.lastName, client.firstName, client.email, client.balance);
        }
    }
}

// enable user to input menu choice
unsigned int enterChoice(void)
{
    unsigned int menuChoice; // variable to store user's choice
    printf("\nEnter your choice\n"
           "1 - store a formatted text file of accounts called\n"
           "    \"accounts.txt\" for printing\n"
           "2 - update an account\n"
           "3 - add a new account\n"
           "4 - delete an account\n"
           "5 - list accounts (admin)\n"
           "6 - end program\n? ");

    if (scanf("%u", &menuChoice) != 1)
    {
        while (getchar() != '\n') ;
        return 0;
    }
    while (getchar() != '\n') ;
    return menuChoice;
}
