#include <fstream>
#include <sstream>
#include "Network.h"
#include <ctime>
#include <iomanip>

Network::Network() {

}

void Network::process_commands(vector<Client> &clients, vector<string> &commands, int message_limit,
                               const string &sender_port, const string &receiver_port) {
    for (int i=0;i<commands.size();i++){
        size_t pos = commands[i].find(' ');
        std::string firstPart = commands[i].substr(0, pos);
        std::string secondPart = commands[i].substr(pos + 1);

        std::cout<<std::string (commands[i].size()+9,'-')<<std::endl;
        std::cout<<"Command: "<<commands[i]<<std::endl;
        std::cout<<std::string (commands[i].size()+9,'-')<<std::endl;
        if(firstPart=="MESSAGE"){

            //split the command
            size_t posIn = secondPart.find(' ');
            std::string sender=secondPart.substr(0,posIn);
            std::string secondSecond = secondPart.substr(posIn + 1);
            size_t posInTwo = secondSecond.find(' ');
            std::string receiver=secondSecond.substr(0,posInTwo);
            std::string message=secondSecond.substr(posInTwo+1);


            //clear the message
            message.erase(0,1);
            message.erase(message.size()-1,message.size());
            size_t chunkSize = message_limit;
            size_t strLength = message.length();
            std::cout<<"Message to be sent: \""<< message<<"\"\n\n";

            //find senderMac,receiverMac,senderIp,receiverIp
            std::string senderMac,receiverMac,senderIp,receiverIp;
            for(int j =0;j<clients.size();j++){
                if (clients[j].client_id==sender){
                    senderMac=clients[j].client_mac;
                    receiverMac=clients[j].routing_table[receiver];
                    senderIp=clients[j].client_ip;
                } else if (clients[j].client_id==receiver){
                    receiverIp=clients[j].client_ip;
                }
            }
            for(int k =0;k<clients.size();k++){
                if (clients[k].client_id==receiverMac){
                    receiverMac=clients[k].client_mac;
                }
            }


            int frameCounter=0;
            for (size_t j = 0; j < strLength; j += chunkSize) {

                frameCounter+=1;
                std::cout<<"Frame #"<<frameCounter<<std::endl;
                stack<Packet*> packetStack;
                std::string subString = message.substr(j, std::min(chunkSize, strLength - j));


                packetStack.push(new ApplicationLayerPacket(0, sender, receiver,subString));
                packetStack.push(new TransportLayerPacket(1, sender_port, receiver_port));
                packetStack.push(new NetworkLayerPacket(2, senderIp, receiverIp));
                packetStack.push(new PhysicalLayerPacket(3, senderMac, receiverMac));



                PhysicalLayerPacket(0, senderMac, receiverMac).print();
                NetworkLayerPacket(1, senderIp, receiverIp).print();
                TransportLayerPacket(2, sender_port, receiver_port).print();
                ApplicationLayerPacket(3, sender, receiver,subString).print();

                for(int d=0;d<clients.size();d++){
                    if(clients[d].client_id==sender){
                        clients[d].outgoing_queue.push(packetStack);
                    }
                }
                std::cout<<"--------"<<std::endl;
            }
        }
        else if(firstPart=="SEND"){
            for(int k =0;k<clients.size();k++){
                if(!clients[k].outgoing_queue.empty()){
                    int frameCounter=0;
                    string allMessage="";

                    while (!clients[k].outgoing_queue.empty()){
                        frameCounter+=1;

                        stack<Packet*> temp;
                        temp=clients[k].outgoing_queue.front();
                        PhysicalLayerPacket* physical=dynamic_cast<PhysicalLayerPacket*>(temp.top());
                        temp.pop();
                        NetworkLayerPacket* network=dynamic_cast<NetworkLayerPacket*>(temp.top());
                        temp.pop();
                        TransportLayerPacket* transport=dynamic_cast<TransportLayerPacket*>(temp.top());
                        temp.pop();
                        ApplicationLayerPacket* application=dynamic_cast<ApplicationLayerPacket*>(temp.top());
                        allMessage+=application->message_data;

                        application->hop+=1;
                        std::string hopId;
                        std::string startId;
                        for (int p=0;p<clients.size();p++){
                            if(clients[p].client_mac==physical->receiver_MAC_address){
                                hopId=clients[p].client_id;
                            }
                            else if(clients[p].client_mac==physical->sender_MAC_address){
                                startId=clients[p].client_id;

                            }
                        }
                        std::cout<<"Client "<<startId<<" sending frame #"<<frameCounter<<" to client "<<hopId<<std::endl;
                        physical->print();
                        network->print();
                        transport->print();
                        application->print();
                        std::cout<<"--------"<<std::endl;
                        std::string sentMac="";
                        char lastCharacter = application->message_data.back();
                        for (int l=0;l<clients.size();l++){
                            if(clients[l].client_id==application->sender_ID){
                             sentMac=clients[l].client_mac;
                            }
                        }


                        if(application->message_data.back()=='.' ||application->message_data.back()=='!' || application->message_data.back()=='?'){
                            if(clients[k].client_mac==sentMac){
                                auto t = std::time(nullptr);
                                auto tm = *std::localtime(&t);

                                std::ostringstream oss;
                                oss << std::put_time(&tm, "%Y-%m-%d %H-%M-%S");
                                auto str = oss.str();
                                Log tempLog(str,allMessage,frameCounter,0,application->sender_ID,application->receiver_ID,true,ActivityType::MESSAGE_SENT);
                                clients[k].log_entries.push_back(tempLog);
                                frameCounter=0;
                                allMessage="";

                            }
                        }

                        for (int t=0;t<clients.size();t++){
                            if(clients[t].client_mac==physical->receiver_MAC_address){
                                clients[t].incoming_queue.push(clients[k].outgoing_queue.front());
                            }

                        }
                        clients[k].outgoing_queue.pop();

                    }

                }

            }

        }
        else if(firstPart=="SHOW_FRAME_INFO"){
            size_t posIn = secondPart.find(' ');
            std::string clientID=secondPart.substr(0,posIn);
            std::string secondSecond = secondPart.substr(posIn + 1);
            size_t posInTwo = secondSecond.find(' ');
            std::string inOut=secondSecond.substr(0,posInTwo);
            std::string count=secondSecond.substr(posInTwo+1);
            int number = std::stoi(count);
            bool check=false;
            for (int a =0;a<clients.size();a++){
                if(clients[a].client_id==clientID){
                    check=true;
                    queue<stack<Packet*>> temp;
                    if(inOut=="out"){
                        temp=clients[a].outgoing_queue;
                    }else if (inOut=="in"){
                        temp=clients[a].incoming_queue;
                    }
                    if(number==1){
                        if(temp.empty()){
                            std::cout<<"No such frame."<<std::endl;
                        }
                    }
                    else{
                        for (int c=0;c<number-1;c++){
                            if(!temp.empty()){
                                temp.pop();
                            }else{
                                std::cout<<"No such frame."<<std::endl;
                                break;

                            }
                        }
                    }
                    if(!temp.empty()){
                        if(inOut=="out"){
                            std::cout<<"Current Frame #"<<number<<" on the outgoing queue of client "<< clientID<<std::endl;
                        }else if (inOut=="in"){
                            std::cout<<"Current Frame #"<<number<<" on the incoming queue of client "<< clientID<<std::endl;
                        }
                        stack<Packet*> tempStack;
                        tempStack=temp.front();
                        PhysicalLayerPacket* physicalShow=dynamic_cast<PhysicalLayerPacket*>(tempStack.top());
                        tempStack.pop();
                        NetworkLayerPacket* networkShow=dynamic_cast<NetworkLayerPacket*>(tempStack.top());
                        tempStack.pop();
                        TransportLayerPacket* transportShow=dynamic_cast<TransportLayerPacket*>(tempStack.top());
                        tempStack.pop();
                        ApplicationLayerPacket* applicationShow=dynamic_cast<ApplicationLayerPacket*>(tempStack.top());
                        std::cout<<"Carried Message: \""<<applicationShow->message_data<<"\""<<std::endl;
                        std::cout<<"Layer 0 info: "<<"Sender ID: "<< applicationShow->sender_ID<<", Receiver ID: "<<applicationShow->receiver_ID<<std::endl;
                        std::cout<<"Layer 1 info: ";
                        transportShow->print();
                        std::cout<<"Layer 2 info: ";
                        networkShow->print();
                        std::cout<< "Layer 3 info: ";
                        physicalShow->print();
                        std::cout<<"Number of hops so far: ";
                        std::cout<<applicationShow->hop<<std::endl;
                    }



                }
            }
        }
        else if(firstPart=="SHOW_Q_INFO"){
            size_t posIn = secondPart.find(' ');
            std::string clientID=secondPart.substr(0,posIn);
            std::string secondSecond = secondPart.substr(posIn + 1);
            size_t posInTwo = secondSecond.find(' ');
            std::string inOut=secondSecond.substr(0,posInTwo);
            for (int a =0;a<clients.size();a++){
                if(clients[a].client_id==clientID){
                    queue<stack<Packet*>> temp;
                    if(inOut=="out"){
                        temp=clients[a].outgoing_queue;
                    }else if (inOut=="in"){
                        temp=clients[a].incoming_queue;
                    }
                    int frameCounter=0;
                    while(!temp.empty()){
                        temp.pop();
                        frameCounter+=1;
                    }
                    if(inOut=="out"){
                        std::cout<<"Client "<<clientID<<" Outgoing Queue Status"<<std::endl;
                    }else if (inOut=="in"){
                        std::cout<<"Client "<<clientID<<" Incoming Queue Status"<<std::endl;
                    }
                    std::cout<<"Current total number of frames: "<<frameCounter<<std::endl;


                }

            }


        }
        else if (firstPart=="RECEIVE"){
            for(int k =0;k<clients.size();k++){
                int checkTitle=0;
                int frameNum=0;
                std::string allMessage="";
                while (!clients[k].incoming_queue.empty()){
                    frameNum+=1;
                    stack<Packet*> temp;
                    temp=clients[k].incoming_queue.front();
                    PhysicalLayerPacket* physical=dynamic_cast<PhysicalLayerPacket*>(temp.top());
                    temp.pop();
                    NetworkLayerPacket* network=dynamic_cast<NetworkLayerPacket*>(temp.top());
                    temp.pop();
                    TransportLayerPacket* transport=dynamic_cast<TransportLayerPacket*>(temp.top());
                    temp.pop();
                    ApplicationLayerPacket* application=dynamic_cast<ApplicationLayerPacket*>(temp.top());

                    std::string tempMac=clients[k].routing_table[application->receiver_ID];

                    int check =0;


                    for (int q=0;q<clients.size();q++){

                        if(clients[q].client_id==tempMac){
                            check+=1;
                            tempMac=clients[q].client_mac;
                        }
                    }

                    string macFrom=physical->sender_MAC_address;
                    allMessage+=application->message_data;
                    for (int o=0;o<clients.size();o++){
                        if(clients[o].client_mac==macFrom){
                            macFrom=clients[o].client_id;
                        }
                    }
                    //arrived
                    if (tempMac==""){
                        std::cout<<"Client "<<application->receiver_ID<<" receiving frame #"<<frameNum<<" from client "<<macFrom<<", originating from client "<<application->sender_ID<<std::endl;
                        physical->print();
                        network->print();
                        transport->print();
                        application->print();
                        std::cout<<"--------"<<std::endl;
                        if(application->message_data.back()=='.' ||application->message_data.back()=='!' || application->message_data.back()=='?'){
                            frameNum=0;
                            std::cout<<"Client "<<application->receiver_ID<<" received the message \""<<allMessage<<"\" from client "<< application->sender_ID<<"."<<std::endl;
                            std::cout<<"--------"<<std::endl;
                            for(int h=0;h<clients.size();h++){
                                if(application->receiver_ID==clients[h].client_id){
                                    auto t = std::time(nullptr);
                                    auto tm = *std::localtime(&t);

                                    std::ostringstream oss;
                                    oss << std::put_time(&tm, "%Y-%m-%d %H-%M-%S");
                                    auto str = oss.str();
                                    Log tempLog(str,allMessage,frameNum,application->hop,application->sender_ID,application->receiver_ID,true,ActivityType::MESSAGE_RECEIVED);
                                    clients[h].log_entries.push_back(tempLog);

                                }
                                else if(macFrom==clients[h].client_id){
                                    auto t = std::time(nullptr);
                                    auto tm = *std::localtime(&t);

                                    std::ostringstream oss;
                                    oss << std::put_time(&tm, "%Y-%m-%d %H-%M-%S");
                                    auto str = oss.str();
                                    Log tempLog(str,allMessage,frameNum,application->hop,application->sender_ID,application->receiver_ID,true,ActivityType::MESSAGE_FORWARDED);
                                    clients[h].log_entries.push_back(tempLog);

                                }

                            }
                            allMessage="";

                        }
                        clients[k].incoming_queue.front().pop();





                    }
                        //dropped
                    else if (check==0){
                        std::cout<<"Client "<<clients[k].client_id<< " receiving frame #"<<frameNum <<" from client "<<macFrom<<", but intended for client "<<application->receiver_ID<<". Forwarding... "<<std::endl;
                        std::cout<<"Error: Unreachable destination. Packets are dropped after "<<application->hop<<" hops!"<<std::endl;
                        if(application->message_data.back()=='.' ||application->message_data.back()=='!' || application->message_data.back()=='?'){
                            frameNum=0;
                            std::cout<<"--------"<<std::endl;
                            for(int h=0;h<clients.size();h++){
                                if(clients[k].client_id==clients[h].client_id){
                                    auto t = std::time(nullptr);
                                    auto tm = *std::localtime(&t);

                                    std::ostringstream oss;
                                    oss << std::put_time(&tm, "%Y-%m-%d %H-%M-%S");
                                    auto str = oss.str();
                                    Log tempLog(str,allMessage,frameNum,application->hop,application->sender_ID,application->receiver_ID,false,ActivityType::MESSAGE_DROPPED);
                                    clients[h].log_entries.push_back(tempLog);

                                }
                            }
                            allMessage="";

                        }


                    }
                        //forwarded
                    else{
                        physical->sender_MAC_address=clients[k].client_mac;
                        checkTitle+=1;
                        if (frameNum==1){
                            std::cout<<"Client "<<clients[k].client_id<<" receiving a message from client "<<macFrom<<", ""but intended for client "<<application->receiver_ID<<". Forwarding... "<<std::endl;
                        }
                        std::cout<<"Frame #"<<frameNum<<" MAC address change: New sender MAC "<<clients[k].client_mac<<", new receiver MAC "<<tempMac<<std::endl;

                        if(application->message_data.back()=='.' || application->message_data.back()=='!' || application->message_data.back()=='?'){
                            frameNum=0;
                            std::cout<<"--------"<<std::endl;
                            for(int h=0;h<clients.size();h++){
                                if(macFrom==clients[h].client_id & clients[h].client_id!=application->sender_ID){
                                    auto t = std::time(nullptr);
                                    auto tm = *std::localtime(&t);

                                    std::ostringstream oss;
                                    oss << std::put_time(&tm, "%Y-%m-%d %H-%M-%S");
                                    auto str = oss.str();
                                    Log tempLog(str,allMessage,frameNum,application->hop,application->sender_ID,application->receiver_ID,true,ActivityType::MESSAGE_FORWARDED);
                                    clients[h].log_entries.push_back(tempLog);

                                }
                            }
                            allMessage="";


                        }

                        physical->receiver_MAC_address=tempMac;
                        clients[k].incoming_queue.front().pop();
                        clients[k].incoming_queue.front().push(physical);
                        clients[k].outgoing_queue.push(clients[k].incoming_queue.front());




                    }
                    clients[k].incoming_queue.pop();



                }
            }


        }
        else if(firstPart=="PRINT_LOG"){
            size_t posIn = secondPart.find(' ');
            std::string clientID=secondPart.substr(0,posIn);
            for (int y=0;y<clients.size();y++){
                if(clients[y].client_id==clientID){
                    if(!clients[y].log_entries.empty()){
                        for (int p=0;p<clients[y].log_entries.size();p++){
                            for (int v=0;v<clients[y].log_entries.size();p++){
                                if(clients[y].log_entries[p].message_content==clients[y].log_entries[v].message_content){
                                    if (clients[y].log_entries[p].activity_type == ActivityType::MESSAGE_FORWARDED){
                                        clients[y].log_entries.erase(clients[y].log_entries.begin()+p);
                                    }else if(clients[y].log_entries[v].activity_type == ActivityType::MESSAGE_FORWARDED){
                                        clients[y].log_entries.erase(clients[y].log_entries.begin()+v);
                                    }
                                    break;
                                }
                                break;

                            }
                        }
                        for(int e=0;e<clients[y].log_entries.size();e++){
                            if(e!=0){
                                std::cout<<"--------------"<<std::endl;
                            }else if (e==0){
                                std::cout<<"Client "<<clientID<<" Logs:"<<std::endl;
                                std::cout<<"--------------"<<std::endl;
                            }
                            std::string type="";
                            if(clients[y].log_entries[e].activity_type==ActivityType::MESSAGE_SENT){
                                type="Message Sent";
                            }else if(clients[y].log_entries[e].activity_type==ActivityType::MESSAGE_DROPPED){
                                type="Message Dropped";
                            }else if(clients[y].log_entries[e].activity_type==ActivityType::MESSAGE_RECEIVED){
                                type="Message Received";
                            }else if(clients[y].log_entries[e].activity_type==ActivityType::MESSAGE_FORWARDED){
                                type="Message Forwarded";
                            }
                            std::cout<<"Log Entry #"<<e+1<<":"<<std::endl;
                            std::cout<<"Activity: "<<type<<std::endl;
                            std::cout<<"Timestamp: "<<clients[y].log_entries[e].timestamp<<std::endl;
                            int div=clients[y].log_entries[e].message_content.size()/message_limit;
                            int remainder=clients[y].log_entries[e].message_content.size() % message_limit;
                            if(remainder!=0){
                                div+=1;
                            }
                            std::cout<<"Number of frames: "<<div<<std::endl;
                            std:cout<<"Number of hops: "<<clients[y].log_entries[e].number_of_hops<<std::endl;
                            std::cout<<"Sender ID: "<<clients[y].log_entries[e].sender_id<<std::endl;
                            std::cout<<"Receiver ID: "<<clients[y].log_entries[e].receiver_id<<std::endl;
                            std::string checkStatus="";
                            if(clients[y].log_entries[e].success_status){
                                checkStatus="Yes";
                                std::cout<<"Success: "<<checkStatus<<std::endl;
                                if(clients[y].log_entries[e].activity_type!=ActivityType::MESSAGE_FORWARDED){
                                    std::cout<<"Message: \""<<clients[y].log_entries[e].message_content <<"\""<<std::endl;
                                }
                            }
                            else{
                                checkStatus="No";
                                std::cout<<"Success: "<<checkStatus<<std::endl;

                            }

                        }
                    }
                }
            }


        }
        else {
            std::cout<<"Invalid command."<<std::endl;
        }


    }

}



