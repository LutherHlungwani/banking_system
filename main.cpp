#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <iomanip>
#include <ctime>
#include <algorithm>
#include <cstring> 

using namespace std;

// Required Data Files
const string TELLERS_FILE = "tellers.dat";
const string CUSTOMERS_FILE = "customers.dat";

// --- UTILITIES ---

string encrypt(string data) {
    for (int i = 0; i < data.length(); i++) data[i] = data[i] + 5;
    return data;
}

void clearBuffer() {
    cin.clear();
    cin.ignore(10000, '\n');
}

// --- DATA MODELS ---

struct Teller {
    char tellerID[10];
    char fullName[50];
    char password[50]; 
    char branchCode[10];
};

// Abstract Base Class
class Account {
public:
    char accountNumber[25];
    char fullName[50];
    char idNumber[15];
    char contact[15];
    char address[100];
    char dob[15];
    char pin[10];
    double balance;
    char branchCode[10];
    char accountType[20]; // "Savings" or "Cheque"

    virtual double getInterestRate() = 0; 
};

class SavingsAccount : public Account {
public:
    double getInterestRate() override { return 0.04; } // 4%
};

class ChequeAccount : public Account {
public:
    double getInterestRate() override { return 0.01; } // 1%
};

// --- SYSTEM FUNCTIONS ---

void initializeSystem() {
    ifstream check(TELLERS_FILE, ios::binary);
    if (!check.is_open() || check.peek() == EOF) {
        check.close();
        ofstream out(TELLERS_FILE, ios::binary | ios::trunc);
        Teller admin;
        memset(&admin, 0, sizeof(Teller));
        strcpy(admin.tellerID, "T001");
        strcpy(admin.fullName, "Head Office Admin");
        string enc = encrypt("admin123");
        strncpy(admin.password, enc.c_str(), 49);
        strcpy(admin.branchCode, "HQ01");
        out.write((char*)&admin, sizeof(Teller));
        out.close();
    } else {
        check.close();
    }
}

// New Function: Register a new Teller (HQ Admin only)
void registerNewTeller() {
    Teller nt;
    memset(&nt, 0, sizeof(Teller));
    cout << "\n--- REGISTER NEW TELLER ---" << endl;
    cout << "Enter New Teller ID: "; cin >> nt.tellerID;
    cout << "Enter Full Name: "; cin.ignore(); cin.getline(nt.fullName, 49);
    cout << "Enter Branch Code (e.g., BR02): "; cin >> nt.branchCode;
    string pass;
    cout << "Set Password: "; cin >> pass;
    string enc = encrypt(pass);
    strncpy(nt.password, enc.c_str(), 49);

    ofstream out(TELLERS_FILE, ios::binary | ios::app);
    out.write((char*)&nt, sizeof(Teller));
    out.close();
    cout << "Teller registered successfully for branch " << nt.branchCode << endl;
}

// Updated: Registration with Account Type Choice
void registerCustomer(string bCode) {
    int typeChoice;
    Account* accPtr;
    SavingsAccount s;
    ChequeAccount c;

    cout << "\nSelect Account Type:\n1. Savings (4% Interest)\n2. Cheque (1% Interest)\nChoice: ";
    cin >> typeChoice;

    if (typeChoice == 2) {
        memset(&c, 0, sizeof(ChequeAccount));
        strcpy(c.accountType, "Cheque");
        accPtr = &c;
    } else {
        memset(&s, 0, sizeof(SavingsAccount));
        strcpy(s.accountType, "Savings");
        accPtr = &s;
    }

    string accStr = "ACC-" + bCode + "-" + to_string(rand() % 9000 + 1000);
    strcpy(accPtr->accountNumber, accStr.c_str());
    strcpy(accPtr->branchCode, bCode.c_str());

    cin.ignore();
    cout << "Full Name: "; cin.getline(accPtr->fullName, 49);
    cout << "National ID: "; cin.getline(accPtr->idNumber, 14);
    cout << "Initial Deposit: R"; cin >> accPtr->balance;
    
    string rawPin;
    cout << "Set 5-digit PIN: "; cin >> rawPin;
    strncpy(accPtr->pin, encrypt(rawPin).c_str(), 9);

    ofstream out(CUSTOMERS_FILE, ios::binary | ios::app);
    if (typeChoice == 2) out.write((char*)&c, sizeof(ChequeAccount));
    else out.write((char*)&s, sizeof(SavingsAccount));
    out.close();

    cout << "\nAccount " << accPtr->accountNumber << " opened successfully!" << endl;
    clearBuffer();
}

