#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <vector>
#include <thread>
#include <mutex>

// Constants
#define PORT 8080
#define BUFFER_SIZE 1024

// Mutex for thread safety with chat history
std::mutex mtx;

// Struct to hold message data
struct Message {
    std::string sender;
    std::string message;
    std::string timestamp;

    // Constructor to initialize Message
    Message(const std::string& sender, const std::string& message, const std::string& timestamp)
        : sender(sender), message(message), timestamp(timestamp) {}
};

// Chat history vector
std::vector<Message> chatHistory;

// Function to clear the console screen
void clearScreen() {
    std::cout << "\033[2J\033[1;1H";  // Clears the console screen
}

// Function to print header
void printHeader() {
    std::cout << "\n-------------------------------------------------------------\n";
    std::cout << "                  Client Chat Application\n";
    std::cout << "-------------------------------------------------------------\n";
}

// Function to get the current timestamp
std::string getCurrentTime() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

// Function to convert text to emojis (simple implementation)
std::string convertToEmoji(const std::string& input) {
    std::string output = input;
    // Basic replacements (you can expand this)
    size_t pos = 0;
    while ((pos = output.find(":smile:", pos)) != std::string::npos) {
        output.replace(pos, 7, "😊");
        pos += 2;
    }
    return output;
}

// Function to display chat history
void displayChatHistory() {
    mtx.lock();
    std::cout << "\nChat History:\n";
    for (const Message& msg : chatHistory) {  // Use explicit type instead of 'auto'
        std::cout << "[" << msg.timestamp << "] " << msg.sender << ": " << msg.message << "\n";
    }
    mtx.unlock();
}

// Function to simulate typing indicator
void typingIndicator() {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << "Server is typing...\n";
    std::this_thread::sleep_for(std::chrono::seconds(2));
}

// Main function
int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};

    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "Socket creation failed" << std::endl;
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IPv4 address from text to binary
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        std::cerr << "Invalid address/Address not supported" << std::endl;
        return -1;
    }

    // Connect to the server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "Connection failed" << std::endl;
        return -1;
    }

    printHeader();
    std::cout << "Connected to the server!" << std::endl;

    // Communication loop
    while (true) {
        std::string message;
        std::cout << "\nYou (Client): ";
        std::getline(std::cin, message);

        if (message == "exit") {
            std::cout << "Exiting chat..." << std::endl;
            break;
        }

        // Convert message to emoji
        message = convertToEmoji(message);

        // Send message to server
        send(sock, message.c_str(), message.size(), 0);

        memset(buffer, 0, BUFFER_SIZE);
        int bytes_read = read(sock, buffer, BUFFER_SIZE);
        if (bytes_read > 0) {
            clearScreen();
            printHeader();
            std::cout << "Server (" << getCurrentTime() << "): " << buffer << std::endl;
        }

        // Save the message to chat history (corrected syntax)
        mtx.lock();
        chatHistory.push_back(Message("Client", message, getCurrentTime()));  // Use parentheses here
        mtx.unlock();

        // Display chat history
        displayChatHistory();
    }

    close(sock);
    return 0;
}
