#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
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
    std::cout << "                  Server Chat Application\n";
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
    std::cout << "Client is typing...\n";
    std::this_thread::sleep_for(std::chrono::seconds(2));
}

// Main function
int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};

    // Create socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket creation failed");
        return -1;
    }

    // Attach socket to the port
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("Setsockopt failed");
        return -1;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind the socket to the port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        return -1;
    }

    // Start listening for incoming connections
    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        return -1;
    }

    printHeader();
    std::cout << "Server is listening on port " << PORT << std::endl;

    // Accept incoming connection
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
        perror("Accept failed");
        return -1;
    }
    std::cout << "Client connected!" << std::endl;

    // Communication loop
    while (true) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_read = read(new_socket, buffer, BUFFER_SIZE);
        if (bytes_read <= 0) {
            std::cout << "Client disconnected" << std::endl;
            break;
        }
        
        // Convert received message to emoji
        std::string receivedMessage = convertToEmoji(std::string(buffer));

        clearScreen();
        printHeader();
        std::cout << "Client (" << getCurrentTime() << "): " << receivedMessage << std::endl;

        // Save the message to chat history (corrected syntax)
        mtx.lock();
        chatHistory.push_back(Message("Client", receivedMessage, getCurrentTime())); // Use parentheses here
        mtx.unlock();

        // Display chat history
        displayChatHistory();

        // Simulate typing indicator
        typingIndicator();

        // Send response back to client
        std::string response;
        std::cout << "You (Server): ";
        std::getline(std::cin, response);

        if (response == "exit") {
            std::cout << "Exiting chat..." << std::endl;
            break;
        }

        // Convert response to emoji
        response = convertToEmoji(response);
        send(new_socket, response.c_str(), response.size(), 0);

        // Save response to chat history
        mtx.lock();
        chatHistory.push_back(Message("Server", response, getCurrentTime()));
        mtx.unlock();
    }

    close(new_socket);
    close(server_fd);
    return 0;
}
