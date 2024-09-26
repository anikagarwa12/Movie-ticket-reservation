#include <iostream>
#include <mysql.h>
#include <windows.h>
#include <sstream>
#include <ctime>
#include <cstdlib> // For random payment simulation

using namespace std;

const char* HOST = "localhost";
const char* USER = "root";
const char* PW = "your_password"; // Update with actual password
const char* DB = "mydb";

class Seats {
private:
    int Seat[5][10]; // Seat matrix
    float Price[5][10]; // Price matrix for dynamic pricing

public:
    Seats() {
        // Initialize all seats as available and set base price
        for (int i = 0; i < 5; i++) {
            for (int j = 0; j < 10; j++) {
                Seat[i][j] = 1; 
                Price[i][j] = 10.0; // Base price for all seats
            }
        }
    }

    int getSeatStatus(int row, int seatNumber) {
        if (row < 1 || row > 5 || seatNumber < 1 || seatNumber > 10) {
            return -1;
        }
        return Seat[row - 1][seatNumber - 1];
    }

    void reserveSeat(int row, int seatNumber) {
        if (row < 1 || row > 5 || seatNumber < 1 || seatNumber > 10) {
            return;
        }
        Seat[row - 1][seatNumber - 1] = 0; // Reserve seat
    }

    float getSeatPrice(int row, int seatNumber) {
        return Price[row - 1][seatNumber - 1];
    }

    void adjustPricing(int row, int seatNumber) {
        // Increase the price as more seats are reserved
        int reservedSeats = 0;
        for (int i = 0; i < 5; i++) {
            for (int j = 0; j < 10; j++) {
                if (Seat[i][j] == 0) reservedSeats++;
            }
        }
        float increaseRate = (float(reservedSeats) / 50.0) * 0.5; // 50% max price hike
        Price[row - 1][seatNumber - 1] = 10.0 + 10.0 * increaseRate;
    }

    void display() {
        cout << "   ";
        for (int i = 0; i < 10; i++) {
            cout << " " << i + 1;
        }
        cout << endl;

        for (int row = 0; row < 5; row++) {
            cout << row + 1 << " ";
            for (int col = 0; col < 10; col++) {
                cout << (Seat[row][col] == 1 ? "- " : "X ");
            }
            cout << "|" << endl;
        }
        cout << "-----------------------" << endl;
    }

    void getDB(MYSQL* conn) {
        string query = "SELECT RowNumber, SeatNumber, Seat, Price FROM Ticket";
        if (mysql_query(conn, query.c_str())) {
            cout << "Error: " << mysql_error(conn) << endl;
        }

        MYSQL_RES* result;
        result = mysql_store_result(conn);
        if (!result) {
            cout << "Error: " << mysql_error(conn) << endl;
            return;
        }

        MYSQL_ROW row;
        while ((row = mysql_fetch_row(result))) {
            int rowNumber = atoi(row[0]);
            int seatNumber = atoi(row[1]);
            int seatStatus = atoi(row[2]);
            float price = atof(row[3]);
            Seat[rowNumber - 1][seatNumber - 1] = seatStatus;
            Price[rowNumber - 1][seatNumber - 1] = price;
        }
        mysql_free_result(result);
    }
};

bool authenticateUser(MYSQL* conn, const string& username, const string& password) {
    stringstream ss;
    ss << "SELECT * FROM Users WHERE Username = '" << username << "' AND Password = '" << password << "'";
    if (mysql_query(conn, ss.str().c_str())) {
        cout << "Error: " << mysql_error(conn) << endl;
        return false;
    }

    MYSQL_RES* result = mysql_store_result(conn);
    if (result && mysql_num_rows(result) > 0) {
        mysql_free_result(result);
        return true;
    }
    mysql_free_result(result);
    return false;
}

bool simulatePayment() {
    srand(time(0)); // Seed the random number generator
    int outcome = rand() % 2; // Random 0 or 1 (50% chance)
    return outcome == 1;
}

int main() {
    Seats s;
    MYSQL* conn = mysql_init(NULL);
    if (conn == NULL) {
        cout << "mysql_init() failed\n";
        return EXIT_FAILURE;
    }

    if (!mysql_real_connect(conn, HOST, USER, PW, DB, 3306, NULL, 0)) {
        cout << "Error: " << mysql_error(conn) << endl;
        mysql_close(conn);
        return EXIT_FAILURE;
    } else {
        cout << "Logged In Database!" << endl;
    }

    // User Authentication
    string username, password;
    cout << "Enter Username: ";
    cin >> username;
    cout << "Enter Password: ";
    cin >> password;

    if (!authenticateUser(conn, username, password)) {
        cout << "Authentication Failed!" << endl;
        mysql_close(conn);
        return EXIT_FAILURE;
    }
    cout << "Welcome, " << username << "!" << endl;

    if (mysql_query(conn, "CREATE TABLE IF NOT EXISTS Ticket (RowNumber INT, SeatNumber INT, Seat INT, Price FLOAT, HoldUntil TIMESTAMP NULL)")) {
        cout << "Error: " << mysql_error(conn) << endl;
    }

    bool exit = false;
    while (!exit) {
        system("cls");
        cout << endl;
        cout << "Welcome To Movie Ticket Booking System" << endl;
        cout << "******************************************" << endl;
        cout << "1. Reserve A Ticket" << endl;
        cout << "2. Exit" << endl;
        cout << "Enter Your Choice: ";
        int val;
        cin >> val;

        if (val == 1) {
            s.getDB(conn);
            s.display();

            int row, col;
            cout << "Enter Row (1-5): ";
            cin >> row;
            cout << "Enter Seat Number (1-10): ";
            cin >> col;

            if (row < 1 || row > 5 || col < 1 || col > 10) {
                cout << "Invalid Row or Seat Number!" << endl;
                Sleep(3000);
                continue;
            }

            int seatStatus = s.getSeatStatus(row, col);
            if (seatStatus == -1) {
                cout << "Invalid Row or Seat Number!" << endl;
                Sleep(3000);
                continue;
            }

            if (seatStatus == 0) {
                cout << "Sorry: Seat is already reserved!" << endl;
                Sleep(3000);
                continue;
            }

            // Display seat price and confirm reservation
            float seatPrice = s.getSeatPrice(row, col);
            cout << "The price for Row " << row << " Seat " << col << " is $" << seatPrice << ". Confirm (y/n): ";
            char confirm;
            cin >> confirm;
            if (confirm == 'y' || confirm == 'Y') {
                // Simulate payment
                if (simulatePayment()) {
                    s.reserveSeat(row, col);
                    stringstream ss;
                    ss << "UPDATE Ticket SET Seat = 0, Price = " << seatPrice << " WHERE RowNumber = " << row << " AND SeatNumber = " << col;
                    if (mysql_query(conn, ss.str().c_str())) {
                        cout << "Error: " << mysql_error(conn) << endl;
                    } else {
                        cout << "Seat Reserved Successfully!" << endl;
                    }
                } else {
                    cout << "Payment Failed! Please try again." << endl;
                }
            }
            Sleep(3000);
        } else if (val == 2) {
            exit = true;
            cout << "Good Luck!" << endl;
            Sleep(3000);
        } else {
            cout << "Invalid Input" << endl;
            Sleep(3000);
        }
    }

    mysql_close(conn);
    return 0;
}
