#include <iostream>
#include <vector>
#include <map>
#include <random>
#include <string>
#include <sqlite3.h>

class SlotMachine {
private:
    std::map<char, int> symbols = {{'A', 5}, {'B', 4}, {'C', 3}, {'D', 2}};
    sqlite3* db;

    void createTables() {
        const char* playerTableQuery = R"(
            CREATE TABLE IF NOT EXISTS players (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                name TEXT NOT NULL,
                age INTEGER,
                card TEXT,
                balance REAL DEFAULT 0
            );
        )";

        const char* historyTableQuery = R"(
            CREATE TABLE IF NOT EXISTS spin_history (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                player_id INTEGER,
                bet REAL NOT NULL,
                winnings REAL NOT NULL,
                balance REAL NOT NULL,
                FOREIGN KEY (player_id) REFERENCES players (id)
            );
        )";

        executeQuery(playerTableQuery);
        executeQuery(historyTableQuery);
    }

    void executeQuery(const char* query) {
        char* errorMessage = nullptr;
        if (sqlite3_exec(db, query, nullptr, nullptr, &errorMessage) != SQLITE_OK) {
            std::cerr << "Error executing query: " << errorMessage << std::endl;
            sqlite3_free(errorMessage);
        }
    }

    int getPlayerID(const std::string& name, int age, const std::string& card) {
        const char* query = "SELECT id FROM players WHERE name = ? AND age = ? AND card = ?";
        sqlite3_stmt* stmt;
        int playerId = -1;

        sqlite3_prepare_v2(db, query, -1, &stmt, nullptr);
        sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 2, age);
        sqlite3_bind_text(stmt, 3, card.c_str(), -1, SQLITE_STATIC);

        if (sqlite3_step(stmt) == SQLITE_ROW) {
            playerId = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
        return playerId;
    }

    void addPlayer(const std::string& name, int age, const std::string& card) {
        const char* query = "INSERT INTO players (name, age, card) VALUES (?, ?, ?)";
        sqlite3_stmt* stmt;

        sqlite3_prepare_v2(db, query, -1, &stmt, nullptr);
        sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 2, age);
        sqlite3_bind_text(stmt, 3, card.c_str(), -1, SQLITE_STATIC);

        if (sqlite3_step(stmt) != SQLITE_DONE) {
            std::cerr << "Error adding player." << std::endl;
        }
        sqlite3_finalize(stmt);
    }

    void updateBalance(int playerId, double newBalance) {
        const char* query = "UPDATE players SET balance = ? WHERE id = ?";
        sqlite3_stmt* stmt;

        sqlite3_prepare_v2(db, query, -1, &stmt, nullptr);
        sqlite3_bind_double(stmt, 1, newBalance);
        sqlite3_bind_int(stmt, 2, playerId);

        if (sqlite3_step(stmt) != SQLITE_DONE) {
            std::cerr << "Error updating balance." << std::endl;
        }
        sqlite3_finalize(stmt);
    }

    void saveSpinHistory(int playerId, double bet, double winnings, double balance) {
        const char* query = "INSERT INTO spin_history (player_id, bet, winnings, balance) VALUES (?, ?, ?, ?)";
        sqlite3_stmt* stmt;

        sqlite3_prepare_v2(db, query, -1, &stmt, nullptr);
        sqlite3_bind_int(stmt, 1, playerId);
        sqlite3_bind_double(stmt, 2, bet);
        sqlite3_bind_double(stmt, 3, winnings);
        sqlite3_bind_double(stmt, 4, balance);

        if (sqlite3_step(stmt) != SQLITE_DONE) {
            std::cerr << "Error saving spin history." << std::endl;
        }
        sqlite3_finalize(stmt);
    }

    std::vector<std::vector<char>> getSpinResult() {
        std::vector<std::vector<char>> result(3, std::vector<char>(3));
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dist(0, symbols.size() - 1);

        auto it = symbols.begin();
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                std::advance(it, dist(gen));
                result[i][j] = it->first;
                it = symbols.begin();
            }
        }
        return result;
    }

    void displaySpinResult(const std::vector<std::vector<char>>& result) {
        for (const auto& row : result) {
            for (const char& symbol : row) {
                std::cout << symbol << " ";
            }
            std::cout << std::endl;
        }
    }

public:
    SlotMachine() {
        if (sqlite3_open("slot_machine.db", &db) != SQLITE_OK) {
            std::cerr << "Error opening database." << std::endl;
            exit(1);
        }
        createTables();
    }

    ~SlotMachine() {
        sqlite3_close(db);
    }

    void play() {
        std::string name, card;
        int age;
        std::cout << "Enter your name: ";
        std::cin >> name;
        std::cout << "Enter your age: ";
        std::cin >> age;
        std::cout << "Enter your card: ";
        std::cin >> card;

        int playerId = getPlayerID(name, age, card);
        if (playerId == -1) {
            std::cout << "New player detected. Adding to database." << std::endl;
            addPlayer(name, age, card);
            playerId = getPlayerID(name, age, card);
        }

        double balance = 100.0; // Default balance
        updateBalance(playerId, balance);

        while (true) {
            std::cout << "Press 'p' to play, 'q' to quit: ";
            char choice;
            std::cin >> choice;
            if (choice == 'q') break;

            double bet;
            std::cout << "Enter your bet amount: ";
            std::cin >> bet;

            auto result = getSpinResult();
            displaySpinResult(result);

            double winnings = 0; // Calculate based on logic
            balance += winnings - bet;
            updateBalance(playerId, balance);

            saveSpinHistory(playerId, bet, winnings, balance);
            std::cout << "New Balance: " << balance << std::endl;
        }
    }
};

int main() {
    SlotMachine slotMachine;
    slotMachine.play();
    return 0;
}
