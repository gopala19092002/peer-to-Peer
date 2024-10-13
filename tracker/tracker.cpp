#include <iostream>         // cerr
#include <sys/socket.h>     // sockets
#include <netdb.h>          // getnameinfo  - address-to-name translation in protocol-independent manner
#include <string.h>         // Used for memset
#include <unistd.h>         // read/write/close
#include <fstream>          // fstream
#include <vector>           // Vector
#include <thread>           // Threading
#include <algorithm>        // find
#include <unordered_map>    // Unordered MAP
#include <arpa/inet.h>      // inet_pton

using namespace std;
#define MAX_CLIENTS SOMAXCONN
#define ll long long

class user
{
public:
    string username, password, ip_address, port_number;
    unordered_map<string, string> fnameToPath;
    bool isLoggedIn;

    user(string uid, string pass)
    {
        username = uid;
        password = pass;
        isLoggedIn = 1;
        // display_user();
    }
    user(string uid, string pass, int number)
    {
        username = uid;
        password = pass;
        isLoggedIn = number;
        // display_user();
    }

    void login(string ip_add, string port)
    {
        ip_address = ip_add;
        port_number = port;
        isLoggedIn = 1;
    }
    void logout()
    {
        ip_address = "";
        port_number = "";
        isLoggedIn = 0;
    }
    void display_user()
    {
        cout << "username -> " << username << endl;
        cout << "Password -> " << password << endl;
        cout << "LoggedIn -> " << isLoggedIn << endl;
    }
};

class group
{
public:
    string groupId, admin;
    int count_member;
    vector<string> allMembers;
    vector<string> group_files;
    vector<string> pendingRequests;

    group(string gid, string owner_name)
    {
        groupId = gid;
        admin = owner_name;
        allMembers.push_back(admin);
    }

    bool isRequest(string str)
    {
        for (int i = 0; i < pendingRequests.size(); i++)
        {
            if (str == pendingRequests[i])
                return true;
        }
        return false;
    }
    void remove(vector<string> &v, const string &target)
    {
        v.erase(std::remove(v.begin(), v.end(), target), v.end());
    }
    void accept_request(string str)
    {
        remove(pendingRequests, str);
        allMembers.push_back(str);
    }
    void removeMember(string str)
    {
        auto member = find(allMembers.begin(), allMembers.end(), str);
        allMembers.erase(member);
    }
    bool isFile(string str)
    {
        for (int i = 0; i < group_files.size(); i++)
        {
            if (str == group_files[i])
                return true;
        }
        return false;
    }
    bool isMember(string str)
    {
        for (int i = 0; i < allMembers.size(); i++)
        {
            if (str == allMembers[i])
                return true;
        }
        return false;
    }
};

class myFile
{
public:
    string filename;
    ll filesize;
    vector<string> sha1;
    vector<pair<string, bool>> fileshare;
    vector<string> uname;
    myFile(string fname, ll size)
    {
        filename = fname;
        filesize = size;
    }

    bool isUser(string str)
    {
        for (auto i = fileshare.begin(); i != fileshare.end(); i++)
        {
            if (str == i->first)
                return 1;
        }
        return 0;
    }
    void add(string str)
    {
        fileshare.push_back(make_pair(str, 1));
    }
};

unordered_map<string, user *> users;
unordered_map<string, group *> groups;
unordered_map<string, myFile *> files;

bool isUserExists(string str)
{
    if (users.find(str) == users.end())
        return 0;
    else
        return 1;
}
bool isFileExists(string str)
{
    if (files.find(str) == files.end())
        return 0;
    else
        return 1;
}
bool isGroupExists(string str)
{
    if (groups.find(str) == groups.end())
        return 0;
    else
        return 1;
}