// New Function: Generate Branch Report (.csv)
void generateBranchReport(string bCode) {
    ifstream in(CUSTOMERS_FILE, ios::binary);
    string fileName = "Report_" + bCode + ".csv";
    ofstream out(fileName);

    out << "Account Number,Full Name,Account Type,Balance\n";

    SavingsAccount temp;
    int count = 0;
    while (in.read((char*)&temp, sizeof(SavingsAccount))) {
        if (string(temp.branchCode) == bCode) {
            out << temp.accountNumber << "," << temp.fullName << "," 
                << temp.accountType << "," << fixed << setprecision(2) << temp.balance << "\n";
            count++;
        }
    }
    in.close();
    out.close();
    cout << "Report generated: " << fileName << " (" << count << " records found)." << endl;
}

// New Function: Apply Interest to all branch accounts
void applyBranchInterest(string bCode) {
    fstream file(CUSTOMERS_FILE, ios::binary | ios::in | ios::out);
    SavingsAccount temp;
    int count = 0;

    while (file.read((char*)&temp, sizeof(SavingsAccount))) {
        if (string(temp.branchCode) == bCode) {
            double rate = (string(temp.accountType) == "Savings") ? 0.04 : 0.01;
            temp.balance += (temp.balance * rate);

            int pos = (int)file.tellg() - sizeof(SavingsAccount);
            file.seekp(pos);
            file.write((char*)&temp, sizeof(SavingsAccount));
            file.seekg(file.tellp()); // Reset for next read
            count++;
        }
    }
    file.close();
    cout << "Interest applied to " << count << " accounts in branch " << bCode << endl;
}

void processTransaction(string targetAcc) {
    fstream file(CUSTOMERS_FILE, ios::binary | ios::in | ios::out);
    SavingsAccount acc;
    bool found = false;

    while (file.read((char*)&acc, sizeof(SavingsAccount))) {
        if (string(acc.accountNumber) == targetAcc) {
            found = true;
            string pin;
            cout << "Enter PIN: "; cin >> pin;
            if (encrypt(pin) == string(acc.pin)) {
                int act; double amt;
                cout << "1. Deposit\n2. Withdraw\nChoice: "; cin >> act;
                cout << "Amount: R"; cin >> amt;
                if (act == 2 && amt > acc.balance) cout << "Insufficient funds." << endl;
                else {
                    acc.balance += (act == 1) ? amt : -amt;
                    file.seekp((int)file.tellg() - sizeof(SavingsAccount));
                    file.write((char*)&acc, sizeof(SavingsAccount));
                    cout << "Transaction Success. New Balance: R" << acc.balance << endl;
                }
            } else cout << "Incorrect PIN." << endl;
            break;
        }
    }
    if (!found) cout << "Account not found." << endl;
    file.close();
    clearBuffer();
}

// --- MENUS ---

void tellerPortal() {
    string tid, tpass;
    cout << "\n--- TELLER LOGIN ---" << endl;
    cout << "ID: "; cin >> tid;
    cout << "Pass: "; cin >> tpass;

    ifstream in(TELLERS_FILE, ios::binary);
    Teller t; bool auth = false;
    while (in.read((char*)&t, sizeof(Teller))) {
        if (string(t.tellerID) == tid && string(t.password) == encrypt(tpass)) {
            auth = true; break;
        }
    }
    in.close();

    if (auth) {
        int tChoice = 0;
        while (tChoice != 6) {
            cout << "\n--- BRANCH: " << t.branchCode << " | TELLER: " << t.fullName << " ---" << endl;
            cout << "1. Open New Customer Account\n2. Assisted Transaction\n3. Apply Monthly Interest\n4. Generate Branch CSV Report\n";
            if (string(t.tellerID) == "T001") cout << "5. Register New Teller (Admin Only)\n";
            cout << "6. Logout\nChoice: ";
            if (!(cin >> tChoice)) { clearBuffer(); continue; }

            switch(tChoice) {
                case 1: registerCustomer(t.branchCode); break;
                case 2: { string a; cout << "Acc No: "; cin >> a; processTransaction(a); break; }
                case 3: applyBranchInterest(t.branchCode); break;
                case 4: generateBranchReport(t.branchCode); break;
                case 5: if (string(t.tellerID) == "T001") registerNewTeller(); break;
            }
        }
    } else cout << "Login Failed." << endl;
}

int main() {
    srand(time(0));
    initializeSystem();
    int choice = 0;
    while (choice != 3) {
        cout << "\n--- STANDARD BANK MULTI-BRANCH SYSTEM ---\n1. Teller Portal\n2. Customer Portal\n3. Exit\nSelection: ";
        if (!(cin >> choice)) { clearBuffer(); continue; }
        if (choice == 1) tellerPortal();
        if (choice == 2) { string a; cout << "Enter Acc No: "; cin >> a; processTransaction(a); }
    }
    return 0;
}