vector<Client> Network::read_clients(const string &filename) {
    vector<Client> clients;
    std::ifstream file(filename);

    std::string skipLine;
    std::getline(file, skipLine);
    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string name , ip , mac;
        iss >> name >> ip >> mac;
        clients.push_back(Client(name,ip,mac));
    }
    file.close();
    return clients;
}

void Network::read_routing_tables(vector<Client> &clients, const string &filename) {
    std::ifstream file(filename);
    std::string line;
    std::unordered_map<std::string, std::string> currentMap;

    for (int i = 0; std::getline(file, line);) {
        if (line == "-") {
            clients[i].routing_table = currentMap;
            currentMap.clear();
            i++;
        } else {
            std::istringstream iss(line);
            std::string key, value;
            if (iss >> key >> value) {
                currentMap[key] = value;
            }
        }
        if (file.eof()) {
            clients[i].routing_table = currentMap;
            currentMap.clear();
        }
    }

    file.close();
}

// Returns a list of token lists for each command
vector<string> Network::read_commands(const string &filename) {
    vector<string> commands;
    std::ifstream file(filename);
    std::string skipLine;
    std::getline(file, skipLine);
    std::string line;
    while (std::getline(file, line)) {
        commands.push_back(line);
    }
    file.close();
    return commands;
}

Network::~Network() {
}