void getCommand(int client_socket)
{
    cout << "Listening to " << client_socket << endl;
    while (true)
    {
        char buffer[32768];
        memset(buffer, 0, 32768); // Clear the buffer

        // Read data from client
        int read_data = read(client_socket, buffer, 32768);
        if (read_data == -1)
        {
            cout << "Error in reading.\n";
            return;
        }
        else if (read_data == 0)
        {
            cout << client_socket << " closed...\n";
            return;
        }
        string buff = string(buffer);
        buff = buff.substr(0,50);
        cout << "Recieved data from " << client_socket << "(50 characters) -> " << buff << "\n";

        char *token = strtok(buffer, " ");
        vector<string> command; // Converting each letter from buffer into commands[i]
        while (token != NULL)
        {
            string temp = token;
            command.push_back(temp);
            token = strtok(NULL, " ");
        }
        memset(buffer, 0, 32768);

        if (strcmp(command[0].c_str(), "create_user") == 0)
        {
            if (isUserExists(command[1]))
            {
                string reply = "User already Exists...\n";
                send(client_socket, reply.c_str(), reply.size(), 0);
            }
            else
            {
                user *user_client = new user(command[1], command[2]);
                users[command[1]] = user_client;
                string reply = command[1] + "'s account created Successfully...\n";
                send(client_socket, reply.c_str(), reply.size(), 0);
                cout << reply;

                ofstream outdata; // For making the file persistent
                const char *filename = "user_details.txt";
                outdata.open(filename, ios_base::app);
                if (!outdata)
                    cerr << "Error: file could not be opened\n";
                // cout<<"File Created.\n";
                outdata << command[1] << " ";
                outdata << command[2] << "\n";
                outdata.close();

                ofstream outdata_uig; // For making the file persistent
                const char *filename_uig = "user_in_group.txt";
                outdata_uig.open(filename_uig, ios_base::app);
                if (!outdata_uig)
                    cerr << "Error: file could not be opened\n";
                // cout<<"File Created.\n";
                outdata_uig << command[1] << " ";
                outdata_uig << command[2] << "\n";
                outdata_uig.close();
            }
        }
        else if (strcmp(command[0].c_str(), "login") == 0)
        {
            if (!isUserExists(command[1]))
            {
                string reply = "User doesn't Exists...\n";
                send(client_socket, reply.c_str(), reply.size(), 0);
            }
            else
            {
                if (users[command[1]]->password != command[2])
                {
                    string reply = "Password Doesn't Match...\n";
                    send(client_socket, reply.c_str(), reply.size(), 0);
                }
                else
                {
                    users[command[1]]->login(command[3], command[4]);
                    string reply = "Hello " + command[1] + "\n";

                    for (auto i = users[command[1]]->fnameToPath.begin(); i != users[command[1]]->fnameToPath.end(); i++)
                        reply += i->first + " " + i->second;
                    send(client_socket, reply.c_str(), reply.size(), 0);



                }
            }
        }
        else if (strcmp(command[0].c_str(), "logout") == 0)
        {
            if (!isUserExists(command[1]))
            {
                string reply = "User doesn't Exists...\n";
                send(client_socket, reply.c_str(), reply.size(), 0);
            }
            else
            {
                users[command[1]]->logout();
                string reply = command[1] + " Logged Out Successfully...\n";
                send(client_socket, reply.c_str(), reply.size(), 0);
            }
        }

        else if (strcmp(command[0].c_str(), "create_group") == 0)
        {
            if (!isUserExists(command[3]))
            {
                string reply = "User doesn't Exists...\n";
                send(client_socket, reply.c_str(), reply.size(), 0);
            }
            else if (isGroupExists(command[1]))
            {
                string reply = "Group already Exists...\n";
                send(client_socket, reply.c_str(), reply.size(), 0);
            }
            else
            {
                // create_group + groupid + by + username;
                group *new_group = new group(command[1], command[3]);
                groups[command[1]] = new_group;
                string reply = "Group Created by " + command[3] + " successfully...\n";
                cout << "Reply = " << reply << "\n";
                send(client_socket, reply.c_str(), reply.size(), 0);
                cout << reply << "\n";

                    ofstream outdata; // For making the file persistent
                    const char *filename = "group_details.txt";
                    outdata.open(filename, ios_base::app);
                    if (!outdata)
                        cerr << "Error: file could not be opened\n";
                    // cout<<"File Created.\n";
                    outdata << command[1] << " ";
                    outdata << command[3] << "\n";
                    outdata.close();

                    ofstream user_outdata; // For making the file persistent
                    const char *user_filename = "user_in_group.txt";
                    user_outdata.open(user_filename, ios_base::app);
                    if (!user_outdata)
                        cerr << "Error: file could not be opened\n";
                    // cout<<"File Created.\n";
                    user_outdata << command[1] << " ";
                    user_outdata << command[3] << "\n";
                    user_outdata.close();
            }
        }
        else if (strcmp(command[1].c_str(), "Joined_group") == 0)
        {
            // username + Joined_group + groupid;
            if (!isUserExists(command[0]))
            {
                string reply = "User doesn't Exists...\n";
                send(client_socket, reply.c_str(), reply.size(), 0);
            }
            else if (!isGroupExists(command[2]))
            {
                string reply = "Group doesn't Exists...\n";
                send(client_socket, reply.c_str(), reply.size(), 0);
            }
            else if (groups[command[2]]->isMember(command[0]))
            {
                string reply = "You are already the member of this group...\n";
                send(client_socket, reply.c_str(), reply.size(), 0);
            }
            else
            {
                groups[command[2]]->pendingRequests.push_back(command[0]);
                string reply = "Wait for some time until someone accepts your request to join\n";
                send(client_socket, reply.c_str(), reply.size(), 0);
            }
        }
        else if (strcmp(command[0].c_str(), "leave_group") == 0)
        {
            //   leave_group + group_id + by + username;
            if (!isUserExists(command[3]))
            {
                string reply = "User doesn't Exists...\n";
                send(client_socket, reply.c_str(), reply.size(), 0);
            }
            else if (!isGroupExists(command[1]))
            {
                string reply = "Group doesn't Exists...\n";
                send(client_socket, reply.c_str(), reply.size(), 0);
            }
            else if (!groups[command[1]]->isMember(command[3]))
            {
                string reply = "You are not the member of this group...\n";
                send(client_socket, reply.c_str(), reply.size(), 0);
            }
            else
            {
                int flag = 0;
                groups[command[1]]->removeMember(command[3]);
                string reply = "You left the group...\n";
                send(client_socket, reply.c_str(), reply.size(), 0);

                // If Admin leaves itself
                if (command[3] == groups[command[1]]->admin)
                {
                    cout << "Admin leaves the group\n";
                    if (groups[command[1]]->allMembers.size() > 1){
                        cout << "New admin = " << groups[command[1]]->allMembers[1] << "\n";
                        groups[command[1]]->admin = groups[command[1]]->allMembers[1];
                    }
                    else{
                        cout << "Group deleted\n";
                        
                        groups.erase(command[1]); // Delete that group permanently
                        flag = 1;

                        string change_admin;
                        string admin;
                        ifstream ad_fin;
                        ad_fin.open("group_details.txt");
                        ofstream ad_temp;
                        ad_temp.open("ad_temp.txt");
                        change_admin = command[1] + " " + command[3];
                        cout << "Group_details.txt = " << change_admin << change_admin.size() << "\n";
                        while (getline(ad_fin, admin))
                        {
                            if (admin != change_admin)
                                ad_temp << admin << "\n";
                        }
                        ad_temp.close();
                        ad_fin.close();
                        remove("group_details.txt");
                        rename("ad_temp.txt", "group_details.txt");
                        // cout << command[1] << " in group_details.txt should be deleted\n";
                    }
                }

                // Remove that member from that user_in_group.txt
                string deleteline;
                string line;
                ifstream fin;
                fin.open("user_in_group.txt");
                ofstream temp;
                temp.open("temp.txt");
                deleteline = command[1] + " " + command[3];
                // cout << "user_in_group.txt = " << deleteline << deleteline.size() << "\n";
                while (getline(fin, line))
                {
                    if (line != deleteline)
                        temp << line << "\n";
                }
                temp.close();
                fin.close();
                remove("user_in_group.txt");
                rename("temp.txt", "user_in_group.txt");

                // Change in group_details.txt
                if (flag == 0){
                    cout << groups[command[1]]->allMembers.size() << "\n";
                    cout << "Going in...\n";
                    string change_admin;
                    string admin;
                    ifstream ad_fin;
                    ad_fin.open("group_details.txt");
                    ofstream ad_temp;
                    ad_temp.open("ad_temp.txt");
                    change_admin = command[1] + " " + command[3];
                    // cout << "Group_details.txt = " << change_admin << change_admin.size() << "\n";
                    while (getline(ad_fin, admin))
                    {
                        if (admin != change_admin)
                            ad_temp << admin << "\n";
                        else
                        {
                            ad_temp << command[1] << " ";
                            ad_temp << groups[command[1]]->admin << "\n";
                        }
                    }
                    ad_temp.close();
                    ad_fin.close();
                    remove("group_details.txt");
                    rename("ad_temp.txt", "group_details.txt");
                }
            }
        }
        else if (strcmp(command[0].c_str(), "list_requests") == 0)
        {
            if (!isUserExists(command[5]))
            {
                string reply = "User doesn't exists...\n";
                send(client_socket, reply.c_str(), reply.size(), 0);
            }
            else if (!isGroupExists(command[3]))
            {
                string reply = "Group doesn't exists...\n";
                send(client_socket, reply.c_str(), reply.size(), 0);
            }
            else if (groups[command[3]]->admin != command[5])
            {
                string reply = "Only owner has the right to list_request...\n";
                send(client_socket, reply.c_str(), reply.size(), 0);
            }
            else
            {
                string reply = "";
                for (auto i = 0; i < groups[command[3]]->pendingRequests.size(); i++)
                    reply += groups[command[3]]->pendingRequests[i] + "\n";
                if (reply == "")
                    reply = "No request yet...\n";
                send(client_socket, reply.c_str(), reply.size(), 0);
            }
        }
        else if (strcmp(command[0].c_str(), "accept_request") == 0)
        {
            cout << command[2] << " " << command[7] << "\n";
            if (!isUserExists(command[7]))
            {
                string reply = "User doesn't exists...\n";
                send(client_socket, reply.c_str(), reply.size(), 0);
            }
            else if (!isGroupExists(command[5]))
            {
                string reply = "Group doesn't exists...\n";
                send(client_socket, reply.c_str(), reply.size(), 0);
            }
            else if (groups[command[5]]->admin != command[7])
            {
                string reply = "Only owner can accept request...\n";
                send(client_socket, reply.c_str(), reply.size(), 0);
            }
            else if (!groups[command[5]]->isRequest(command[2]))
            {
                char msg[] = "No pending request found...\n";
                send(client_socket, msg, strlen(msg), 0);
            }
            else
            { // accept_request  of  userid(2)  in  group  group_id(5)  by  username(7)
                groups[command[5]]->accept_request(command[2]);
                string reply = "Request Accepted..\n"; // "";
                send(client_socket, reply.c_str(), reply.size(), 0);
                cout << reply << "\n";

                ofstream outdata; // For making the file persistent
                const char *filename = "user_in_group.txt";
                outdata.open(filename, ios_base::app);
                if (!outdata)
                    cerr << "Error: file could not be opened\n";
                outdata << command[5] << " ";
                outdata << command[2] << "\n";
                outdata.close();
            }
        }
        else if (strcmp(command[0].c_str(), "list_groups") == 0)
        {
            if (!isUserExists(command[1]))
            {
                string reply = "User doesn't exists...\n";
                send(client_socket, reply.c_str(), reply.size(), 0);
            }
            else
            {
                string reply = "";
                for (auto i = groups.begin(); i != groups.end(); i++)
                    reply += i->first + "\n";

                if (reply == "")
                    reply = "You are not in any group...\n";
                send(client_socket, reply.c_str(), reply.size(), 0);
            }
        }
        else if (strcmp(command[0].c_str(), "list_files") == 0)
        {
            if (!isUserExists(command[2]))
            {
                string reply = "User doesn't exists...\n";
                send(client_socket, reply.c_str(), reply.size(), 0);
            }
            else if (!isGroupExists(command[1]))
            {
                string reply = "Group doesn't exists...\n";
                send(client_socket, reply.c_str(), reply.size(), 0);
            }
            else if (!groups[command[1]]->isMember(command[2]))
            {
                string reply = "You are not the member of the group, " + command[1] + "...\n";
                send(client_socket, reply.c_str(), reply.size(), 0);
            }
            else
            {
                string reply = "";
                if(users[command[2]]->isLoggedIn == 1){
                for (int i = 0; i < groups[command[1]]->group_files.size(); i++)
                    reply += groups[command[1]]->group_files[i] + "\n";
                }
                if (reply == "")
                    reply = "No files to show...\n";
                send(client_socket, reply.c_str(), reply.size(), 0);
            }
        }

        else if (strcmp(command[0].c_str(), "upload_file") == 0)
        { 
            if (!isUserExists(command[3]))
            {
                string reply = "User doesn't exists...\n";
                send(client_socket, reply.c_str(), reply.size(), 0);
                continue;
            }
            else if (!isGroupExists(command[2]))
            {
                string reply = "Group doesn't exists...\n";
                send(client_socket, reply.c_str(), reply.size(), 0);
                continue;
            }
            else if (!groups[command[2]]->isMember(command[3]))
            {
                string reply = "You are not the member of the group, " + command[2] + "...\n";
                send(client_socket, reply.c_str(), reply.size(), 0);
                continue;
            }
            else
            {
                if (!groups[command[2]]->isFile(command[4]))
                    groups[command[2]]->group_files.push_back(command[4]);
                if (files.find(command[4]) != files.end())
                { // If file found in files map
                    if (files[command[4]]->isUser(command[3]))
                    {
                        string reply = "File is already there...\n";
                        send(client_socket, reply.c_str(), reply.size(), 0);
                    }
                    else if (!files[command[4]]->isUser(command[3])) 
                    {
                        files[command[4]]->add(command[3]);
                        // username filename filepath
                        users[command[3]]->fnameToPath[command[4]] = command[5];
                        string reply = command[4] + " Uploaded Successfully(1)...\n";
                        send(client_socket, reply.c_str(), reply.size(), 0);
                    }
                }
                else if (files.find(command[4]) == files.end())
                {
                    // upload_file <file_path> <group_id> <username> <file_name> <file_path> <file_size> <SHA1's>

                    myFile *newfile = new myFile(command[4], stoi(command[6])); // Make a new file with entries file name and file size
                    newfile->add(command[3]);                                   // Add in fileshare vector wrt username
                    files[command[4]] = newfile;                                // Add in map with key as its filename
                    // user *obj = new user()
                    users[command[3]]->fnameToPath[command[4]] = command[5]; 

                    // cout << users[command[3]]->username<< endl;
                    // cout << "upload -> " << command[5] << "\n";  // Add in map wrt username
                    for (int i = 7; i < command.size(); i++)
                        newfile->sha1.push_back(command[i]); // Add in SHA1
                    string reply = command[4] + " Uploaded Successfully(2)...\n";
                    send(client_socket, reply.c_str(), reply.size(), 0);

                // Have to update this due to sha1
                    // ofstream outdata; // For making the file persistent
                    // char *filename = "files_in_group.txt";
                    // outdata.open(filename, ios_base::app);
                    // if (!outdata)
                    //     cerr << "Error: file could not be opened\n";
                    // outdata << command[2] << " ";
                    // outdata << command[1] << "\n";
                    // outdata.close();

                }
            }
        }   
        else if (strcmp(command[0].c_str(), "download_file") == 0)
        {
            // download_file <group_id> <file_name> <destination_path> username
            if (!isUserExists(command[4]))
            {
                string reply = "User doesn't exists...\n";
                send(client_socket, reply.c_str(), reply.size(), 0);
                continue;
            }
            if (!isGroupExists(command[1]))
            {
                string reply = "Group doesn't exists...\n";
                send(client_socket, reply.c_str(), reply.size(), 0);
                continue;
            }
            if (!groups[command[1]]->isFile(command[2]))
            {
                string reply = "File doesn't exists in this group...\n";
                send(client_socket, reply.c_str(), reply.size(), 0);
                continue;
            }
            if (!groups[command[1]]->isMember(command[4]))
            {
                string reply = "You are not the member of the group, " + command[2] + "...\n";
                send(client_socket, reply.c_str(), reply.size(), 0);
            }
            else
            {
            // username filename filepath
            string file_path;
            cout << "aa " << users[command[4]]->username << "\n";
            for(auto u: users){
                if(u.second->fnameToPath.find(command[2]) !=  u.second->fnameToPath.end())
                {
                    file_path = u.second->fnameToPath[command[2]];
                }
            }
            // cout << "File_path = " << file_path << "\n";
                string reply = file_path + " ";
                for(auto i : files[command[2]]->sha1)
                    reply += i + " ";
                    reply += "file ";
                for(auto t_user: files[command[2]]->fileshare){
                    if(users[t_user.first]->isLoggedIn && t_user.second)	// User is logged in and sharing.
                        reply += users[t_user.first]->ip_address+" "+users[t_user.first]->port_number+" ";
			    }
                // reply += "Stopped Sharing...\n";
                send(client_socket, reply.c_str(), reply.size(), 0);
            }
        }     
        else if (strcmp(command[0].c_str(), "stop_share") == 0)
        {
            if (!isUserExists(command[3]))
            {
                string reply = "User doesn't exists...\n";
                send(client_socket, reply.c_str(), reply.size(), 0);
            }
            else if (!isGroupExists(command[1]))
            {
                string reply = "Group doesn't exists...\n";
                send(client_socket, reply.c_str(), reply.size(), 0);
            }
            else if (!isGroupExists(command[2]))
            {
                string reply = "File doesn't exists...\n";
                send(client_socket, reply.c_str(), reply.size(), 0);
            }
            else if (!groups[command[2]]->isMember(command[3]))
            {
                string reply = "You are not the member of the group, " + command[2] + "...\n";
                send(client_socket, reply.c_str(), reply.size(), 0);
            }
            else
            { // stop_share <group_id> <file_name> username;
                for (auto i = files[command[2]]->fileshare.begin(); i != files[command[2]]->fileshare.end(); i++)
                {
                    if (i->first == command[3])
                    {
                        i->second = 0;
                        break;
                    }
                }
                string reply = "Stopped Sharing...\n";
                send(client_socket, reply.c_str(), reply.size(), 0);
            }
        }
        else
        {
            cout << "Wrong Input on server side..\n";
        }
    }
}

