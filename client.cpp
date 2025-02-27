#include <bits/stdc++.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <limits>  
using namespace std;

int N = 10000;
int sock_fd;


int exit_ = 0;
int print_ = 0;
void * Read(void * arg)
{
    char buffer[N];
    while(true)
    {

        if(exit_)
        {
            if(!print_)
            {
                print_ = 1;
                cout<<"You have successfully Exited Group Chat"<<endl;
            }
            exit(EXIT_FAILURE);
        }
        bzero(buffer,N);    
        ssize_t n = read(sock_fd,buffer,N-1);
        if(n<0)
        {
            perror("Error in socket reading");
            exit(EXIT_FAILURE);
        }
        cout<<endl; cout<<endl;
        cout<<buffer<<endl;
    }
}
void * Write(void * arg)
{
    while(true)
    {
        fflush(stdout);
        cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        cout << "Enter your message: ";
        string message;
        getline(cin, message);
        cout << endl; 

        if(message.empty())
        {
            continue; 
        }

        ssize_t n = write(sock_fd, message.c_str(), message.size()); 
        if (n <= 0)
        {
            perror("writing failed");
            exit(EXIT_FAILURE);
        }

        exit_ = (message == "exit");
        if(exit_)
        {
            if(!print_)
            {
                print_ = 1;
                cout << "You have successfully Exited Group Chat" << endl;
            }
            exit(0);
        }
    }
}


int main(int argc, char* argv[])
{
    int port_no;
    char* ip_address;
    struct sockaddr_in server_addr;
    int signup;
    if (argc < 3)
    {
        perror("enter correct inputs");
        exit(EXIT_FAILURE);
    }
    ip_address = argv[1];
    port_no = atoi(argv[2]);

    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0)
    {
        perror("socket error at client");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_port = htons(port_no);
    server_addr.sin_family = AF_INET;

    if (inet_pton(AF_INET, ip_address, &server_addr.sin_addr) <= 0)
    {
        perror("Invalid address");
        exit(EXIT_FAILURE);
    }

    if (connect(sock_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("connection error");
        exit(EXIT_FAILURE);
    }

    cout << "+----------------------------------------------------------------------------------+" << endl;
    cout << "|                                                                                  |" << endl;
    cout << "| SignIn or SignUp                                                                 |" << endl;                     
    cout << "| for SignIn enter 1 and for SignUp enter 0                                        |" << endl;                                                                                                                                               
    cout << "+----------------------------------------------------------------------------------+" << endl;
    cout<<"Enter Choice: "<<endl;

    cin>>signup;
    string message;
    if(signup == 0)
    {
        string temp;
        message = "signup";
        cout<<"enter name: "<<endl;
        cin>>temp;
        message=message+"#"+temp;
        cout<<"enter password: "<<endl;
        cin>>temp;
        message=message+"#"+temp;
        //cout<<message<<endl;
    }
    else
    {
        string temp;
        message = "signin";
        cout<<"enter name: "<<endl;
        cin>>temp;
        message=message+"#"+temp;
        cout<<"enter password: "<<endl;
        cin>>temp;
        message=message+"#"+temp;
       // cout<<message<<endl;
    }
    
    ssize_t n = write(sock_fd, message.c_str(), message.size()); 
    if (n <= 0)
    {
        perror("writing failed");
        exit(EXIT_FAILURE);
    }
    pthread_t t1,t2;
    pthread_create(&t1,NULL,Read,NULL);
    pthread_create(&t2,NULL,Write,NULL);

    pthread_join(t1,NULL);
    pthread_join(t2,NULL);
    close(sock_fd);
    exit(EXIT_SUCCESS);
}
