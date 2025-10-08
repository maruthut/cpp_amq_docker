#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <cstring>
#include <sstream>
#include <iomanip>

class SimpleStompClient {
private:
    int sockfd;
    std::string host;
    int port;
    bool connected;

public:
    SimpleStompClient(const std::string& h, int p) : host(h), port(p), connected(false), sockfd(-1) {}
    
    ~SimpleStompClient() {
        disconnect();
    }

    bool connect() {
        // Create socket
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            std::cerr << "Error creating socket" << std::endl;
            return false;
        }

        // Set up server address
        struct sockaddr_in server_addr;
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        
        // Resolve hostname to IP address
        struct hostent* host_entry = gethostbyname(host.c_str());
        if (host_entry == nullptr) {
            std::cerr << "Error resolving hostname: " << host << std::endl;
            close(sockfd);
            return false;
        }
        server_addr.sin_addr = *((struct in_addr*) host_entry->h_addr_list[0]);

        // Connect to server
        if (::connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            std::cerr << "Error connecting to ActiveMQ at " << host << ":" << port << std::endl;
            close(sockfd);
            return false;
        }

        // Send STOMP CONNECT frame
        std::string connectFrame = "CONNECT\n";
        connectFrame += "accept-version:1.0,1.1,1.2\n";
        connectFrame += "host:" + host + "\n";
        connectFrame += "\n\0";

        if (send(sockfd, connectFrame.c_str(), connectFrame.length(), 0) < 0) {
            std::cerr << "Error sending CONNECT frame" << std::endl;
            close(sockfd);
            return false;
        }

        // Read response
        char buffer[1024];
        int bytes_read = recv(sockfd, buffer, sizeof(buffer), 0);
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            std::string response(buffer);
            if (response.find("CONNECTED") != std::string::npos) {
                connected = true;
                std::cout << "[PRODUCER] Successfully connected to ActiveMQ via STOMP" << std::endl;
                return true;
            }
        }

        std::cerr << "Failed to receive CONNECTED frame" << std::endl;
        close(sockfd);
        return false;
    }

    bool sendMessage(const std::string& destination, const std::string& message) {
        if (!connected) {
            std::cerr << "Not connected to broker" << std::endl;
            return false;
        }

        std::string sendFrame = "SEND\n";
        sendFrame += "destination:" + destination + "\n";
        sendFrame += "content-type:text/plain\n";
        sendFrame += "content-length:" + std::to_string(message.length()) + "\n";
        sendFrame += "\n" + message + "\0";

        if (send(sockfd, sendFrame.c_str(), sendFrame.length(), 0) < 0) {
            std::cerr << "Error sending message" << std::endl;
            return false;
        }

        return true;
    }

    void disconnect() {
        if (connected && sockfd >= 0) {
            std::string disconnectFrame = "DISCONNECT\n\n\0";
            send(sockfd, disconnectFrame.c_str(), disconnectFrame.length(), 0);
            close(sockfd);
            connected = false;
            std::cout << "[PRODUCER] Disconnected from ActiveMQ" << std::endl;
        }
    }
};

std::string generateMessageId(int index) {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream ss;
    ss << "MSG_" << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S") 
       << "_INDEX_" << index;
    return ss.str();
}

int main() {
    std::cout << "[PRODUCER] Starting C++ Producer Application" << std::endl;
    
    // Connection parameters
    std::string broker_host = "activemq";  // Docker service name
    int broker_port = 61613;  // STOMP port
    std::string queue_destination = "/queue/ProjectQueue";
    
    // Retry connection logic to handle broker startup delays
    SimpleStompClient client(broker_host, broker_port);
    bool connection_successful = false;
    int max_retries = 10;
    
    for (int retry = 1; retry <= max_retries; ++retry) {
        std::cout << "[PRODUCER] Connection attempt " << retry << "/" << max_retries << std::endl;
        
        if (client.connect()) {
            connection_successful = true;
            break;
        }
        
        if (retry < max_retries) {
            std::cout << "[PRODUCER] Retrying in 3 seconds..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(3));
        }
    }
    
    if (!connection_successful) {
        std::cerr << "[PRODUCER] Failed to connect to ActiveMQ after " << max_retries << " attempts" << std::endl;
        return 1;
    }
    
    // Send 10 messages
    const int message_count = 10;
    std::cout << "[PRODUCER] Sending " << message_count << " messages to " << queue_destination << std::endl;
    
    for (int i = 1; i <= message_count; ++i) {
        std::string message_id = generateMessageId(i);
        std::string full_message = "Hello from C++ Producer - " + message_id;
        
        std::cout << "[PRODUCER] Sending message " << i << "/" << message_count 
                  << ": " << full_message << std::endl;
        
        if (client.sendMessage(queue_destination, full_message)) {
            std::cout << "[PRODUCER] Message " << i << " sent successfully" << std::endl;
        } else {
            std::cerr << "[PRODUCER] Failed to send message " << i << std::endl;
        }
        
        // Sleep for 1 second between messages
        if (i < message_count) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    
    std::cout << "[PRODUCER] All messages sent. Disconnecting..." << std::endl;
    client.disconnect();
    
    std::cout << "[PRODUCER] Producer application completed successfully" << std::endl;
    return 0;
}