int main(int argc, char *argv[])
{
    vector<thread> peers;
    int i, p_ind = 0;
    bool isQuit = 0;
    sockaddr_in client_addr;

    if (argc != 3)
    {
        cout << "Required command line input is \" ./tracker tracker_info.txt tracker_no\" ";
        exit(1);
    }

    fstream tracker_info;
    tracker_info.open(argv[1]);
    string tracker1_ip, tracker1_port, tracker2_ip, tracker2_port;
    tracker_info >> tracker1_ip;
    tracker_info >> tracker1_port;
    tracker_info >> tracker2_ip;
    tracker_info >> tracker2_port;
    tracker_info.close();

    string aaa = string(argv[2]);
    if(aaa == "1")
        cout << "Tracker IP Address is " << tracker1_ip << " and port number is " << tracker1_port << "\n";
    
    else if (aaa == "2")
        cout << "Tracker IP Address is " << tracker2_ip << " and port number is " << tracker2_port << "\n";
    
    
    {
        ifstream fin("user_details.txt");
        char ch;
        string word;
        vector<string> words;
        fin.seekg(0, ios::beg);
        if (!fin)
            cerr << "Error: Input file could not be opened(user_details.txt)\n";
        while (fin)
        {
            fin.get(ch);
            word += ch;
            if (ch == ' ' || ch == '\n')
            {
                word = word.substr(0, word.length() - 1);
                words.push_back(word);
                word = "";
            }
        }
        if (words.size() > 1)
        {
            for (i = 0; i < words.size() - 1; i += 2)
            {
                if (!isUserExists(words[i]))
                {
                    user *user_client = new user(words[i], words[i + 1], 0);
                    users[words[i]] = user_client;
                }
            }
        }
        fin.close();

            ifstream g_fin("group_details.txt");
            char g_ch;
            string gr_word;
            vector<string> group_words;
            g_fin.seekg(0, ios::beg);
            if (!g_fin)
                cerr << "Error: Input file could not be opened(group_details.txt)\n";
            while (g_fin)
            {
                g_fin.get(g_ch);
                gr_word += g_ch;
                if (g_ch == ' ' || g_ch == '\n')
                {
                    gr_word = gr_word.substr(0, gr_word.length() - 1);
                    group_words.push_back(gr_word);
                    gr_word = "";
                }
            }
            if (group_words.size() > 1)
            {
                for (i = 0; i < group_words.size() - 1; i += 2)
                {
                    if (!isUserExists(group_words[i]))
                    {
                        group *group_client = new group(group_words[i], group_words[i + 1]);
                        groups[group_words[i]] = group_client;
                    }
                }
            }
            g_fin.close();

        ifstream u_fin("user_in_group.txt");
        char u_ch;
        string user_word;
        vector<string> user_group;
        u_fin.seekg(0, ios::beg);
        if (!u_fin)
            cerr << "Error: Input file could not be opened(user_in_group.txt)\n";
        while (u_fin)
        {
            u_fin.get(u_ch);
            user_word += u_ch;
            if (u_ch == ' ' || u_ch == '\n')
            {
                user_word = user_word.substr(0, user_word.length() - 1);
                user_group.push_back(user_word);
                user_word = "";
            }
        }
        if (user_group.size() > 1)
        {
            for (i = 0; i < user_group.size() - 1; i += 2)
            {
                if (!isUserExists(user_group[i]))
                    groups[user_group[i]]->allMembers.push_back(user_group[i + 1]);
            }
        }
        u_fin.close();
    }

    // Create a socket                      // int socket(int domain, int type, int protocol);
    int listening = socket(AF_INET, SOCK_STREAM, 0); // AF_INET is used for IPv4
                                                     // Stream sockets allow processes to communicate using TCP.
                                                     // After the connection has been established, data can be read from and written to these sockets as a byte stream.
    if (listening == -1)                             // On success, a file descriptor for the new socket is returned. On error, -1 is returned
    {
        cerr << "Can't create a socket\n";
        return -1;
    }
    // cout << "Socket Created...\n";

    // Bind the socket to a IP/Port
    sockaddr_in server_addr;                                       // sockaddr_in is for IPv4
                                                                   // Sockaddr_in is for IPv6
    server_addr.sin_family = AF_INET;                              // sin is shorthand for the name of the struct (sockaddr_in) that the member-variables are a part of.
                                                                   // sin_family refers to an address family which in most of the cases is set to “AF_INET”.
    if(aaa == "1")
        server_addr.sin_port = htons(stoi(tracker1_port.c_str()));      // The htons function takes a 16-bit number in host byte order and returns a 16-bit number in network byte order used in TCP/IP networks.
    else if (aaa == "2")                                            // Intel supports little-endian format, which means the leats significant bytes are stored before the more significant bytes.
        server_addr.sin_port = htons(stoi(tracker2_port.c_str()));      // htons refers to host-to-network short.
    
    if(aaa == "1")
        inet_pton(AF_INET, tracker1_ip.c_str(), &server_addr.sin_addr); // The inet_pton() function converts an Internet address in its standard text format into its numeric binary form.
    else if (aaa == "2")
        inet_pton(AF_INET, tracker2_ip.c_str(), &server_addr.sin_addr);



    int binding = ::bind(listening, (sockaddr *)&server_addr, sizeof(server_addr)); // The bind() function binds a unique local name to the socket with descriptor socket.
                                                                                  // Bind socket "listening" using format "AF_NET" to "server_addr" structure
    if (binding == -1)
    {
        cerr << "Can't bind the socket to IP / Port Address.\n";
        return -2;
    }
    // cout << "Socket Binded...\n";

    // Mark the socket for listening in
    int socket_listening = listen(listening, SOMAXCONN); // Maximum connections can be SOMAXCON(4096)
    if (socket_listening == -1)
    {
        cerr << "Can't Listen.\n";
        return -3;
    }
    cout << "Socket Start Listening...\n";

    // sockaddr_in client_addr;
    char host[NI_MAXHOST];
    char service[NI_MAXSERV];

    // thread quit(quit1, ref(isQuit));

    // While recieving, display the msg
    char buffer[32768];
    while (true)
    {
        
        memset(buffer, 0, 32768);
        // Accept a call
        socklen_t client_size = sizeof(client_addr);
        int client_socket = accept(listening, (sockaddr *)&client_addr, &client_size);
        // The accept() call creates a new socket descriptor with the same properties as socket and returns it to the caller.
        // If the queue has no pending connection requests, accept() blocks the caller unless socket is in nonblocking mode.
        if (client_socket == -1)
        {
            cerr << "Probem with client connecting.\n";
            return -4;
        }
        cout << "Server and Client connected...\n";

        memset(host, 0, NI_MAXHOST); // Cleaning up the arrays
        memset(service, 0, NI_MAXSERV);

        int result = getnameinfo((sockaddr *)&client_addr, sizeof(client_addr), host, NI_MAXHOST, service, NI_MAXSERV, 0);
        if (result)
        {
            cout << host << " connected on " << service << "\n";
        }
        else
        { // inet_ntop is opposite to inet_pton, i.e., converts an Internet address from its numeric binary form to its standard text format.
          // ntohs is opposite to htons, i.e., takes a 16-bit number in TCP/IP network byte order (AF_INET) and returns a 16-bit number in host byte order.
            inet_ntop(AF_INET, &client_addr.sin_addr, host, NI_MAXHOST);
            cout << host << " connented on " << ntohs(client_addr.sin_port) << "\n";
        }

        if(isQuit == 1)
            break;
        peers.push_back(thread(getCommand, client_socket));

    }
    cout << "Total threads = " << peers.size() << "\n";
    for (i = 0; i < peers.size(); i++)
        peers[i].join();

    // Close the Socket
    close(listening);
    cout << "Socket Closed...\n";

    return 0;
}
