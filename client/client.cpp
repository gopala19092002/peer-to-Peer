#include <sys/socket.h> // Used for sockets
#include <string.h>     // Used for memset
#include <netdb.h>      // getnameinfo  - address-to-name translation in protocol-independent manner
#include <vector>       // VECTORS
#include <thread>       // Used for threads
#include <pwd.h>        // Used for finding user name for absolute path
#include <fstream>      // Used for fstream
#include <sstream>      // stringstream(Input)
#include <unistd.h>     // Read/Write, etc... operations
#include <algorithm>    // Find(map)
#include <arpa/inet.h>  // Close
#include <unordered_map>// UNORDERED MAP
#include "sha1.h"       // SHA1
#include <sys/stat.h>   // For Current/Present Working Directory
#include <fcntl.h>      // Used for finding user name for absolute path

using namespace std;
#define chunksize 524288
#define arraysize 16384
string home_dir;
string usid;
char ch[4100];
string cur_direc;
bool synchronization;
string fname;

vector<string> split(string st, string slash)
{
    string str;
    size_t loc = 0;
    vector<string> res;
    while ((loc = st.find(slash)) != string::npos)
    {
        str = st.substr(0, loc);
        res.push_back(str);
        st.erase(0, loc + slash.length());
    }
    res.push_back(st);
    return res;
}

string absolute_path(string path)
{
    string pwd = cur_direc;
    string abs_path = "";
    if (path[0] == '~')
        abs_path = string(home_dir) + path.substr(1, path.length() - 1);

    else if (path[0] == '.' && path[1] == '/')
        abs_path = pwd + "/" + path.substr(2, path.length() - 2);

    else if (path[0] == '/')
        abs_path = path;

    else
        abs_path = pwd + "/" + path;

    return abs_path;
}

int check_dir(string path)
{
    struct stat str;

    if (stat(path.c_str(), &str) != 0)
        return 0;
    if (S_ISDIR(str.st_mode))
        return 1;
    else
        return 0;
}

mode_t check_mode(string path)
{
    struct stat str;
    stat(path.c_str(), &str);
    mode_t mode = 0;

    mode = mode | ((str.st_mode & S_IRGRP) ? 0040 : 0);
    mode = mode | ((str.st_mode & S_IWGRP) ? 0020 : 0);
    mode = mode | ((str.st_mode & S_IXGRP) ? 0010 : 0);
    mode = mode | ((str.st_mode & S_IROTH) ? 0004 : 0);
    mode = mode | ((str.st_mode & S_IWOTH) ? 0002 : 0);
    mode = mode | ((str.st_mode & S_IXOTH) ? 0001 : 0);
    mode = mode | ((str.st_mode & S_IRUSR) ? 0400 : 0);
    mode = mode | ((str.st_mode & S_IWUSR) ? 0200 : 0);
    mode = mode | ((str.st_mode & S_IXUSR) ? 0100 : 0);

    return mode;
}

void download_file(string file_name, string destination_path)
{
    string source = absolute_path(file_name);
    string dest = absolute_path(destination_path);
    string destination = dest + "/" + file_name;

    if (!(check_dir(dest)))
    {
        cout << "Destination type is not a directory";
        return;
    }
    mode_t mode = check_mode(source);

    int src_open = open(source.c_str(), O_RDONLY);
    int des_open = open(destination.c_str(), O_CREAT | O_WRONLY, mode);
    char c;
    if (src_open == -1)
    {
        cout << "Error in Opening Source File";
        return;
    }
    if (des_open == -1)
    {
        cout << "File Already Exist";
        // return;
    }

    while (read(src_open, &c, 1))
        write(des_open, &c, 1);

    close(src_open);
    close(des_open);

    return;
}

string send_message(int listening, string str)
{
    char buffer[32768];
    memset(buffer, 0, 32768);
    send(listening, str.c_str(), str.size(), 0);
    int read_msg = read(listening, buffer, 32768);
    string reply = buffer;
    if (read_msg == -1)
    {
        cout << "Error in reading from client side(1).\n";
        exit(4);
    }
    return reply;
}

