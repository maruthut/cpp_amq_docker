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
        connectFrame += "heart-beat:0,0\n";
        connectFrame += "\n";
        connectFrame += char(0); // null terminator

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
                std::cout << "[CONSUMER] Successfully connected to ActiveMQ via STOMP" << std::endl;
                return true;
            }
        }

        std::cerr << "Failed to receive CONNECTED frame" << std::endl;
        close(sockfd);
        return false;
    }

    bool subscribe(const std::string& destination, const std::string& subscription_id = "sub-1") {
        if (!connected) {
            std::cerr << "Not connected to broker" << std::endl;
            return false;
        }

        std::string subscribeFrame = "SUBSCRIBE\n";
        subscribeFrame += "destination:" + destination + "\n";
        subscribeFrame += "id:" + subscription_id + "\n";
        subscribeFrame += "ack:auto\n";
        subscribeFrame += "\n";
        subscribeFrame += char(0); // null terminator

        if (send(sockfd, subscribeFrame.c_str(), subscribeFrame.length(), 0) < 0) {
            std::cerr << "Error sending SUBSCRIBE frame" << std::endl;
            return false;
        }

        std::cout << "[CONSUMER] Successfully subscribed to " << destination << std::endl;
        return true;
    }

    std::string receiveMessage() {
        if (!connected) {
            return "";
        }

        char buffer[4096];
        int bytes_read = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
        
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            std::string response(buffer);
            
            // Parse STOMP MESSAGE frame
            if (response.find("MESSAGE") != std::string::npos) {
                // Find the start of the message body (after double newline)
                size_t body_start = response.find("\n\n");
                if (body_start != std::string::npos) {
                    body_start += 2;  // Skip the double newline
                    size_t body_end = response.find('\0', body_start);
                    if (body_end != std::string::npos) {
                        return response.substr(body_start, body_end - body_start);
                    } else {
                        // If no null terminator found, take rest of string
                        return response.substr(body_start);
                    }
                }
            }
        }
        
        return "";
    }

    void disconnect() {
        if (connected && sockfd >= 0) {
            std::string disconnectFrame = "DISCONNECT\n\n";
            disconnectFrame += char(0); // null terminator
            send(sockfd, disconnectFrame.c_str(), disconnectFrame.length(), 0);
            close(sockfd);
            connected = false;
            std::cout << "[CONSUMER] Disconnected from ActiveMQ" << std::endl;
        }
    }
};

int main() {
    std::cout << "[CONSUMER] Starting C++ Consumer Application" << std::endl;
    
    // Connection parameters
    std::string broker_host = "activemq";  // Docker service name
    int broker_port = 61613;  // STOMP port
    std::string queue_destination = "/queue/ProjectQueue";
    
    // Retry connection logic to handle broker startup delays
    SimpleStompClient client(broker_host, broker_port);
    bool connection_successful = false;
    int max_retries = 10;
    
    for (int retry = 1; retry <= max_retries; ++retry) {
        std::cout << "[CONSUMER] Connection attempt " << retry << "/" << max_retries << std::endl;
        
        if (client.connect()) {
            connection_successful = true;
            break;
        }
        
        if (retry < max_retries) {
            std::cout << "[CONSUMER] Retrying in 3 seconds..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(3));
        }
    }
    
    if (!connection_successful) {
        std::cerr << "[CONSUMER] Failed to connect to ActiveMQ after " << max_retries << " attempts" << std::endl;
        return 1;
    }
    
    // Subscribe to the queue
    if (!client.subscribe(queue_destination)) {
        std::cerr << "[CONSUMER] Failed to subscribe to " << queue_destination << std::endl;
        return 1;
    }
    
    // Receive messages
    const int expected_messages = 10;
    int messages_received = 0;
    
    std::cout << "[CONSUMER] Waiting for messages from " << queue_destination << std::endl;
    std::cout << "[CONSUMER] Expected to receive " << expected_messages << " messages" << std::endl;
    
    while (messages_received < expected_messages) {
        std::string message = client.receiveMessage();
        
        if (!message.empty()) {
            messages_received++;
            std::cout << "[CONSUMER] Received message " << messages_received << "/" 
                      << expected_messages << ": " << message << std::endl;
            
            // TODO: This is the ideal place to deserialize future JSON payloads
            // For now, we're processing simple string messages
            
            if (messages_received >= expected_messages) {
                std::cout << "[CONSUMER] All " << expected_messages 
                          << " messages received successfully!" << std::endl;
                break;
            }
        } else {
            // Small delay to prevent busy waiting
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    
    std::cout << "[CONSUMER] Total messages received: " << messages_received << std::endl;
    std::cout << "[CONSUMER] Shutting down gracefully..." << std::endl;
    
    client.disconnect();
    
    std::cout << "[CONSUMER] Consumer application completed successfully" << std::endl;
    return 0;
}