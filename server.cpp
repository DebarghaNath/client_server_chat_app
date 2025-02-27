#include <bits/stdc++.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdlib.h>
#include <mysql_driver.h>
#include <mysql_connection.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>

using namespace std;
using namespace sql;


mysql::MySQL_Driver * driver;
Connection *con = nullptr;
set<string> S;
map<string, int> nameToSockfd;
int N = 10000;
pthread_mutex_t map_mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct
{
    int sock_fd;
}Data;

void *Client(void *arg) 
{
    int correct  = 0;
   

    Data *d = (Data*)arg;  
    int clientsock_fd = d->sock_fd;
    delete d; 

    char temp_buffer[N];
    char buffer[N];
    bzero(buffer, N);

    int n = read(clientsock_fd, buffer, N - 1);
    if (n <= 0) {
        close(clientsock_fd);
        pthread_exit(NULL);
    }
    string type,name,password;
    stringstream ss(buffer);
    getline(ss, type, '#');
    getline(ss, name, '#'); 
    getline(ss, password);
    //cout<<type<<endl;
    if(type=="signup")
    {
        try 
        {
            con->setSchema("chatapp");
            Statement *stmt = con->createStatement();
            if (type == "signup") 
            {
                string query = "INSERT INTO users (name, password) VALUES ('" + name + "', '" + password + "');";
                stmt->execute(query);
                cout << "User added successfully!" << endl;
                correct  = 1;
                delete stmt;
            }
        }
        catch (SQLException &e) 
        {
            cerr << "SQL Error: " << e.what() << std::endl;
            close(clientsock_fd);
            pthread_exit(NULL);
        }
    }
    if(type=="signin")
    {
       
        try 
        {
            con->setSchema("chatapp");
            Statement *stmt = con->createStatement();
            string query_check = "SELECT * FROM users WHERE name = '" + name + "' and password = '" +password + "';";
            ResultSet *res = stmt->executeQuery(query_check);
            if (res->next()) 
            {
                cout<<"User exists!"<<endl;
                correct = 1;
            } 
            else 
            {
                cout<<"Not a valid user"<<endl;
            }

            delete res;
        }
        catch (SQLException &e) 
        {
            cerr << "SQL Error: " << e.what() << std::endl;
            close(clientsock_fd);
            pthread_exit(NULL);
        }
    }
    
    if(correct==1)
    {
        name.erase(name.find_last_not_of("\r\n") + 1);
        string nm = name;
        pthread_mutex_lock(&map_mutex);
        nameToSockfd[name] = clientsock_fd;
        pthread_mutex_unlock(&map_mutex);

        string joinMsg = "\n" + name + " joined the chat.\n";
        cout << joinMsg << endl;cout<<endl;
        pthread_mutex_lock(&map_mutex);
        for (auto &p : nameToSockfd) 
        {
            if(p.second!=clientsock_fd)
                write(p.second, joinMsg.c_str(), joinMsg.size());
        }
        string clientmsg = "successfully joined the chat \n";
        write(clientsock_fd,clientmsg.c_str(),clientmsg.size());
        pthread_mutex_unlock(&map_mutex);
        while (true) {
            bzero(temp_buffer, N);
            ssize_t bytes_read = read(clientsock_fd, temp_buffer, N - 1);
            if (bytes_read < 0) {
                perror("Read failed");
                break;
            }
            if (bytes_read == 0) {
                break;
            }
            
            string msg(temp_buffer);
            cout<<nm<<": "<<msg<<endl;
            pthread_mutex_lock(&map_mutex);
            msg = nm+": "+msg;
            for(int i=0;i<msg.size();i++)
            {
                temp_buffer[i] = msg[i];
            }
            for (auto &p : nameToSockfd) {
                write(p.second, temp_buffer, msg.size()+1);
            }
            pthread_mutex_unlock(&map_mutex);

            if (msg == "bye") {
                break;
            }
        }
    
        pthread_mutex_lock(&map_mutex);
        for (auto it = nameToSockfd.begin(); it != nameToSockfd.end(); ) {
            if (it->second == clientsock_fd) {
                it = nameToSockfd.erase(it);
            } else {
                ++it;
            }
        }
        pthread_mutex_unlock(&map_mutex);
        delete con;
        shutdown(clientsock_fd, SHUT_RDWR);
        close(clientsock_fd);
        pthread_exit(NULL);
    }
    else
    {
        delete con;
        string clientmsg = "Wrong input for sql";
        write(clientsock_fd,clientmsg.c_str(),clientmsg.size());
        shutdown(clientsock_fd, SHUT_RDWR);
        close(clientsock_fd);
        pthread_exit(NULL);
    }
}

int main(int argc, char* argv[]) 
{
    try 
    {
        driver = mysql::get_mysql_driver_instance();
        con = driver->connect("tcp://127.0.0.1:3306", "root", "Parak@2004");
        con->setSchema("chatapp");
    } 
    catch (SQLException &e) 
    {
        cerr << "MySQL Connection Error: " << e.what() << endl;
    }

    int port_no, sock_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len;
    
    if (argc < 2) {
        perror("provide valid inputs");
        exit(EXIT_FAILURE);
    }
    port_no = atoi(argv[1]);

    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("socket not created properly");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("wrong set socket option");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_port = htons(port_no);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("binding error at port");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(sock_fd, 5) < 0) {
        perror("listen failed");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }

    cout << "server is listening at port no: " << port_no << endl;
    client_len = sizeof(client_addr);

    while (true) {
        int clientsock_fd = accept(sock_fd, (struct sockaddr*)&client_addr, &client_len);
        if (clientsock_fd < 0) {
            perror("failed accept");
            continue;
        }

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
        cout << "client connected to server" << endl;
        cout << "ip address: " << client_ip << endl;

        pthread_t thread_id;
        Data *d = new Data();
        d->sock_fd = clientsock_fd;
        if (pthread_create(&thread_id, NULL, Client, (void*)d) != 0) {
            perror("pthread_create failed");
            delete d;
            continue;
        }
        pthread_detach(thread_id);
    }

    close(sock_fd);
    exit(EXIT_SUCCESS);
}