vector<string> downloading; // Used in show_downloads
string username;
bool isLoggedIn;
unordered_map<string, string> filenameToPath;

long long file_size_stat(string filename)
{
    struct stat stat_buf;
    int rc = stat(filename.c_str(), &stat_buf);
    return rc == 0 ? stat_buf.st_size : -1;
}

void login(string str)
{
    username = str;
    isLoggedIn = 1;
}
void logout()
{
    username = "";
    isLoggedIn = 0;
    filenameToPath.clear();
}

void download_chunks(int number_of_chunks, string sha1_hash_of_this_chunk, string ip, string port, int chunk_number, string file_name, string destination_path, string file_pathh)
{
    int down_peer = socket(AF_INET, SOCK_STREAM, 0);
    if (down_peer == -1)
    {
        cerr << "Can't create a socket on this peer\n";
        exit(1);
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(stoi(port.c_str()));
    int cl_pton = inet_pton(AF_INET, ip.c_str(), &server_addr.sin_addr);
    if (cl_pton == -1)
    {
        cerr << "Invalid Address.\n";
        exit(2);
    }

    int client_connect = connect(down_peer, (sockaddr *)&server_addr, sizeof(server_addr));
    if (client_connect == -1)
    {
        cerr << "Connection Failed2.\n";
        exit(3);
    }

    string reply = "down_load " + file_name + " " + to_string(chunk_number);
    send(down_peer, reply.c_str(), reply.size(), 0);

    // cout << "Number of chunks -> " << number_of_chunks << "\n";
    // cout << "chunk_number -> " << chunk_number << "\n";
    cout << "Filename -> " << file_pathh<< "\n";

    char buffer[chunksize];
    memset(buffer, 0, chunksize);
    long int file_size = file_size_stat(file_pathh);
    cout << "File size = " << file_size << "\n";

    string destination = destination_path;
    fstream outfile(destination, std::fstream::in | std::fstream::out | std::fstream::binary);
    outfile.seekp(chunksize * (chunk_number - 1), ios::beg);
    if (number_of_chunks == chunk_number)
    {
        // cout << "number_of_chunks == chunk_number(" << chunk_number << ")\n";
        for (int i = 0; i < (file_size % chunksize) / arraysize; i++)
        {
            char file_arr[arraysize];
            read(down_peer, file_arr, arraysize);
            outfile.write(file_arr, arraysize);
        }
        char file_arr[file_size % arraysize];
        read(down_peer, file_arr, file_size % arraysize);
        outfile.write(file_arr, file_size % arraysize);
    }
    else
    {
        // cout << "number_of_chunks != chunk_number(" << number_of_chunks << " != " << chunk_number << ")\n";
        for (int i = 0; i < 32; i++)
        {
            char file_arr[arraysize];
            read(down_peer, file_arr, arraysize);
            outfile.write(file_arr, arraysize);
        }
    }
    outfile.close();

    return;
}

void client_as_a_client(vector<string> sha1_hash, vector<pair<string, string>> socket, string file_name, string destination_path, string file_pathh)
{
    int number_of_chunks = sha1_hash.size();
    int number_of_peers = socket.size();
    downloading.push_back(file_name);
    vector<thread> fetch_chunks;
    destination_path = absolute_path(destination_path) + "/" + file_name;
    // cout << "Number of Chunks = " << number_of_chunks << "\n";
    // cout << "Number of Peers = " << number_of_peers << "\n";
    // cout << "Downloading " << file_name << "...\n";
    // cout << "Destination Path" << destination_path << "\n";
    ofstream destination_file(destination_path.c_str(), std::ios::out);

    // Piece Selection Algorithm
    if (number_of_chunks <= number_of_peers)
    {
        // cout << "number_of_chunks <= number_of_peers\n";
        for (int i = 0; i < number_of_chunks; i++)
            fetch_chunks.push_back(thread(download_chunks, number_of_chunks, sha1_hash[i], socket[i].first, socket[i].second, i+1, file_name, destination_path, file_pathh));

        while (fetch_chunks.size() >= 1)
        {
            fetch_chunks[0].join();
            fetch_chunks.erase(fetch_chunks.begin());
        }
    }

    else if (number_of_chunks > number_of_peers)
    {
        // cout << "number_of_chunks > number_of_peers\n";
        int divisor = number_of_chunks / number_of_peers;
        int remainder = number_of_chunks % number_of_peers;
        int chunk_number = 0;
        for (int i = 0; i < number_of_peers; i++)
        {
            for (int j = 0; j < divisor; j++)
            {
                fetch_chunks.push_back(thread(download_chunks, number_of_chunks, sha1_hash[chunk_number], socket[i].first, socket[i].second, chunk_number+1, file_name, destination_path, file_pathh));
                chunk_number++;
            }
        }

        for (int i = 0; i < remainder; i++)
        {
            fetch_chunks.push_back(thread(download_chunks, number_of_chunks, sha1_hash[chunk_number], socket[i].first, socket[i].second, chunk_number + 1, file_name, destination_path, file_pathh));
            chunk_number++;
        }

        while (fetch_chunks.size() > 0)
        {
            fetch_chunks[0].join();
            fetch_chunks.erase(fetch_chunks.begin());
        }
    }
    downloading.erase(find(downloading.begin(), downloading.end(), file_name)); // After downloading, erase it
    cout << "Download complete for " << file_name << "\n";
    filenameToPath[file_name] = destination_path;
    
    char *chunk = new char[chunksize];
    ifstream fin;
    fin.open(destination_path, ios::binary);
    vector<string> sha1;
    // int cnt = 0;
    while (fin)
    {
        fin.read(chunk, chunksize);
        if (!fin.gcount())
            break;
        
        string sha_chunk = sha11((chunk)).substr(0, 20);
        sha1.push_back(sha_chunk);
        // cnt++;
        // cout << "SHA(" << cnt << ")" << sha_chunk << "\n";
    }
    
    int flag = 0;
    for(unsigned long int i = 0; i<min(sha1_hash.size(), sha1.size()); i++)
    {
        if(sha1_hash[i] != sha1[i]){
            flag = 1;
            break;
        }
    }
    if(flag == 0)
        cout << "File obtained is perfect\n";
    else   cout << "File may be currupted\n";

    return;
}

void client_as_a_server(int new_socket)
{
    char buffer[32768];
    memset(buffer, 0, 32768);

    int bytes_read = read(new_socket, buffer, 32768);
    if (bytes_read == -1)
    {
        cerr << "Error in reading files in the peer" + string(buffer) + "\n";
        return;
    }
    else if (bytes_read == 0)
    {
        cerr << "Peer disconnected...\n";
    }
    // cout << "Reading in client_as_a_server\n";
    // cout << "Read = " << bytes_read << "\n";
    // cout << "Buffer(CS) -> " << buffer << endl;
    vector<string> command;
    char *token = strtok(buffer, " "); // const_cast is used to remove the const qualifier.
    while (token != NULL)
    {
        string temp = token;
        command.push_back(token);
        token = strtok(NULL, " ");
    }
    // <down_load> <file_name> <to_string(chunk_number)>
    if (command[0] == "down_load")
    {
        int chunk_number = stoi(command[2]);
        string downloading_file = filenameToPath[command[1]];
        cout << "downloading_file -> " << downloading_file << "  chunk_number -> " << chunk_number << "\n";
        ifstream fin(downloading_file.c_str(), std::ios::in | std::ios::binary);
        fin.seekg(chunksize * (chunk_number - 1), fin.beg);
        char chunk[chunksize] = {0};
        fin.read(chunk, chunksize);
        send(new_socket, chunk, chunksize, 0);
    }
    else
    {
        cout << "Invalid message from peer\n";
    }

    return;
}

void create_thread_after_each_client_formation(string ip, string port)
{
    int listening = socket(AF_INET, SOCK_STREAM, 0);
    if (listening == -1)
    {
        cerr << "Can't create a socket (in thread)\n";
        exit(EXIT_FAILURE);
    }

    sockaddr_in server_addr, client_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(stoi(port.c_str()));
    inet_pton(AF_INET, ip.c_str(), &server_addr.sin_addr);

    int binding = ::bind(listening, (sockaddr *)&server_addr, sizeof(server_addr));
    if (binding == -1)
    {
        cerr << "Can't bind the socket to IP / Port Address.\n";
        exit(EXIT_FAILURE);
    }

    int socket_listening = listen(listening, SOMAXCONN);
    if (socket_listening == -1)
    {
        cerr << "Can't Listen.\n";
        exit(EXIT_FAILURE);
    }

    char host[NI_MAXHOST];
    char service[NI_MAXSERV];
    vector<thread> servingPeerThread;
    char buffer[32768];
    while (true)
    {
        memset(buffer, 0, 32768);
        socklen_t client_size = sizeof(client_addr);
        int client_socket = accept(listening, (sockaddr *)&client_addr, &client_size);
        if (client_socket == -1)
        {
            cerr << "Probem with client connecting.\n";
            exit(EXIT_FAILURE);
        }

        memset(host, 0, NI_MAXHOST); // Cleaning up the arrays
        memset(service, 0, NI_MAXSERV);

        int result = getnameinfo((sockaddr *)&client_addr, sizeof(client_addr), host, NI_MAXHOST, service, NI_MAXSERV, 0);
        if (result)
        {
            cout << host << " connected on " << service << "\n";
        }
        else
        {
            inet_ntop(AF_INET, &client_addr.sin_addr, host, NI_MAXHOST);
            cout << host << " connented on " << ntohs(client_addr.sin_port) << "\n";
        }
        thread tid(&client_as_a_server, client_socket);
        tid.detach();
    }
    return;
}

int main(int argc, char *argv[])
{
    getcwd(ch, sizeof(ch));
    cur_direc = ch;
    struct stat str;
    usid = getpwuid(str.st_uid)->pw_name;
    home_dir = "/home/" + usid;
    synchronization = 0;

    if (argc != 3)
    {
        cout << "Required command line input in \"./client <IP>:<PORT> tracker_info.txt0\" ";
        exit(1);
    }

    int ip_port = 0;
    string client_ip, tracker_ip, client_port, tracker_port, tracker2_ip, tracker2_port;
    while (argv[1][ip_port] != ':')
    {
        client_ip.push_back(argv[1][ip_port]);
        ip_port++;
    }
    ip_port++;
    while (argv[1][ip_port] != '\0')
    {
        client_port.push_back(argv[1][ip_port]);
        ip_port++;
    }
    fstream tracker_info;
    tracker_info.open(argv[2]);
    tracker_info >> tracker_ip;
    tracker_info >> tracker_port;
    tracker_info >> tracker2_ip;
    tracker_info >> tracker2_port;
    tracker_info.close();

    int listening = socket(AF_INET, SOCK_STREAM, 0);
    if (listening == -1)
    {
        cerr << "Can't create a socket on client Side\n";
        exit(1);
    }
    cout << "Client socket Created...\n";

    if (synchronization == 0)
    {
        sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(stoi(tracker_port.c_str()));
        int cl_pton = inet_pton(AF_INET, tracker_ip.c_str(), &server_addr.sin_addr);
        if (cl_pton == -1)
        {
            cerr << "Invalid Address.\n";
            exit(2);
        }

        int client_connect = connect(listening, (sockaddr *)&server_addr, sizeof(server_addr));
        if (client_connect == -1)
        {
            synchronization = 1;
            cerr << "Connection Shifted to tracker 2.\n";
            cout << "Reload the command\n";
        }
        else{
            cout << "Client Connected with server 1...\n";
            cout << "Tracker IP Address is " << tracker_ip << " and port number is " << tracker_port << "\n";
        }
    }
    else if (synchronization == 1)
    {
        sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(stoi(tracker2_port.c_str()));
        int cl_pton = inet_pton(AF_INET, tracker2_ip.c_str(), &server_addr.sin_addr);
        if (cl_pton == -1)
        {
            cerr << "Invalid Address.\n";
            exit(2);
        }

        int client_connect = connect(listening, (sockaddr *)&server_addr, sizeof(server_addr));
        if (client_connect == -1)
        {
            synchronization = 0;
            cerr << "Connection Shifted to tracker 1.\n";
            cout << "Reload the command\n";
        }
        else{
            cout << "Client Connected with server 2...\n";
            cout << "Tracker IP Address is " << tracker2_ip << " and port number is " << tracker2_port << "\n";
        }
    }

   cout << "Client IP Address is " << client_ip << " and port number is " << client_port << "\n";

    thread thr(&create_thread_after_each_client_formation, client_ip, client_port);
    // Whenever we make any client, it can also seld files, hence have to be act as a server.
    // Therefore, we have to make a thread after connection to ensure the same.
    char buffer[32768];
    while (true)
    {
        string s;
        getline(cin, s);
        stringstream ss(s);
        string word;
        string argument = s;

        vector<string> command; // Converting each letter from string s into commands[i]
        while (ss >> word)
        {
            command.push_back(word);
        }
        memset(buffer, 0, 32768);

        if (command.size() == 0)
            continue;

        if (strcmp(command[0].c_str(), "create_user") == 0)
        {
            if (command.size() != 3)
            {
                cout << "Invalid Arguments\n";
                continue;
            }
            if (isLoggedIn == 1)
            {
                cout << username << " Logging out...\n";
                logout();
            }
            string reply = send_message(listening, argument);
            cout << reply << "\n";
        }
        else if (strcmp(command[0].c_str(), "login") == 0)
        {
            if (command.size() != 3)
            {
                cout << "Invalid Arguments\n";
                continue;
            }
            if (isLoggedIn == 1)
            {
                cout << username << " Logging out...\n";
                string reply = send_message(listening, "logout " + username);
                cout << reply << "\n";
                logout();
            }
            string argument = command[0] + " " + command[1] + " " + command[2] + " " + client_ip + " " + client_port;
            string reply = send_message(listening, argument);
            if (reply[0] == 'H')
            {
                logout();
                login(command[1]);
                vector<string> reply_token;
                cout << command[1] << " logged in\n";
                char *token = strtok(const_cast<char *>(reply.c_str()), " ");
                while (token != NULL)
                {
                    string temp = token;
                    reply_token.push_back(token);
                    token = strtok(NULL, " ");
                }
                for (long unsigned int i = 2; i < reply_token.size(); i += 2)
                {
                    filenameToPath[reply_token[i]] = reply_token[i + 1];
                    cout << reply_token[i] << " -> " << reply_token[i + 1] << "\n";
                }
            }
            else
                cout << reply << "\n";
        }
        else if (strcmp(command[0].c_str(), "logout") == 0)
        {
            if (command.size() != 1)
            {
                cout << "Invalid Argument\n";
                continue;
            }
            else
            {
                if (isLoggedIn == 0)
                {
                    cout << "Login first to use logout command...\n";
                    continue;
                }
                else if (isLoggedIn == 1)
                {
                    string reply = send_message(listening, argument + " " + username);
                    cout << reply << "\n";
                    logout();
                }
            }
        }

        else if (strcmp(command[0].c_str(), "create_group") == 0)
        {
            if (command.size() != 2)
            {
                cout << "Invalid Arguments\n";
                continue;
            }
            if (isLoggedIn == 0)
            {
                cout << "Login First..\n";
                continue;
            }
            else
            {
                string argument = "create_group " + command[1] + " by " + username;
                string reply = send_message(listening, argument);
                cout << reply << endl;
            }
        }
        else if (strcmp(command[0].c_str(), "join_group") == 0)
        {
            if (command.size() != 2)
            {
                cout << "Invalid Arguments\n";
                continue;
            }
            if (isLoggedIn == 0)
            {
                cout << "Login First..\n";
                continue;
            }
            else
            {
                string argument = username + " Joined_group " + command[1];
                string reply = send_message(listening, argument);
                cout << reply << endl;
            }
        }
        else if (strcmp(command[0].c_str(), "leave_group") == 0)
        {
            if (command.size() != 2)
            {
                cout << "Invalid Arguments\n";
                continue;
            }
            if (isLoggedIn == 0)
            {
                cout << "Login First..\n";
                continue;
            }
            else
            {
                string argument = "leave_group " + command[1] + " by " + username;
                string reply = send_message(listening, argument);
                cout << reply << endl;
            }
        }
        else if (strcmp(command[0].c_str(), "list_requests") == 0)
        {
            if (command.size() != 2)
            {
                cout << "Invalid Arguments\n";
                continue;
            }
            if (isLoggedIn == 0)
            {
                cout << "Login First..\n";
                continue;
            }
            else
            {
                string argument = "list_requests of group " + command[1] + " by " + username;
                string reply = send_message(listening, argument);
                cout << reply << "\n";
            }
        }
        else if (strcmp(command[0].c_str(), "accept_request") == 0)
        {
            if (command.size() != 3)
            {
                cout << "Invalid Arguments\n";
                continue;
            }
            if (isLoggedIn == 0)
            {
                cout << "Login First..\n";
                continue;
            }
            else
            { // accept_request <group_id> <user_id> username
                string argument = "accept_request of " + command[2] + " in group " + command[1] + " by " + username;
                string reply = send_message(listening, argument);
                cout << reply << endl;
            }
        }
        else if (strcmp(command[0].c_str(), "list_groups") == 0)
        {
            if (command.size() != 1)
            {
                cout << "Invalid Arguments\n";
                continue;
            }
            if (isLoggedIn == 0)
            {
                cout << "Login First..\n";
                // continue;
            }
            else
            {
                string reply = send_message(listening, "list_groups " + username);
                cout << reply << endl;
            }
        }
        else if (strcmp(command[0].c_str(), "list_files") == 0)
        {
            if (command.size() != 2)
            {
                cout << "Invalid Arguments\n";
                continue;
            }
            if (isLoggedIn == 0)
            {
                cout << "Login First..\n";
                // continue;
            }
            else
            {
                argument += " " + username;
                string reply = send_message(listening, argument);
                cout << reply << endl;
            }
        }
        else if (strcmp(command[0].c_str(), "upload_file") == 0)
        {
            if (command.size() != 3)
            {
                cout << "Invalid Arguments\n";
                continue;
            }
            if (isLoggedIn == 0)
                cout << "Login First..\n";
            else if (isLoggedIn == 1)
            {
                string file_path = absolute_path(command[1]);
                fname = file_path;
                // cout << file_path << "\n";
                // cout << fname << "\n";
                string file_name = "";
                int size_of_file_path = command[1].size();
                int i = size_of_file_path - 1;
                while (command[1][i--] != '/');
                i += 2;
                while (i < size_of_file_path)
                    file_name += command[1][i++];

                FILE *fp = fopen(file_path.c_str(), "r");
                if (fp == NULL)
                {
                    cout << "File not found..\n";
                    continue;
                }
                fseek(fp, 0L, SEEK_END);
                fclose(fp);
                long int file_size = file_size_stat(file_path);
                cout << "File size of " << file_name << " is " << file_size << "\n";
                if (file_size >= 0)
                {
                    size_t chunk_size = chunksize;
                    char *chunk = new char[chunk_size];
                    filenameToPath[file_name] = file_path;

                    ifstream fin;
                    fin.open(file_path, ios::binary);
                    vector<string> sha1;
                    while (fin)
                    {
                        fin.read(chunk, chunk_size);
                        if (!fin.gcount())
                        {
                            cout << "Error...\n";
                            break;
                        }
                        string sha_chunk = sha11((chunk)).substr(0, 20);
                        sha1.push_back(sha_chunk);
                    }

                    argument += " " + username + " " + file_name + " " + file_path + " " + to_string(file_size) + " ";
                    for (long unsigned int i = 0; i < sha1.size(); i++)
                        argument += sha1[i] + " ";
                    string reply = send_message(listening, argument);
                    cout << reply << "\n";
                }
            }
        }
        else if (strcmp(command[0].c_str(), "download_file") == 0)
        {
            if (command.size() != 4)
            {
                cout << "Invalid Arguments\n";
                continue;
            }
            if (isLoggedIn == 0)
                cout << "Login First..\n";
            else if (isLoggedIn == 1)
            {
                string dest = absolute_path(command[3]);
                if (!check_dir(dest))
                {
                    cout << "Destination is not a directory\n";
                    continue;
                }

                argument += " " + username;
                string reply = send_message(listening, argument);
                if (reply[0] == 'Y' || reply[0] == 'F')
                {
                    cout << reply << "\n";
                    continue;
                }

                vector<pair<string, string>> socket;
                vector<string> sha1_hash;
                vector<string> reply_token;

                char *token = strtok(const_cast<char *>(reply.c_str()), " ");
                int check = 1;
                while (token != NULL)
                {
                    string temp = token;
                    reply_token.push_back(token);
                    check++;
                    token = strtok(NULL, " ");
                }
                string file_path = reply_token[0];
                cout << file_path << "\n";
                long unsigned int index = 1;
                while (reply_token[index] != "file")
                {
                    sha1_hash.push_back(reply_token[index]);
                    index++;
                }
                index++;
                for (; index < reply_token.size(); index++)
                {
                    socket.push_back(make_pair(reply_token[index], reply_token[index + 1]));
                    index++;
                }

                if (socket.size() == 0)
                    cout << "No peer is serving this file currently...\n";
                else
                {
                    vector<thread> current_downloaders;
                    current_downloaders.push_back(thread(client_as_a_client, sha1_hash, socket, command[2], command[3], file_path));
                    while (current_downloaders.size() > 0)
                    {
                        current_downloaders[0].join();
                        current_downloaders.erase(current_downloaders.begin());
                    }
                }
            }
        }
        else if (strcmp(command[0].c_str(), "show_downloads") == 0)
        {
            if (command.size() != 1)
            {
                cout << "Invalid Arguments\n";
                continue;
            }
            if (isLoggedIn == 0)
                cout << "Login First..\n";
            else if (isLoggedIn == 1)
            {
                for (long unsigned int i = 0; i < downloading.size(); i++)
                    cout << "[D] " << downloading[i] << "\n";
                for (auto i = filenameToPath.begin(); i != filenameToPath.end(); i++)
                    cout << "[C] " << i->first << "\n";
            }
        }
        else if (strcmp(command[0].c_str(), "stop_share") == 0)
        {
            if (command.size() != 3)
            {
                cout << "Invalid Arguments\n";
                continue;
            }
            if (isLoggedIn == 0)
                cout << "Login First..\n";
            else if (isLoggedIn == 1)
            {
                argument += " " + username;
                string reply = send_message(listening, argument);
            }
        }
        else
        {
            cout << "Wrong Input\n";
        }
    }
    thr.join();
    close(listening);
    cout << "Socket Connection Closed...\n";

    return 0;
}
