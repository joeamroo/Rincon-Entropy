#include <iostream>
#include <iomanip>
#include <cmath>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sstream>
#include <vector>
#include <map>


struct Task {
    char ids;
    int times;
};

// Function to Handle Requests and do the main bulk of the work.
void handle_requests(int newsockfd) {


    size_t numTasks;
    read(newsockfd, &numTasks, sizeof(size_t));

    // vector to store the tasks
    std::vector<Task> tasks(numTasks);

    // Receive the vector of tasks from the client
    read(newsockfd, tasks.data(), numTasks * sizeof(Task));

    // Run the Entropy Function
    std::map<char, int> freq;
    int currFreq = 0;
    double currH = 0.0;
    std::vector<double> entropy;

    for (const auto& task : tasks) {
        int extraFreq = task.times;
        double NFreq = currFreq + extraFreq;
        double H = 0.0;
        if (NFreq == extraFreq) {
            H = 0;
        } else {
            double currentTerm = 0.0;
            if (freq[task.ids] != 0) {
                currentTerm = freq[task.ids] * log2(freq[task.ids]);
            }
            double newTerm = (freq[task.ids] + extraFreq) * log2(freq[task.ids] + extraFreq);
            H = log2(NFreq) - ((log2(currFreq) - currH) * currFreq - currentTerm + newTerm) / NFreq;
        }
        currH = H;
        currFreq = NFreq;
        freq[task.ids] += extraFreq;
        entropy.push_back(round(H * 100) / 100);
    }

    // Send back the entropy to client
    write(newsockfd, entropy.data(), entropy.size() * sizeof(double));
}
//Fireman
void fireman(int) {
    while (waitpid(-1, nullptr, WNOHANG) > 0) {}
}

int main(int argc, char *argv[]) {
    int sockfd, newsockfd, portno;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;
    if (argc < 2) {
        fprintf(stderr, "ERROR, no port provided\n");
        exit(1);
    }
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("ERROR opening socket");
        exit(1);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    int yes = 1;
   if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &yes, sizeof(yes)) == -1) {
       perror("setsockopt");
       return 1;
   }

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("ERROR on binding");
        exit(1);
    }

    listen(sockfd, 5);
    clilen = sizeof(cli_addr);

    // Signal SIGCHLD
    signal(SIGCHLD, fireman);

    while (1) {
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd < 0) {
            perror("ERROR on accept");
            exit(1);
        }
        int pid = fork();
        if (pid < 0) {
            perror("ERROR on fork");
            exit(1);
        }
        if (pid == 0) {
            // Child process
            close(sockfd);
            handle_requests(newsockfd);
            close(newsockfd);
            exit(0);
        } else {
            // Parent process
            close(newsockfd);
        }
    }
    close(sockfd);
    return 0;
}