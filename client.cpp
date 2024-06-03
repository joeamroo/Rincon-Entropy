#include <iomanip>
#include <iostream>
#include <netdb.h>
#include <vector>
#include <pthread.h>
#include <sstream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>




struct Task {
    char ids;
    int times;
};

struct ThreadData {
    std::string portNumber;
    std::string hostName;
    std::vector<Task> input;
    std::vector<double>* entropy;
};


//Create the vector of tasks
std::vector<Task> createTasks(const std::string& input) {
    std::istringstream iss(input);
    char ids;
    int times;
    std::vector<Task> tasks;
    while (iss >> ids >> times) {
        tasks.push_back({ids, times});
    }
    return tasks;
}

void* handle_threads(void *arg) {
    ThreadData* data = (ThreadData*)arg;

    int sockfd, portno;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    portno = atoi(data->portNumber.c_str()); 
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    server = gethostbyname(data->hostName.c_str());

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(portno);

    connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));


    size_t numTasks = data->input.size();
    write(sockfd, &numTasks, sizeof(size_t)); 

    write(sockfd, data->input.data(), data->input.size() * sizeof(Task));


    // Receive the entropy vector from server
    std::vector<double> temp(numTasks);
    read(sockfd, temp.data(), numTasks * sizeof(double));
    *(data->entropy) = temp;

    close(sockfd); // Close the socket (very important!)
}

int main(int argc, char *argv[]) {
    // 1. Read the information from STDIN.
    // 2. Create n child threads (where n is the number of strings from the input).
    // 3. Assign each thread to handle_thread function.
    // 4. Wait for all threads to complete.
    // 5. Print the scheduling information for each CPU and the entropy.

    //buffer contains each line
    std::string buffer;
    std::vector<std::string> inputs;
    //parse each line into an organized structure
    while(std::getline(std::cin, buffer)) {
      inputs.push_back(buffer);
    }

    //amount of threads
    int n = inputs.size();
    std::vector<pthread_t> threads(n);
    std::vector<ThreadData> threadData(n);
    std::vector<std::vector<double>> entropy(n);

    for (int i = 0; i < n; i++) {
        threadData[i].input = createTasks(inputs[i]);
        threadData[i].hostName = argv[1];
        threadData[i].portNumber = argv[2];
        threadData[i].entropy = &entropy[i];

        pthread_create(&threads[i], NULL, handle_threads, (void*)&threadData[i]);
    }

    for (int i = 0; i < n; i++) {
        pthread_join(threads[i], NULL);
        std::cout << "CPU " << i + 1 << std::endl;
        std::cout << "Task scheduling information: ";
        for(int j = 0; j < threadData[i].input.size(); j++) {
          if(j != threadData[i].input.size()-1)
            std::cout << threadData[i].input[j].ids << "(" << threadData[i].input[j].times << "), ";
          else 
            std::cout << threadData[i].input[j].ids << "(" << threadData[i].input[j].times << ")";
        }
        std::cout << std::endl << "Entropy for CPU " << i + 1 << std::endl;
        for (double e : entropy[i]) {
            std::cout << std::fixed << std::setprecision(2) << e << " ";
        }
      if(i != n-1)
          std::cout << std::endl << std::endl;
    }
    return 0;
}